#include "workflow.hpp"
#include "algorithms.hpp"
#include "validator.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace {

std::string promptLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return "";
    }
    // Trim
    size_t first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = line.find_last_not_of(" \t\r\n");
    return line.substr(first, (last - first + 1));
}

int promptInt(const std::string& prompt, int defaultVal = 0) {
    while (true) {
        std::string s = promptLine(prompt);
        if (s.empty()) return defaultVal;
        try {
            return std::stoi(s);
        } catch (...) {
            std::cout << "Invalid integer. Please try again.\n";
        }
    }
}

int promptDate(const std::string& prompt, int defaultDayIndex = 0) {
    while (true) {
        std::string s = promptLine(prompt);
        if (s.empty()) return defaultDayIndex;
        int dayIndex = 0;
        if (parseDate(s, dayIndex)) {
            return dayIndex;
        }
        std::cout << "Invalid date format. Expected YYYY-MM-DD. Please try again.\n";
    }
}

int promptTime(const std::string& prompt, int defaultMinute = 540) {
    while (true) {
        std::string s = promptLine(prompt);
        if (s.empty()) return defaultMinute;
        int minuteOfDay = 0;
        if (parseTime(s, minuteOfDay)) {
            return minuteOfDay;
        }
        std::cout << "Invalid time format. Expected HH:MM (24-hour). Please try again.\n";
    }
}

void printHeader(const std::string& title) {
    std::cout << "\n=======================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "=======================================================\n";
}

void printComparisonTable(const std::vector<ComparisonRow>& rows) {
    std::cout << "\n" << std::string(88, '-') << "\n";
    std::cout << std::left << std::setw(34) << "Algorithm"
              << std::right << std::setw(12) << "Allocated"
              << std::setw(16) << "Total weight"
              << std::setw(14) << "Runtime (ms)"
              << std::setw(12) << "Violations"
              << "\n";
    std::cout << std::string(88, '-') << "\n";
    for (const auto& r : rows) {
        std::cout << std::left << std::setw(34) << r.algorithmName
                  << std::right << std::setw(12) << r.allocatedCount
                  << std::setw(16) << r.totalWeight
                  << std::setw(14) << std::fixed << std::setprecision(4) << r.runtimeMs
                  << std::setw(12) << r.violations
                  << "\n";
    }
    std::cout << std::string(88, '-') << "\n";
}

