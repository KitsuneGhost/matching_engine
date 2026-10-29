# Engine comparison

Run: 2026-10-29T10:14:25.747834+00:00

CPU: 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz; logical CPU 0; compiler: c++ (GCC) 16.2.1 20260810.

| Cancel commands | Engine | Median commands/s | Submit p50/p99 ns | Cancel p50/p99 ns |
|---|---|---:|---:|---:|
| 0% | [deque-baseline](./deque-baseline-cross_0.md) | 4,791,724 | N/A/N/A | N/A/N/A |
| 0% | [main](./main-cross_0.md) | 2,764,311 | N/A/N/A | N/A/N/A |
| 0.1% | [deque-baseline](./deque-baseline-cross_0p1.md) | 4,265,802 | N/A/N/A | N/A/N/A |
| 0.1% | [main](./main-cross_0p1.md) | 2,852,413 | N/A/N/A | N/A/N/A |
| 0.25% | [deque-baseline](./deque-baseline-cross_0p25.md) | 3,204,202 | N/A/N/A | N/A/N/A |
| 0.25% | [main](./main-cross_0p25.md) | 2,560,353 | N/A/N/A | N/A/N/A |
| 0.5% | [deque-baseline](./deque-baseline-cross_0p5.md) | 2,833,834 | N/A/N/A | N/A/N/A |
| 0.5% | [main](./main-cross_0p5.md) | 2,713,317 | N/A/N/A | N/A/N/A |
| 1% | [deque-baseline](./deque-baseline-cross_1.md) | 2,064,647 | N/A/N/A | N/A/N/A |
| 1% | [main](./main-cross_1.md) | 2,747,574 | N/A/N/A | N/A/N/A |
| 2% | [deque-baseline](./deque-baseline-cross_2.md) | 1,367,949 | N/A/N/A | N/A/N/A |
| 2% | [main](./main-cross_2.md) | 2,751,741 | N/A/N/A | N/A/N/A |
| 5% | [deque-baseline](./deque-baseline-cross_5.md) | 710,069 | N/A/N/A | N/A/N/A |
| 5% | [main](./main-cross_5.md) | 2,863,618 | N/A/N/A | N/A/N/A |

## Observed throughput ratios

- 0% cancellations: indexed list / deque = **0.58×**.
- 0.1% cancellations: indexed list / deque = **0.67×**.
- 0.25% cancellations: indexed list / deque = **0.80×**.
- 0.5% cancellations: indexed list / deque = **0.96×**.
- 1% cancellations: indexed list / deque = **1.33×**.
- 2% cancellations: indexed list / deque = **2.01×**.
- 5% cancellations: indexed list / deque = **4.03×**.

These are synthetic, single-threaded results on this machine, not production capacity. Book occupancy evolves differently across cancellation ratios; compare engines within each workload rather than attributing cross-workload differences solely to the ratio. Peak RSS includes trace storage and the harness. Throughput includes result digesting and loop overhead; per-operation latency uses a separate pass with timer overhead. No engine-only memory or allocation measurements were made.

Both branch test suites passed. Every replay matched the independent Python reference model for execution results and final live/absent orders. See [methodology](../../README.md), [environment](./environment.json), and individual JSON files for configuration, immutable source commits, trace hashes, and all repetitions.
