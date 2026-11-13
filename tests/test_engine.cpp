#include "matching_engine.hpp"

#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace tests {

using matching::MatchingEngine;
using matching::Price;
using matching::Quantity;
using matching::Side;
using matching::SubmitResult;

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw TestFailure(std::string(message));
    }
}

void test_unmatched_order_rests() {
    MatchingEngine engine{"AAPL"};

    const SubmitResult sell = engine.submit_limit_order(Side::Sell, 101, "AAPL", 4, 42);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, "AAPL", 4, 42);

    require(sell.trades.empty(), "unmatched sell should produce no trades");
    require(sell.remaining_quantity == 4, "unmatched sell should fully remain");
    require(buy.trades.empty(), "unmatched buy should produce no trades");
    require(buy.remaining_quantity == 4, "unmatched buy should fully remain");
}

void test_full_fill() {
    MatchingEngine engine{"AAPL"};

    const SubmitResult sell = engine.submit_limit_order(Side::Sell, 100, "AAPL", 5, 42);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, "AAPL", 5, 42);

    require(buy.trades.size() == 1, "crossing order should create one trade");
    require(buy.trades[0].sell_order_id == sell.order_id, "trade should reference the sell");
    require(buy.trades[0].quantity == 5, "trade should fill all 5 units");
    require(buy.trades[0].price == 100, "trade should use the resting price");
    require(buy.remaining_quantity == 0, "fully filled order should have zero remaining");
}

void test_partial_fill_and_resting_remainder() {
    MatchingEngine engine{"AAPL"};

    engine.submit_limit_order(Side::Sell, 100, "AAPL", 3, 42);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, "AAPL", 5, 42);

    require(buy.trades.size() == 1, "partial fill should create one trade");
    require(buy.trades[0].quantity == 3, "partial fill should trade available quantity");
    require(buy.remaining_quantity == 2, "partial fill should report the remainder");

    const SubmitResult later_sell = engine.submit_limit_order(Side::Sell, 100, "AAPL", 2, 42);
    require(later_sell.trades.size() == 1, "remainder should rest and match later");
    require(later_sell.trades[0].quantity == 2, "later sell should consume remainder");
}

void test_price_then_time_priority() {
    MatchingEngine engine{"AAPL"};

    const SubmitResult expensive = engine.submit_limit_order(Side::Sell, 101, "AAPL", 2, 42);
    const SubmitResult first_cheap = engine.submit_limit_order(Side::Sell, 100, "AAPL", 2, 42);
    const SubmitResult second_cheap = engine.submit_limit_order(Side::Sell, 100, "AAPL", 2, 42);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 101, "AAPL", 6, 42);

    require(buy.trades.size() == 3, "buy should sweep all three orders");
    require(buy.trades[0].sell_order_id == first_cheap.order_id,
            "best price and first arrival should execute first");
    require(buy.trades[1].sell_order_id == second_cheap.order_id,
            "same-price orders should execute FIFO");
    require(buy.trades[2].sell_order_id == expensive.order_id,
            "worse price should execute last");
}

void test_cancellation() {
    MatchingEngine engine{"AAPL"};

    const SubmitResult sell = engine.submit_limit_order(Side::Sell, 100, "AAPL", 3, 42);
    require(engine.cancel_order(sell.order_id), "existing resting order should cancel");
    require(!engine.cancel_order(sell.order_id), "order should not cancel twice");

    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, "AAPL", 3, 42);
    require(buy.trades.empty(), "cancelled order must not execute");
}

void test_input_validation() {
    MatchingEngine engine{"AAPL"};

    for (const auto [price, quantity] : {
             std::pair<Price, Quantity>{0, 1},
             std::pair<Price, Quantity>{-1, 1},
             std::pair<Price, Quantity>{1, 0},
         }) {
        const auto result = engine.submit_limit_order(Side::Buy, price, "AAPL", quantity, 42);
        require(result.status == matching::Status::Error, "invalid input should return Error");
        require(result.order_id == 0 && result.trades.empty(), "rejection must not create an order or fills");
    }
}

