# main: cross_0p5

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 0.5% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 2,713,317 commands/s
- Throughput range: 2,497,917–2,789,725 commands/s
- Submit p50 / p99: N/A / N/A ns
- Cancel p50 / p99: N/A / N/A ns
- Median process peak RSS: 25,976 KiB (includes harness)
- Active orders: min 20,000, max 31,946, final 31,946
- Executed trades: 7,772

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-cross_0p5.json).
