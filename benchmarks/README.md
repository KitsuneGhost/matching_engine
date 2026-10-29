# Order-storage benchmark

Compare the deque/scanning engine on `deque-baseline` with the list/index engine on `main` using the same C++ harness and immutable Git snapshots. The runner never checks out or modifies either engine branch. It defaults to the exact measured source commits, not branch tips or uncommitted edits. Use `--baseline-ref` and `--candidate-ref` to select other Git refs; report labels remain `deque-baseline` and `main`.

## Run

Requires Linux, Python 3, Git, CMake, and a C++20 compiler; no Python packages are needed.

```bash
python3 -m unittest discover -s benchmarks -p 'test_*.py'
python3 benchmarks/run.py
```

Optional arguments:

```bash
python3 benchmarks/run.py --configs benchmarks/configs --output /tmp/engine-results --compiler c++
```

The default output is `results/latest/`. Re-running overwrites the same report filenames; choose a different output directory to preserve another experiment. Temporary snapshots, binaries, and traces are automatically removed. Reports retain full source commits, workload configuration, trace SHA-256 hashes, harness hashes, compiler, machine, and CPU affinity information. Keep this harness commit together with results to reproduce trace generation exactly. The first run is linked in [the comparison report](results/latest/REPORT.md).

## Workloads

The three JSON files in `configs/` specify **30%, 50%, and 70% cancellation commands out of all measured commands**, not the percentage of submitted orders eventually canceled. Every cancellation targets a uniformly sampled currently live order; failed cancellation requests are not part of these workloads.

Each workload starts with 20,000 non-crossing orders and then measures 20,000 shuffled commands. The remainder after cancellation commands consists of submissions. Submissions randomly select a side and quantity from 1 through 100. Of measured submissions, approximately 20% use aggressive crossing limits; the others use one of 100 bid prices (900–999) or 100 ask prices (1001–1100). Aggressive buy and sell limits are 1100 and 900 respectively. Aggressive selection is probabilistic, and execution depends on available liquidity. The traces exercise partial fills, full fills, sweeps, resting remainders, and cancellation on both sides. Prefill is not timed. Each workload uses seed `20261029`.

| Config | Measured cancellations | Measured submissions | Prefill |
|---|---:|---:|---:|
| `cancel_30.json` | 6,000 | 14,000 | 20,000 |
| `cancel_50.json` | 10,000 | 10,000 | 20,000 |
| `cancel_70.json` | 14,000 | 6,000 | 20,000 |

Book size is deliberately allowed to evolve: 70% successful cancellations cannot maintain a fixed-size book when only 30% of commands add orders. Prefill supplies sufficient live cancellation targets. Reports include minimum, maximum, and final live order counts. Both engines see exactly the same trace within each workload. Different ratios change book occupancy and matching activity, so they are not controlled experiments on cancellation ratio alone.

Increase `prefill_orders` and `measured_commands` to explore larger books and longer runs. The generator fails if live orders run out. `measured_commands` must be a positive multiple of 100. Other fields are `name`, `cancel_percent` (0–100, including fractional percentages), `seed`, `price_levels_per_side` (1–100), `max_quantity`, `aggressive_submit_percent` (0–100), and `repetitions` (at least 2). Cancellation counts round down to whole commands; JSON records `actual_cancel_percent`. A new scale or price distribution should have its own configuration and result directory. Use `--modes batch` to skip instrumented latency passes; reports mark those percentiles N/A.

## Correctness and timing

1. Export each branch's committed source into a temporary directory. Build in Release mode and run its existing CTest suite. Save the logs.
2. Compile both engines with the exact same harness and `-std=c++20 -O3 -DNDEBUG`, without LTO.
3. Generate traces outside the measured process with an independent Python price-time reference model. Build expected digests covering every submission ID, remaining quantity, ordered trade fields, cancellation outcome, and final presence/side/price/quantity for every assigned ID.
4. Run one untallied batch replay per engine/workload, verifying correctness and warming OS caches. Each measured replay uses a fresh process and fresh engine; the prefill also exercises its allocation and insertion paths.
5. Run three repetitions of each timing mode per engine/workload, alternating engine order between repetitions. Pin children to one available logical CPU. CPU frequency, interrupts, and other host load are not controlled; no processes run concurrently from this runner.
6. Fail the experiment if a cancellation fails or either output digest differs from the reference. Final state lookups, trace parsing, prefill, formatting, and teardown are outside the measured interval.

**Batch mode** uses one timer around the measured command loop. Commands/s includes engine calls, result digesting, result destruction, and loop overhead. It measures replay throughput, not pure matching-function throughput.

