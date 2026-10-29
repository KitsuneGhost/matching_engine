# LOBSTER sample cancellation analysis

Official level-10 NASDAQ samples, 2012-06-21. These depth-filtered historical samples are not a claim about the whole market or current trading.

| Symbol | Events | New orders (1) | Partial cancels (2) | Full deletes (3) | Cancels/all events | Full deletes/(adds + full deletes) |
|---|---:|---:|---:|---:|---:|---:|
| [AAPL](https://php.lobsterdata.com/info/sample/LOBSTER_SampleFile_AAPL_2012-06-21_10.zip) | 400,391 | 191,015 | 3,260 | 171,126 | 43.554% | 47.254% |
| [AMZN](https://php.lobsterdata.com/info/sample/LOBSTER_SampleFile_AMZN_2012-06-21_10.zip) | 269,748 | 131,954 | 2,917 | 123,458 | 46.849% | 48.337% |
| [MSFT](https://php.lobsterdata.com/info/sample/LOBSTER_SampleFile_MSFT_2012-06-21_10.zip) | 668,765 | 329,566 | 5,602 | 300,183 | 45.724% | 47.667% |

## Interpretation

- Cancellation event share counts types 2 + 3 over all messages, including executions and other events.
- Benchmark command share uses type 3 / (type 1 + type 3). The engine supports new orders and full cancels; partial cancellation (type 2) is excluded rather than incorrectly converted to a full deletion. Execution messages are outcomes, not incoming matching commands. This renormalization changes the denominator.
- JSON also records (types 2 + 3) / (types 1 + 2 + 3), all event counts, file hashes, time bounds, and the share of observed submissions with a later observed cancellation. The latter is censored by the sample window and depth filtering; it is not the eventual lifetime cancellation probability.
- Only cancellation frequency is calibrated. Order sizes, price placement, aggressiveness (20% of submissions), uniform live-order cancellation selection, initial occupancy, and timing remain synthetic. No historical replay or order-lifetime calibration is claimed. Fractional requested ratios are rounded down to an integer command count; benchmark JSON records the actual ratio.

Format source: [LOBSTER output specification](https://php.lobsterdata.com/info/DataStructure.php). Samples: [official downloads](https://php.lobsterdata.com/info/DataSamples.php).
