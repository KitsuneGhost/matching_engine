# Engine comparison

Run: 2026-10-29T09:59:42.510498+00:00

CPU: 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz; logical CPU 0; compiler: c++ (GCC) 16.2.1 20260810.

| Cancel commands | Engine | Median commands/s | Submit p50/p99 ns | Cancel p50/p99 ns |
|---|---|---:|---:|---:|
| 30% | [deque-baseline](./deque-baseline-cancel_30.md) | 190,364 | 245/542 | 15,332/44,061 |
| 30% | [main](./main-cancel_30.md) | 2,872,257 | 264/1,014 | 318/790 |
| 50% | [deque-baseline](./deque-baseline-cancel_50.md) | 138,032 | 249/489 | 13,478/31,572 |
| 50% | [main](./main-cancel_50.md) | 3,647,512 | 274/878 | 301/696 |
| 70% | [deque-baseline](./deque-baseline-cancel_70.md) | 113,906 | 257/470 | 11,193/29,257 |
| 70% | [main](./main-cancel_70.md) | 3,535,908 | 281/896 | 325/668 |

## Observed throughput ratios

- 30% cancellations: indexed list / deque = **15.09×**.
- 50% cancellations: indexed list / deque = **26.43×**.
- 70% cancellations: indexed list / deque = **31.04×**.

These are synthetic, single-threaded results on this machine, not production capacity. Book occupancy evolves differently across cancellation ratios; compare engines within each workload rather than attributing cross-workload differences solely to the ratio. Peak RSS includes trace storage and the harness. Throughput includes result digesting and loop overhead; per-operation latency uses a separate pass with timer overhead. No engine-only memory or allocation measurements were made.

Both branch test suites passed. Every replay matched the independent Python reference model for execution results and final live/absent orders. See [methodology](../../README.md), [environment](./environment.json), and individual JSON files for configuration, immutable source commits, trace hashes, and all repetitions.