**Latency mode** runs separately, with a `steady_clock` timestamp immediately before and after each engine call. It excludes digesting and result destruction from each operation's sample but includes timer overhead. Submit latency includes all execution/report allocation work done inside the API call. Record nearest-rank p50/p99 separately for submissions and cancellations. Summary values are medians of per-run percentiles, not percentiles pooled across runs. Batch throughput is never calculated from instrumented latency passes.

Raw JSON retains every repetition and both digests. The summary gives median batch throughput and its min/max range, per-operation p50/p99, and median batch-process peak RSS. On Linux RSS is KiB and includes the harness, trace buffers, allocator retention, and process overhead; it is **not engine-only memory usage**. No claims about allocation counts or engine-only memory savings are made.

## Reports and interpretation

Each of the six engine/workload pairs gets Markdown and JSON reports. `REPORT.md` compares them; `environment.json` records machine/build metadata. Test logs are also retained.

These are synthetic single-threaded workloads. Repeat on an otherwise idle machine and increase duration/repetitions before drawing deployment conclusions, especially from p99 latency. Hash-table lookup is average O(1), not a worst-case constant-time guarantee. List cancellation also requires a price-map lookup. Faster cancellation does not imply faster submissions or better cache locality, and performance here is not a production exchange capacity claim.

## LOBSTER cancellation-frequency calibration

Start with the [combined experiment report](STAGE1_REPORT.md) for sample frequencies, measured speedups, and the crossover result. Regenerate it after rerunning the analysis and benchmarks with `python3 benchmarks/stage1_report.py`.

The [sample analysis](analysis/lobster/REPORT.md) counts all event types in the official AAPL, AMZN, and MSFT level-10 samples for 2012-06-21. Download ZIPs into the ignored `data/lobster/` directory; the analyzer reads message CSVs directly from each ZIP without extraction:

```bash
mkdir -p benchmarks/data/lobster
curl -fL https://php.lobsterdata.com/info/sample/LOBSTER_SampleFile_AAPL_2012-06-21_10.zip -o benchmarks/data/lobster/LOBSTER_SampleFile_AAPL_2012-06-21_10.zip
curl -fL https://php.lobsterdata.com/info/sample/LOBSTER_SampleFile_AMZN_2012-06-21_10.zip -o benchmarks/data/lobster/LOBSTER_SampleFile_AMZN_2012-06-21_10.zip
curl -fL https://php.lobsterdata.com/info/sample/LOBSTER_SampleFile_MSFT_2012-06-21_10.zip -o benchmarks/data/lobster/LOBSTER_SampleFile_MSFT_2012-06-21_10.zip
python3 benchmarks/analyze_lobster.py
python3 benchmarks/run.py --configs benchmarks/configs_lobster --output benchmarks/results/lobster
```

The analysis retains source URLs, ZIP/message hashes, event counts, sample time bounds, and multiple clearly defined cancellation ratios. Configurations use **full deletions / (new orders + full deletions)** because the current engine cannot reduce a live order's quantity. Partial cancellations are reported separately and excluded from this mapping. Execution messages are not interpreted as incoming orders. Only cancellation frequency is calibrated; sizes, cancellation targets/ages, prices, timing, matching intensity, and occupancy remain synthetic. These historical depth-filtered samples do not establish present-day market-wide behavior.

See the [six calibrated engine reports](results/lobster/REPORT.md). This experiment supplements the original controlled 30/50/70% workloads; it does not replace them or claim historical replay.

## Cancellation crossover search

The [coarse sweep](results/crossover_coarse/REPORT.md) uses 0%, 0.1%, 0.25%, 0.5%, 1%, 2%, and 5% cancellation commands, one seed, and five repetitions. Its configs are in `configs_crossover_coarse/`. A second experiment samples the initial boundary more closely across three seeds and seven paired repetitions:

```bash
python3 benchmarks/run.py --configs benchmarks/configs_crossover_coarse --output benchmarks/results/crossover_coarse --modes batch
python3 benchmarks/crossover.py generate --output benchmarks/configs_crossover_refined --ratios 0.5,0.6,0.7,0.8,1
python3 benchmarks/run.py --configs benchmarks/configs_crossover_refined --output benchmarks/results/crossover_refined --modes batch
python3 benchmarks/crossover.py summarize benchmarks/results/crossover_refined
```

The [crossover report](results/crossover_refined/CROSSOVER.md) gives the smallest **tested consistently winning** cancellation percentage. It pairs engines by seed and repetition and uses an exploratory hierarchical bootstrap (resampling seeds and paired repetitions). A consistent winner requires every seed median to favor the same engine and the 95% bootstrap interval to exclude equal performance. Inconclusive points remain inconclusive; the search does not assume that noisy measurements are monotonic. This is a grid/noise bracket, not an exact or universal threshold. Book size, matching intensity, cancellation distribution, compiler, and hardware can move it. Throughput, rather than cancellation-call latency alone, defines the winner.
