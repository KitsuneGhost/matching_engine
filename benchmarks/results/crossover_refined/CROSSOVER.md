# Cancellation crossover experiment

Speedup = indexed-list throughput / deque throughput. Above 1 favors the indexed version. The metric is end-to-end batch replay throughput, including the same digest/loop overhead in both engines.

| Cancellation commands | Median paired speedup | 95% bootstrap interval | Classification | Seeds |
|---|---:|---:|---|---:|
| 0.5% | 0.976× | 0.858–1.040× | inconclusive | 3 |
| 0.6% | 1.087× | 0.944–1.115× | inconclusive | 3 |
| 0.7% | 1.125× | 1.036–1.194× | indexed wins | 3 |
| 0.8% | 1.243× | 1.144–1.296× | indexed wins | 3 |
| 1% | 1.375× | 1.237–1.441× | indexed wins | 3 |

**Smallest tested consistently winning ratio: 0.7%.**

The grid locates the first consistent win above 0.6% and at 0.7%. This is a resolution/noise bracket, not a proof of an exact mathematical threshold.

Classification requires the bootstrap interval to exclude 1 and every seed median to favor the same engine. Bootstrap resamples seeds, then paired repetitions within each seed (5,000 draws, fixed seed). Intervals describe only this finite experiment and are exploratory; they do not correct for testing multiple ratios or establish a universal market crossover.

Each engine uses a fresh 20,000-order prefill, 20,000 measured commands, 100 price levels per side, uniform live-order cancellation targets, and 20% aggressive submissions. Occupancy evolves during the run. Other book sizes, matching intensity, hardware, and cancellation-age distributions can move the boundary. Short batch durations and host noise limit precision; use the raw repetitions rather than a single point estimate.

All measurements passed reference-event and final-state verification. Individual engine/workload reports and raw repetitions are linked in [REPORT.md](REPORT.md). The [coarse sweep](../crossover_coarse/REPORT.md) provided the initial search interval. See [methodology](../../README.md) and [machine metadata](environment.json).
