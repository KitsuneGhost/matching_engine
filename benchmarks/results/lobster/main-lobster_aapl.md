# main: lobster_aapl

Commit: `843ed414da0873ae9d537ed795a93855d0295a77`

Cancellation commands: 47.25397013870288% of measured commands. 20,000 initial orders; 20,000 measured commands; 5 repetitions.

- Median throughput: 3,395,875 commands/s
- Throughput range: 3,332,726–3,693,843 commands/s
- Submit p50 / p99: 280 / 937 ns
- Cancel p50 / p99: 326 / 829 ns
- Median process peak RSS: 21,408 KiB (includes harness)
- Active orders: min 16,960, max 20,000, final 16,960
- Executed trades: 4,097

All runs matched reference event and final-state digests. N/A means not measured. Latency figures are medians of per-run percentiles, from separate instrumented passes. See [methodology](../../README.md) and [raw results](./main-lobster_aapl.json).
