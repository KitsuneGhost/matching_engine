# main: lobster_amzn

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 48.33680484863671% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 3,497,742 commands/s
- Throughput range: 2,831,697–3,641,870 commands/s
- Submit p50 / p99: 285 / 888 ns
- Cancel p50 / p99: 322 / 706 ns
- Median process peak RSS: 22,236 KiB (includes harness)
- Active orders: min 16,521, max 20,000, final 16,521
- Executed trades: 4,106

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-lobster_amzn.json).
