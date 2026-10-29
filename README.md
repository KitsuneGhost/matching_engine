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
- Trade records with buy ID, sell ID, quantity, and execution price
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

The demo submits two sell orders, then a crossing buy order, prints the generated trades, and displays the remaining order book.

## Run the tests

```bash
ctest --test-dir build --output-on-failure
```

Or run the test executable directly:

```bash
./build/matching_engine_tests
```

To configure without tests:

```bash
cmake -S . -B build -DBUILD_TESTING=OFF
```

## Matching rules

Prices are signed integers (think of them as integer ticks); quantities are unsigned integers.

Order IDs are assigned sequentially per engine instance. Usable IDs range from `1` through `std::numeric_limits<OrderId>::max() - 1` (inclusive). With the current `uint64_t` type, the last usable ID is `18446744073709551614`. Zero and the maximum value are never assigned. Once the ID space is exhausted, submissions return `Status::Error` without changing the book; IDs are not reused after fills or cancellations.

The number of available order IDs is finite, but exhausting them would take centuries even at a hypothetical rate of one billion new orders per second: `(2^64 - 2) / (1,000,000,000 × 60 × 60 × 24 × 365.25) ≈ 584.5 years`. This illustrates the ID capacity, not the engine's measured throughput.

An exhaustion rejection returns `SubmitResult` with `status == Status::Error` and `order_id == 0`, meaning no order was created. Callers should check `status` before using `order_id`. Invalid input throws `std::invalid_argument` instead of returning a result.

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

Lists provide stable iterators and efficient arbitrary removal, at the cost of node overhead, per-node allocations, and poorer cache locality than deques. These are design tradeoffs, not measured latency improvements; a workload-based benchmark comparison with the original deque version is still pending.

`OrderBook` is non-copyable, which also makes `MatchingEngine` non-copyable. A default copy would duplicate the lists while leaving copied index iterators pointing into the original book. Supporting copies would require rebuilding the index for the copied lists. Neither class currently supports moving.

Allocation-failure recovery remains unfinished: if index insertion throws after a list append, the appended order is not rolled back. The current implementation does not guarantee that the book and index remain synchronized after an allocation failure during submission.

## Project layout

```text
include/    Public headers and domain types
src/        Matching engine and order-book implementation
app/        Small demo program
tests/      Dependency-free test executable
```

## Scope and limitations

This first release intentionally supports one in-memory order book and limit orders only. It does not provide market orders, symbols/instruments, persistence, networking, concurrency, risk checks, or exchange-grade performance characteristics.

## Version

Current toy release: **0.1.0**