void test_order_and_trade_identity() {
    MatchingEngine engine{"AAPL"};
    const auto sell = engine.submit_limit_order(Side::Sell, 100, "AAPL", 5, 11);
    const auto resting = engine.get_order(sell.order_id);
    require(resting.has_value(), "sell should rest");
    require(resting->symbol == "AAPL" && resting->trader_id == 11, "resting order should preserve identity");

    const auto buy = engine.submit_limit_order(Side::Buy, 101, "AAPL", 2, 22);
    require(buy.trades.size() == 1, "incoming buy should partially fill sell");
    const auto& first = buy.trades.front();
    require(first.symbol == "AAPL" && first.buyer_id == 22 && first.seller_id == 11,
            "incoming-buy trade should identify both participants");
    require(first.buy_order_id == buy.order_id && first.sell_order_id == sell.order_id,
            "trade should identify both orders");
    const auto remainder = engine.get_order(sell.order_id);
    require(remainder && remainder->quantity == 3 && remainder->trader_id == 11,
            "partial fill should preserve resting owner");
    require(engine.cancel_order(sell.order_id), "remaining sell should cancel");

    const auto bid = engine.submit_limit_order(Side::Buy, 100, "AAPL", 2, 33);
    const auto ask = engine.submit_limit_order(Side::Sell, 99, "AAPL", 3, 44);
    require(ask.trades.size() == 1, "incoming sell should fill resting buy");
    const auto& second = ask.trades.front();
    require(second.symbol == "AAPL" && second.buyer_id == 33 && second.seller_id == 44,
            "incoming-sell trade should identify both participants");
    require(second.buy_order_id == bid.order_id && second.sell_order_id == ask.order_id,
            "incoming-sell trade should identify both orders");
    require(second.price == 100 && second.quantity == 2, "sell should execute at resting bid price");
    const auto ask_remainder = engine.get_order(ask.order_id);
    require(ask_remainder && ask_remainder->quantity == 1 && ask_remainder->trader_id == 44,
            "incoming sell remainder should preserve owner");
    require(!engine.get_order(bid.order_id), "fully filled bid should leave index");
}

void test_rejection_preserves_book_and_ids() {
    MatchingEngine engine{"AAPL"};
    const auto sell = engine.submit_limit_order(Side::Sell, 100, "AAPL", 5, 11);
    const auto wrong_symbol = engine.submit_limit_order(Side::Buy, 101, "MSFT", 5, 22);
    const auto invalid_side = engine.submit_limit_order(static_cast<Side>(99), 101, "AAPL", 5, 22);
    const auto invalid_trader = engine.submit_limit_order(
        Side::Buy, 101, "AAPL", 5, std::numeric_limits<matching::TraderId>::max());
    for (const auto* rejected : {&wrong_symbol, &invalid_side, &invalid_trader}) {
        require(rejected->status == matching::Status::Error && rejected->order_id == 0,
                "invalid submission should return Error and no order ID");
        require(rejected->trades.empty(), "invalid crossing submission must not trade");
    }
    require(engine.get_asks_quantity_at(100) == 5, "rejections must leave resting liquidity unchanged");
    require(!engine.get_best_bid(), "rejections must not create bids");
    const auto buy = engine.submit_limit_order(Side::Buy, 100, "AAPL", 5, 22);
    require(buy.order_id == sell.order_id + 1, "rejections must not consume IDs");
    require(buy.trades.size() == 1 && buy.trades.front().quantity == 5,
            "original liquidity should remain executable");
}

void test_independent_symbol_engines() {
    MatchingEngine aapl{"AAPL"};
    MatchingEngine msft{"MSFT"};
    const auto sell = aapl.submit_limit_order(Side::Sell, 100, "AAPL", 3, 11);
    const auto buy = msft.submit_limit_order(Side::Buy, 101, "MSFT", 3, 22);
    require(sell.order_id == buy.order_id, "order IDs are local to each engine");
    require(buy.trades.empty(), "different instruments must not match");
    require(aapl.get_asks_quantity_at(100) == 3 && msft.get_bids_quantity_at(101) == 3,
            "engines should keep independent books");
    require(aapl.cancel_order(sell.order_id), "AAPL order should cancel");
    require(msft.get_order(buy.order_id).has_value(), "AAPL cancellation must not affect MSFT");
    const auto execution = msft.submit_limit_order(Side::Sell, 101, "MSFT", 3, 33);
    require(execution.trades.size() == 1 && execution.trades.front().symbol == "MSFT",
            "MSFT engine should produce MSFT executions");
}

void test_symbol_length_validation() {
    MatchingEngine engine{"ABC"};
    const auto result = engine.submit_limit_order(Side::Buy, 100, "ABC", 1, 11);
    require(result.status == matching::Status::Error && result.order_id == 0,
            "current API requires four-character symbols");
    require(!engine.get_best_bid(), "invalid symbol must not create liquidity");
}


