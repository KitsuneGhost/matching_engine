# deque-baseline: cancel_30

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 30% of measured commands. 20,000 initial orders; 20,000 measured commands; 3 repetitions.

- Median throughput: 190,364 commands/s
- Throughput range: 177,823–191,270 commands/s
- Submit p50 / p99: 245 / 542 ns
- Cancel p50 / p99: 15,332 / 44,061 ns
- Median process peak RSS: 22,232 KiB (includes harness)
- Active orders: min 19,997, max 22,452, final 22,450
- Executed trades: 5,488

All runs matched reference event and final-state digests. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-cancel_30.json).
