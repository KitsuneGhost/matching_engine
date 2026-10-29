#!/usr/bin/env python3
"""Combine existing LOBSTER analysis and measured crossover results into a landing report."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).parent


def main():
    samples = json.loads((ROOT / 'analysis/lobster/analysis.json').read_text())
    cross = json.loads((ROOT / 'results/crossover_refined/CROSSOVER.json').read_text())
    env = json.loads((ROOT / 'results/lobster/environment.json').read_text())
    lines = ['# LOBSTER frequency calibration and cancellation crossover', '',
             f"Engine commits: deque `deque-baseline` = `{env['engines']['deque-baseline']}`; indexed-list `main` = "
             f"`{env['engines']['main']}`.", '',
             '## What the samples show', '',
             'Analyzed three official LOBSTER level-10 samples for 2012-06-21. '
             'Cancellation share of all messages counts partial cancellations (type 2) and full deletions (type 3). '
             'The benchmark uses only full deletions divided by adds plus full deletions, '
             'because partial quantity reductions are not supported by the current engine.', '',
             '| Sample | Cancellation events / all messages | Benchmark full-cancel command ratio | Indexed / deque throughput |',
             '|---|---:|---:|---:|']
    for sample in samples:
        symbol = sample['archive'].split('_')[2]
        name = f'lobster_{symbol.lower()}'
        a = json.loads((ROOT / f'results/lobster/deque-baseline-{name}.json').read_text())
        b = json.loads((ROOT / f'results/lobster/main-{name}.json').read_text())
        speedup = b['summary']['throughput_median_ops_s'] / a['summary']['throughput_median_ops_s']
        lines.append(f"| {symbol} | {sample['cancellation_event_percent']:.3f}% "
                     f"| {a['trace']['actual_cancel_percent']:.3f}% | {speedup:.2f}× |")
    lines.extend(['', 'The indexed version improves overall throughput at all three calibrated frequencies. '
                  'It still pays insertion/index-maintenance overhead; this does not mean it is faster for every operation.', '',
                  '**Scope:** these are cancellation-frequency-calibrated synthetic workloads, not historical replay. '
                  'Order sizes, price placement, cancellation targets/ages, timestamps, initial occupancy, and matching '
                  'intensity remain synthetic. The historical depth-filtered samples do not establish current market-wide behavior. '
                  'See [event counts and provenance](analysis/lobster/REPORT.md) and '
                  '[all six calibrated engine/workload reports](results/lobster/REPORT.md).', '',
                  '## Where the indexed design starts winning', '',
                  'The initial sweep tested 0%, 0.1%, 0.25%, 0.5%, 1%, 2%, and 5%. '
                  'The refined sweep tested 0.5%, 0.6%, 0.7%, 0.8%, and 1% over three seeds, '
                  'with seven paired repetitions per seed and ratio.', '',
                  '| Cancellation commands | Median paired speedup | Exploratory 95% bootstrap interval | Outcome |',
                  '|---|---:|---:|---|'])
    for r in cross['rows']:
        lo, hi = r['bootstrap_95_interval']
        lines.append(f"| {r['cancel_percent']:g}% | {r['median_speedup']:.3f}× | {lo:.3f}–{hi:.3f}× "
                     f"| {r['classification']} |")
    minimum = cross['smallest_tested_consistent_indexed_win_percent']
    lines.extend(['', f'**{minimum:g}% is the smallest tested consistently winning ratio in this experiment.** '
                  'This is a measured grid result, not an exact universal threshold. Lower inconclusive points may '
                  'still favor the indexed engine; the measurements do not establish that they reliably do.', '',
                  'The coarse 0%-cancellation control favors the deque implementation, confirming that the index '
                  'has a cost when cancellation savings disappear. The refined search requires all seed medians '
                  'to favor the same engine and an exploratory hierarchical-bootstrap interval excluding 1. '
                  'The interval captures measured seed/repetition variation, not all possible market workloads, '
                  'and is not corrected for multiple comparisons.', '',
                  'All workloads use 20,000 initial orders, 20,000 measured commands, 100 levels per side, '
                  'and 20% aggressive submissions. The book evolves during measurement. Different occupancy, '
                  'cancellation targeting, matching intensity, or hardware can change the crossover. '
                  'Short batch durations and host noise limit precision.', '',
                  'See [crossover analysis](results/crossover_refined/CROSSOVER.md), '
                  '[coarse measurements](results/crossover_coarse/REPORT.md), and '
                  '[refined per-engine reports](results/crossover_refined/REPORT.md).', '',
                  '## Verification and reproduction', '',
                  'Both committed engine versions passed their existing CTest suites. Every untallied and measured '
                  'replay matched the independent Python reference model for execution results and final live/absent orders. '
                  'Latency passes are separate from batch throughput; the crossover uses batch mode only. '
                  'Process peak RSS includes the harness and is not an engine-only memory measurement.', '',
                  'The [benchmark README](README.md) provides downloads, analysis commands, configs, test instructions, '
                  'and exact rerun commands. Raw data ZIPs remain local under ignored `data/lobster/`; source URLs '
                  'and SHA-256 hashes are retained in the analysis. Results retain immutable engine commits, '
                  'trace hashes, CPU/compiler metadata, and every repetition.', ''])
    (ROOT / 'STAGE1_REPORT.md').write_text('\n'.join(lines))
    manifest = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                for name in ('analyze_lobster.py', 'crossover.py', 'run.py', 'engine_bench.cpp', 'stage1_report.py')}
    (ROOT / 'analysis/lobster/tooling_sha256.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    main()
