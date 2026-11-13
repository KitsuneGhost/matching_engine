# Roadmap: from matching engine to execution research

Updated: 2026-11-13. Planning document: the unchecked items below are not implemented.

## Read this when returning to the project

The original plan was to connect the engine to Alpaca. The current first
objective is to understand market mechanics by rebuilding a recorded
market and experimenting with pretend orders. Alpaca integration comes later.

The eventual research question is: **when does waiting with a limit order save
money compared with buying immediately, once missed fills and price movements
are included?** A simple strategy is enough to begin answering it.

The engine is the foundation, not the whole simulator:

1. A matching engine decides trades between the orders supplied to it.
2. A historical book reconstructor applies messages describing trades and order
   changes that already happened.
3. An execution simulator estimates what would have happened to a hypothetical
   order alongside that recorded market.
4. A research runner compares decisions and measures the results.

These jobs can share storage machinery, but have different rules. In particular,
historical reconstruction must not invent executions by automatically matching
recorded orders. The feed already specifies which executions occurred.

**Next task: implement Step 1, explicit event timestamps.** Then follow the
numbered steps below. Do not begin with networking or a new trading strategy.

## What exists today

- `include/types.hpp`: integer prices/quantities, numeric IDs, wall-clock timestamps.
- `include/order.hpp`: resting order data, including symbol and trader identity.
- `include/trade.hpp`: simulated trade data with both counterparties identified.
- `src/matching_engine.cpp`: validates submissions and allocates local IDs.
- `src/order_book.cpp`: matching, FIFO price queues, indexed lookup/cancellation,
  and resting-insertion rollback.
- `tests/`: matching tests, allocation-failure tests, and differential testing.
- `app/main.cpp`: a fixed AAPL demonstration, not a replay runner.

Important gaps: execution timestamps call the wall clock inside the matching
loops; replay cannot supply order IDs; cancellation only removes whole orders;
there is no public structured book snapshot. Submission-wide exception atomicity
is not guaranteed. Symbols are currently restricted to four characters.

## First milestone

**Replay a tiny, hand-written event file for one symbol and verify the entire
book after every event.** No external account, paid dataset, or live connection
is needed for this milestone. Determinism means the same input produces the same
orders, timestamps, outputs, and failures on every run.

### Step 0 — Establish a working baseline

Purpose: know whether a failure was already present before making changes.

- [ ] Read `include/matching_engine.hpp`, `include/order_book.hpp`, and their
  implementations. Follow one buy through validation, matching, and resting.
- [ ] Configure, build, and run existing tests:

  ```bash
  cmake -S . -B build
  cmake --build build
  ctest --test-dir build --output-on-failure
  ```

- [ ] Record any existing failures before changing code. Preserve existing
  matching behavior while adding the replay functionality.

**Done when:** the baseline is understood and its checks pass, or pre-existing
failures have been clearly identified.

### Step 1 — Let the caller provide time

Purpose: historical events should keep their original times. Replaying yesterday
must not produce timestamps saying today.

Current behavior: `MatchingEngine::submit_limit_order()` calls
`current_timestamp()` for acceptance, and both matching loops call it again for
executions. Changing only the acceptance time would leave trades nondeterministic.

- [ ] Add an explicit timestamp submission path. Keep the existing convenience
  API by having it obtain the wall-clock time once and delegate to that path.
  An overload is enough; a general clock framework is optional.
- [ ] Pass that timestamp into matching. For the current synchronous operation,
  all executions caused by one submission use the supplied event time.
- [ ] Keep `current_timestamp()` at the live/demo entry boundary, outside the
  deterministic matching path.
- [ ] Document timestamp units and meaning. Use integer nanoseconds in the toy
  fixture. Recorded exchange time, local receive time, and simulated arrival
  time will eventually be separate concepts.
- [ ] Plan a separate event sequence number: two messages can have the same
  timestamp. Their input order must still be preserved.

Example: submit an order at time `1000`, then a crossing order at time `2000`.
The first order's acceptance is `1000`; resulting executions occur at `2000`,
regardless of how slowly the program runs.

