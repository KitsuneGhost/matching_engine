#pragma once

#include "trade.hpp"
#include "order.hpp"
#include "types.hpp"

#include <deque>
#include <functional>
#include <map>
#include <vector>

namespace matching {

    struct AddOrderResult {
        Quantity remaining_quantity;
        std::vector<Trade> trades;
    };

    class OrderBook {

        public:
            AddOrderResult add_order(Order order);
            bool cancel_order(OrderId order_id);
            void print_book() const;

        private:
            using OrderQueue = std::deque<Order>;

            // highest buy price first
            // each price corresponds to a deque of orders
            std::map<Price, OrderQueue, std::greater<Price>> bids_;

            // lowest sell price first
            // each price corresponds to a deque of orders
            std::map<Price, OrderQueue> asks_;

            AddOrderResult add_buy_order(Order order);
            AddOrderResult add_sell_order(Order order);
    };
};
