# deque-baseline: lobster_amzn

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 48.33680484863671% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 129,783 commands/s
- Throughput range: 126,950–131,174 commands/s
- Submit p50 / p99: 261 / 584 ns
- Cancel p50 / p99: 14,255 / 41,313 ns
- Median process peak RSS: 22,236 KiB (includes harness)
- Active orders: min 16,521, max 20,000, final 16,521
- Executed trades: 4,106

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-lobster_amzn.json).
