## v0.2.0 — Indexed Order Book and Trader Metadata

This release adds symbol-aware matching, trader metadata, indexed order lookup, and broader regression coverage.

### Highlights

- **Symbols and traders:** Each engine handles one configured symbol. Orders and trades include symbol and trader IDs.
- **Indexed order storage:** FIFO lists and an order ID index provide average constant-time lookup and efficient cancellation.
- **Book queries:** Access best bid/ask, individual orders, and quantity at a price level.
- **Stronger validation:** Improved handling of invalid inputs, order ID exhaustion, and quantity overflow.
- **Insertion rollback:** Failed resting-order insertions preserve existing orders and FIFO positions.
- **Expanded testing:** Regression, allocation-failure, and randomized differential tests.

### Breaking API changes

Construct the engine with a symbol and supply the symbol and trader ID when submitting orders:

```cpp
matching::MatchingEngine engine{"AAPL"};
auto result = engine.submit_limit_order(
    matching::Side::Buy, 100, "AAPL", 5, 33
);
```

### Known limitations

The engine remains an educational, in-memory limit-order matching engine. Submission-wide exception atomicity is not guaranteed; discard the session after a submission exception. Published benchmarks cover earlier revisions.

### Validation

All three CTest suites and seven benchmark-tool tests passed.
