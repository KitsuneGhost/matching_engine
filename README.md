# Toy Matching Engine

A small C++20 limit-order matching engine built to learn the core mechanics of an order book.

It accepts buy and sell limit orders, matches crossing orders, records trades, and keeps any unfilled quantity resting in the book.

> This is an educational toy project, not production trading infrastructure.

## Features

- Limit buy and sell orders
- Price-time priority
  - Buys match the lowest available ask at or below their limit price.
  - Sells match the highest available bid at or above their limit price.
  - Orders at the same price are matched first-in, first-out (FIFO).
- Partial fills and resting remainders
- One matching engine and its own order book per symbol; mismatched symbols are rejected
- Trader ownership on orders
- Trade records with symbol, buy/sell order IDs, buyer/seller trader IDs, quantity, and execution price
- Indexed order lookup and cancellation
- Input validation for invalid sides, zero quantity, and non-positive prices
- Dependency-free tests run through CTest

## Requirements

- A C++20-capable compiler
- CMake 3.20 or newer

## Build and run

From the repository root:

```bash
cmake -S . -B build
cmake --build build
./build/matching_engine
```

The demo creates an AAPL engine, submits sell orders from traders 11 and 22, then a crossing buy from trader 33. It prints each execution with its symbol, order IDs, and buyer/seller IDs, then displays the remaining order book.

## Run the tests

```bash
ctest --test-dir build --output-on-failure
```

Or run the test executable directly:

```bash
./build/matching_engine_tests
```

The executable runs 23 C++ cases covering matching priority, partial sweeps,
lookup cleanup, cancellation, identity propagation, symbol isolation, validation,
ID allocation, quantity aggregation and overflow, and the current self-trade policy.

CTest also runs a separate allocation-failure executable. It fails successive
allocations during resting submissions on both sides, at new and existing price
levels, and checks list/index consistency, cleanup, cancellation, and FIFO priority.
It tests insertion rollback; it does not establish submission-wide atomicity.

If Python 3 is available when CMake configures the project, CTest also runs
`matching_engine_differential`. It compares execution and complete order-state
digests against the independent Python reference model after each of 1,500
operations across three fixed seeds. This comparison covers one symbol and the
reference model's price/quantity/order-ID fields; trader identities are checked
by the C++ cases. No third-party Python packages are required. Without Python,
the C++ suite still builds and runs.

The existing benchmark-tool tests can be run separately:

```bash
python3 -m unittest discover -s benchmarks -p 'test_*.py'
```

To configure without tests:

```bash
cmake -S . -B build -DBUILD_TESTING=OFF
```

## Symbols and participants

**Each symbol has its own `MatchingEngine` instance and its own `OrderBook`.** Many traders submit to the same instrument's engine; books are not separated by trader.

```cpp
matching::MatchingEngine aapl{"AAPL"};
matching::MatchingEngine msft{"MSFT"};

auto sell = aapl.submit_limit_order(matching::Side::Sell, 100, "AAPL", 5, 11);
auto buy = aapl.submit_limit_order(matching::Side::Buy, 100, "AAPL", 2, 22);
// buy.trades contains 2 AAPL units at 100: buyer 22, seller 11.
```

Submission arguments are side, price, symbol, quantity, and trader ID. The engine rejects a request whose symbol differs from its configured symbol before changing the book or assigning an order ID. AAPL and MSFT orders never match across these independent engines. There is currently no exchange wrapper that registers or routes symbols automatically; callers create and select the appropriate engine.

Order IDs are local to each engine and can overlap across symbols. Use `(symbol, order_id)` when identifying an order across engines, and call lookup or cancellation on the corresponding engine. Trader IDs identify owners and appear on resting orders and both sides of execution records. Cancellation currently takes only an order ID and does not authenticate the caller or enforce ownership. Self-trades are not prevented.

The current submission validation requires exactly four characters in a symbol and rejects the maximum `TraderId` value. These are current implementation rules, not general exchange conventions. The constructor stores its symbol without validating it; configuring an engine with a symbol of another length results in rejected submissions.

## Matching rules

Prices are signed integers (think of them as integer ticks); quantities are unsigned integers.

Order IDs are assigned sequentially per engine instance. Usable IDs range from `1` through `std::numeric_limits<OrderId>::max() - 1` (inclusive). With the current `uint64_t` type, the last usable ID is `18446744073709551614`. Zero and the maximum value are never assigned. Once the ID space is exhausted, submissions return `Status::Error` without changing the book; IDs are not reused after fills or cancellations.

The number of available order IDs is finite, but exhausting them would take centuries even at a hypothetical rate of one billion new orders per second: `(2^64 - 2) / (1,000,000,000 × 60 × 60 × 24 × 365.25) ≈ 584.5 years`. This illustrates the ID capacity, not the engine's measured throughput.

