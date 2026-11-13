# Roadmap: using this project with Alpaca

> Deferred integration track. Start with [ROADMAP.md](ROADMAP.md), which explains
> the current priority: deterministic historical replay, engine foundations,
> and execution simulation. This file preserves the later Alpaca implementation plan.

Initial scope: one Alpaca paper account, US equities, whole-share limit orders,
`day` time in force, regular trading hours, and no short selling. Expand only
after order recovery works. This is an implementation plan, not a trading strategy.

## Architecture decision

Alpaca executes broker orders; your `MatchingEngine` executes local orders.
Keep these as separate execution backends. An order routed to Alpaca must not
also pass through `OrderBook::add_order()` and generate a local fill.

```text
Strategy / CLI -> Order manager -> Risk checks -> Execution backend
                                               |-- Local simulator
                                               `-- Alpaca adapter
Alpaca trade_updates / REST reconciliation -> Order manager -> Account state
Market-data stream -> Quote cache -> Strategy / Risk checks
```

Your current engine already supplies price-time matching, partial fills,
indexed cancellation, per-symbol isolation, and useful correctness tests.
The missing integration work is mostly around it, rather than inside its
matching algorithm.

## 1. Define broker-facing types and boundaries

- [ ] Add `OrderRequest`, `OrderState`, `ExecutionReport`, and `BrokerOrderId`.
      Keep local numeric order IDs, persistent client IDs, and Alpaca's string
      order IDs distinct. Scope mappings to account and environment.
- [ ] Define an execution interface for submit, cancel, query, and updates.
      Returning a submission acknowledgement must not imply a fill.
- [ ] Keep the current `Trade` for simulated trades. Broker execution reports
      need your order ID, fill quantity/price, timestamps, and execution identity;
      they cannot require the counterparty's order or trader IDs.
- [ ] Define a fixed-point price scale and checked decimal-string conversion.
      `Price` is currently an unscaled integer: explicitly define how `100`
      becomes a dollar price. Validate permitted increments without silently
      rounding. Avoid floating-point money calculations.
- [ ] Keep whole-share submission initially. Handle decimal broker fields
      explicitly; reject unsupported fractional values instead of truncating.
- [ ] Replace the four-character symbol restriction in
      `src/matching_engine.cpp` with a documented simulator policy. Broker
      eligibility should come from Alpaca asset metadata, not symbol length.

**Done when:** a fake asynchronous backend can acknowledge, partially fill,
fill, and reject an order without invoking local matching. Conversion tests
cover precision, overflow, invalid values, and symbols such as `F` and `GOOGL`.

## 2. Add paper REST connectivity

- [ ] Add a separate CMake adapter target with HTTPS and JSON dependencies
      (for example libcurl and nlohmann/json). Keep the core dependency-free.
- [ ] Load credentials from environment/configuration; redact them from logs.
      Default to `https://paper-api.alpaca.markets`. Require explicit live mode
      and matching live credentials before accepting a live endpoint.
- [ ] Implement account, assets, positions, clock/calendar, submit order,
      get order by broker/client ID, list orders, and cancel order operations.
- [ ] Validate account trading status, asset tradability, session eligibility,
      and supported order fields before enabling submission.
- [ ] Classify authentication errors, validation rejections, throttling,
      transport failures, and ambiguous outcomes. Add bounded retries/backoff
      for safe reads; do not blindly repeat order submissions.

**Done when:** a manual paper CLI can inspect account state, submit a limit
order with a unique client ID, query it, request cancellation, and verify its
final state. A cancel response alone is not proof of cancellation.

## 3. Implement asynchronous order lifecycle

- [ ] Add trading WebSocket authentication and confirmed `trade_updates`
      subscription. Support the paper stream's binary frames.
- [ ] Model pending submission, accepted/new, partially filled, filled,
      pending cancellation, canceled, expired, rejected, and unknown/reconciling
      states. Preserve unfamiliar events and trigger reconciliation.
- [ ] Track requested, cumulative filled, and remaining quantity separately.
      Distinguish per-event execution price from cumulative average fill price.
- [ ] Process duplicate and delayed events without double-counting fills or
      regressing terminal state. Handle a WebSocket update arriving before
      the corresponding REST response.
- [ ] Handle cancel/fill races: cancellation is a request, and further fills
      can arrive before it completes.
- [ ] Use one serialized owner for order/account state, with a bounded queue
      from network callbacks. Keep network work outside the matching core.

**Done when:** fixture-driven tests cover partial fills, rejection, duplicate
updates, update-before-response, late fills, and cancel/fill races.

## 4. Persist intent and recover safely

- [ ] Persist client IDs and outbound intent before sending orders; persist
      broker IDs, executions, and state changes. SQLite is a reasonable start.
