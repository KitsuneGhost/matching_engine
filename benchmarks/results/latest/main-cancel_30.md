# main: cancel_30

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 30% of measured commands. 20,000 initial orders; 20,000 measured commands; 3 repetitions.

- Median throughput: 2,872,257 commands/s
- Throughput range: 2,842,984–3,277,886 commands/s
- Submit p50 / p99: 264 / 1,014 ns
- Cancel p50 / p99: 318 / 790 ns
- Median process peak RSS: 22,232 KiB (includes harness)
- Active orders: min 19,997, max 22,452, final 22,450
- Executed trades: 5,488

All runs matched reference event and final-state digests. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-cancel_30.json).
