#pragma once

#include "types.hpp"

namespace matching {

    enum class Side{
        Buy, Sell
    };

    struct Order {
        OrderId id;
        Side side;
        Price price;
        Quantity quantity;

        // Symbol symbol;
        // Timestamp timestamp;
        // TraderId trader_id;
    };
} // end of namespace