- [ ] On a submission timeout, query by the original client ID and reconcile.
      Do not generate a new ID and resubmit while acceptance is uncertain.
- [ ] At startup/reconnect, buffer stream updates while fetching broker
      snapshots, then merge/deduplicate. Reconcile open orders, recently closed
      tracked orders, positions, and account state; open orders alone miss fills
      completed while disconnected.
- [ ] Detect manual/external orders in the same account. Define whether to
      adopt them or block trading until the mismatch is reviewed.
- [ ] Disable new submissions during unresolved state or connectivity gaps.

**Done when:** killing the process after broker acceptance but before the REST
response, then restarting, recovers the same order without a duplicate trade.
Repeat with a fill occurring while disconnected.

## 5. Add market data and account risk controls

- [ ] Add a separate market-data WebSocket, quote cache, source timestamp,
      receive timestamp, reconnect/resubscribe logic, and stale-data detection.
- [ ] Configure the feed explicitly. IEX and SIP coverage differ, and feed
      access depends on the account's subscription.
- [ ] Treat quotes as market observations, not synthetic resting orders.
      Stock quotes/trades/bars do not reconstruct a complete order-by-order
      exchange book or establish your queue position.
- [ ] Add configurable symbol allowlist, maximum order quantity/notional,
      position/exposure limits, open-order limits, and price-distance checks.
- [ ] Track reservations for outstanding orders; prevent concurrent requests
      from spending the same budget or selling the same inventory twice.
      Broker buying power and positions remain authoritative.
- [ ] Add a kill switch that stops new submissions and requests cancellation
      of tracked open orders. Show unresolved orders; cancellation does not
      guarantee zero further fills or flatten existing positions.
- [ ] Reconcile a local execution/position ledger with broker state. Display
      connectivity, reconciliation status, orders, fills, and risk rejections.

**Done when:** stale quotes, a blocked account, exceeded limits, or an
unreconciled position prevent new orders and produce an actionable reason.

## 6. Validate the complete paper workflow

- [ ] Add deterministic mock REST/WebSocket tests for throttling, malformed
      payloads, authentication failure, disconnects, timeout ambiguity,
      duplicate updates, and restart recovery.
- [ ] Add an explicitly invoked paper integration test; keep credentials and
      network-dependent trading out of ordinary CTest runs.
- [ ] Record structured, redacted logs linking client ID, broker ID, request,
      response, and execution. Measure disconnects, reconciliation failures,
      queue backlog, and unknown-order count.
- [ ] Run paper sessions across market open/close and process restarts. Require
      all tracked orders and positions to reconcile before calling this ready.
- [ ] Document setup, supported features, recovery, shutdown, and live-mode
      configuration. Live use is a separate milestone after these checks.

**First usable milestone:** phases 1–4 yield a recoverable paper order CLI.
**Strategy-driven paper trading:** add phases 5–6.

## Later additions

Add fractional shares, market/stop orders, replacement, extended hours, short
selling, bracket orders, and other asset classes individually, with explicit
capability validation and lifecycle tests. None is required for the first
whole-share limit-order integration.

For the local simulator, separately prioritize submission-wide exception
atomicity (the README documents partially applied submissions on exceptions),
an injectable clock for replay, account/cash accounting, session/TIF rules,
and a market-data-driven fill model. Current FIFO matching only models the
orders supplied locally; it does not predict broker execution. Alpaca paper
fills also omit effects such as queue position and market impact.

Do performance work after integration correctness. The existing README also
notes that historical benchmark harnesses need adaptation to the current API.

## Suggested source layout

```text
include/execution/       Requests, states, reports, backend interface
src/execution/           Order manager and local simulator adapter
src/alpaca/              REST, trading stream, market-data stream, codecs
src/risk/                Pre-trade validation and reservations
src/storage/             Durable intent, order mappings, execution journal
app/alpaca_paper.cpp     Explicit paper CLI
tests/alpaca/            Fixtures, mock transports, recovery scenarios
```

## Official references

Reviewed 2026-11-13. Verify current capabilities when implementing each feature.

- [Order rules, client IDs, lifecycle, price precision and buying power](https://docs.alpaca.markets/us/docs/orders-at-alpaca)
- [Order submission and management examples](https://docs.alpaca.markets/us/docs/working-with-orders)
- [Trading WebSocket authentication, frames and execution events](https://docs.alpaca.markets/us/docs/websocket-streaming)
- [Stock market-data feeds and message schemas](https://docs.alpaca.markets/us/docs/real-time-stock-pricing-data)
- [Paper endpoints, credentials, entitlements and simulation limitations](https://docs.alpaca.markets/us/docs/paper-trading)
