# deque-baseline: lobster_msft

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 47.667086410617564% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 128,248 commands/s
- Throughput range: 122,605–130,531 commands/s
- Submit p50 / p99: 266 / 535 ns
- Cancel p50 / p99: 14,634 / 40,995 ns
- Median process peak RSS: 22,388 KiB (includes harness)
- Active orders: min 16,726, max 20,000, final 16,729
- Executed trades: 4,148

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-lobster_msft.json).
