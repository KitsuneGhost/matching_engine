#pragma once

#include "trade.hpp"
#include "order.hpp"
#include "types.hpp"
#include <vector>

namespace matching {

    class OrderBook {
    public:
        std::vector<Trade> add_order(Order order);
        bool cancel_order(OrderId order_id);
        void print_book() const;

    private:

        // Coming soon:
        // bids_
        // asks_
    };
};