An exhaustion rejection returns `SubmitResult` with `status == Status::Error` and `order_id == 0`, meaning no order was created. Callers should check `status` before using `order_id`. Invalid submissions also return `Status::Error`, an explanatory message, an order ID of zero, and no trades. Validation rejections do not modify the book or consume an order ID.

Price-level quantity queries and book printing throw `std::overflow_error` if a level's aggregate quantity exceeds the range of `Quantity`. Submissions currently allow such levels; overflow is detected when computing their totals.

An incoming buy order matches the best ask while that ask is less than or equal to the buy limit. An incoming sell order matches the best bid while that bid is greater than or equal to the sell limit. Trades execute at the resting order's price.

For example, if sells of `5 @ 100` and `4 @ 101` are resting, an incoming buy of `7 @ 101` produces:

```text
5 @ 100
2 @ 101
```

The remaining `2 @ 101` stays in the book.

## Order storage design

This version replaces the original per-price `std::deque<Order>` queues with `std::list<Order>` queues and adds an `std::unordered_map` index keyed by order ID. The bid and ask maps still sort price levels, and orders within each level still execute FIFO.

Each index entry stores a private `OrderLocation`: the order's side, price, and iterator into its owning list. Lookup dereferences that iterator and returns a copy of the order. Cancellation uses the side and price to find the owning list, then erases the order directly through the iterator.

Resting orders are indexed after insertion. Partial fills preserve their position and index entry; full fills remove the index entry before deleting the list node. Cancellation removes both the node and its index entry. Empty price levels are removed from the corresponding map. Fully filled incoming orders never enter the index.

| Operation | Cost |
|---|---|
| Lookup by order ID | Average `O(1)` |
| Cancel by order ID | Average `O(1)` index lookup plus `O(log P)` price-level lookup |
| Append to an existing level's list | `O(1)`, excluding map lookup and index insertion |
| Erase a list node using its iterator | `O(1)` |
| Query quantity at a price | `O(log P + K)` |

Here, `P` is the number of price levels on the relevant side and `K` is the number of orders at the queried level. Hash-table operations have worst-case linear complexity in the number of indexed orders.

Lists provide stable iterators and efficient arbitrary removal, at the cost of node overhead, per-node allocations, and poorer cache locality than deques. The [benchmark suite](benchmarks/README.md) compares committed versions on `deque-baseline` and `main` with 30%, 50%, and 70% cancellation-command workloads. See the [measured comparison and six individual reports](benchmarks/results/latest/REPORT.md) for results, workload definitions, and limitations; these synthetic measurements do not establish production capacity.

Additional experiments use [cancellation frequencies measured from LOBSTER samples](benchmarks/analysis/lobster/REPORT.md), with [per-engine calibrated results](benchmarks/results/lobster/REPORT.md). A separate [crossover experiment](benchmarks/results/crossover_refined/CROSSOVER.md) searches for the smallest tested cancellation ratio at which the indexed design consistently improves total throughput. Calibration currently covers cancellation frequency only; it is not historical market replay, and the crossover depends on the workload and hardware.

The [combined experiment report](benchmarks/STAGE1_REPORT.md) summarizes the LOBSTER calibration and crossover findings. These results describe the pinned historical engine revisions, before the symbol/trader API changes. The benchmark C++ harness still uses the earlier submission signature and needs adaptation before benchmarking the current engine; the existing reports do not measure the current checkout.

`OrderBook` is non-copyable, which also makes `MatchingEngine` non-copyable. A default copy would duplicate the lists while leaving copied index iterators pointing into the original book. Supporting copies would require rebuilding the index for the copied lists. Neither class currently supports moving.

Resting-order insertion rolls back its list node and any newly created empty price level if appending or indexing fails. Existing orders and their FIFO positions are preserved. Duplicate resting IDs are treated as a logic error and the attempted insertion is rolled back.

This is **insertion rollback, not submission-wide atomicity**. An exception later in a multi-fill submission can leave earlier executions applied without returning their trade records; even building the success response can allocate after the book changes. Order IDs may also be consumed by failed submissions. Treat any exception during submission as fatal to the session: stop and discard that session rather than catching it and continuing as if the order had been rejected. Ordinary validation failures returning `Status::Error` remain safe to continue after. Full prepare/commit handling for submissions is not implemented yet.

## Project layout

```text
include/    Public headers and domain types
src/        Matching engine and order-book implementation
app/        Small demo program
tests/      Dependency-free test executable
```

## Scope and limitations

Each engine supports one in-memory order book for its configured symbol and limit orders only. Multiple symbols require separate engine instances. The project does not yet provide an exchange routing layer, trader accounts, cash/inventory accounting, ownership-checked cancellation, market orders, persistence, networking, concurrency, risk checks, or exchange-grade performance characteristics.

## Version

Current toy release: **0.2.0**

See [the changelog](CHANGELOG.md) for release details and API changes.
