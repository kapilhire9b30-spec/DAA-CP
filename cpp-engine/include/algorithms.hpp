#pragma once

#include "types.hpp"
#include <vector>

namespace priority_greedy {
    Result run(
        const std::vector<Request>& requests,
        const std::vector<Room>& rooms,
        const std::vector<Booking>& fixedBookings,
        const Config& config = Config{}
    );
}

namespace earliest_finish {
    Result run(
        const std::vector<Request>& requests,
        const std::vector<Room>& rooms,
        const std::vector<Booking>& fixedBookings,
        const Config& config = Config{}
    );
}

namespace exhaustive {
    Result run(
        const std::vector<Request>& requests,
        const std::vector<Room>& rooms,
        const std::vector<Booking>& fixedBookings,
        const Config& config = Config{}
    );
}
