# LOBSTER frequency calibration and cancellation crossover

Engine commits: deque `deque-baseline` = `0e66556cd827a073bd152f2b899b6cb526e03327`; indexed-list `main` = `843ed414da0873ae9d537ed795a93855d0295a77`.

## What the samples show

Analyzed three official LOBSTER level-10 samples for 2012-06-21. Cancellation share of all messages counts partial cancellations (type 2) and full deletions (type 3). The benchmark uses only full deletions divided by adds plus full deletions, because partial quantity reductions are not supported by the current engine.

| Sample | Cancellation events / all messages | Benchmark full-cancel command ratio | Indexed / deque throughput |
|---|---:|---:|---:|
| AAPL | 43.554% | 47.250% | 25.33× |
| AMZN | 46.849% | 48.335% | 26.95× |
| MSFT | 45.724% | 47.665% | 25.86× |

The indexed version improves overall throughput at all three calibrated frequencies. It still pays insertion/index-maintenance overhead; this does not mean it is faster for every operation.

**Scope:** these are cancellation-frequency-calibrated synthetic workloads, not historical replay. Order sizes, price placement, cancellation targets/ages, timestamps, initial occupancy, and matching intensity remain synthetic. The historical depth-filtered samples do not establish current market-wide behavior. See [event counts and provenance](analysis/lobster/REPORT.md) and [all six calibrated engine/workload reports](results/lobster/REPORT.md).

## Where the indexed design starts winning

The initial sweep tested 0%, 0.1%, 0.25%, 0.5%, 1%, 2%, and 5%. The refined sweep tested 0.5%, 0.6%, 0.7%, 0.8%, and 1% over three seeds, with seven paired repetitions per seed and ratio.

| Cancellation commands | Median paired speedup | Exploratory 95% bootstrap interval | Outcome |
|---|---:|---:|---|
| 0.5% | 0.976× | 0.858–1.040× | inconclusive |
| 0.6% | 1.087× | 0.944–1.115× | inconclusive |
| 0.7% | 1.125× | 1.036–1.194× | indexed wins |
| 0.8% | 1.243× | 1.144–1.296× | indexed wins |
| 1% | 1.375× | 1.237–1.441× | indexed wins |

**0.7% is the smallest tested consistently winning ratio in this experiment.** This is a measured grid result, not an exact universal threshold. Lower inconclusive points may still favor the indexed engine; the measurements do not establish that they reliably do.

The coarse 0%-cancellation control favors the deque implementation, confirming that the index has a cost when cancellation savings disappear. The refined search requires all seed medians to favor the same engine and an exploratory hierarchical-bootstrap interval excluding 1. The interval captures measured seed/repetition variation, not all possible market workloads, and is not corrected for multiple comparisons.

All workloads use 20,000 initial orders, 20,000 measured commands, 100 levels per side, and 20% aggressive submissions. The book evolves during measurement. Different occupancy, cancellation targeting, matching intensity, or hardware can change the crossover. Short batch durations and host noise limit precision.

See [crossover analysis](results/crossover_refined/CROSSOVER.md), [coarse measurements](results/crossover_coarse/REPORT.md), and [refined per-engine reports](results/crossover_refined/REPORT.md).

## Verification and reproduction

Both committed engine versions passed their existing CTest suites. Every untallied and measured replay matched the independent Python reference model for execution results and final live/absent orders. Latency passes are separate from batch throughput; the crossover uses batch mode only. Process peak RSS includes the harness and is not an engine-only memory measurement.

The [benchmark README](README.md) provides downloads, analysis commands, configs, test instructions, and exact rerun commands. Raw data ZIPs remain local under ignored `data/lobster/`; source URLs and SHA-256 hashes are retained in the analysis. Results retain immutable engine commits, trace hashes, CPU/compiler metadata, and every repetition.
