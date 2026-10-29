#!/usr/bin/env python3
"""Build immutable branch snapshots, generate reference traces, and report comparisons."""
import argparse
import collections
import datetime
from decimal import Decimal
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MASK = (1 << 64) - 1


def run(args, **kwargs):
    return subprocess.check_output([str(x) for x in args], text=True, **kwargs).strip()


class Digest:
    def __init__(self):
        self.value = 14695981039346656037

    def add(self, *values):
        for value in values:
            self.value = ((self.value ^ value) * 1099511628211) & MASK


class Reference:
    def __init__(self):
        self.orders = {}
        self.ids = []
        self.positions = {}
        self.levels = [{}, {}]
        self.next_id = 1
        self.digest = Digest()
        self.fills = 0

    def remove(self, oid):
        pos = self.positions.pop(oid)
        last = self.ids.pop()
        if pos < len(self.ids):
            self.ids[pos] = last
            self.positions[last] = pos
        del self.orders[oid]

    def cancel(self, oid):
        self.remove(oid)
        self.digest.add(1)

    def submit(self, side, price, quantity):
        oid = self.next_id
        self.next_id += 1
        trades = []
        opposite = self.levels[1 - side]
        for p in sorted(opposite, reverse=side == 1):
            if (side == 0 and p > price) or (side == 1 and p < price):
                break
            queue = opposite[p]
            while queue and quantity:
                maker = queue[0]
                if maker not in self.orders:
                    queue.popleft()
                    continue
                resting = self.orders[maker]
                traded = min(quantity, resting[2])
                trades.append((oid if side == 0 else maker,
                               maker if side == 0 else oid, traded, p))
                quantity -= traded
                resting[2] -= traded
                if resting[2] == 0:
                    self.remove(maker)
                    queue.popleft()
            if not queue:
                del opposite[p]
            if not quantity:
                break
        if quantity:
            self.orders[oid] = [side, price, quantity]
            self.positions[oid] = len(self.ids)
            self.ids.append(oid)
            self.levels[side].setdefault(price, collections.deque()).append(oid)
        self.digest.add(oid, quantity, len(trades))
        for trade in trades:
            self.digest.add(*trade)
        self.fills += len(trades)

    def state_digest(self):
        d = Digest()
        for oid in range(1, self.next_id):
            order = self.orders.get(oid)
            d.add(oid, int(order is not None))
            if order:
                d.add(*order)
        return d.value


def make_trace(config, path):
    rng = random.Random(config['seed'])
    ref = Reference()
    lines = []
    levels = config['price_levels_per_side']

    def submit(aggressive=False):
        side = rng.randrange(2)
        price = (1100 if side == 0 else 900) if aggressive else (
            999 - rng.randrange(levels) if side == 0 else 1001 + rng.randrange(levels))
        quantity = rng.randint(1, config['max_quantity'])
        lines.append(f'S {side} {price} {quantity} 0\n')
        ref.submit(side, price, quantity)

    for _ in range(config['prefill_orders']):
        submit()
    cancel_count = int(Decimal(str(config['cancel_percent'])) * config['measured_commands'] / 100)
    kinds = [True] * cancel_count + [False] * (config['measured_commands'] - cancel_count)
    rng.shuffle(kinds)
    minimum = maximum = len(ref.ids)
    fills_before = ref.fills
    for cancel in kinds:
        if cancel:
            if not ref.ids:
                raise ValueError('book drained: increase prefill_orders')
            oid = ref.ids[rng.randrange(len(ref.ids))]
            lines.append(f'C 0 0 0 {oid}\n')
            ref.cancel(oid)
        else:
            submit(rng.randrange(100) < config['aggressive_submit_percent'])
        minimum = min(minimum, len(ref.ids))
        maximum = max(maximum, len(ref.ids))
    header = (f"{config['prefill_orders']} {config['measured_commands']} {ref.next_id - 1} "
              f'{ref.digest.value} {ref.state_digest()}\n')
    path.write_text(header + ''.join(lines))
    return dict(trace_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                cancellation_commands=cancel_count, submission_commands=len(kinds) - cancel_count,
                executed_trades=ref.fills - fills_before, active_min=minimum,
                active_max=maximum, active_final=len(ref.ids), events_digest=ref.digest.value,
                state_digest=ref.state_digest(),
                actual_cancel_percent=100 * cancel_count / config['measured_commands'])