// -------------------------------------------------------------
// Club Workflow UI
// -------------------------------------------------------------
void clubMenu(WorkflowEngine& engine) {
    printHeader("Club Portal - Login");
    std::string clubId = promptLine("Enter your Club ID (e.g., ACM, Robotics, CodingClub): ");
    if (clubId.empty()) {
        std::cout << "Club ID cannot be empty.\n";
        return;
    }

    while (true) {
        printHeader("Club Menu - Logged in as: " + clubId);
        std::cout << "1) Submit new resource request\n";
        std::cout << "2) View my requests and status\n";
        std::cout << "0) Back to Main Portal\n";
        int choice = promptInt("Select option [0-2]: ", -1);

        if (choice == 0) break;

        if (choice == 1) {
            printHeader("Submit Request - " + clubId);
            std::string title = promptLine("Event Title: ");
            if (title.empty()) {
                std::cout << "Error: Title cannot be empty.\n";
                continue;
            }
            int dateIdx = promptDate("Event Date (YYYY-MM-DD) [default 2026-10-26]: ", 298);
            int startMin = promptTime("Start Time (HH:MM) [e.g. 09:00]: ", 540);
            int endMin = promptTime("End Time (HH:MM) [e.g. 11:00]: ", 660);
            int attendance = promptInt("Expected Attendance: ", 30);

            std::cout << "Facilities bitmask (0: None, 1: Projector, 2: Audio, 4: Workstations, combinations like 3=1|2):\n";
            int facilities = promptInt("Required Facilities bitmask [default 0]: ", 0);

            std::cout << "Priority Tier (1: Low/1pt, 2: Medium/10pt, 3: High/100pt, 4: Critical/1000pt):\n";
            int tier = promptInt("Priority Tier [1-4, default 1]: ", 1);
            if (tier < 1) tier = 1;
            if (tier > 4) tier = 4;

            StoredRequest req;
            req.req.clubId = clubId;
            req.req.title = title;
            req.req.date = dateIdx;
            req.req.start = startMin;
            req.req.end = endMin;
            req.req.attendance = attendance;
            req.req.facilities = static_cast<uint32_t>(facilities);
            req.req.tier = tier;
            req.req.submittedAt = static_cast<int64_t>(std::time(nullptr));

            auto res = engine.submitRequest(Role::Club, req);
            std::cout << "\n>>> " << res.message << "\n";

        } else if (choice == 2) {
            printHeader("My Requests - " + clubId);
            auto myReqs = engine.getRequestsForClub(clubId);
            if (myReqs.empty()) {
                std::cout << "No requests submitted yet.\n";
                continue;
            }

            for (const auto& sr : myReqs) {
                std::cout << "\nRequest #" << sr.req.id << ": \"" << sr.req.title << "\"\n";
                std::cout << "  Date & Time: " << formatDate(sr.req.date) << " "
                          << formatTime(sr.req.start) << " - " << formatTime(sr.req.end) << "\n";
                std::cout << "  Attendance: " << sr.req.attendance
                          << " | Tier: " << sr.req.tier
                          << " | Facilities: " << sr.req.facilities << "\n";
                std::cout << "  Status: [" << requestStatusToString(sr.status) << "]\n";

                if (sr.status == RequestStatus::Confirmed) {
                    const auto* booking = engine.getBookingForRequest(sr.req.id);
                    if (booking) {
                        std::string roomName = "Unknown Room";
                        for (const auto& rm : engine.getStore().getRooms()) {
                            if (rm.id == booking->roomId) {
                                roomName = rm.name;
                                break;
                            }
                        }
                        std::cout << "  >> CONFIRMED IN: Room " << booking->roomId << " (" << roomName << ") ["
                                  << formatTime(booking->start % 1440) << " - " << formatTime(booking->end % 1440) << ")\n";
                    }
                } else if (sr.status == RequestStatus::Recommended) {
                    std::cout << "  >> Currently under administrative review with an algorithm recommendation.\n";
                } else if (sr.status == RequestStatus::Rejected) {
                    std::cout << "  >> Reason for rejection: " << sr.unallocatedReason << "\n";
                } else if (!sr.unallocatedReason.empty()) {
                    std::cout << "  >> Last scheduling note: " << sr.unallocatedReason << "\n";
                }
            }
        }
    }
}

