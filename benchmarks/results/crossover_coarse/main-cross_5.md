# main: cross_5

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 5% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 2,863,618 commands/s
- Throughput range: 2,422,066–2,948,423 commands/s
- Submit p50 / p99: N/A / N/A ns
- Cancel p50 / p99: N/A / N/A ns
- Median process peak RSS: 26,832 KiB (includes harness)
- Active orders: min 20,000, max 30,538, final 30,536
- Executed trades: 7,382

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-cross_5.json).