void test_cancel_middle_fifo() {
    for (Side side : {Side::Buy, Side::Sell}) {
        MatchingEngine e{"AAPL"};
        auto a = e.submit_limit_order(side, 100, "AAPL", 2, 11);
        auto b = e.submit_limit_order(side, 100, "AAPL", 2, 22);
        auto c = e.submit_limit_order(side, 100, "AAPL", 2, 33);
        require(e.cancel_order(b.order_id), "middle cancellation should succeed");
        require(!e.get_order(b.order_id), "cancelled order must leave index");
        auto fill = e.submit_limit_order(side == Side::Buy ? Side::Sell : Side::Buy, 100, "AAPL", 4, 44);
        require(fill.trades.size() == 2, "only surviving orders should execute");
        auto resting_id = [side](const matching::Trade& t) { return side == Side::Buy ? t.buy_order_id : t.sell_order_id; };
        require(resting_id(fill.trades[0]) == a.order_id && resting_id(fill.trades[1]) == c.order_id,
                "middle cancellation must preserve survivor FIFO order");
    }
}

void test_cancel_partially_filled_order() {
    MatchingEngine e{"AAPL"};
    auto sell = e.submit_limit_order(Side::Sell, 100, "AAPL", 10, 11);
    e.submit_limit_order(Side::Buy, 100, "AAPL", 4, 22);
    auto remainder = e.get_order(sell.order_id);
    require(remainder && remainder->quantity == 6, "six units should remain");
    require(e.cancel_order(sell.order_id) && !e.get_order(sell.order_id), "remainder must cancel and leave index");
    auto buy = e.submit_limit_order(Side::Buy, 100, "AAPL", 6, 22);
    require(buy.trades.empty() && buy.remaining_quantity == 6, "cancelled remainder must not execute");
}

void test_lookup_after_full_execution() {
    for (Side side : {Side::Buy, Side::Sell}) {
        MatchingEngine e{"AAPL"};
        auto resting = e.submit_limit_order(side, 100, "AAPL", 5, 11);
        auto incoming = e.submit_limit_order(side == Side::Buy ? Side::Sell : Side::Buy, 100, "AAPL", 5, 22);
        require(incoming.trades.size() == 1, "orders should fully execute");
        require(!e.get_order(resting.order_id) && !e.get_order(incoming.order_id), "filled orders must not remain indexed");
    }
}

void test_empty_book_queries() {
    MatchingEngine e{"AAPL"};
    auto check = [&] {
        require(!e.get_best_bid() && !e.get_best_ask(), "empty book must have no best orders");
        require(e.get_bids_quantity_at(100) == 0 && e.get_asks_quantity_at(100) == 0, "empty levels must report zero");
        require(!e.get_order(0) && !e.get_order(1) && !e.get_order(2) && !e.get_order(3), "absent IDs must return nullopt");
    };
    check();
    auto bid = e.submit_limit_order(Side::Buy, 100, "AAPL", 1, 11);
    require(e.cancel_order(bid.order_id), "last bid should cancel");
    check();
    e.submit_limit_order(Side::Sell, 100, "AAPL", 1, 11);
    e.submit_limit_order(Side::Buy, 100, "AAPL", 1, 22);
    check();
}

void test_sell_price_time_priority() {
    MatchingEngine e{"AAPL"};
    auto low = e.submit_limit_order(Side::Buy, 99, "AAPL", 2, 11);
    auto first = e.submit_limit_order(Side::Buy, 101, "AAPL", 2, 22);
    auto second = e.submit_limit_order(Side::Buy, 101, "AAPL", 2, 33);
    auto sell = e.submit_limit_order(Side::Sell, 99, "AAPL", 6, 44);
    require(sell.trades.size() == 3, "sell should sweep three bids");
    require(sell.trades[0].buy_order_id == first.order_id && sell.trades[1].buy_order_id == second.order_id
            && sell.trades[2].buy_order_id == low.order_id, "highest price then FIFO should execute");
    require(sell.trades[0].price == 101 && sell.trades[1].price == 101 && sell.trades[2].price == 99,
            "each execution must use its resting price");
}

