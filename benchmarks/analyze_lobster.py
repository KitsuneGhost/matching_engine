#!/usr/bin/env python3
"""Analyze local official LOBSTER sample ZIPs and emit frequency-calibrated configs."""
import argparse
import collections
import csv
from decimal import Decimal
import hashlib
import io
import json
from pathlib import Path
import statistics
import zipfile

BASE_URL = 'https://php.lobsterdata.com/info/sample/'
TYPES = {1: 'submission', 2: 'partial cancellation', 3: 'full deletion',
         4: 'visible execution', 5: 'hidden execution', 6: 'cross trade', 7: 'halt'}


def analyze(path):
    counts = collections.Counter()
    sizes = collections.defaultdict(list)
    sides = collections.Counter()
    first = last = None
    submitted = set()
    canceled_submitted = set()
    with zipfile.ZipFile(path) as archive:
        messages = [n for n in archive.namelist() if 'message' in n and n.endswith('.csv')]
        if len(messages) != 1:
            raise ValueError(f'expected one message file: {path}')
        member = messages[0]
        payload = archive.read(member)
    for row in csv.reader(io.StringIO(payload.decode())):
        if len(row) != 6:
            raise ValueError(f'invalid LOBSTER row: {row}')
        time = Decimal(row[0])
        kind, oid, size, price, side = map(int, row[1:])
        if kind not in TYPES or size < 0 or (last is not None and time < last):
            raise ValueError(f'invalid event: {row}')
        if first is None:
            first = time
        last = time
        counts[kind] += 1
        sizes[kind].append(size)
        if kind == 1:
            submitted.add(oid)
            sides[side] += 1
        if kind in (2, 3) and oid in submitted:
            canceled_submitted.add(oid)
    if not counts[1] or not counts[3]:
        raise ValueError('sample has no submissions or full deletions')
    total = sum(counts.values())
    return dict(archive=path.name, source_url=BASE_URL + path.name,
                archive_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                message_file=member, message_sha256=hashlib.sha256(payload).hexdigest(),
                first_time_seconds=str(first), last_time_seconds=str(last), total_events=total,
                counts={str(k): counts[k] for k in TYPES},
                event_percent={str(k): 100 * counts[k] / total for k in TYPES},
                cancellation_event_percent=100 * (counts[2] + counts[3]) / total,
                full_delete_supported_command_percent=100 * counts[3] / (counts[1] + counts[3]),
                combined_cancel_add_command_percent=100 * (counts[2] + counts[3]) / (counts[1] + counts[2] + counts[3]),
                observed_submissions=len(submitted), observed_submissions_later_canceled=len(canceled_submitted),
                observed_submissions_with_later_cancel_percent=100 * len(canceled_submitted) / len(submitted),
                submission_size_median=statistics.median(sizes[1]),
                submission_buy_percent=100 * sides[1] / counts[1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, default=Path(__file__).parent / 'data/lobster')
    parser.add_argument('--output', type=Path, default=Path(__file__).parent / 'analysis/lobster')
    parser.add_argument('--configs', type=Path, default=Path(__file__).parent / 'configs_lobster')
    args = parser.parse_args()
    samples = [analyze(p) for p in sorted(args.data.glob('*.zip'))]
    if not samples:
        raise ValueError('no sample ZIPs; see README download instructions')
    args.output.mkdir(parents=True, exist_ok=True)
    args.configs.mkdir(parents=True, exist_ok=True)
    (args.output / 'analysis.json').write_text(json.dumps(samples, indent=2) + '\n')
    lines = ['# LOBSTER sample cancellation analysis', '',
             'Official level-10 NASDAQ samples, 2012-06-21. These depth-filtered historical samples '
             'are not a claim about the whole market or current trading.', '',
             '| Symbol | Events | New orders (1) | Partial cancels (2) | Full deletes (3) | Cancels/all events | Full deletes/(adds + full deletes) |',
             '|---|---:|---:|---:|---:|---:|---:|']
    for sample in samples:
        symbol = sample['archive'].split('_')[2]
        counts = sample['counts']
        lines.append(f"| [{symbol}]({sample['source_url']}) | {sample['total_events']:,} | {counts['1']:,} "
                     f"| {counts['2']:,} | {counts['3']:,} | {sample['cancellation_event_percent']:.3f}% "
                     f"| {sample['full_delete_supported_command_percent']:.3f}% |")
        config = dict(name=f'lobster_{symbol.lower()}', cancel_percent=sample['full_delete_supported_command_percent'],
                      seed=20261029, prefill_orders=20000, measured_commands=20000,
                      price_levels_per_side=100, max_quantity=100, aggressive_submit_percent=20,
                      repetitions=5, calibration=dict(source_url=sample['source_url'],
                      message_sha256=sample['message_sha256'], formula='type3 / (type1 + type3)',
                      scope='Cancellation frequency only; other synthetic generator parameters unchanged.'))
        (args.configs / f"{config['name']}.json").write_text(json.dumps(config, indent=2) + '\n')
    lines.extend(['', '## Interpretation', '',
                  '- Cancellation event share counts types 2 + 3 over all messages, including executions and other events.',
                  '- Benchmark command share uses type 3 / (type 1 + type 3). The engine supports new orders and full cancels; '
                  'partial cancellation (type 2) is excluded rather than incorrectly converted to a full deletion. '
                  'Execution messages are outcomes, not incoming matching commands. This renormalization changes the denominator.',
                  '- JSON also records (types 2 + 3) / (types 1 + 2 + 3), all event counts, file hashes, time bounds, and '
                  'the share of observed submissions with a later observed cancellation. The latter is censored by the sample '
                  'window and depth filtering; it is not the eventual lifetime cancellation probability.',
                  '- Only cancellation frequency is calibrated. Order sizes, price placement, aggressiveness (20% of submissions), '
                  'uniform live-order cancellation selection, initial occupancy, and timing remain synthetic. No historical replay '
                  'or order-lifetime calibration is claimed. Fractional requested ratios are rounded down to an integer command count; '
                  'benchmark JSON records the actual ratio.', '',
                  'Format source: [LOBSTER output specification](https://php.lobsterdata.com/info/DataStructure.php). '
                  'Samples: [official downloads](https://php.lobsterdata.com/info/DataSamples.php).', ''])
    (args.output / 'REPORT.md').write_text('\n'.join(lines))
    print('\n'.join(lines))


if __name__ == '__main__':
    main()
