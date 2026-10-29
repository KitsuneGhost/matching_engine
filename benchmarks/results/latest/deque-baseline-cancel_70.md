# deque-baseline: cancel_70

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 70% of measured commands. 20,000 initial orders; 20,000 measured commands; 3 repetitions.

- Median throughput: 113,906 commands/s
- Throughput range: 107,542–121,175 commands/s
- Submit p50 / p99: 257 / 470 ns
- Cancel p50 / p99: 11,193 / 29,257 ns
- Median process peak RSS: 22,924 KiB (includes harness)
- Active orders: min 9,654, max 20,000, final 9,654
- Executed trades: 2,322

All runs matched reference event and final-state digests. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-cancel_70.json).