void test_partial_sweep_preserves_priority() {
    MatchingEngine e{"AAPL"};
    auto first = e.submit_limit_order(Side::Sell, 100, "AAPL", 3, 11);
    auto second = e.submit_limit_order(Side::Sell, 101, "AAPL", 5, 22);
    auto third = e.submit_limit_order(Side::Sell, 101, "AAPL", 1, 33);
    auto buy = e.submit_limit_order(Side::Buy, 101, "AAPL", 6, 44);
    require(buy.trades.size() == 2, "buy should partially sweep two levels");
    require(buy.trades[0].sell_order_id == first.order_id && buy.trades[0].quantity == 3 && buy.trades[0].price == 100,
            "first level should execute three at 100");
    require(buy.trades[1].sell_order_id == second.order_id && buy.trades[1].quantity == 3 && buy.trades[1].price == 101,
            "second level should execute three at 101");
    auto remaining = e.get_order(second.order_id);
    require(remaining && remaining->quantity == 2 && remaining->trader_id == 22, "remainder should retain owner and quantity");
    auto next = e.submit_limit_order(Side::Buy, 101, "AAPL", 3, 44);
    require(next.trades.size() == 2 && next.trades[0].sell_order_id == second.order_id
            && next.trades[1].sell_order_id == third.order_id, "partial fill must retain FIFO position");
}

void test_limit_stops_sweep() {
    MatchingEngine e{"AAPL"};
    e.submit_limit_order(Side::Sell, 100, "AAPL", 2, 11);
    e.submit_limit_order(Side::Sell, 101, "AAPL", 2, 22);
    auto expensive = e.submit_limit_order(Side::Sell, 102, "AAPL", 2, 33);
    auto buy = e.submit_limit_order(Side::Buy, 101, "AAPL", 7, 44);
    require(buy.trades.size() == 2 && buy.trades[0].price == 100 && buy.trades[1].price == 101,
            "buy must stop before exceeding its limit");
    auto remainder = e.get_order(buy.order_id);
    require(remainder && remainder->price == 101 && remainder->quantity == 3, "buy remainder must rest at its limit");
    auto untouched = e.get_order(expensive.order_id);
    require(untouched && untouched->quantity == 2, "out-of-limit ask must remain untouched");
}

void test_quantity_aggregation() {
    for (Side side : {Side::Buy, Side::Sell}) {
        MatchingEngine e{"AAPL"};
        auto first = e.submit_limit_order(side, 100, "AAPL", 5, 11);
        auto second = e.submit_limit_order(side, 100, "AAPL", 7, 22);
        auto total = [&] { return side == Side::Buy ? e.get_bids_quantity_at(100) : e.get_asks_quantity_at(100); };
        require(total() == 12, "level must sum all resting orders");
        e.submit_limit_order(side == Side::Buy ? Side::Sell : Side::Buy, 100, "AAPL", 2, 33);
        require(total() == 10, "partial execution must reduce aggregate");
        require(e.cancel_order(second.order_id) && total() == 3, "cancellation must reduce aggregate");
        require(e.cancel_order(first.order_id) && total() == 0, "removing last order must empty level");
    }
}

void test_ids_never_reused() {
    MatchingEngine e{"AAPL"};
    auto sell = e.submit_limit_order(Side::Sell, 100, "AAPL", 1, 11);
    auto buy = e.submit_limit_order(Side::Buy, 100, "AAPL", 1, 22);
    auto resting = e.submit_limit_order(Side::Buy, 99, "AAPL", 1, 22);
    require(e.cancel_order(resting.order_id), "resting order should cancel");
    auto next = e.submit_limit_order(Side::Buy, 98, "AAPL", 1, 22);
    require(sell.order_id == 1 && buy.order_id == 2 && resting.order_id == 3 && next.order_id == 4,
            "filled and cancelled IDs must never be reused");
}

void test_invalid_price_quantity_preserve_state() {
    MatchingEngine e{"AAPL"};
    auto sell = e.submit_limit_order(Side::Sell, 100, "AAPL", 5, 11);
    for (auto [price, qty] : {std::pair<Price, Quantity>{0, 5}, {-1, 5}, {101, 0}}) {
        auto result = e.submit_limit_order(Side::Buy, price, "AAPL", qty, 22);
        require(result.status == matching::Status::Error && result.order_id == 0 && result.trades.empty(),
                "invalid requests must not create orders or trades");
        auto order = e.get_order(sell.order_id);
        require(order && order->quantity == 5 && !e.get_best_bid(), "rejection must preserve book");
    }
    auto buy = e.submit_limit_order(Side::Buy, 100, "AAPL", 5, 22);
    require(buy.order_id == sell.order_id + 1 && buy.trades.size() == 1 && buy.trades[0].quantity == 5,
            "rejections must preserve next ID and executable liquidity");
}

