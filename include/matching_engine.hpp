#pragma once

#include "order_book.hpp"
#include "trade.hpp"
#include "types.hpp"

#include <optional>
#include <vector>

namespace matching {

    struct SubmitResult {
        OrderId order_id;
        Quantity remaining_quantity;
        std::vector<Trade> trades;
    };

    class MatchingEngine {
    public:
        // a method to submit a new order
        SubmitResult submit_limit_order(
            Side side,
            Price price,
            Quantity quantity
        );

        bool cancel_order(OrderId order_id);

        void print_book() const;

        std::optional<Order> get_best_bid() const;
        std::optional<Order> get_best_ask() const;

        Quantity get_bids_quantity_at(Price price) const;
        Quantity get_asks_quantity_at(Price price) const;

        std::optional<Order> get_order(OrderId id) const;

    private:
        OrderId next_order_id_{1};
        OrderBook order_book_;
    };

} // namespace matching