**Tests:** inspect both acceptance and execution timestamps; submit a multi-fill
order and check every trade; run an identical sequence twice and compare outputs.

**Done when:** the explicit-time path makes no wall-clock reads and the old demo
still works. Do not use sleeps to test simulated time.

### Step 2 — Define historical events and order identity

Purpose: a later message saying “reduce order 42” must find exactly the order
that an earlier message added as 42.

- [ ] Define a small normalized event model, independent of any vendor format:
  `Add`, `Reduce`, `Delete`, and `Execute`, with symbol, timestamp, sequence,
  external order ID, and fields required by each event.
- [ ] Use positive integer quantity and integer price units. For the toy
  fixture, define one price unit as $0.01; for a real dataset, preserve its
  documented scale explicitly rather than assuming cents.
- [ ] Define `Reduce.quantity` as the amount removed, not the new remaining
  amount. `Delete` removes everything. `Execute.quantity` removes traded shares.
- [ ] Keep historical identity separate from generated matching IDs. A separate
  replay book can preserve the feed IDs directly. If conversion is necessary,
  maintain an explicit mapping scoped to instrument/session/source.
- [ ] Specify duplicate active IDs as errors. Decide ID reuse rules from the
  selected feed before implementing its adapter; never assume global uniqueness.
- [ ] Do not fabricate historical trader identities. Use a documented unknown
  value or a replay-specific record without ownership fields.
- [ ] Remove the four-character-only symbol restriction; validate nonempty,
  consistent symbols and let source-specific validation live in its adapter.

**Tests:** add IDs 42 and 99, update 42, and prove 99 is unchanged. Reject an
active duplicate without changing the book. Check `F`, `AAPL`, and `GOOGL`.

**Done when:** event meanings, units, identity scope, and validation rules are
written down, and historical IDs do not collide with pretend/local order IDs.

### Step 3 — Implement reconstruction operations

Purpose: reproduce recorded facts without asking the matcher to decide what
should have traded.

Suggested interface: a separate `ReplayBook`/`BookReconstructor` accepting
normalized events. Share tested list/index helpers where useful, but keep its
public operations distinct from `submit_limit_order()`. Do not expose every
private container just to make replay possible.

- [ ] **Add:** insert at the supplied side/price with the supplied ID and time.
  Append at the back of that price queue. Do not run crossing/matching logic.
- [ ] **Reduce:** remove the stated quantity from an existing order. A partial
  reduction keeps its FIFO position; reaching zero removes the order.
- [ ] **Delete:** remove the identified order entirely, cleaning the index and
  any empty price level.
- [ ] **Execute:** remove the stated traded quantity from the identified resting
  order. Produce a historical execution record if needed, without inventing a
  missing counterparty ID or emitting a simulated two-sided `Trade`.
- [ ] Reject unknown IDs, zero quantities, excessive reductions/executions,
  duplicate active adds, and wrong symbols before mutation. Never silently clamp
  quantities or skip invalid messages in the initial strict replay mode.
- [ ] Preserve list/index consistency. Return useful errors containing the event
  sequence and order ID. Stop replay on an inconsistent event.
- [ ] Leave replacements and venue-specific priority rules to the real feed
  adapter. A replacement may lose priority; do not guess its semantics.

Crossed books: do not silently “repair” them by matching. Flag them diagnostically
and interpret them using the selected feed's venue/session rules. The toy fixture
should avoid them; the reconstruction primitive must still not create trades.

**Tests:** partial reduction preserves queue order; full reduction removes the
index; deleting the last order removes the level; execution changes only its
target; invalid operations leave an identical pre/post snapshot. Add a crossing
order in a dedicated test and verify it does not automatically execute.

**Done when:** all four event types work independently of automatic matching,
and existing matching tests still pass.

### Step 4 — Expose snapshots and check invariants

Purpose: a printed book is useful to a person, but replay needs machine-readable
state to prove it is correct.

- [ ] Return a read-only snapshot with descending bid levels and ascending ask
  levels, including price, aggregate quantity, order count, and FIFO order data.
  Copies are fine initially; never return mutable internal list iterators.
