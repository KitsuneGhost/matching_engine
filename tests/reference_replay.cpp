#include "matching_engine.hpp"

#include <cstdint>
#include <iostream>

// Same canonical digest format as benchmarks/run.py; no timing or benchmark code.
struct Digest {
    std::uint64_t value = 14695981039346656037ULL;
    void add(std::uint64_t x) { value = (value ^ x) * 1099511628211ULL; }
};

int main() {
    matching::MatchingEngine engine{"AAPL"};
    matching::OrderId last_id = 0;
    Digest events;
    char kind;
    while (std::cin >> kind) {
        if (kind == 'S') {
            int side;
            matching::Price price;
            matching::Quantity quantity;
            if (!(std::cin >> side >> price >> quantity)) return 1;
            auto r = engine.submit_limit_order(side == 0 ? matching::Side::Buy : matching::Side::Sell,
                                              price, "AAPL", quantity, 42);
            if (r.status != matching::Status::Success) return 1;
            last_id = r.order_id;
            events.add(r.order_id); events.add(r.remaining_quantity); events.add(r.trades.size());
            for (const auto& t : r.trades) {
                events.add(t.buy_order_id); events.add(t.sell_order_id);
                events.add(t.quantity); events.add(t.price);
            }
        } else if (kind == 'C') {
            matching::OrderId id;
            if (!(std::cin >> id)) return 1;
            events.add(engine.cancel_order(id));
        } else return 1;

        Digest state;
        for (matching::OrderId id = 1; id <= last_id; ++id) {
            auto order = engine.get_order(id);
            state.add(id); state.add(order.has_value());
            if (order) {
                state.add(order->side == matching::Side::Buy ? 0 : 1);
                state.add(order->price); state.add(order->quantity);
            }
        }
        std::cout << events.value << ' ' << state.value << '\n';
    }
    return 0;
}
