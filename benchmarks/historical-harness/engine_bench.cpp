#include "matching_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/resource.h>

using namespace matching;
using Clock = std::chrono::steady_clock;

struct Command {
    char kind;
    int side;
    Price price;
    Quantity quantity;
    OrderId id;
};

struct Digest {
    std::uint64_t value = 14695981039346656037ULL;
    void add(std::uint64_t x) { value = (value ^ x) * 1099511628211ULL; }
};

void record(Digest& d, const SubmitResult& r) {
    d.add(r.order_id);
    d.add(r.remaining_quantity);
    d.add(r.trades.size());

    for (const auto& t : r.trades) {
        d.add(t.buy_order_id); d.add(t.sell_order_id);
        d.add(t.quantity); d.add(t.price);
    }
}

std::uint64_t percentile(std::vector<std::uint64_t> values, double p) {
    if (values.empty()) return 0;
    std::sort(values.begin(), values.end());
    auto rank = static_cast<std::size_t>(p * values.size() + 0.999999);

    return values[std::max<std::size_t>(1, rank) - 1];
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: engine_bench TRACE batch|latency");

        const bool latency = std::string(argv[2]) == "latency";
        if (!latency && std::string(argv[2]) != "batch") throw std::runtime_error("invalid mode");

        std::ifstream in(argv[1]);
        std::size_t prefill, count;
        OrderId last_id;
        std::uint64_t expected_events, expected_state;

        if (!(in >> prefill >> count >> last_id >> expected_events >> expected_state))
            throw std::runtime_error("invalid trace header");

        std::vector<Command> commands(prefill + count);
        for (auto& c : commands)
            if (!(in >> c.kind >> c.side >> c.price >> c.quantity >> c.id))
                throw std::runtime_error("truncated trace");

        MatchingEngine engine;
        Digest events;

        auto execute = [&](const Command& c, bool timed, std::vector<std::uint64_t>& samples) {
            if (c.kind == 'S') {
                auto start = timed ? Clock::now() : Clock::time_point{};
                auto r = engine.submit_limit_order(c.side == 0 ? Side::Buy : Side::Sell, c.price, c.quantity);

                if (timed) samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count());
                record(events, r);
            } else {
                auto start = timed ? Clock::now() : Clock::time_point{};
                bool canceled = engine.cancel_order(c.id);

                if (timed) samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count());
                events.add(canceled);

                if (!canceled) throw std::runtime_error("cancellation targeted a non-live order");
            }
        };

        std::vector<std::uint64_t> submits, cancels;
        submits.reserve(count); cancels.reserve(count);

        for (std::size_t i = 0; i < prefill; ++i) execute(commands[i], false, submits);
        const auto start = Clock::now();

        for (std::size_t i = prefill; i < commands.size(); ++i)
            execute(commands[i], latency, commands[i].kind == 'S' ? submits : cancels);

        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();

        // Full state verification is outside the measured interval, including absent IDs.
        Digest state;
        std::size_t active = 0;
        for (OrderId id = 1; id <= last_id; ++id) {
            auto order = engine.get_order(id);
            state.add(id); state.add(order.has_value());

            if (order) {
                ++active;
                state.add(order->side == Side::Buy ? 0 : 1);
                state.add(order->price); state.add(order->quantity);
            }
        }

        if (events.value != expected_events || state.value != expected_state)
            throw std::runtime_error("reference digest mismatch");

        struct rusage usage{};
        getrusage(RUSAGE_SELF, &usage);

        std::cout << "{\"elapsed_ns\":" << elapsed
                  << ",\"operations\":" << count
                  << ",\"active_final\":" << active
                  << ",\"events_digest\":" << events.value
                  << ",\"state_digest\":" << state.value
                  << ",\"process_peak_rss_kib\":" << usage.ru_maxrss
                  << ",\"submit_p50_ns\":" << percentile(submits, .50)
                  << ",\"submit_p99_ns\":" << percentile(submits, .99)
                  << ",\"cancel_p50_ns\":" << percentile(cancels, .50)
                  << ",\"cancel_p99_ns\":" << percentile(cancels, .99) << "}\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
