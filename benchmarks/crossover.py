#!/usr/bin/env python3
"""Generate crossover workloads or summarize paired throughput measurements."""
import argparse
import collections
import json
from pathlib import Path
import random
import statistics


def generate(directory, ratios, seeds, repetitions):
    directory.mkdir(parents=True, exist_ok=True)
    for ratio in ratios:
        for seed in seeds:
            name = f'cross_{ratio:g}_seed_{seed}'.replace('.', 'p')
            config = dict(name=name, cancel_percent=ratio, seed=seed, prefill_orders=20000,
                          measured_commands=20000, price_levels_per_side=100, max_quantity=100,
                          aggressive_submit_percent=20, repetitions=repetitions)
            (directory / f'{name}.json').write_text(json.dumps(config, indent=2) + '\n')


def summarize(directory):
    groups = collections.defaultdict(list)
    commits = set()
    shapes = set()
    for path in sorted(directory.glob('deque-baseline-*.json')):
        main = json.loads(path.read_text())
        indexed = json.loads((directory / path.name.replace('deque-baseline-', 'main-', 1)).read_text())
        if main['config'] != indexed['config'] or main['trace'] != indexed['trace']:
            raise ValueError('unpaired workloads')
        commits.add((main['commit'], indexed['commit']))
        config = main['config']
        shapes.add(tuple((k, config[k]) for k in ('prefill_orders', 'measured_commands',
                   'price_levels_per_side', 'max_quantity', 'aggressive_submit_percent')))
        a = {r['repetition']: r for r in main['runs'] if r['mode'] == 'batch'}
        b = {r['repetition']: r for r in indexed['runs'] if r['mode'] == 'batch'}
        if a.keys() != b.keys():
            raise ValueError('unpaired repetitions')
        ratios = [a[i]['elapsed_ns'] / b[i]['elapsed_ns'] for i in a]
        groups[config['cancel_percent']].append(dict(seed=config['seed'], paired_speedups=ratios,
               median_paired_speedup=statistics.median(ratios),
               actual_cancel_percent=main['trace']['actual_cancel_percent']))
    if not groups or len(commits) != 1 or len(shapes) != 1:
        raise ValueError('missing results or inconsistent engine/workload settings')
    rng = random.Random(829104)
    rows = []
    for percent, seeds in sorted(groups.items()):
        # Hierarchical paired bootstrap: resample seeds, then paired repetitions within each seed.
        # This describes uncertainty in this small experiment, not all market workloads.
        draws = []
        for _ in range(5000):
            medians = []
            for group in rng.choices(seeds, k=len(seeds)):
                values = group['paired_speedups']
                medians.append(statistics.median(rng.choices(values, k=len(values))))
            draws.append(statistics.median(medians))
        draws.sort()
        lo, hi = draws[124], draws[4874]
        seed_medians = [s['median_paired_speedup'] for s in seeds]
        label = ('indexed wins' if lo > 1 and min(seed_medians) > 1 else
                 'deque wins' if hi < 1 and max(seed_medians) < 1 else 'inconclusive')
        rows.append(dict(cancel_percent=percent, median_speedup=statistics.median(seed_medians),
                         bootstrap_95_interval=[lo, hi], classification=label, seeds=seeds))
    winners = [r['cancel_percent'] for r in rows if r['classification'] == 'indexed wins']
    minimum = min(winners) if winners else None
    result = dict(metric='batch replay throughput', rows=rows,
                  smallest_tested_consistent_indexed_win_percent=minimum,
                  engine_commits=list(next(iter(commits))), workload=dict(next(iter(shapes))),
                  bootstrap_seed=829104, bootstrap_draws=5000)
    (directory / 'CROSSOVER.json').write_text(json.dumps(result, indent=2) + '\n')
    lines = ['# Cancellation crossover experiment', '',
             'Speedup = indexed-list throughput / deque throughput. Above 1 favors the indexed version. '
             'The metric is end-to-end batch replay throughput, including the same digest/loop overhead in both engines.', '',
             '| Cancellation commands | Median paired speedup | 95% bootstrap interval | Classification | Seeds |',
             '|---|---:|---:|---|---:|']
    for r in rows:
        lo, hi = r['bootstrap_95_interval']
        lines.append(f"| {r['cancel_percent']:g}% | {r['median_speedup']:.3f}× | {lo:.3f}–{hi:.3f}× "
                     f"| {r['classification']} | {len(r['seeds'])} |")
    if minimum is not None:
        lower = [r['cancel_percent'] for r in rows if r['cancel_percent'] < minimum]
        lines.extend(['', f'**Smallest tested consistently winning ratio: {minimum:g}%.**'])
        if lower:
            lines.extend(['', f'The grid locates the first consistent win above {max(lower):g}% and at '
                          f'{minimum:g}%. This is a resolution/noise bracket, not a proof of an exact mathematical threshold.'])
    else:
        lines.extend(['', 'No consistently winning indexed ratio was established on this grid.'])
    lines.extend(['', 'Classification requires the bootstrap interval to exclude 1 and every seed median '
                  'to favor the same engine. Bootstrap resamples seeds, then paired repetitions within each seed '
                  '(5,000 draws, fixed seed). Intervals describe only this finite experiment and are exploratory; '
                  'they do not correct for testing multiple ratios or establish a universal market crossover.', '',
                  'Each engine uses a fresh 20,000-order prefill, 20,000 measured commands, 100 price levels per side, '
                  'uniform live-order cancellation targets, and 20% aggressive submissions. Occupancy evolves during the run. '
                  'Other book sizes, matching intensity, hardware, and cancellation-age distributions can move the boundary. '
                  'Short batch durations and host noise limit precision; use the raw repetitions rather than a single point estimate.', '',
                  'All measurements passed reference-event and final-state verification. '
                  'Individual engine/workload reports and raw repetitions are linked in [REPORT.md](REPORT.md). '
                  'The [coarse sweep](../crossover_coarse/REPORT.md) provided the initial search interval. '
                  'See [methodology](../../README.md) and [machine metadata](environment.json).', ''])
    (directory / 'CROSSOVER.md').write_text('\n'.join(lines))
    print('\n'.join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    gen = sub.add_parser('generate')
    gen.add_argument('--output', type=Path, required=True)
    gen.add_argument('--ratios', required=True, help='comma-separated percentages')
    gen.add_argument('--seeds', default='20261029,20261030,20261031')
    gen.add_argument('--repetitions', type=int, default=7)
    summary = sub.add_parser('summarize')
    summary.add_argument('results', type=Path)
    args = parser.parse_args()
    if args.command == 'generate':
        generate(args.output, [float(x) for x in args.ratios.split(',')],
                 [int(x) for x in args.seeds.split(',')], args.repetitions)
    else:
        summarize(args.results)


if __name__ == '__main__':
    main()
