#include "algorithms.hpp"
#include "validator.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

std::string formatTime(int minutesSinceDay0) {
    int day = minutesSinceDay0 / 1440;
    int rem = minutesSinceDay0 % 1440;
    int hour = rem / 60;
    int minute = rem % 60;
    std::ostringstream oss;
    if (day > 0) {
        oss << "D" << day << " ";
    }
    oss << std::setfill('0') << std::setw(2) << hour << ":" << std::setw(2) << minute;
    return oss.str();
}

void printAlgorithmReport(
    const Result& result,
    const std::vector<Request>& requests,
    const std::vector<Room>& rooms,
    const std::vector<Booking>& fixedBookings,
    const Config& config
) {
    std::cout << "\n======================================================\n";
    std::cout << "Algorithm: " << result.algorithmName << "\n";
    std::cout << "======================================================\n";

    // Validate schedule
    auto violations = validate(requests, rooms, fixedBookings, result.assignments, config);

    std::cout << "Assignments (" << result.assignments.size() << "):\n";
    if (result.assignments.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& asgn : result.assignments) {
            std::string reqTitle;
            int startMin = 0, endMin = 0, tier = 1;
            for (const auto& r : requests) {
                if (r.id == asgn.requestId) {
                    reqTitle = r.title;
                    startMin = r.getAbsoluteStart();
                    endMin = r.getAbsoluteEnd();
                    tier = r.tier;
                    break;
                }
            }
            std::string roomName;
            for (const auto& rm : rooms) {
                if (rm.id == asgn.roomId) {
                    roomName = rm.name;
                    break;
                }
            }
            std::cout << "  - Req #" << asgn.requestId << " (\"" << reqTitle << "\", Tier " << tier << ") "
                      << "--> Room " << asgn.roomId << " (" << roomName << ") "
                      << "[" << formatTime(startMin) << " - " << formatTime(endMin) << ")\n";
        }
    }

    std::cout << "Unallocated Requests (" << result.unallocated.size() << "):\n";
    if (result.unallocated.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& un : result.unallocated) {
            std::string reqTitle;
            for (const auto& r : requests) {
                if (r.id == un.requestId) {
                    reqTitle = r.title;
                    break;
                }
            }
            std::cout << "  - Req #" << un.requestId << " (\"" << reqTitle << "\"): "
                      << unallocatedReasonToString(un.reason) << "\n";
        }
    }

    if (!result.warnings.empty()) {
        std::cout << "Warnings:\n";
        for (const auto& w : result.warnings) {
            std::cout << "  [WARN] " << w << "\n";
        }
    }

    std::cout << "Validation: ";
    if (violations.empty()) {
        std::cout << "PASSED (0 violations)\n";
    } else {
        std::cout << "FAILED (" << violations.size() << " violations):\n";
        for (const auto& v : violations) {
            std::cout << "  [VIOLATION] " << v << "\n";
        }
    }
}

} // anonymous namespace

