#include "matching_engine.hpp"

#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

// This executable alone replaces allocation. Never enable failure injection while
// printing or asserting: only the submission under test should be interrupted.
namespace {
    long allocations_before_failure = -1;
}

void* operator new(std::size_t size) {
    if (allocations_before_failure == 0) {
        allocations_before_failure = -1;
        throw std::bad_alloc{};
    }
    if (allocations_before_failure > 0) --allocations_before_failure;
    if (void* memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void check_side(matching::Side side, bool existing_level) {
    using namespace matching;
    int failures = 0;
    int rollbacks = 0;
    bool reached_success = false;
    // Repeat with a fresh engine, failing each successive allocation. Exercise
    // both creating a price level and appending to an existing FIFO queue.
    for (long fail_at = 0; fail_at < 64; ++fail_at) {
        MatchingEngine engine{"AAPL"};
        auto original = engine.submit_limit_order(side, 100, "AAPL", 3, 11);
        const Price price = existing_level ? 100 : 101;
        bool threw = false;
        allocations_before_failure = fail_at;
        try {
            engine.submit_limit_order(side, price, "AAPL", 2, 22);
        } catch (const std::bad_alloc&) {
            threw = true;
        } catch (...) {
            allocations_before_failure = -1;
            throw;
        }
        allocations_before_failure = -1;
        if (threw) ++failures;

        const auto first = engine.get_order(original.order_id);
        const auto candidate = engine.get_order(original.order_id + 1);
        require(first && first->quantity == 3 && first->trader_id == 11,
                "failed insertion changed an existing order");
        const auto total = side == Side::Buy ? engine.get_bids_quantity_at(price)
                                             : engine.get_asks_quantity_at(price);
        require(total == (existing_level ? 3 : 0) + (candidate ? 2 : 0),
                "list and index disagree after allocation failure");
        // A throw while constructing the final success message happens AFTER
        // insertion. That is outside the helper's rollback guarantee, so an
        // indexed, fully consistent candidate is allowed in that case.
        if (candidate) {
            require(candidate->quantity == 2 && candidate->trader_id == 22,
                    "candidate identity or quantity corrupted");
            if (existing_level) {
                const auto best = side == Side::Buy ? engine.get_best_bid() : engine.get_best_ask();
                require(best && best->id == original.order_id, "existing FIFO priority changed");
            }
            require(engine.cancel_order(candidate->id), "indexed candidate cannot be cancelled");
        } else if (threw) ++rollbacks;

        // Removing the original exposes any empty price level left behind.
        require(engine.cancel_order(original.order_id), "original cannot be cancelled");
        require(!engine.get_best_bid() && !engine.get_best_ask(), "empty price level or orphan order survived rollback");
        if (!threw) { reached_success = true; break; }
    }
    require(reached_success && failures > 0 && rollbacks > 0, "failure sweep did not exercise rollback and success");
    std::cout << "PASS: " << (side == Side::Buy ? "buy" : "sell")
              << (existing_level ? " existing level" : " new level")
              << ": " << failures << " allocation failures checked\n";
}

int main() {
    try {
        for (auto side : {matching::Side::Buy, matching::Side::Sell}) {
            check_side(side, false);
            check_side(side, true);
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
