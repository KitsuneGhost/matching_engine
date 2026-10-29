# deque-baseline: lobster_aapl

Commit: `0e66556cd827a073bd152f2b899b6cb526e03327`

Cancellation commands: 47.25397013870288% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 134,090 commands/s
- Throughput range: 132,891–138,006 commands/s
- Submit p50 / p99: 260 / 565 ns
- Cancel p50 / p99: 14,160 / 41,231 ns
- Median process peak RSS: 21,408 KiB (includes harness)
- Active orders: min 16,960, max 20,000, final 16,960
- Executed trades: 4,097

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./deque-baseline-lobster_aapl.json).