- [ ] Distinguish best price-level quantity from the first order's quantity.
  Existing `get_best_bid()`/`get_best_ask()` return one order, not the level total.
- [ ] Check aggregation overflow explicitly. Empty sides should be represented
  clearly, without a made-up price of zero.
- [ ] Add a debug/test invariant checker: every resting order has exactly one
  correct index entry; each index points to a real order; no zero-quantity orders
  or empty price levels remain; IDs are unique and queue order is preserved.
- [ ] Compare full snapshots in the tiny fixture. Later, derive a stable digest
  from canonical ordered fields, never unordered-map iteration or memory addresses.

**Done when:** each test event can be followed by an invariant check and an exact
snapshot comparison. Snapshot inspection must not change book state.

### Step 5 — Make mutation failures explicit

Purpose: a failed operation must not quietly leave a book that later produces
plausible but wrong research results.

Current resting insertion has rollback, but a matching submission can perform
earlier fills before a later allocation fails. Building a response can also fail
after mutation. The README already warns that such a session must be discarded.

- [ ] Keep that stop-and-discard rule until a stronger guarantee is implemented.
  A failed replay run must never be reported as a successful experiment.
- [ ] For replay operations, validate before mutation and use the insertion
  rollback machinery. Avoid allocating output after irreversible changes unless
  rollback or a prepared output makes the operation safe.
- [ ] For matching, design prepare/commit handling: identify affected orders and
  fills, prepare all fallible allocations including a possible resting remainder
  and response, then commit with operations that cannot throw. Document the
  actual guarantee; merely reserving a trade vector is insufficient.
- [ ] Extend allocation-failure tests to multi-level matching sweeps and resting
  remainders. Inspect state, quantities, FIFO, index, and reported executions.
- [ ] Define whether rejected/failed submissions consume IDs. Keep that behavior
  explicit in tests and documentation.

**Done when:** ordinary rejected replay events leave state unchanged; unexpected
exceptions terminate the run. Before using automatic matching for trusted
simulation, demonstrate the chosen submission-wide failure guarantee.

### Step 6 — Build the tiny replay runner

Purpose: connect events, time, reconstruction, and validation into one program.

- [ ] Add a small CSV reader and `app/replay.cpp` target. Keep parsing separate
  from book logic. Check malformed fields, numeric overflow, and missing fields.
- [ ] Require increasing sequence numbers; allow equal timestamps in sequence
  order. For this toy format reject backward timestamps. Real feed adapters must
  explicitly handle any source-specific ordering behavior rather than sorting
  blindly by timestamp.
- [ ] Run as fast as possible without sleeping. Stop after a configurable event
  count for inspection. Playback speed does not change simulated timestamps.
- [ ] Print event count, final best prices, and a deterministic state digest.
  On error, print the line/sequence/reason and exit unsuccessfully.
- [ ] Register a small fixture-based integration test in CTest.

Toy file, for `AAPL`, with timestamps in nanoseconds and prices in cents:

```csv
sequence,timestamp_ns,type,order_id,side,price,quantity
1,1000,ADD,42,BUY,1000,10
2,1000,ADD,43,BUY,1000,5
3,2000,ADD,90,SELL,1001,8
4,3000,REDUCE,42,,,3
5,4000,EXECUTE,42,,,4
6,5000,DELETE,90,,,
7,6000,EXECUTE,42,,,3
```

Expected state after each line:

| Sequence | Bid queue at $10.00 | Ask queue at $10.01 |
|---|---|---|
| 1 | 42: 10 shares | empty |
| 2 | 42: 10, then 43: 5 | empty |
| 3 | 42: 10, then 43: 5 | 90: 8 |
| 4 | 42: 7, then 43: 5 | 90: 8 |
| 5 | 42: 3, then 43: 5 | 90: 8 |
| 6 | 42: 3, then 43: 5 | empty |
| 7 | 43: 5 | empty |

**Milestone complete when:** every row matches the expected full snapshot, two
runs produce identical results, and an invalid extra event stops with a precise
error. Keep the old matching demo working alongside the replay application.