int main() {
    std::cout << "===============================================================\n";
    std::cout << "   Campus Resource Allocation Optimizer - Console Demo (Phase 1)\n";
    std::cout << "===============================================================\n";

    // Seed Data
    // Facilities bitmask:
    // Bit 0 (1): Projector
    // Bit 1 (2): Audio System
    // Bit 2 (4): Workstations
    std::vector<Room> rooms = {
        {101, "Main Auditorium", 300, 1 | 2, true},   // Projector + Audio, Active
        {102, "Seminar Hall A", 80, 1, true},         // Projector, Active
        {103, "Computer Lab 1", 40, 4, true},         // Workstations, Active
        {104, "Conference Room B", 20, 1 | 2, false}  // Projector + Audio, INACTIVE
    };

    // 2 Fixed Bookings
    std::vector<Booking> fixedBookings = {
        {1, 101, 540, 630},  // Room 101: 09:00 - 10:30
        {2, 102, 840, 960}   // Room 102: 14:00 - 16:00
    };

    // 8 Requests
    std::vector<Request> requests = {
        // Req 1: High priority keynote, needs Audio+Projector (1|2=3). Conflicts with Room 101 booking!
        {1, "ACM", "Keynote Address", 0, 540, 660, 250, 1 | 2, 4, 100},
        // Req 2: Tier 3 workshop, fits Room 101 after 10:30
        {2, "Robotics", "Hands-on Workshop", 0, 630, 750, 100, 1 | 2, 3, 110},
        // Req 3: Tier 2 contest needing Workstations (4). Fits Room 103.
        {3, "CodingClub", "Algorithm Contest", 0, 600, 840, 35, 4, 2, 120},
        // Req 4: Tier 1 poetry slam, needs Projector (1).
        {4, "Literary", "Poetry Slam", 0, 660, 780, 45, 1, 1, 130},
        // Req 5: Tier 2 long symposium, overlaps multiple slots
        {5, "DramaClub", "Theatre Symposium", 0, 540, 1020, 60, 0, 2, 80},
        // Req 6: Tier 2 guest lecture, 14:00 - 16:00
        {6, "IEEE", "Guest Lecture", 0, 840, 960, 70, 1, 2, 140},
        // Req 7: Tier 1 evening meetup
        {7, "ArtClub", "Evening Sketching", 0, 960, 1080, 20, 0, 1, 150},
        // Req 8: Invalid request (end <= start: 12:00 to 11:00)
        {8, "Gaming", "Speedrunning", 0, 720, 660, 15, 0, 1, 160}
    };

    Config config;
    config.facilityCheckEnabled = true;
    config.maxExhaustiveRequests = 12;

    std::cout << "\nInput Seed Summary:\n";
    std::cout << "  Rooms: " << rooms.size() << " (1 inactive)\n";
    std::cout << "  Fixed Bookings: " << fixedBookings.size() << "\n";
    std::cout << "  Requests: " << requests.size() << " (1 invalid time)\n";

    // Execute Algorithms
    Result resA = priority_greedy::run(requests, rooms, fixedBookings, config);
    Result resB = earliest_finish::run(requests, rooms, fixedBookings, config);
    Result resC = exhaustive::run(requests, rooms, fixedBookings, config);

    // Print Detailed Per-Algorithm Output
    printAlgorithmReport(resA, requests, rooms, fixedBookings, config);
    printAlgorithmReport(resB, requests, rooms, fixedBookings, config);
    printAlgorithmReport(resC, requests, rooms, fixedBookings, config);

    // Validate all to get violation counts
    size_t violA = validate(requests, rooms, fixedBookings, resA.assignments, config).size();
    size_t violB = validate(requests, rooms, fixedBookings, resB.assignments, config).size();
    size_t violC = validate(requests, rooms, fixedBookings, resC.assignments, config).size();

    // Summary Comparison Table
    std::cout << "\n========================================================================================\n";
    std::cout << "                               ALGORITHM COMPARISON TABLE                                \n";
    std::cout << "========================================================================================\n";
    std::cout << std::left << std::setw(32) << "Algorithm"
              << std::right << std::setw(12) << "Allocated"
              << std::setw(16) << "Total weight"
              << std::setw(16) << "Runtime (ms)"
              << std::setw(14) << "Violations"
              << "\n";
    std::cout << std::string(90, '-') << "\n";

    auto printTableRow = [](const Result& r, size_t viols) {
        std::cout << std::left << std::setw(32) << r.algorithmName
                  << std::right << std::setw(12) << r.allocatedCount
                  << std::setw(16) << r.totalWeight
                  << std::setw(16) << std::fixed << std::setprecision(4) << r.runtimeMs
                  << std::setw(14) << viols
                  << "\n";
    };

    printTableRow(resA, violA);
    printTableRow(resB, violB);
    printTableRow(resC, violC);
    std::cout << "========================================================================================\n";

    return 0;
}
