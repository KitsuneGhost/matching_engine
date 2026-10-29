#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>
#include <optional>

namespace matching {

    // adding an order
    AddOrderResult OrderBook::add_order(Order order) {
        if (order.side == Side::Buy) {
            return add_buy_order(order);
        }

        return add_sell_order(order);
    }

    // adds the buy order
    AddOrderResult OrderBook::add_buy_order(Order order) {
        std::vector<Trade> trades;

        while (order.quantity > 0 && !asks_.empty()) {

            // get the first element
            auto best_ask_it = asks_.begin();

            // best sell price
            Price best_ask_price = best_ask_it->first;

            // if the best selling price is higher than the order -> abort
            // the buyer simply doesnt want to pay that much
            if (best_ask_price > order.price) {break;}

            // first deque of orders with the best price
            OrderQueue& orders_at_price = best_ask_it -> second;

            // keep making trades
            while (order.quantity > 0 && !orders_at_price.empty()) {
                // first order
                Order& sell_order = orders_at_price.front();

                // how much of a prduct we trade
                Quantity traded_quantity = std::min(order.quantity, sell_order.quantity);

                // adding trade to trades list
                trades.push_back(Trade {
                    .buy_order_id = order.id,
                    .sell_order_id = sell_order.id,
                    .quantity = traded_quantity,
                    .price = sell_order.price
                });

                order.quantity -= traded_quantity;
                sell_order.quantity -= traded_quantity;

                // if sell order ran out of product - pick next best order
                if (sell_order.quantity <= 0) {
                    order_index_.erase(sell_order.id);   // deleting the order id from index list
                    orders_at_price.pop_front();
                }
            }

            // erase the price when there are no orders for it
            if (orders_at_price.empty()) {
                asks_.erase(best_ask_it);
            }
        }

        // if the order is not finished -> save it
        if (order.quantity > 0) {
            // a list of orders at current order's price
            auto& queue = bids_[order.price];
            queue.push_back(order);

            // iterator to current order in the list
            auto order_it = std::prev(queue.end());

            order_index_.emplace(order.id, OrderLocation {
                order.side,
                order.price,
                order_it,
            });
        }

        return AddOrderResult {
            .remaining_quantity = order.quantity,
            .trades = std::move(trades)
        };
    }

    // essentially the same as add_buy_order, but for selling orders
    AddOrderResult OrderBook::add_sell_order(Order order) {
        std::vector<Trade> trades;

        while (order.quantity > 0 && !bids_.empty()) {

            // getting best bids (highest buy price)
            auto best_bid_it = bids_.begin();
            Price best_bid_price = best_bid_it->first;

            // The seller is not willing to sell this cheaply
            if (best_bid_price < order.price) {
                break;
            }

            OrderQueue& orders_at_price = best_bid_it->second;

            while (order.quantity > 0 && !orders_at_price.empty()) {
                Order& buy_order = orders_at_price.front();

                Quantity traded_quantity = std::min(order.quantity, buy_order.quantity);

                trades.push_back(Trade{
                    .buy_order_id = buy_order.id,
                    .sell_order_id = order.id,
                    .quantity = traded_quantity,
                    .price = buy_order.price
                });

                order.quantity -= traded_quantity;
                buy_order.quantity -= traded_quantity;

                if (buy_order.quantity <= 0) {
                    order_index_.erase(buy_order.id);
                    orders_at_price.pop_front();
                }
            }

            if (orders_at_price.empty()) {
                bids_.erase(best_bid_it);
            }
        }

        if (order.quantity > 0) {
            auto& queue = asks_[order.price];
            queue.push_back(order);

            auto order_it = std::prev(queue.end());
            order_index_.emplace(order.id, OrderLocation {
                order.side,
                order.price,
                order_it
            });
        }

        return AddOrderResult{
            .remaining_quantity = order.quantity,
            .trades = trades
        };
    }

