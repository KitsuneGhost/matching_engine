# main: lobster_msft

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 47.667086410617564% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 3,315,974 commands/s
- Throughput range: 3,293,617–3,350,901 commands/s
- Submit p50 / p99: 288 / 947 ns
- Cancel p50 / p99: 329 / 793 ns
- Median process peak RSS: 22,388 KiB (includes harness)
- Active orders: min 16,726, max 20,000, final 16,729
- Executed trades: 4,148

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-lobster_msft.json).
