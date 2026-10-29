# Engine comparison

Run: 2026-10-29T10:17:11.108773+00:00

CPU: 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz; logical CPU 0; compiler: c++ (GCC) 16.2.1 20260810.

| Cancel commands | Engine | Median commands/s | Submit p50/p99 ns | Cancel p50/p99 ns |
|---|---|---:|---:|---:|
| 0.5% | [deque-baseline](./deque-baseline-cross_0p5_seed_20261029.md) | 2,746,108 | N/A/N/A | N/A/N/A |
| 0.5% | [main](./main-cross_0p5_seed_20261029.md) | 2,655,728 | N/A/N/A | N/A/N/A |
| 0.5% | [deque-baseline](./deque-baseline-cross_0p5_seed_20261030.md) | 2,785,284 | N/A/N/A | N/A/N/A |
| 0.5% | [main](./main-cross_0p5_seed_20261030.md) | 2,503,473 | N/A/N/A | N/A/N/A |
| 0.5% | [deque-baseline](./deque-baseline-cross_0p5_seed_20261031.md) | 2,323,295 | N/A/N/A | N/A/N/A |
| 0.5% | [main](./main-cross_0p5_seed_20261031.md) | 2,693,240 | N/A/N/A | N/A/N/A |
| 0.6% | [deque-baseline](./deque-baseline-cross_0p6_seed_20261029.md) | 2,453,327 | N/A/N/A | N/A/N/A |
| 0.6% | [main](./main-cross_0p6_seed_20261029.md) | 2,617,314 | N/A/N/A | N/A/N/A |
| 0.6% | [deque-baseline](./deque-baseline-cross_0p6_seed_20261030.md) | 2,573,622 | N/A/N/A | N/A/N/A |
| 0.6% | [main](./main-cross_0p6_seed_20261030.md) | 2,713,819 | N/A/N/A | N/A/N/A |
| 0.6% | [deque-baseline](./deque-baseline-cross_0p6_seed_20261031.md) | 2,523,240 | N/A/N/A | N/A/N/A |
| 0.6% | [main](./main-cross_0p6_seed_20261031.md) | 2,826,136 | N/A/N/A | N/A/N/A |
| 0.7% | [deque-baseline](./deque-baseline-cross_0p7_seed_20261029.md) | 2,581,275 | N/A/N/A | N/A/N/A |
| 0.7% | [main](./main-cross_0p7_seed_20261029.md) | 2,674,644 | N/A/N/A | N/A/N/A |
| 0.7% | [deque-baseline](./deque-baseline-cross_0p7_seed_20261030.md) | 2,364,115 | N/A/N/A | N/A/N/A |
| 0.7% | [main](./main-cross_0p7_seed_20261030.md) | 2,711,615 | N/A/N/A | N/A/N/A |
| 0.7% | [deque-baseline](./deque-baseline-cross_0p7_seed_20261031.md) | 2,577,488 | N/A/N/A | N/A/N/A |
| 0.7% | [main](./main-cross_0p7_seed_20261031.md) | 2,899,575 | N/A/N/A | N/A/N/A |
| 0.8% | [deque-baseline](./deque-baseline-cross_0p8_seed_20261029.md) | 2,237,500 | N/A/N/A | N/A/N/A |
| 0.8% | [main](./main-cross_0p8_seed_20261029.md) | 2,810,093 | N/A/N/A | N/A/N/A |
| 0.8% | [deque-baseline](./deque-baseline-cross_0p8_seed_20261030.md) | 2,209,179 | N/A/N/A | N/A/N/A |
| 0.8% | [main](./main-cross_0p8_seed_20261030.md) | 2,723,603 | N/A/N/A | N/A/N/A |
| 0.8% | [deque-baseline](./deque-baseline-cross_0p8_seed_20261031.md) | 2,352,569 | N/A/N/A | N/A/N/A |
| 0.8% | [main](./main-cross_0p8_seed_20261031.md) | 2,730,156 | N/A/N/A | N/A/N/A |
| 1.0% | [deque-baseline](./deque-baseline-cross_1_seed_20261029.md) | 2,085,540 | N/A/N/A | N/A/N/A |
| 1.0% | [main](./main-cross_1_seed_20261029.md) | 2,631,436 | N/A/N/A | N/A/N/A |
| 1.0% | [deque-baseline](./deque-baseline-cross_1_seed_20261030.md) | 2,091,834 | N/A/N/A | N/A/N/A |
| 1.0% | [main](./main-cross_1_seed_20261030.md) | 2,756,921 | N/A/N/A | N/A/N/A |
| 1.0% | [deque-baseline](./deque-baseline-cross_1_seed_20261031.md) | 2,191,754 | N/A/N/A | N/A/N/A |
| 1.0% | [main](./main-cross_1_seed_20261031.md) | 2,964,436 | N/A/N/A | N/A/N/A |

## Observed throughput ratios

- 0.5% cancellations: indexed list / deque = **0.97×**.
- 0.5% cancellations: indexed list / deque = **0.90×**.
- 0.5% cancellations: indexed list / deque = **1.16×**.
- 0.6% cancellations: indexed list / deque = **1.07×**.
- 0.6% cancellations: indexed list / deque = **1.05×**.
- 0.6% cancellations: indexed list / deque = **1.12×**.
- 0.7% cancellations: indexed list / deque = **1.04×**.
- 0.7% cancellations: indexed list / deque = **1.15×**.
- 0.7% cancellations: indexed list / deque = **1.12×**.
- 0.8% cancellations: indexed list / deque = **1.26×**.
- 0.8% cancellations: indexed list / deque = **1.23×**.
- 0.8% cancellations: indexed list / deque = **1.16×**.
- 1.0% cancellations: indexed list / deque = **1.26×**.
- 1.0% cancellations: indexed list / deque = **1.32×**.
- 1.0% cancellations: indexed list / deque = **1.35×**.

These are synthetic, single-threaded results on this machine, not production capacity. Book occupancy evolves differently across cancellation ratios; compare engines within each workload rather than attributing cross-workload differences solely to the ratio. Peak RSS includes trace storage and the harness. Throughput includes result digesting and loop overhead; per-operation latency uses a separate pass with timer overhead. No engine-only memory or allocation measurements were made.

Both branch test suites passed. Every replay matched the independent Python reference model for execution results and final live/absent orders. See [methodology](../../README.md), [environment](./environment.json), and individual JSON files for configuration, immutable source commits, trace hashes, and all repetitions.