## Second milestone — Reconstruct a real recorded market

Only begin after the tiny fixture is correct.

- [ ] Inspect the existing LOBSTER analysis files: the README says they currently
  calibrate cancellation frequency, not historical replay. Inventory what data
  is actually available and what its license permits.
- [ ] Select one instrument/session with documented event semantics and reference
  snapshots. Read the source specification before writing the adapter.
- [ ] Record data provenance, price scale, timestamp units, session boundaries,
  starting state, ID rules, and supported/unsupported event types.
- [ ] Implement a source adapter into normalized events. Explicitly account for
  hidden executions, trading halts, replacements, and other source-specific
  messages; not every reported trade removes visible book quantity.
- [ ] Initialize from a valid starting snapshot or a complete event history.
  Starting halfway through a file with an empty book is generally insufficient.
- [ ] Compare reconstructed prices/quantities against supplied snapshots at the
  correct event boundary and available depth. Explain the first mismatch.
- [ ] Save source checksums, run configuration, counts, and validation results.

**Done when:** a complete chosen session validates against the available
reference data with no unexplained mismatches. This proves reconstruction to
the available reference depth, not hypothetical execution accuracy.

## Third milestone — Estimate fills for a pretend passive order

Keep the reconstructed historical book authoritative. Track hypothetical orders
in a separate simulation layer so they cannot corrupt historical validation.

- [ ] Start with one small, whole-share passive order at a recorded price.
- [ ] Record visible quantity ahead at simulated arrival. Track identifiable
  cancellations/reductions ahead separately from changes behind the order.
- [ ] Model executions consuming the queue ahead before filling the pretend
  order. Maintain separate simulated fills; never double-count historical trades.
- [ ] Add send/cancel latency using a deterministic scheduler. Define same-time
  event priority explicitly; cancellation takes effect on arrival.
- [ ] Document the no-market-impact assumption, hidden-liquidity limitations,
  and how hypothetical liquidity consumption affects later fill estimates.
  Counterfactual outcomes are estimates, not recorded facts.
- [ ] Test hand-calculated queues before using real data. Vary latency and queue
  assumptions to see how much results depend on them.

**Done when:** the toy queue gives manually verifiable fills and cancellations,
and the real historical book still reproduces its original reference snapshots.

## Fourth milestone — Compare simple execution decisions

- [ ] Define identical arrival times, target quantities, and deadlines for each
  policy. Use only information observable at the decision time.
- [ ] Compare immediate execution with posting passively until a timeout, then
  crossing for the remainder. Specify depth and liquidity-consumption assumptions.
- [ ] Record fill rate, completion time, execution cost versus arrival price,
  fees, later price movement after fills, and treatment of unfilled quantity.
- [ ] Add cash/position accounting and explicit budget/inventory limits.
- [ ] Split development and evaluation chronologically. Evaluate across multiple
  sessions/instruments and report uncertainty and sensitivity to assumptions.
- [ ] Write a reproducible report, including unsuccessful results and limitations.

**Done when:** another person can reproduce a defensible comparison and explain
why waiting helped or hurt. Profitability is not a prerequisite for good research.

## Later — Alpaca and engineering extensions

The separate [Alpaca integration roadmap](ROADMAP_ALPACA.md) retains the detailed
broker plan. Use it after the replay/simulation foundations, or as a separate
integration exercise. Alpaca paper execution is another backend; its fills do
not validate historical queue-position estimates.

Later extensions include broader order types, session rules, multiple symbols,
durable experiments, profiling, and latency optimization. Add each to answer a
specific need. Multithreading, a dashboard, machine learning, and live trading
are not requirements for the first milestones.

## Session handoff notes

Update this block after each implementation session:

```text
Last completed step: Roadmap written; implementation has not started.
Current task: Step 0 baseline, then Step 1 explicit timestamps.
Last verification: No code changes or tests run for this documentation update.
Open design decisions: Exact replay interface and real dataset adapter.
Next smallest action: Build/test baseline and trace all current_timestamp calls.
```
