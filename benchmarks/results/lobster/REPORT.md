# Engine comparison

Run: 2026-10-29T10:15:58.764274+00:00

CPU: 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz; logical CPU 0; compiler: c++ (GCC) 16.2.1 20260810.

| Cancel commands | Engine | Median commands/s | Submit p50/p99 ns | Cancel p50/p99 ns |
|---|---|---:|---:|---:|
| 47.25397013870288% | [deque-baseline](./deque-baseline-lobster_aapl.md) | 134,090 | 260/565 | 14,160/41,231 |
| 47.25397013870288% | [main](./main-lobster_aapl.md) | 3,395,875 | 280/937 | 326/829 |
| 48.33680484863671% | [deque-baseline](./deque-baseline-lobster_amzn.md) | 129,783 | 261/584 | 14,255/41,313 |
| 48.33680484863671% | [main](./main-lobster_amzn.md) | 3,497,742 | 285/888 | 322/706 |
| 47.667086410617564% | [deque-baseline](./deque-baseline-lobster_msft.md) | 128,248 | 266/535 | 14,634/40,995 |
| 47.667086410617564% | [main](./main-lobster_msft.md) | 3,315,974 | 288/947 | 329/793 |

## Observed throughput ratios

- 47.25397013870288% cancellations: indexed list / deque = **25.33×**.
- 48.33680484863671% cancellations: indexed list / deque = **26.95×**.
- 47.667086410617564% cancellations: indexed list / deque = **25.86×**.

These are synthetic, single-threaded results on this machine, not production capacity. Book occupancy evolves differently across cancellation ratios; compare engines within each workload rather than attributing cross-workload differences solely to the ratio. Peak RSS includes trace storage and the harness. Throughput includes result digesting and loop overhead; per-operation latency uses a separate pass with timer overhead. No engine-only memory or allocation measurements were made.

Both branch test suites passed. Every replay matched the independent Python reference model for execution results and final live/absent orders. See [methodology](../../README.md), [environment](./environment.json), and individual JSON files for configuration, immutable source commits, trace hashes, and all repetitions.