// -------------------------------------------------------------
// Admin Workflow UI
// -------------------------------------------------------------
void adminReviewRecommendations(WorkflowEngine& engine, const std::string& adminName) {
    auto& recs = engine.getStore().getRecommendations();
    if (recs.empty()) {
        std::cout << "\nNo active recommendations to review. Run allocation first.\n";
        return;
    }

    size_t idx = 0;
    while (idx < recs.size()) {
        const auto rec = recs[idx];
        const auto& reqs = engine.getStore().getRequests();
        auto reqIt = std::find_if(reqs.begin(), reqs.end(), [rec](const StoredRequest& sr) {
            return sr.req.id == rec.requestId;
        });
        if (reqIt == reqs.end()) {
            idx++;
            continue;
        }
        const auto& req = reqIt->req;

        const auto& rooms = engine.getStore().getRooms();
        auto roomIt = std::find_if(rooms.begin(), rooms.end(), [rec](const Room& r) {
            return r.id == rec.roomId;
        });

        std::string roomName = (roomIt != rooms.end()) ? roomIt->name : "Unknown Room";
        int roomCap = (roomIt != rooms.end()) ? roomIt->capacity : 0;

        printHeader("Review Recommendation (" + std::to_string(idx + 1) + " of " + std::to_string(recs.size()) + ")");
        std::cout << "Recommendation ID: #" << rec.id << " [Generated by: " << rec.algorithmName << "]\n";
        std::cout << "STATUS: *** SUGGESTION - not confirmed ***\n";
        std::cout << "Request #" << req.id << ": \"" << req.title << "\" (Club: " << req.clubId << ", Tier: " << req.tier << ")\n";
        std::cout << "  Requested Window: " << formatDate(req.date) << " ["
                  << formatTime(req.start) << " - " << formatTime(req.end) << ")\n";
        std::cout << "  Attendance: " << req.attendance << " | Facilities Needed: " << req.facilities << "\n";
        std::cout << "  Proposed Room: Room " << rec.roomId << " (" << roomName << ", Listed Capacity: " << roomCap << ")\n";

        // Informational capacity notice
        if (req.attendance > roomCap) {
            std::cout << "  [INFORMATIONAL WARNING] Attendance (" << req.attendance
                      << ") exceeds listed room capacity (" << roomCap << "). Does not block feasibility.\n";
        }

        // Show other feasible rooms
        auto otherRooms = engine.getFeasibleRoomsForRequest(req.id);
        std::cout << "  Other Feasible Rooms Available:\n";
        bool hasAlternative = false;
        for (const auto& orm : otherRooms) {
            if (orm.id != rec.roomId) {
                std::cout << "    - Room " << orm.id << " (" << orm.name << ", Cap: " << orm.capacity << ")\n";
                hasAlternative = true;
            }
        }
        if (!hasAlternative) {
            std::cout << "    (No other feasible rooms currently conflict-free)\n";
        }

        std::cout << "\nActions:\n";
        std::cout << "  1) Approve recommendation as suggested\n";
        std::cout << "  2) Modify room and approve\n";
        std::cout << "  3) Reject request with reason\n";
        std::cout << "  4) Skip to next recommendation\n";
        std::cout << "  0) Exit Review\n";

        int action = promptInt("Select action [0-4]: ", 4);
        if (action == 0) break;
        if (action == 4) {
            idx++;
            continue;
        }

        if (action == 1) {
            std::string confirm = promptLine("Confirm approval for Room " + roomName + "? (y/n): ");
            if (confirm == "y" || confirm == "Y") {
                auto res = engine.approve(Role::Admin, adminName, rec.id);
                std::cout << "\n>>> " << res.message << "\n";
                // List changes dynamically
                continue;
            } else {
                std::cout << "Approval cancelled.\n";
            }
        } else if (action == 2) {
            int newRoomId = promptInt("Enter Target Room ID: ", 0);
            std::string confirm = promptLine("Confirm modification to Room " + std::to_string(newRoomId) + "? (y/n): ");
            if (confirm == "y" || confirm == "Y") {
                auto res = engine.modifyAndApprove(Role::Admin, adminName, rec.id, newRoomId);
                std::cout << "\n>>> " << res.message << "\n";
                continue;
            } else {
                std::cout << "Modification cancelled.\n";
            }
        } else if (action == 3) {
            std::string reason = promptLine("Enter required rejection reason: ");
            if (reason.empty()) {
                std::cout << "Error: Rejection reason cannot be empty.\n";
                continue;
            }
            auto res = engine.reject(Role::Admin, adminName, req.id, reason);
            std::cout << "\n>>> " << res.message << "\n";
            continue;
        }
    }
}

void adminDemoGreedyVsOptimal() {
    printHeader("Demo 10: In-Memory Demonstration (Greedy vs Optimal)");
    std::cout << "Loading the 3 same-tier requests from Phase 1 Test 9 into an in-memory test instance...\n";
    std::cout << "(Note: Physical data files in cpp-engine/data/ are untouched).\n\n";

    // 1 room
    std::vector<Room> rooms = {
        {1, "Main Auditorium", 100, 0, true}
    };
    std::vector<Booking> fixedBookings = {};

    // 3 same-tier requests (Tier 2, weight 10 each):
    // X: 09:00 - 17:00 (540 - 1020), submitted first (timestamp 10)
    // Y: 09:00 - 12:00 (540 - 720), submitted second (timestamp 20)
    // Z: 12:00 - 17:00 (720 - 1020), submitted third (timestamp 30)
    std::vector<Request> requests = {
        {1, "ClubX", "Request X (Full Day)", 0, 540, 1020, 50, 0, 2, 10},
        {2, "ClubY", "Request Y (Morning)", 0, 540, 720, 50, 0, 2, 20},
        {3, "ClubZ", "Request Z (Afternoon)", 0, 720, 1020, 50, 0, 2, 30}
    };
    Config config;

    auto resA = priority_greedy::run(requests, rooms, fixedBookings, config);
    auto resB = earliest_finish::run(requests, rooms, fixedBookings, config);
    auto resC = exhaustive::run(requests, rooms, fixedBookings, config);

    auto violsA = validate(requests, rooms, fixedBookings, resA.assignments, config);
    auto violsB = validate(requests, rooms, fixedBookings, resB.assignments, config);
    auto violsC = validate(requests, rooms, fixedBookings, resC.assignments, config);

    std::vector<ComparisonRow> rows = {
        {resA.algorithmName, resA.allocatedCount, resA.totalWeight, resA.runtimeMs, static_cast<int>(violsA.size())},
        {resB.algorithmName, resB.allocatedCount, resB.totalWeight, resB.runtimeMs, static_cast<int>(violsB.size())},
        {resC.algorithmName, resC.allocatedCount, resC.totalWeight, resC.runtimeMs, static_cast<int>(violsC.size())}
    };

    printComparisonTable(rows);

    std::cout << "\nDetailed Explanation:\n";
    std::cout << "Algorithm A (Priority Greedy) sorts by submission timestamp and assigns Request X (09:00-17:00) first,\n"
              << "blocking Room 1 for the whole day (Allocated: 1, Weight: 10). In contrast, Algorithm C (Exhaustive)\n"
              << "explores the complete search space and discovers that scheduling both Request Y (09:00-12:00) and\n"
              << "Request Z (12:00-17:00) together yields twice the weight (20 vs 10) and doubles utilization (2 vs 1).\n";
}

