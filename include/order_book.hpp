#pragma once

#include "trade.hpp"
#include "order.hpp"
#include "types.hpp"

#include <functional>
#include <map>
#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

namespace matching {

    struct AddOrderResult {
        Quantity remaining_quantity;
        std::vector<Trade> trades;
    };

    class OrderBook {
        friend class MatchingEngine;

        public:
            // Allows normal construction
            OrderBook() = default;
            // Forbids constructing a new book by copying another one
            OrderBook(const OrderBook&) = delete;
            // Forbids copying into already existing book
            OrderBook& operator = (const OrderBook&) = delete;

        private:
            using OrderQueue = std::list<Order>;

            struct OrderLocation {
                Side side;
                Price price;
                OrderQueue::iterator iterator;
            };

            // highest buy price first
            // each price corresponds to a deque of orders
            std::map<Price, OrderQueue, std::greater<Price>> bids_;

            // lowest sell price first
            // each price corresponds to a deque of orders
            std::map<Price, OrderQueue> asks_;

            // index map for orders to insure O(1) search
            std::unordered_map<OrderId, OrderLocation> order_index_;

            AddOrderResult add_order(Order order);
            bool cancel_order(OrderId order_id);
            void print_book() const;
            std::optional<Order> get_best_bid() const;
            std::optional<Order> get_best_ask() const;
            Quantity get_bids_quantity_at(Price price) const;
            Quantity get_asks_quantity_at(Price price) const;
            std::optional<Order> get_order(OrderId id) const;
            AddOrderResult add_buy_order(Order order);
            AddOrderResult add_sell_order(Order order);
    };
};
