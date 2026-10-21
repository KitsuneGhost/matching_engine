#pragma once
#include "cstdint"
#include <string>

// types definitions
namespace matching {

    enum class Status {
        Success, Error, Warning
    };

    using Message = std::string;
    using OrderId = std::uint64_t;
    using Price = std::int64_t;
    using Quantity = std::uint64_t;

}
