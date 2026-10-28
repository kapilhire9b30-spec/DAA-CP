#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

enum class UnallocatedReason {
    TimeConflict,
    NoActiveRoom,
    MissingFacility,
    InvalidTime,
    TooLargeForSearch
};

enum class PreferenceMode {
    Flexible = 0,
    Strict = 1
};

inline std::string unallocatedReasonToString(UnallocatedReason reason) {
    switch (reason) {
        case UnallocatedReason::TimeConflict: return "TimeConflict";
        case UnallocatedReason::NoActiveRoom: return "NoActiveRoom";
        case UnallocatedReason::MissingFacility: return "MissingFacility";
        case UnallocatedReason::InvalidTime: return "InvalidTime";
        case UnallocatedReason::TooLargeForSearch: return "TooLargeForSearch";
    }
    return "Unknown";
}

inline int tierToWeight(int tier) {
    switch (tier) {
        case 1: return 1;
        case 2: return 10;
        case 3: return 100;
        case 4: return 1000;
        default: return 0;
    }
}

struct Request {
    int id = 0;
    std::string clubId;
    std::string title;
    int date = 0;        // Day index (0, 1, 2, ...)
    int start = 0;       // Minute of day (0..1439)
    int end = 0;         // Minute of day
    int attendance = 0;
    uint32_t facilities = 0; // Bitmask of required facilities
    int tier = 1;        // Priority tier 1..4
    int64_t submittedAt = 0;
    int preferredRoomId = 0; // 0 = no preference
    PreferenceMode roomMode = PreferenceMode::Flexible;

    int getAbsoluteStart() const {
        return date * 1440 + start;
    }

    int getAbsoluteEnd() const {
        return date * 1440 + end;
    }
};

struct Room {
    int id = 0;
    std::string name;
    int capacity = 0;
    uint32_t facilities = 0; // Bitmask of offered facilities
    bool active = true;
};

struct Booking {
    int id = 0;
    int roomId = 0;
    int start = 0; // Minutes since day 0
    int end = 0;   // Minutes since day 0
};

struct Assignment {
    int requestId = 0;
    int roomId = 0;
};

struct Unallocated {
    int requestId = 0;
    UnallocatedReason reason = UnallocatedReason::TimeConflict;
};

struct Result {
    std::string algorithmName;
    std::vector<Assignment> assignments;
    std::vector<Unallocated> unallocated;
    int totalWeight = 0;
    int allocatedCount = 0;
    int satisfiedPreferences = 0;
    double runtimeMs = 0.0;
    std::vector<std::string> warnings;
};

struct Config {
    bool facilityCheckEnabled = true;
    int maxExhaustiveRequests = 12;
};
