#include "matching_engine.hpp"

#include <exception>
#include <functional>
#include <iostream>
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
    MatchingEngine engine;

    const SubmitResult sell = engine.submit_limit_order(Side::Sell, 101, 4);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, 4);

    require(sell.trades.empty(), "unmatched sell should produce no trades");
    require(sell.remaining_quantity == 4, "unmatched sell should fully remain");
    require(buy.trades.empty(), "unmatched buy should produce no trades");
    require(buy.remaining_quantity == 4, "unmatched buy should fully remain");
}

void test_full_fill() {
    MatchingEngine engine;

    const SubmitResult sell = engine.submit_limit_order(Side::Sell, 100, 5);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, 5);

    require(buy.trades.size() == 1, "crossing order should create one trade");
    require(buy.trades[0].sell_order_id == sell.order_id, "trade should reference the sell");
    require(buy.trades[0].quantity == 5, "trade should fill all 5 units");
    require(buy.trades[0].price == 100, "trade should use the resting price");
    require(buy.remaining_quantity == 0, "fully filled order should have zero remaining");
}

void test_partial_fill_and_resting_remainder() {
    MatchingEngine engine;

    engine.submit_limit_order(Side::Sell, 100, 3);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, 5);

    require(buy.trades.size() == 1, "partial fill should create one trade");
    require(buy.trades[0].quantity == 3, "partial fill should trade available quantity");
    require(buy.remaining_quantity == 2, "partial fill should report the remainder");

    const SubmitResult later_sell = engine.submit_limit_order(Side::Sell, 100, 2);
    require(later_sell.trades.size() == 1, "remainder should rest and match later");
    require(later_sell.trades[0].quantity == 2, "later sell should consume remainder");
}

void test_price_then_time_priority() {
    MatchingEngine engine;

    const SubmitResult expensive = engine.submit_limit_order(Side::Sell, 101, 2);
    const SubmitResult first_cheap = engine.submit_limit_order(Side::Sell, 100, 2);
    const SubmitResult second_cheap = engine.submit_limit_order(Side::Sell, 100, 2);
    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 101, 6);

    require(buy.trades.size() == 3, "buy should sweep all three orders");
    require(buy.trades[0].sell_order_id == first_cheap.order_id,
            "best price and first arrival should execute first");
    require(buy.trades[1].sell_order_id == second_cheap.order_id,
            "same-price orders should execute FIFO");
    require(buy.trades[2].sell_order_id == expensive.order_id,
            "worse price should execute last");
}

void test_cancellation() {
    MatchingEngine engine;

    const SubmitResult sell = engine.submit_limit_order(Side::Sell, 100, 3);
    require(engine.cancel_order(sell.order_id), "existing resting order should cancel");
    require(!engine.cancel_order(sell.order_id), "order should not cancel twice");

    const SubmitResult buy = engine.submit_limit_order(Side::Buy, 100, 3);
    require(buy.trades.empty(), "cancelled order must not execute");
}

void test_input_validation() {
    MatchingEngine engine;

    for (const auto [price, quantity] : {
             std::pair<Price, Quantity>{0, 1},
             std::pair<Price, Quantity>{-1, 1},
             std::pair<Price, Quantity>{1, 0},
         }) {
        try {
            engine.submit_limit_order(Side::Buy, price, quantity);
            throw TestFailure("invalid order should throw invalid_argument");
        } catch (const std::invalid_argument&) {
        }
    }
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
