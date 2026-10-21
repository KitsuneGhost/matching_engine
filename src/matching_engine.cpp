#include "matching_engine.hpp"
#include "order.hpp"
#include "types.hpp"

#include <limits>
#include <optional>
#include <stdexcept>
#include <sys/types.h>
#include <utility>

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

        if (side != Side::Buy && side != Side::Sell) {
            throw std::invalid_argument("Side can only be BUY or SELL");
        }

        if (next_order_id_ >= std::numeric_limits<OrderId>::max()) {
            return SubmitResult {
                .status = Status::Error,
                .message = "Order ID space is exhausted!"
            };
        }
        // creating a new order
        Order order{
            .id = next_order_id_++,
            .side = side,
            .price = price,
            .quantity = quantity
        };

        const OrderId order_id = order.id;

        // a struct with the result data
        AddOrderResult result = order_book_.add_order(order);

        return SubmitResult {
            .status = Status::Success,
            .order_id = order_id,
            .remaining_quantity = result.remaining_quantity,
            .trades = std::move(result.trades),
            .message = "Order submitted successfully!"
        };
    }

    bool MatchingEngine::cancel_order(OrderId order_id) {
         return order_book_.cancel_order(order_id);
    }

    void MatchingEngine::print_book() const {
        order_book_.print_book();
    }

    std::optional<Order> MatchingEngine::get_best_bid() const {
        return order_book_.get_best_bid();
    }

    std::optional<Order> MatchingEngine::get_best_ask() const {
        return order_book_.get_best_ask();
    }

    Quantity MatchingEngine::get_bids_quantity_at(Price price) const {
        return order_book_.get_bids_quantity_at(price);
    }

    Quantity MatchingEngine::get_asks_quantity_at(Price price) const {
        return order_book_.get_asks_quantity_at(price);
    }

    std::optional<Order> MatchingEngine::get_order(OrderId id) const {
        return order_book_.get_order(id);
    }

} // end of namespace matching