void adminMenu(WorkflowEngine& engine) {
    printHeader("Admin Portal - Security Authentication");
    std::cout << "[Note: Default demo password is read from cpp-engine/data/admin.txt]\n";
    std::string pwd = promptLine("Enter Admin Password: ");

    if (!engine.getStore().verifyAdminPassword(pwd)) {
        std::cout << "Authentication failed. Access denied.\n";
        return;
    }

    std::string adminName = promptLine("Enter your Administrator Name [default Admin]: ");
    if (adminName.empty()) adminName = "Admin";

    while (true) {
        printHeader("Admin Control Center - Operator: " + adminName);
        std::cout << "1) Add room\n";
        std::cout << "2) Deactivate room\n";
        std::cout << "3) List rooms\n";
        std::cout << "4) List all requests\n";
        std::cout << "5) Run allocation algorithm (A/B/C)\n";
        std::cout << "6) Compare algorithms on pending requests\n";
        std::cout << "7) Review recommendations (Approve / Reject / Change Room)\n";
        std::cout << "8) View confirmed bookings\n";
        std::cout << "9) View decision audit log\n";
        std::cout << "10) Demo: Greedy vs Optimal analysis\n";
        std::cout << "0) Log out\n";

        int choice = promptInt("Select option [0-10]: ", -1);
        if (choice == 0) break;

        if (choice == 1) {
            printHeader("Add New Room");
            std::string name = promptLine("Room Name: ");
            int cap = promptInt("Capacity: ", 50);
            int fac = promptInt("Facilities bitmask: ", 0);
            Room r;
            r.id = engine.getStore().nextRoomId();
            r.name = name;
            r.capacity = cap;
            r.facilities = static_cast<uint32_t>(fac);
            r.active = true;
            engine.getStore().getRooms().push_back(r);
            engine.getStore().save();
            std::cout << "Room #" << r.id << " (\"" << r.name << "\") added successfully.\n";

        } else if (choice == 2) {
            printHeader("Deactivate Room");
            int id = promptInt("Enter Room ID to deactivate: ", 0);
            bool found = false;
            for (auto& r : engine.getStore().getRooms()) {
                if (r.id == id) {
                    r.active = false;
                    engine.getStore().save();
                    std::cout << "Room #" << id << " (\"" << r.name << "\") is now INACTIVE.\n";
                    found = true;
                    break;
                }
            }
            if (!found) std::cout << "Room ID not found.\n";

        } else if (choice == 3) {
            printHeader("Campus Rooms Inventory");
            for (const auto& r : engine.getStore().getRooms()) {
                std::cout << "Room #" << r.id << ": \"" << r.name << "\"\n"
                          << "  Capacity: " << r.capacity
                          << " | Facilities bitmask: " << r.facilities
                          << " | Status: " << (r.active ? "ACTIVE" : "INACTIVE") << "\n";
            }

        } else if (choice == 4) {
            printHeader("All Requests Registry");
            const auto& reqs = engine.getStore().getRequests();
            if (reqs.empty()) {
                std::cout << "No requests registered.\n";
                continue;
            }
            for (const auto& sr : reqs) {
                std::cout << "Req #" << sr.req.id << " [Club: " << sr.req.clubId << "] \"" << sr.req.title << "\"\n"
                          << "  Time: " << formatDate(sr.req.date) << " " << formatTime(sr.req.start) << " - " << formatTime(sr.req.end)
                          << " | Tier: " << sr.req.tier
                          << " | Att: " << sr.req.attendance
                          << " | Status: " << requestStatusToString(sr.status) << "\n";
                if (!sr.unallocatedReason.empty()) {
                    std::cout << "  Note: " << sr.unallocatedReason << "\n";
                }
            }

        } else if (choice == 5) {
            printHeader("Run Allocation Algorithm");
            std::cout << "Choose algorithm:\n";
            std::cout << "  A) Priority Greedy\n";
            std::cout << "  B) Earliest Finish Time Greedy\n";
            std::cout << "  C) Exhaustive Search with Pruning\n";
            std::string alg = promptLine("Choice (A/B/C): ");
            auto res = engine.runAllocation(Role::Admin, alg);
            std::cout << "\n>>> " << res.message << "\n";

        } else if (choice == 6) {
            printHeader("Compare Allocation Algorithms");
            auto [res, rows] = engine.compareAlgorithms(Role::Admin);
            if (!res.success) {
                std::cout << "Error: " << res.message << "\n";
            } else {
                printComparisonTable(rows);
            }

        } else if (choice == 7) {
            adminReviewRecommendations(engine, adminName);

        } else if (choice == 8) {
            printHeader("Confirmed Bookings");
            const auto& bookings = engine.getStore().getBookings();
            if (bookings.empty()) {
                std::cout << "No bookings confirmed yet.\n";
                continue;
            }
            for (const auto& b : bookings) {
                std::string roomName = "Unknown Room";
                for (const auto& r : engine.getStore().getRooms()) {
                    if (r.id == b.roomId) {
                        roomName = r.name;
                        break;
                    }
                }
                std::cout << "Booking #" << b.id << " --> Room " << b.roomId << " (" << roomName << ") "
                          << "[" << formatDateTime(b.start / 1440, b.start % 1440) << " - "
                          << formatDateTime(b.end / 1440, b.end % 1440) << ")";
                if (b.requestId > 0) {
                    std::cout << " (Request #" << b.requestId << ")";
                } else {
                    std::cout << " (Immutable fixed seed booking)";
                }
                std::cout << "\n";
            }

        } else if (choice == 9) {
            printHeader("Administrative Decision Audit Log");
            const auto& decisions = engine.getStore().getDecisions();
            if (decisions.empty()) {
                std::cout << "No decisions recorded.\n";
                continue;
            }
            for (const auto& d : decisions) {
                std::cout << "[" << d.when << "] Operator: " << d.who << " | Action: " << d.action
                          << " | Req #" << d.requestId << " | Room: " << d.roomId << "\n"
                          << "  Reason: " << d.reason << "\n"
                          << "  Details: " << d.details << "\n\n";
            }

        } else if (choice == 10) {
            adminDemoGreedyVsOptimal();
        }
    }
}

} // anonymous namespace

int main() {
    std::cout << "===============================================================\n";
    std::cout << "  Campus Resource Allocation Optimizer - Console App (Phase 2)\n";
    std::cout << "===============================================================\n";

    DataStore store("cpp-engine/data");
    store.initializeSeedIfMissing();
    store.load();

    WorkflowEngine engine(store);

    while (true) {
        printHeader("Welcome to Campus Resource Allocation System");
        std::cout << "Select your user portal:\n";
        std::cout << "  1) Club Portal (Submit requests & view status)\n";
        std::cout << "  2) Admin Portal (Review recommendations, allocate, manage)\n";
        std::cout << "  0) Exit Application\n";

        int choice = promptInt("Select portal [0-2]: ", -1);
        if (choice == 0) {
            std::cout << "\nExiting application. Goodbye!\n";
            break;
        } else if (choice == 1) {
            clubMenu(engine);
        } else if (choice == 2) {
            adminMenu(engine);
        } else {
            std::cout << "Invalid selection. Please choose 1, 2, or 0.\n";
        }
    }

    return 0;
}
