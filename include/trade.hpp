#pragma once

#include "types.hpp"

namespace matching {

    struct Trade {
        OrderId buy_order_id;
        OrderId sell_order_id;
        Quantity quantity;
        Price price;

        // Symbol symbol;
        // Timestamp timestamp;
        // TraderId trader_id;
    };

}
