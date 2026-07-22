#include "../include/order_book.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

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
            if (best_ask_price > order.price) {
                break;
            }

            // first deque of orders with the best price
            OrderQueue& orders_at_price = best_ask_it -> second;

            // keep making trades
            while (order.quantity > 0 && !orders_at_price.empty()) {
                // first order
                Order& sell_order = orders_at_price.front();

                // how much of a prduct we trade
                Quantity traded_quantity = std::min(order.quantity, sell_order.quantity);

                // adding trade to trades list
                trades.push_back(Trade{
                    .buy_order_id = order.id,
                    .sell_order_id = sell_order.id,
                    .quantity = traded_quantity,
                    .price = sell_order.price
                });

                order.quantity -= traded_quantity;
                sell_order.quantity -= traded_quantity;

                // if sell order ran out of product - pick next best order
                if (sell_order.quantity == 0) {
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
            bids_[order.price].push_back(order);
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

                if (buy_order.quantity == 0) {
                    orders_at_price.pop_front();
                }
            }

            if (orders_at_price.empty()) {
                bids_.erase(best_bid_it);
            }
        }

        if (order.quantity > 0) {
            asks_[order.price].push_back(order);
        }

        return AddOrderResult{
            .remaining_quantity = order.quantity,
            .trades = trades
        };
    }

    // deletes an order from the book
    bool OrderBook::cancel_order(OrderId order_id) {
        // looking for order in bids
        for (auto price_it = bids_.begin(); price_it != bids_.end(); ++price_it) {
            OrderQueue& orders = price_it->second;

            // looking for our order
            auto order_it = std::find_if(
                orders.begin(),
                orders.end(),
                [order_id](const Order order) {
                    return order.id == order_id;
                }
            );

            // if orer in deque -> erase it
            if (order_it != orders.end()) {
                orders.erase(order_it);

                // if after erasing there are no more orders with that price tag
                // delete the price tag
                if (orders.empty()) {
                    bids_.erase(price_it);
                }

                // order was deleted
                return true;
            }
        }

        // looking for order in asks (if order was not in bids)
        for (auto price_it = asks_.begin(); price_it != asks_.end(); ++price_it) {
            OrderQueue& orders = price_it->second;

            // looking for our order
            auto order_it = std::find_if(
                orders.begin(),
                orders.end(),
                [order_id](const Order order) {
                    return order.id == order_id;
                }
            );

            // if orer in deque -> erase it
            if (order_it != orders.end()) {
                orders.erase(order_it);

                // if after erasing there are no more orders with that price tag
                // delete the price tag
                if (orders.empty()) {
                    asks_.erase(price_it);
                }

                // order was deleted
                return true;
            }
        }

        // order was NOT deleted
        return false;
    }

    // prints the book
    void OrderBook::print_book() const {
        std::cout << "\n=== ORDER BOOK ===\n";

        std::cout << "\nASKS\n";

        // printing asks stats
        for (const auto& [price, orders] : asks_) {
            Quantity total_quantity = 0;

            for (const Order& order : orders) {
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

}; // end of namespace matching
