# Changelog

## 0.2.0 — 2026-11-13

### Changes

- Added a configured symbol per engine, symbol validation on submissions, and symbol and trader metadata on orders and trades.
- Replaced per-price deque storage with FIFO lists and an order ID index for average constant-time order lookup and direct list-node removal during cancellation.
- Exposed best bid/ask, order lookup, and price-level quantity queries through the engine.
- Hardened input validation, order ID exhaustion handling, and aggregate quantity overflow detection.
- Added rollback for failed resting-order insertion, preserving existing orders and FIFO positions.
- Expanded regression tests, isolated allocation-failure tests, and randomized differential testing against a reference model.
- Added reproducible benchmark tooling and reports for pinned historical revisions, including cancellation-frequency and crossover experiments.

### API changes

Construct engines with a symbol, for example `MatchingEngine engine{"AAPL"}`.
Submit orders with `submit_limit_order(side, price, symbol, quantity, traderId)`.
Trades include the symbol and buyer/seller trader IDs. `OrderBook` and
`MatchingEngine` do not support copying or moving.

### Known limitations

- This remains a single-symbol, in-memory limit-order engine. Routing, accounts, persistence, networking, concurrency, and risk checks are not implemented.
- Insertion rollback does not provide submission-wide atomicity. An exception during submission can leave earlier fills applied; discard the engine session after such an exception.
- Published benchmark results describe historical revisions before the symbol/trader API changes. The C++ benchmark harness requires adaptation to benchmark this release.

## 0.1.0

Initial demo release, tagged `demo-release`.
