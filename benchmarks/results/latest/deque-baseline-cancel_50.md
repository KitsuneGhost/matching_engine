# deque-baseline: cancel_50

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 50% of measured commands. 20,000 initial orders; 20,000 measured commands; 3 repetitions.

- Median throughput: 138,032 commands/s
- Throughput range: 137,674–141,361 commands/s
- Submit p50 / p99: 249 / 489 ns
- Cancel p50 / p99: 13,478 / 31,572 ns
- Median process peak RSS: 22,912 KiB (includes harness)
- Active orders: min 15,902, max 20,000, final 15,902
- Executed trades: 4,056

All runs matched reference event and final-state digests. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-cancel_50.json).
