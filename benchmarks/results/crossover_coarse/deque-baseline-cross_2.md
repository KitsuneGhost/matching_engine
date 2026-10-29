# deque-baseline: cross_2

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 2% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 1,367,949 commands/s
- Throughput range: 1,317,995–1,370,814 commands/s
- Submit p50 / p99: N/A / N/A ns
- Cancel p50 / p99: N/A / N/A ns
- Median process peak RSS: 26,296 KiB (includes harness)
- Active orders: min 20,000, max 31,462, final 31,461
- Executed trades: 7,671

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-cross_2.json).