void test_unknown_cancellation() {
    MatchingEngine e{"AAPL"};
    auto ask = e.submit_limit_order(Side::Sell, 100, "AAPL", 2, 11);
    auto bid = e.submit_limit_order(Side::Buy, 99, "AAPL", 2, 22);
    require(!e.cancel_order(0) && !e.cancel_order(999), "unknown IDs must not cancel");
    e.submit_limit_order(Side::Buy, 100, "AAPL", 2, 22);
    require(!e.cancel_order(ask.order_id), "filled ID must not cancel");
    auto remaining = e.get_order(bid.order_id);
    require(remaining && remaining->quantity == 2, "failed cancellations must preserve other orders");
}

void test_level_quantity_overflow() {
    for (Side side : {Side::Buy, Side::Sell}) {
        MatchingEngine e{"AAPL"};
        const auto maximum = std::numeric_limits<Quantity>::max();
        auto a = e.submit_limit_order(side, 100, "AAPL", maximum, 11);
        auto b = e.submit_limit_order(side, 100, "AAPL", 1, 22);
        require(a.status == matching::Status::Success && b.status == matching::Status::Success, "large orders are currently accepted");
        bool threw = false;
        try {
            if (side == Side::Buy) e.get_bids_quantity_at(100);
            else e.get_asks_quantity_at(100);
        } catch (const std::overflow_error&) { threw = true; }
        require(threw, "aggregate overflow must throw");
        auto first = e.get_order(a.order_id);
        auto second = e.get_order(b.order_id);
        require(first && first->quantity == maximum && second && second->quantity == 1, "query failure must preserve individual orders");
        require(e.cancel_order(b.order_id), "overflowing level must remain cancellable");
        require((side == Side::Buy ? e.get_bids_quantity_at(100) : e.get_asks_quantity_at(100)) == maximum,
                "exact maximum total must remain valid");
    }
}

void test_self_trade_current_policy() {
    MatchingEngine e{"AAPL"};
    e.submit_limit_order(Side::Sell, 100, "AAPL", 2, 42);
    auto buy = e.submit_limit_order(Side::Buy, 100, "AAPL", 2, 42);
    require(buy.trades.size() == 1 && buy.remaining_quantity == 0, "current policy permits self-trading");
    require(buy.trades[0].buyer_id == 42 && buy.trades[0].seller_id == 42, "both identities must identify same trader");
}

struct TestCase {
    std::string_view name;
    std::function<void()> function;
};

} // end of namespace tests

int main() {
    const tests::TestCase tests[] = {
        {"unmatched order rests", tests::test_unmatched_order_rests},
        {"full fill", tests::test_full_fill},
        {"partial fill and remainder", tests::test_partial_fill_and_resting_remainder},
        {"price then time priority", tests::test_price_then_time_priority},
        {"cancellation", tests::test_cancellation},
        {"input validation", tests::test_input_validation},
        {"order and trade identity", tests::test_order_and_trade_identity},
        {"rejection preserves book and IDs", tests::test_rejection_preserves_book_and_ids},
        {"independent symbol engines", tests::test_independent_symbol_engines},
        {"symbol length validation", tests::test_symbol_length_validation},
        {"cancel middle fifo", tests::test_cancel_middle_fifo},
        {"cancel partially filled order", tests::test_cancel_partially_filled_order},
        {"lookup after full execution", tests::test_lookup_after_full_execution},
        {"empty book queries", tests::test_empty_book_queries},
        {"sell price time priority", tests::test_sell_price_time_priority},
        {"partial sweep preserves priority", tests::test_partial_sweep_preserves_priority},
        {"limit stops sweep", tests::test_limit_stops_sweep},
        {"quantity aggregation", tests::test_quantity_aggregation},
        {"ids never reused", tests::test_ids_never_reused},
        {"invalid price quantity preserve state", tests::test_invalid_price_quantity_preserve_state},
        {"unknown cancellation", tests::test_unknown_cancellation},
        {"level quantity overflow", tests::test_level_quantity_overflow},
        {"self trade current policy", tests::test_self_trade_current_policy},

    };

    int failures = 0;

    for (const tests::TestCase& test : tests) {
        try {
            test.function();
            std::cout << "PASS: " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL: " << test.name << " - " << error.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
