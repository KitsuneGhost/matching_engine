#include "matching_engine.hpp"

#include <iostream>

int main() {
    matching::MatchingEngine engine{"AAPL"};

    auto sell1 = engine.submit_limit_order(
        matching::Side::Sell,
        100,
        "AAPL",
        5,
        11
    );

    auto sell2 = engine.submit_limit_order(
        matching::Side::Sell,
        101,
        "AAPL",
        4,
        22
    );

    engine.print_book();

    auto buy = engine.submit_limit_order(
        matching::Side::Buy,
        101,
        "AAPL",
        7,
        33
    );

    for (const auto* result : {&sell1, &sell2, &buy}) {
        if (result->status != matching::Status::Success) {
            std::cerr << result->message << '\n';
            return 1;
        }
    }

    std::cout << "\nGenerated trades:\n";

    for (const matching::Trade& trade : buy.trades) {
        std::cout
            << trade.symbol << " | Buy order " << trade.buy_order_id
            << " (trader " << trade.buyer_id << ")"
            << " matched sell order " << trade.sell_order_id
            << " (trader " << trade.seller_id << ")"
            << ": " << trade.quantity
            << " units at " << trade.price
            << '\n';
    }

    engine.print_book();

    return 0;
}
