#pragma once

#include "types.hpp"
#include <vector>
#include <string>

std::vector<std::string> validate(
    const std::vector<Request>& requests,
    const std::vector<Room>& rooms,
    const std::vector<Booking>& fixedBookings,
    const std::vector<Assignment>& assignments,
    const Config& config = Config{}
);
