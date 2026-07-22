#include "matching_engine.hpp"

#include <iostream>

int main() {
    matching::MatchingEngine engine;

    auto sell1 = engine.submit_limit_order(
        matching::Side::Sell,
        100,
        5
    );

    auto sell2 = engine.submit_limit_order(
        matching::Side::Sell,
        101,
        4
    );

    engine.print_book();

    auto buy = engine.submit_limit_order(
        matching::Side::Buy,
        101,
        7
    );

    std::cout << "\nGenerated trades:\n";

    for (const matching::Trade& trade : buy.trades) {
        std::cout
            << "Buy order " << trade.buy_order_id
            << " matched sell order " << trade.sell_order_id
            << ": " << trade.quantity
            << " units at " << trade.price
            << '\n';
    }

    engine.print_book();

    return 0;
}
