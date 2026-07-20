#pragma once

#include "order_book.hpp"
#include "trade.hpp"
#include "types.hpp"

#include <vector>

namespace matching {

    struct SubmitResult {
        OrderId order_id;
        Quantity remaining_quantity;
        std::vector<Trade> trades;
    };

    class MatchingEngine {
    public:
        SubmitResult submit_limit_order(
            Side side,
            Price price,
            Quantity quantity
        );

        bool cancel_order(OrderId order_id);

        void print_book() const;

    private:
        OrderId next_order_id_{1};
        OrderBook order_book_;
    };

} // namespace matching
