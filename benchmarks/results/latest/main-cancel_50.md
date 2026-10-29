# main: cancel_50

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 50% of measured commands. 20,000 initial orders; 20,000 measured commands; 3 repetitions.

- Median throughput: 3,647,512 commands/s
- Throughput range: 3,576,656–3,740,743 commands/s
- Submit p50 / p99: 274 / 878 ns
- Cancel p50 / p99: 301 / 696 ns
- Median process peak RSS: 22,912 KiB (includes harness)
- Active orders: min 15,902, max 20,000, final 15,902
- Executed trades: 4,056

All runs matched reference event and final-state digests. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-cancel_50.json).
