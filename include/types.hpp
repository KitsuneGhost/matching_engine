#pragma once
#include "cstdint"
#include <chrono>
#include <string>

// types definitions
namespace matching {

    enum class Status {
        Success, Error, Warning
    };

    using Timestamp = std::chrono::time_point<
        std::chrono::system_clock,
        std::chrono::nanoseconds
    >;

    inline Timestamp current_timestamp() {
        return std::chrono::time_point_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now()
        );
    }

    using Message = std::string;
    using OrderId = std::uint64_t;
    using Price = std::int64_t;
    using Quantity = std::uint64_t;
    using Symbol = std::string;
    using TraderId = std::uint64_t;
}
