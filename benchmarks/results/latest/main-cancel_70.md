# main: cancel_70

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 70% of measured commands. 20,000 initial orders; 20,000 measured commands; 3 repetitions.

- Median throughput: 3,535,908 commands/s
- Throughput range: 3,504,132–3,673,844 commands/s
- Submit p50 / p99: 281 / 896 ns
- Cancel p50 / p99: 325 / 668 ns
- Median process peak RSS: 22,924 KiB (includes harness)
- Active orders: min 9,654, max 20,000, final 9,654
- Executed trades: 2,322

All runs matched reference event and final-state digests. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-cancel_70.json).