    // deletes an order from the book
    bool OrderBook::cancel_order(OrderId order_id) {
        // order's iterator
        auto index_it = order_index_.find(order_id);
        // if not in order_list return false (order doesnt exist)
        if (index_it == order_index_.end()) {return false;}

        const auto& location = index_it->second;

        // figuring out which map order is located in and deleting it
        if (location.side == Side::Buy) {
            auto price_level_it = bids_.find(location.price);
            auto& queue = price_level_it->second;   // actual list with orders

            queue.erase(location.iterator);

            // if price level is now empty -> delete it
            if (queue.empty()) {bids_.erase(price_level_it);}
        } else {
            auto price_level_it = asks_.find(location.price);
            auto& queue = price_level_it->second;

            queue.erase(location.iterator);

            if (queue.empty()) {asks_.erase(price_level_it);}
        }

        order_index_.erase(index_it);
        return true;
    }

    // prints the book
    void OrderBook::print_book() const {
        std::cout << "\n=== ORDER BOOK ===\n";

        std::cout << "\nASKS\n";

        // printing asks stats
        for (const auto& [price, orders] : asks_) {
            Quantity total_quantity = 0;

            for (const Order& order : orders) {
                if (order.quantity > std::numeric_limits<Quantity>::max() - total_quantity) {
                    throw std::overflow_error("Price-level quantity overflow!");
                }
                total_quantity += order.quantity;
            }

            std::cout
                << "Price: " << price
                << " | Quantity: " << total_quantity
                << " | Orders: " << orders.size()
                << "\n";
        }

        std::cout << "\nBIDS\n";

        // printing bids stats
        for (const auto& [price, orders] : bids_) {
            Quantity total_quantity = 0;

            for (const Order& order : orders) {
                if (order.quantity > std::numeric_limits<Quantity>::max() - total_quantity) {
                    throw std::overflow_error("Price-level quantity overflow!");
                }
                total_quantity += order.quantity;
            }

            std::cout
                << "Price: " << price
                << " | Quantity: " << total_quantity
                << " | Orders: " << orders.size()
                << "\n";
        }

        std::cout << "==================\n";
    }

    std::optional<Order> OrderBook::get_best_bid() const {
        if (bids_.empty()) return std::nullopt;     // in case the map is empty to avoid dereferencing .begin()
        auto best_bid_it = bids_.begin();

        const OrderQueue& orders = best_bid_it->second;
        return orders.front();
    }

    std::optional<Order> OrderBook::get_best_ask() const {
        if (asks_.empty()) return std::nullopt;
        auto best_ask_it = asks_.begin();

        const OrderQueue& asks = best_ask_it->second;
        return asks.front();
    }

    Quantity OrderBook::get_bids_quantity_at(Price price) const {
        auto it = bids_.find(price); // iterator
        if (it == bids_.end()) return 0;

        const auto& orders = it->second;
        Quantity quantity = 0;

        for (const Order& order : orders) {
            // checking for overflows
            if (order.quantity > std::numeric_limits<Quantity>::max() - quantity) {
                throw std::overflow_error("Price-level quantity overflow!");
            }
            quantity += order.quantity;
        }

        return quantity;
    }

    Quantity OrderBook::get_asks_quantity_at(Price price) const {
        // this method is almost identical to OrderBook::get_bids_quantity_at
        auto it = asks_.find(price);
        if (it == asks_.end()) return 0;

        const auto& orders = it->second;
        Quantity quantity = 0;

        for (const Order& order : orders) {
            if (order.quantity > std::numeric_limits<Quantity>::max() - quantity) {
                throw std::overflow_error("Price-level quantity overflow!");
            }
            quantity += order.quantity;
        }

        return quantity;
    }

    std::optional<Order> OrderBook::get_order(OrderId id) const {
        auto index_it = order_index_.find(id);
        if (index_it == order_index_.end()) {return std::nullopt;}

        auto& location = index_it->second;

        return *location.iterator;
    }
}; // end of namespace matching