def snapshot(ref, dest):
    commit = run(['git', 'rev-parse', ref], cwd=ROOT)
    data = subprocess.check_output(['git', 'archive', commit], cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        for member in archive.getmembers():
            # Extract only regular source/build files needed for this experiment.
            if member.isfile() and (member.name.startswith(('src/', 'include/', 'tests/', 'app/'))
                                    or member.name == 'CMakeLists.txt'):
                target = dest / member.name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.extractfile(member).read())
    return commit


def summarize(rows):
    batch = [r for r in rows if r['mode'] == 'batch']
    latency = [r for r in rows if r['mode'] == 'latency']
    rates = [r['operations'] * 1e9 / r['elapsed_ns'] for r in batch]
    summary = dict(throughput_median_ops_s=statistics.median(rates),
                   throughput_min_ops_s=min(rates), throughput_max_ops_s=max(rates),
                   process_peak_rss_kib=statistics.median(r['process_peak_rss_kib'] for r in batch))
    for field in ('submit_p50_ns', 'submit_p99_ns', 'cancel_p50_ns', 'cancel_p99_ns'):
        summary[field] = statistics.median(r[field] for r in latency) if latency else None
    return summary


def display(value):
    return 'N/A' if value is None else f'{value:,.0f}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configs', type=Path, default=ROOT / 'benchmarks/configs')
    parser.add_argument('--output', type=Path, default=ROOT / 'benchmarks/results/latest')
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--modes', choices=('both', 'batch'), default='both',
                        help='batch skips instrumented latency passes (useful for crossover sweeps)')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    configs = [json.loads(p.read_text()) for p in sorted(args.configs.glob('*.json'))]
    if not configs:
        raise ValueError('no workload configs')
    for c in configs:
        if (not 0 <= c['cancel_percent'] <= 100 or c['measured_commands'] % 100
                or c['measured_commands'] <= 0 or c['prefill_orders'] <= 0
                or not 1 <= c['price_levels_per_side'] <= 100 or c['repetitions'] < 2
                or not 0 <= c['aggressive_submit_percent'] <= 100 or c['max_quantity'] <= 0):
            raise ValueError(f'invalid config: {c}')
    cpu = min(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else None
    def pin():
        if cpu is not None:
            os.sched_setaffinity(0, {cpu})
    cpu_model = platform.processor()
    if Path('/proc/cpuinfo').exists():
        cpu_model = next((line.split(':', 1)[1].strip() for line in
                          Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')), cpu_model)
    metadata = dict(timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    system=platform.platform(), cpu=cpu_model, pinned_cpu=cpu,
                    compiler=run([args.compiler, '--version']).splitlines()[0],
                    python=platform.python_version(), modes=args.modes, flags=['-std=c++20', '-O3', '-DNDEBUG'],
                    harness_sha256={name: hashlib.sha256((ROOT / 'benchmarks' / name).read_bytes()).hexdigest()
                                    for name in ('engine_bench.cpp', 'run.py')}, engines={})
    reports = []
    with tempfile.TemporaryDirectory(prefix='matching-bench-') as temp:
        temp = Path(temp)
        binaries = {}
        for name in ('main', 'benchmark'):
            source = temp / name
            commit = snapshot(name, source)
            print(f'Building and testing {name} ({commit[:8]})', flush=True)
            log = []
            log.append(run(['cmake', '-S', source, '-B', source / 'build', '-DCMAKE_BUILD_TYPE=Release',
                            f'-DCMAKE_CXX_COMPILER={args.compiler}']))
            log.append(run(['cmake', '--build', source / 'build', '-j2']))
            log.append(run(['ctest', '--test-dir', source / 'build', '--output-on-failure']))
            (output / f'{name}-tests.txt').write_text('\n'.join(log) + '\n')
            binary = temp / f'bench-{name}'
            run([args.compiler, *metadata['flags'], '-I', source / 'include',
                 ROOT / 'benchmarks/engine_bench.cpp', source / 'src/matching_engine.cpp',
                 source / 'src/order_book.cpp', '-o', binary])
            binaries[name] = binary
            metadata['engines'][name] = commit
        for config in configs:
            label = config['name']
            trace = temp / f'{label}.trace'
            trace_stats = make_trace(config, trace)
            # One untallied batch run per engine warms code/data paths and checks correctness.
            for name in binaries:
                run([binaries[name], trace, 'batch'], preexec_fn=pin)
            collected = {name: [] for name in binaries}
            for repeat in range(config['repetitions']):
                order = list(binaries) if repeat % 2 == 0 else list(reversed(binaries))
                for mode in (('batch', 'latency') if args.modes == 'both' else ('batch',)):
                    for name in order:
                        print(f'{label}: {name} {mode} repeat {repeat + 1}', flush=True)
                        row = json.loads(run([binaries[name], trace, mode], preexec_fn=pin))
                        row.update(repetition=repeat + 1, mode=mode)
                        collected[name].append(row)
            for name, rows in collected.items():
                report = dict(engine=name, commit=metadata['engines'][name], config=config,
                              trace=trace_stats, summary=summarize(rows), runs=rows)
                reports.append(report)
                (output / f'{name}-{label}.json').write_text(json.dumps(report, indent=2) + '\n')
                s = report['summary']
                (output / f'{name}-{label}.md').write_text(
                    f"# {name}: {label}\n\nCommit: `{report['commit']}`\n\n"
                    f"Cancellation commands: {config['cancel_percent']}% of measured commands. "
                    f"{config['prefill_orders']:,} initial orders; {config['measured_commands']:,} measured commands; "
                    f"{config['repetitions']} repetitions.\n\n"
                    f"- Median throughput: {s['throughput_median_ops_s']:,.0f} commands/s\n"
                    f"- Throughput range: {s['throughput_min_ops_s']:,.0f}–{s['throughput_max_ops_s']:,.0f} commands/s\n"
                    f"- Submit p50 / p99: {display(s['submit_p50_ns'])} / {display(s['submit_p99_ns'])} ns\n"
                    f"- Cancel p50 / p99: {display(s['cancel_p50_ns'])} / {display(s['cancel_p99_ns'])} ns\n"
                    f"- Median process peak RSS: {s['process_peak_rss_kib']:,.0f} KiB (includes harness)\n"
                    f"- Active orders: min {trace_stats['active_min']:,}, max {trace_stats['active_max']:,}, "
                    f"final {trace_stats['active_final']:,}\n"
                    f"- Executed trades: {trace_stats['executed_trades']:,}\n\n"
                    'All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians '
                    'of per-run percentiles, from separate instrumented passes. See '
                    '[methodology](../../README.md) and [raw results](./' + f'{name}-{label}.json).\n')
    (output / 'environment.json').write_text(json.dumps(metadata, indent=2) + '\n')
    lines = ['# Engine comparison', '', f"Run: {metadata['timestamp_utc']}", '',
             f"CPU: {cpu_model}; logical CPU {cpu}; compiler: {metadata['compiler']}.", '',
             '| Cancel commands | Engine | Median commands/s | Submit p50/p99 ns | Cancel p50/p99 ns |',
             '|---|---|---:|---:|---:|']
    for r in reports:
        s = r['summary']
        lines.append(f"| {r['config']['cancel_percent']}% | [{r['engine']}](./{r['engine']}-{r['config']['name']}.md) "
                     f"| {s['throughput_median_ops_s']:,.0f} | {display(s['submit_p50_ns'])}/{display(s['submit_p99_ns'])} "
                     f"| {display(s['cancel_p50_ns'])}/{display(s['cancel_p99_ns'])} |")
    lines.extend(['', '## Observed throughput ratios', ''])
    for c in configs:
        pair = {r['engine']: r['summary'] for r in reports if r['config']['name'] == c['name']}
        ratio = pair['benchmark']['throughput_median_ops_s'] / pair['main']['throughput_median_ops_s']
        lines.append(f"- {c['cancel_percent']}% cancellations: indexed list / deque = **{ratio:.2f}×**.")
    lines.extend(['', 'These are synthetic, single-threaded results on this machine, not production capacity. '
                  'Book occupancy evolves differently across cancellation ratios; compare engines within each '
                  'workload rather than attributing cross-workload differences solely to the ratio. '
                  'Peak RSS includes trace storage and the harness. Throughput includes result digesting and '
                  'loop overhead; per-operation latency uses a separate pass with timer overhead. '
                  'No engine-only memory or allocation measurements were made.', '',
                  'Both branch test suites passed. Every replay matched the independent Python reference '
                  'model for execution results and final live/absent orders. '
                  'See [methodology](../../README.md), [environment](./environment.json), and individual JSON files '
                  'for configuration, immutable source commits, trace hashes, and all repetitions.', ''])
    (output / 'REPORT.md').write_text('\n'.join(lines))
    print(f'Reports written to {output}', flush=True)


if __name__ == '__main__':
    main()
