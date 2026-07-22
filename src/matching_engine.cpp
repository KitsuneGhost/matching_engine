#include "../include/matching_engine.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace matching {

    SubmitResult MatchingEngine::submit_limit_order(
        Side side,
        Price price,
        Quantity quantity
    ) {
        // input validation
        if (price <= 0) {
            throw std::invalid_argument("Price must be greater than 0!");
        }

        if (quantity <= 0) {
            throw std::invalid_argument("Quantity must be greater than 0!");

        }

        // creating a new order
        Order order{
            .id = next_order_id_++,
            .side = side,
            .price = price,
            .quantity = quantity
        };

        const OrderId order_id = order.id;

        // a vector with a new trade
        std::vector<Trade> trades = order_book_.add_order(order).trades;

        return SubmitResult{
            .order_id = order_id,
            .remaining_quantity = order.quantity,
            .trades = std::move(trades)
        };
    }

    bool MatchingEngine::cancel_order(OrderId order_id) {
         return order_book_.cancel_order(order_id);
    }

    void MatchingEngine::print_book() const {
        order_book_.print_book();
    }

} // end of namespace matching
