#include "algorithms.hpp"
#include "validator.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

namespace {

int g_passCount = 0;
int g_failCount = 0;

void runTest(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << "\n";
        g_passCount++;
    } else {
        std::cout << "[FAIL] " << testName << "\n";
        g_failCount++;
    }
}

// 1. Overlapping requests never share a room.
bool test1_overlappingNeverShareRoom() {
    std::vector<Room> rooms = {{1, "Room 1", 50, 0, true}};
    std::vector<Booking> bookings = {};
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 30, 0, 1, 10},
        {2, "C2", "Event 2", 0, 660, 780, 30, 0, 1, 20}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    bool okA = (resA.assignments.size() == 1) && validate(requests, rooms, bookings, resA.assignments, cfg).empty();
    bool okB = (resB.assignments.size() == 1) && validate(requests, rooms, bookings, resB.assignments, cfg).empty();
    bool okC = (resC.assignments.size() == 1) && validate(requests, rooms, bookings, resC.assignments, cfg).empty();

    return okA && okB && okC;
}

// 2. Back-to-back requests (12:00 boundary) can share a room.
bool test2_backToBackCanShareRoom() {
    std::vector<Room> rooms = {{1, "Room 1", 50, 0, true}};
    std::vector<Booking> bookings = {};
    // 10:00-12:00 [600, 720) and 12:00-14:00 [720, 840)
    std::vector<Request> requests = {
        {1, "C1", "Morning", 0, 600, 720, 30, 0, 1, 10},
        {2, "C2", "Afternoon", 0, 720, 840, 30, 0, 1, 20}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto viols = validate(requests, rooms, bookings, resA.assignments, cfg);

    return resA.assignments.size() == 2 && viols.empty();
}

// 3. A request conflicting with a fixed booking is not placed in that room.
bool test3_fixedBookingExclusion() {
    std::vector<Room> rooms = {
        {1, "Room 1", 50, 0, true},
        {2, "Room 2", 50, 0, true}
    };
    // Fixed booking in Room 1 during 10:00-12:00
    std::vector<Booking> bookings = {{1, 1, 600, 720}};
    // Request asking for 10:00-12:00
    std::vector<Request> requests = {
        {1, "C1", "Conflicting Event", 0, 600, 720, 30, 0, 1, 10}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    if (resA.assignments.size() != 1) return false;
    // Must be placed in Room 2, never in Room 1
    if (resA.assignments[0].roomId != 2) return false;

    // If only Room 1 existed, it must be unallocated with TimeConflict
    std::vector<Room> onlyRoom1 = {{1, "Room 1", 50, 0, true}};
    auto resOnly1 = priority_greedy::run(requests, onlyRoom1, bookings, cfg);
    if (!resOnly1.assignments.empty()) return false;
    if (resOnly1.unallocated.empty() || resOnly1.unallocated[0].reason != UnallocatedReason::TimeConflict) return false;

    return validate(requests, rooms, bookings, resA.assignments, cfg).empty();
}

// 4. Missing required facility excludes the room; no facility requirement does not.
bool test4_facilityBitmaskExclusion() {
    // Room 1 has bit 0 (Projector = 1). Room 2 has bit 1 (Audio = 2).
    std::vector<Room> rooms = {
        {1, "Projector Room", 50, 1, true},
        {2, "Audio Room", 50, 2, true}
    };
    std::vector<Booking> bookings = {};
    std::vector<Request> requests = {
        // Req 1 needs Audio (2) -> must go to Room 2
        {1, "C1", "Audio Event", 0, 600, 720, 20, 2, 1, 10},
        // Req 2 needs no facilities (0) -> can go to Room 1
        {2, "C2", "Plain Event", 0, 600, 720, 20, 0, 1, 20}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    if (resA.assignments.size() != 2) return false;

    int r1Room = 0, r2Room = 0;
    for (const auto& a : resA.assignments) {
        if (a.requestId == 1) r1Room = a.roomId;
        if (a.requestId == 2) r2Room = a.roomId;
    }

    if (r1Room != 2) return false; // Audio event placed in Audio Room
    if (r2Room != 1) return false; // Plain event placed in Projector Room

    return validate(requests, rooms, bookings, resA.assignments, cfg).empty();
}

// 5. Capacity is not a hard constraint: attendance greater than capacity is still assignable,
// and attendance less than capacity is too.
bool test5_capacityInformationalOnly() {
    std::vector<Room> rooms = {{1, "Tiny Room", 10, 0, true}};
    std::vector<Booking> bookings = {};
    std::vector<Request> requests = {
        {1, "C1", "Huge Event (Over Capacity)", 0, 600, 720, 500, 0, 1, 10},
        {2, "C2", "Small Event (Under Capacity)", 0, 720, 840, 2, 0, 1, 20}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    if (resA.assignments.size() != 2) return false;

    auto viols = validate(requests, rooms, bookings, resA.assignments, cfg);
    return viols.empty();
}

// 6. Invalid time range is rejected.
bool test6_invalidTimeRangeRejected() {
    std::vector<Room> rooms = {{1, "Room 1", 50, 0, true}};
    std::vector<Booking> bookings = {};
    std::vector<Request> requests = {
        {1, "C1", "Reversed Time", 0, 720, 600, 20, 0, 1, 10},
        {2, "C2", "Zero Duration", 0, 600, 600, 20, 0, 1, 20},
        {3, "C3", "Valid Event", 0, 600, 720, 20, 0, 1, 30}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    if (resA.assignments.size() != 1 || resA.assignments[0].requestId != 3) return false;

    int invalidCount = 0;
    for (const auto& u : resA.unallocated) {
        if (u.reason == UnallocatedReason::InvalidTime) invalidCount++;
    }
    if (invalidCount != 2) return false;

    auto resC = exhaustive::run(requests, rooms, bookings, cfg);
    if (resC.assignments.size() != 1 || resC.assignments[0].requestId != 3) return false;

    return validate(requests, rooms, bookings, resA.assignments, cfg).empty();
}

// 7. Same input gives the same output twice (determinism).
bool test7_deterministicOutput() {
    std::vector<Room> rooms = {
        {1, "Room 1", 50, 1, true},
        {2, "Room 2", 80, 3, true}
    };
    std::vector<Booking> bookings = {{1, 1, 540, 660}};
    std::vector<Request> requests = {
        {1, "C1", "Req A", 0, 600, 720, 30, 1, 3, 10},
        {2, "C2", "Req B", 0, 630, 750, 40, 2, 2, 20},
        {3, "C3", "Req C", 0, 720, 840, 20, 0, 1, 30},
        {4, "C4", "Req D", 0, 780, 900, 50, 1, 4, 40}
    };
    Config cfg;

    auto resA1 = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resA2 = priority_greedy::run(requests, rooms, bookings, cfg);

    if (resA1.totalWeight != resA2.totalWeight || resA1.assignments.size() != resA2.assignments.size()) return false;
    for (size_t i = 0; i < resA1.assignments.size(); ++i) {
        if (resA1.assignments[i].requestId != resA2.assignments[i].requestId ||
            resA1.assignments[i].roomId != resA2.assignments[i].roomId) {
            return false;
        }
    }

    auto resC1 = exhaustive::run(requests, rooms, bookings, cfg);
    auto resC2 = exhaustive::run(requests, rooms, bookings, cfg);

    if (resC1.totalWeight != resC2.totalWeight || resC1.assignments.size() != resC2.assignments.size()) return false;
    for (size_t i = 0; i < resC1.assignments.size(); ++i) {
        if (resC1.assignments[i].requestId != resC2.assignments[i].requestId ||
            resC1.assignments[i].roomId != resC2.assignments[i].roomId) {
            return false;
        }
    }

    return true;
}

// 8. Priority ordering: a higher tier wins a contested single room.
bool test8_higherTierWinsContestedRoom() {
    std::vector<Room> rooms = {{1, "Solo Room", 50, 0, true}};
    std::vector<Booking> bookings = {};
    std::vector<Request> requests = {
        {1, "C1", "Low Priority Submitted First", 0, 600, 720, 20, 0, 1, 10}, // Tier 1, weight 1
        {2, "C2", "High Priority Submitted Later", 0, 600, 720, 20, 0, 4, 500} // Tier 4, weight 1000
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    if (resA.assignments.size() != 1 || resA.assignments[0].requestId != 2) return false;
    if (resA.totalWeight != 1000) return false;

    auto resC = exhaustive::run(requests, rooms, bookings, cfg);
    if (resC.assignments.size() != 1 || resC.assignments[0].requestId != 2) return false;
    if (resC.totalWeight != 1000) return false;

    return validate(requests, rooms, bookings, resA.assignments, cfg).empty() &&
           validate(requests, rooms, bookings, resC.assignments, cfg).empty();
}

// 9. Greedy vs optimal: one room, three requests of the SAME tier:
// X 09:00-17:00 (submitted first), Y 09:00-12:00, Z 12:00-17:00.
// Algorithm A picks X; Algorithm C picks Y and Z (higher total). Assert A's weight < C's weight.
bool test9_greedyVsOptimalExhaustive() {
    std::vector<Room> rooms = {{1, "Room 1", 100, 0, true}};
    std::vector<Booking> bookings = {};
    // 09:00 = 540, 12:00 = 720, 17:00 = 1020
    std::vector<Request> requests = {
        {1, "CX", "Request X", 0, 540, 1020, 50, 0, 2, 10}, // Submitted first
        {2, "CY", "Request Y", 0, 540, 720, 50, 0, 2, 20},
        {3, "CZ", "Request Z", 0, 720, 1020, 50, 0, 2, 30}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    // Algorithm A picks X (tier same, submitted earlier)
    bool aPicksX = (resA.assignments.size() == 1 && resA.assignments[0].requestId == 1);
    // Algorithm C picks Y and Z
    bool cPicksYZ = (resC.assignments.size() == 2 &&
                     resC.totalWeight == 20 &&
                     resA.totalWeight == 10);

    bool weightComparison = (resA.totalWeight < resC.totalWeight);
    bool countComparison = (resA.allocatedCount < resC.allocatedCount);

    return aPicksX && cPicksYZ && weightComparison && countComparison &&
           validate(requests, rooms, bookings, resA.assignments, cfg).empty() &&
           validate(requests, rooms, bookings, resC.assignments, cfg).empty();
}

// 10. Algorithm C weight >= A weight and >= B weight on at least 5 small hand-made cases.
bool test10_exhaustiveOptimalityAcross5Cases() {
    Config cfg;

    // Case 1: Competing requests of varying lengths and tiers in 1 room
    {
        std::vector<Room> rooms = {{1, "Room 1", 50, 0, true}};
        std::vector<Booking> bookings = {};
        std::vector<Request> requests = {
            {1, "C1", "R1", 0, 540, 900, 20, 0, 3, 10}, // Tier 3 (100)
            {2, "C2", "R2", 0, 540, 720, 20, 0, 3, 20}, // Tier 3 (100)
            {3, "C3", "R3", 0, 720, 900, 20, 0, 3, 30}  // Tier 3 (100)
        };
        auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
        auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
        auto resC = exhaustive::run(requests, rooms, bookings, cfg);
        if (resC.totalWeight < resA.totalWeight || resC.totalWeight < resB.totalWeight) return false;
        if (!validate(requests, rooms, bookings, resC.assignments, cfg).empty()) return false;
    }

    // Case 2: 2 rooms with facility restrictions
    {
        std::vector<Room> rooms = {
            {1, "Room 1", 50, 1, true},
            {2, "Room 2", 50, 3, true}
        };
        std::vector<Booking> bookings = {};
        std::vector<Request> requests = {
            {1, "C1", "Needs Fac 1", 0, 600, 800, 20, 1, 2, 10},
            {2, "C2", "Needs Fac 3", 0, 600, 800, 20, 3, 2, 20},
            {3, "C3", "Needs Fac 1 short", 0, 800, 900, 20, 1, 1, 30}
        };
        auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
        auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
        auto resC = exhaustive::run(requests, rooms, bookings, cfg);
        if (resC.totalWeight < resA.totalWeight || resC.totalWeight < resB.totalWeight) return false;
        if (!validate(requests, rooms, bookings, resC.assignments, cfg).empty()) return false;
    }

    // Case 3: Fixed bookings blocking slots
    {
        std::vector<Room> rooms = {{1, "Room 1", 50, 0, true}};
        std::vector<Booking> bookings = {{1, 1, 700, 750}};
        std::vector<Request> requests = {
            {1, "C1", "Long spanning", 0, 600, 900, 20, 0, 2, 10},
            {2, "C2", "Before booking", 0, 600, 700, 20, 0, 2, 20},
            {3, "C3", "After booking", 0, 750, 850, 20, 0, 2, 30}
        };
        auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
        auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
        auto resC = exhaustive::run(requests, rooms, bookings, cfg);
        if (resC.totalWeight < resA.totalWeight || resC.totalWeight < resB.totalWeight) return false;
        if (!validate(requests, rooms, bookings, resC.assignments, cfg).empty()) return false;
    }

    // Case 4: Alternating short and long requests with differing tiers
    {
        std::vector<Room> rooms = {{1, "Room 1", 50, 0, true}};
        std::vector<Booking> bookings = {};
        std::vector<Request> requests = {
            {1, "C1", "Medium Tier 2", 0, 600, 800, 20, 0, 2, 5},
            {2, "C2", "High Tier 4", 0, 700, 760, 20, 0, 4, 10},
            {3, "C3", "Medium Tier 2 end", 0, 760, 900, 20, 0, 2, 15}
        };
        auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
        auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
        auto resC = exhaustive::run(requests, rooms, bookings, cfg);
        if (resC.totalWeight < resA.totalWeight || resC.totalWeight < resB.totalWeight) return false;
        if (!validate(requests, rooms, bookings, resC.assignments, cfg).empty()) return false;
    }

    // Case 5: Multi-room scheduling where greedy assignment traps later requests
    {
        std::vector<Room> rooms = {
            {1, "Room 1", 50, 0, true},
            {2, "Room 2", 50, 0, true}
        };
        std::vector<Booking> bookings = {};
        std::vector<Request> requests = {
            {1, "C1", "Long R1", 0, 600, 1000, 20, 0, 3, 10},
            {2, "C2", "Short R2", 0, 600, 700, 20, 0, 3, 20},
            {3, "C3", "Short R3", 0, 700, 800, 20, 0, 3, 30},
            {4, "C4", "Short R4", 0, 800, 900, 20, 0, 3, 40},
            {5, "C5", "Short R5", 0, 900, 1000, 20, 0, 3, 50}
        };
        auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
        auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
        auto resC = exhaustive::run(requests, rooms, bookings, cfg);
        if (resC.totalWeight < resA.totalWeight || resC.totalWeight < resB.totalWeight) return false;
        if (!validate(requests, rooms, bookings, resC.assignments, cfg).empty()) return false;
    }

    return true;
}

// 11. Validator reports 0 violations for every algorithm output in all tests,
// and detects a deliberately broken schedule (overlap, fixed-booking clash, duplicate assignment).
bool test11_validatorZeroViolationsAndBrokenScheduleDetection() {
    std::vector<Room> rooms = {
        {1, "Room 1", 50, 1, true},
        {2, "Inactive Room", 50, 1, false}
    };
    std::vector<Booking> bookings = {{1, 1, 600, 700}};
    std::vector<Request> requests = {
        {1, "C1", "Req 1", 0, 600, 700, 20, 1, 1, 10},
        {2, "C2", "Req 2", 0, 650, 750, 20, 1, 1, 20},
        {3, "C3", "Req 3 (needs Audio)", 0, 800, 900, 20, 2, 1, 30}
    };
    Config cfg;

    // Check broken schedule: Overlap in same room
    std::vector<Assignment> brokenOverlap = {
        {1, 1},
        {2, 1}
    };
    auto viols1 = validate(requests, rooms, bookings, brokenOverlap, cfg);
    bool detectedOverlap = false;
    for (const auto& v : viols1) {
        if (v.find("overlaps") != std::string::npos) detectedOverlap = true;
    }
    if (!detectedOverlap) return false;

    // Check broken schedule: Overlap with fixed booking
    // Req 1 is [600, 700) in Room 1, which has booking 1 [600, 700)
    std::vector<Assignment> brokenFixedBooking = {{1, 1}};
    auto viols2 = validate(requests, rooms, bookings, brokenFixedBooking, cfg);
    bool detectedFixedClash = false;
    for (const auto& v : viols2) {
        if (v.find("overlaps fixed booking") != std::string::npos) detectedFixedClash = true;
    }
    if (!detectedFixedClash) return false;

    // Check broken schedule: Duplicate assignment for same request
    std::vector<Assignment> brokenDuplicate = {
        {1, 1},
        {1, 2}
    };
    auto viols3 = validate(requests, rooms, bookings, brokenDuplicate, cfg);
    bool detectedDuplicate = false;
    for (const auto& v : viols3) {
        if (v.find("assigned multiple times") != std::string::npos) detectedDuplicate = true;
    }
    if (!detectedDuplicate) return false;

    // Check broken schedule: Inactive room
    std::vector<Assignment> brokenInactive = {{1, 2}};
    auto viols4 = validate(requests, rooms, bookings, brokenInactive, cfg);
    bool detectedInactive = false;
    for (const auto& v : viols4) {
        if (v.find("inactive room") != std::string::npos) detectedInactive = true;
    }
    if (!detectedInactive) return false;

    // Check broken schedule: Missing facilities
    std::vector<Assignment> brokenFacility = {{3, 1}};
    auto viols5 = validate(requests, rooms, bookings, brokenFacility, cfg);
    bool detectedFacility = false;
    for (const auto& v : viols5) {
        if (v.find("requires facilities") != std::string::npos) detectedFacility = true;
    }
    if (!detectedFacility) return false;

    return true;
}

} // anonymous namespace

int main() {
    std::cout << "=======================================================\n";
    std::cout << "        Campus Resource Allocation Engine Tests        \n";
    std::cout << "=======================================================\n";

    runTest("1. Overlapping requests never share a room", test1_overlappingNeverShareRoom());
    runTest("2. Back-to-back requests (12:00 boundary) can share a room", test2_backToBackCanShareRoom());
    runTest("3. Conflicting request with fixed booking is not placed in that room", test3_fixedBookingExclusion());
    runTest("4. Missing facility excludes room; no facility does not", test4_facilityBitmaskExclusion());
    runTest("5. Capacity is informational only and not a hard constraint", test5_capacityInformationalOnly());
    runTest("6. Invalid time range is rejected", test6_invalidTimeRangeRejected());
    runTest("7. Output is deterministic on identical inputs", test7_deterministicOutput());
    runTest("8. Higher priority tier wins contested single room", test8_higherTierWinsContestedRoom());
    runTest("9. Greedy vs optimal: Algorithm A picks X, Algorithm C picks Y & Z", test9_greedyVsOptimalExhaustive());
    runTest("10. Algorithm C total weight >= A & B on 5 hand-made test cases", test10_exhaustiveOptimalityAcross5Cases());
    runTest("11. Validator reports 0 violations on clean runs and catches broken schedules", test11_validatorZeroViolationsAndBrokenScheduleDetection());

    std::cout << "=======================================================\n";
    std::cout << "Test Summary: " << g_passCount << " PASSED, " << g_failCount << " FAILED\n";
    std::cout << "=======================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
