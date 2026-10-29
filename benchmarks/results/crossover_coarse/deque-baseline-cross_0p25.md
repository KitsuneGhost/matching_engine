# deque-baseline: cross_0p25

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 0.25% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 3,204,202 commands/s
- Throughput range: 2,790,059–3,380,911 commands/s
- Submit p50 / p99: N/A / N/A ns
- Cancel p50 / p99: N/A / N/A ns
- Median process peak RSS: 25,364 KiB (includes harness)
- Active orders: min 20,000, max 32,091, final 32,091
- Executed trades: 7,735

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-cross_0p25.json).
