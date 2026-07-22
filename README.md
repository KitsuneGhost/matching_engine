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
- Order cancellation
- Input validation for zero quantity and non-positive prices
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

An incoming buy order matches the best ask while that ask is less than or equal to the buy limit. An incoming sell order matches the best bid while that bid is greater than or equal to the sell limit. Trades execute at the resting order's price.

For example, if sells of `5 @ 100` and `4 @ 101` are resting, an incoming buy of `7 @ 101` produces:

```text
5 @ 100
2 @ 101
```

The remaining `2 @ 101` stays in the book.

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
