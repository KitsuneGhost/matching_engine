# deque-baseline: cross_0p1

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 0.1% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 4,265,802 commands/s
- Throughput range: 4,098,647–4,351,647 commands/s
- Submit p50 / p99: N/A / N/A ns
- Cancel p50 / p99: N/A / N/A ns
- Median process peak RSS: 24,096 KiB (includes harness)
- Active orders: min 20,000, max 32,128, final 32,127
- Executed trades: 7,768

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-cross_0p1.json).
