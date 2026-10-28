#include "types.hpp"
#include "algorithms.hpp"
#include "validator.hpp"
#include "interval.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <random>
#include <cassert>
#include <algorithm>

namespace fs = std::filesystem;

namespace solver_cli {
    int runSolver(int argc, char* argv[], std::ostream& out);
}

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

const std::string TEST_DIR = "cpp-engine/build/pref_test_data";

void setupTestDir() {
    if (!fs::exists(TEST_DIR)) {
        fs::create_directories(TEST_DIR);
    }
}

void writeTestFile(const std::string& filename, const std::string& content) {
    setupTestDir();
    std::string path = TEST_DIR + "/" + filename;
    std::ofstream ofs(path);
    ofs << content;
}

std::string runSolverCapture(const std::vector<std::string>& args, int& outCode) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("solver.exe"));
    for (const auto& a : args) {
        argv.push_back(const_cast<char*>(a.c_str()));
    }
    std::ostringstream oss;
    outCode = solver_cli::runSolver(static_cast<int>(argv.size()), argv.data(), oss);
    return oss.str();
}

// ----------------------------------------------------------------------
// 1. No preference: results identical to before
// ----------------------------------------------------------------------
bool test1_noPreferenceIdentical() {
    std::vector<Room> rooms = {
        {101, "Room 101", 100, 0, true},
        {102, "Room 102", 50, 0, true}
    };
    std::vector<Booking> bookings = {
        {1, 101, 600, 720}
    };
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 20, 0, 2, 10, 0, PreferenceMode::Flexible},
        {2, "C2", "Event 2", 0, 720, 840, 20, 0, 2, 20, 0, PreferenceMode::Flexible}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    // Baseline: Req 1 in 102, Req 2 in 101.
    // satisfiedPreferences must be 0 for all.
    bool okA = (resA.allocatedCount == 2 && resA.satisfiedPreferences == 0 && resA.assignments[0].roomId == 102);
    bool okB = (resB.allocatedCount == 2 && resB.satisfiedPreferences == 0);
    bool okC = (resC.allocatedCount == 2 && resC.satisfiedPreferences == 0);

    return okA && okB && okC;
}

// ----------------------------------------------------------------------
// 2. Flexible, preferred room free: chosen and preferenceSatisfied is true
// ----------------------------------------------------------------------
bool test2_flexiblePreferredFree() {
    std::vector<Room> rooms = {
        {101, "Room 101", 100, 0, true},
        {102, "Room 102", 50, 0, true}
    };
    std::vector<Booking> bookings = {};
    // Req 1 prefers 102 (by default ascending order would choose 101)
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 20, 0, 2, 10, 102, PreferenceMode::Flexible}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    bool okA = (resA.assignments.size() == 1 && resA.assignments[0].roomId == 102 && resA.satisfiedPreferences == 1);
    bool okB = (resB.assignments.size() == 1 && resB.assignments[0].roomId == 102 && resB.satisfiedPreferences == 1);
    bool okC = (resC.assignments.size() == 1 && resC.assignments[0].roomId == 102 && resC.satisfiedPreferences == 1);

    return okA && okB && okC;
}

// ----------------------------------------------------------------------
// 3. Flexible, preferred room busy: falls back to another eligible room
// ----------------------------------------------------------------------
bool test3_flexiblePreferredBusyFallback() {
    std::vector<Room> rooms = {
        {101, "Room 101", 100, 0, true},
        {102, "Room 102", 50, 0, true}
    };
    // Room 102 is busy with fixed booking
    std::vector<Booking> bookings = {
        {1, 102, 600, 720}
    };
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 20, 0, 2, 10, 102, PreferenceMode::Flexible}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    // Falls back to Room 101, satisfiedPreferences is 0
    bool okA = (resA.assignments.size() == 1 && resA.assignments[0].roomId == 101 && resA.satisfiedPreferences == 0);
    bool okB = (resB.assignments.size() == 1 && resB.assignments[0].roomId == 101 && resB.satisfiedPreferences == 0);
    bool okC = (resC.assignments.size() == 1 && resC.assignments[0].roomId == 101 && resC.satisfiedPreferences == 0);

    return okA && okB && okC;
}

// ----------------------------------------------------------------------
// 4. Strict, preferred room busy: unallocated, TimeConflict
// ----------------------------------------------------------------------
bool test4_strictPreferredBusyUnallocated() {
    std::vector<Room> rooms = {
        {101, "Room 101", 100, 0, true},
        {102, "Room 102", 50, 0, true}
    };
    std::vector<Booking> bookings = {
        {1, 102, 600, 720}
    };
    // Req strictly wants 102
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 20, 0, 2, 10, 102, PreferenceMode::Strict}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    bool okA = (resA.assignments.empty() && resA.unallocated.size() == 1 && resA.unallocated[0].reason == UnallocatedReason::TimeConflict);
    bool okB = (resB.assignments.empty() && resB.unallocated.size() == 1 && resB.unallocated[0].reason == UnallocatedReason::TimeConflict);
    bool okC = (resC.assignments.empty() && resC.unallocated.size() == 1 && resC.unallocated[0].reason == UnallocatedReason::TimeConflict);

    return okA && okB && okC;
}

// ----------------------------------------------------------------------
// 5. Strict, preferred room lacks required facility: unallocated, MissingFacility
// ----------------------------------------------------------------------
bool test5_strictLacksFacilityUnallocated() {
    std::vector<Room> rooms = {
        {101, "Projector Room", 100, 1, true}, // bit 0 = 1
        {102, "Audio Room", 50, 2, true}       // bit 1 = 2
    };
    std::vector<Booking> bookings = {};
    // Req requires facility 2 (Audio), but strictly requests Room 101 (Projector)
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 20, 2, 2, 10, 101, PreferenceMode::Strict}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    bool okA = (resA.assignments.empty() && resA.unallocated.size() == 1 && resA.unallocated[0].reason == UnallocatedReason::MissingFacility);
    bool okB = (resB.assignments.empty() && resB.unallocated.size() == 1 && resB.unallocated[0].reason == UnallocatedReason::MissingFacility);
    bool okC = (resC.assignments.empty() && resC.unallocated.size() == 1 && resC.unallocated[0].reason == UnallocatedReason::MissingFacility);

    return okA && okB && okC;
}

// ----------------------------------------------------------------------
// 6. Strict, preferred room inactive: unallocated, NoActiveRoom
// ----------------------------------------------------------------------
bool test6_strictInactiveRoomUnallocated() {
    std::vector<Room> rooms = {
        {101, "Inactive Room", 100, 0, false},
        {102, "Active Room", 50, 0, true}
    };
    std::vector<Booking> bookings = {};
    std::vector<Request> requests = {
        {1, "C1", "Event 1", 0, 600, 720, 20, 0, 2, 10, 101, PreferenceMode::Strict}
    };
    Config cfg;

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    bool okA = (resA.assignments.empty() && resA.unallocated.size() == 1 && resA.unallocated[0].reason == UnallocatedReason::NoActiveRoom);
    bool okB = (resB.assignments.empty() && resB.unallocated.size() == 1 && resB.unallocated[0].reason == UnallocatedReason::NoActiveRoom);
    bool okC = (resC.assignments.empty() && resC.unallocated.size() == 1 && resC.unallocated[0].reason == UnallocatedReason::NoActiveRoom);

    return okA && okB && okC;
}

// ----------------------------------------------------------------------
// 7. Two clubs strictly want same room at same time: higher tier wins; equal tier earlier submission wins
// ----------------------------------------------------------------------
bool test7_twoClubsStrictContested() {
    std::vector<Room> rooms = {
        {101, "Main Hall", 100, 0, true}
    };
    std::vector<Booking> bookings = {};
    Config cfg;

    // 7a: Higher tier wins
    std::vector<Request> reqsA = {
        {1, "ClubHigh", "High Event", 0, 600, 720, 30, 0, 3, 200, 101, PreferenceMode::Strict}, // Tier 3
        {2, "ClubLow", "Low Event",   0, 600, 720, 30, 0, 1, 100, 101, PreferenceMode::Strict}  // Tier 1
    };
    auto res7a = priority_greedy::run(reqsA, rooms, bookings, cfg);
    if (res7a.assignments.size() != 1 || res7a.assignments[0].requestId != 1) return false;
    if (res7a.unallocated.size() != 1 || res7a.unallocated[0].requestId != 2) return false;

    // 7b: Equal tier, earlier submission wins
    std::vector<Request> reqsB = {
        {1, "ClubEarlier", "Early Event", 0, 600, 720, 30, 0, 2, 100, 101, PreferenceMode::Strict}, // submittedAt 100
        {2, "ClubLater",   "Late Event",  0, 600, 720, 30, 0, 2, 200, 101, PreferenceMode::Strict}  // submittedAt 200
    };
    auto res7b = priority_greedy::run(reqsB, rooms, bookings, cfg);
    if (res7b.assignments.size() != 1 || res7b.assignments[0].requestId != 1) return false;
    if (res7b.unallocated.size() != 1 || res7b.unallocated[0].requestId != 2) return false;

    return true;
}

// ----------------------------------------------------------------------
// 8. Same two clubs, but one is flexible: both allocated
// ----------------------------------------------------------------------
bool test8_twoClubsOneFlexibleBothAllocated() {
    std::vector<Room> rooms = {
        {101, "Hall 1", 100, 0, true},
        {102, "Hall 2", 50, 0, true}
    };
    std::vector<Booking> bookings = {};
    Config cfg;

    // Club 1 is strict for 101, Club 2 is flexible for 101
    std::vector<Request> requests = {
        {1, "ClubStrict",   "Strict Event",   0, 600, 720, 30, 0, 2, 100, 101, PreferenceMode::Strict},
        {2, "ClubFlexible", "Flexible Event", 0, 600, 720, 30, 0, 2, 200, 101, PreferenceMode::Flexible}
    };

    auto resA = priority_greedy::run(requests, rooms, bookings, cfg);
    auto resB = earliest_finish::run(requests, rooms, bookings, cfg);
    auto resC = exhaustive::run(requests, rooms, bookings, cfg);

    // Both must be allocated! Req 1 in 101, Req 2 in 102.
    bool okA = (resA.allocatedCount == 2 && resA.satisfiedPreferences == 1);
    bool okB = (resB.allocatedCount == 2 && resB.satisfiedPreferences == 1);
    bool okC = (resC.allocatedCount == 2 && resC.satisfiedPreferences == 1);

    auto violsA = validate(requests, rooms, bookings, resA.assignments, cfg);
    auto violsB = validate(requests, rooms, bookings, resB.assignments, cfg);
    auto violsC = validate(requests, rooms, bookings, resC.assignments, cfg);

    return okA && okB && okC && violsA.empty() && violsB.empty() && violsC.empty();
}

// ----------------------------------------------------------------------
// 9. Satisfied-preferences tie-break: equal weight & count, more preferences wins
// ----------------------------------------------------------------------
bool test9_satisfiedPreferencesTieBreak() {
    std::vector<Room> rooms = {
        {101, "Main Hall", 100, 0, true}
    };
    std::vector<Booking> bookings = {};
    Config cfg;

    // Two mutually exclusive requests of equal weight and count:
    // Req 1: Flexible for 101 (satisfies preference if placed)
    // Req 2: No preference (0)
    std::vector<Request> requests = {
        {1, "ClubPref", "Pref Event", 0, 600, 720, 30, 0, 2, 100, 101, PreferenceMode::Flexible},
        {2, "ClubNoPref", "NoPref Event", 0, 600, 720, 30, 0, 2, 100, 0, PreferenceMode::Flexible}
    };

    auto resC = exhaustive::run(requests, rooms, bookings, cfg);
    if (resC.allocatedCount != 1) return false;
    if (resC.assignments[0].requestId != 1) return false;
    if (resC.satisfiedPreferences != 1) return false;

    // Test with reverse order in requests vector: Req 2 first, Req 1 second
    std::vector<Request> reversedRequests = {
        {2, "ClubNoPref", "NoPref Event", 0, 600, 720, 30, 0, 2, 100, 0, PreferenceMode::Flexible},
        {1, "ClubPref", "Pref Event", 0, 600, 720, 30, 0, 2, 100, 101, PreferenceMode::Flexible}
    };
    auto resC2 = exhaustive::run(reversedRequests, rooms, bookings, cfg);
    if (resC2.allocatedCount != 1) return false;
    if (resC2.assignments[0].requestId != 1) return false;
    if (resC2.satisfiedPreferences != 1) return false;

    return true;
}

// ----------------------------------------------------------------------
// 10. Exhaustive search equals naive no-pruning brute force on >= 30 seeded random cases
// ----------------------------------------------------------------------
struct NaiveBest {
    int weight = -1;
    int count = -1;
    int satisfied = -1;
    std::vector<Assignment> assignments;
};

void naiveSearch(
    size_t reqIdx,
    const std::vector<Request>& validRequests,
    const std::vector<Room>& sortedRooms,
    const std::vector<Booking>& fixedBookings,
    const Config& config,
    std::vector<Assignment>& currentAssignments,
    NaiveBest& best
) {
    if (reqIdx == validRequests.size()) {
        int weight = 0;
        int count = static_cast<int>(currentAssignments.size());
        int satisfied = 0;
        for (const auto& asgn : currentAssignments) {
            for (const auto& r : validRequests) {
                if (r.id == asgn.requestId) {
                    weight += tierToWeight(r.tier);
                    if (r.preferredRoomId != 0 && asgn.roomId == r.preferredRoomId) {
                        satisfied++;
                    }
                    break;
                }
            }
        }
        bool isBetter = false;
        if (weight > best.weight) isBetter = true;
        else if (weight == best.weight) {
            if (count > best.count) isBetter = true;
            else if (count == best.count) {
                if (satisfied > best.satisfied) isBetter = true;
            }
        }
        if (isBetter) {
            best.weight = weight;
            best.count = count;
            best.satisfied = satisfied;
            best.assignments = currentAssignments;
        }
        return;
    }

    const auto& req = validRequests[reqIdx];
    Interval reqInt{req.getAbsoluteStart(), req.getAbsoluteEnd()};

    for (const auto& room : sortedRooms) {
        if (req.preferredRoomId != 0 && req.roomMode == PreferenceMode::Strict) {
            if (room.id != req.preferredRoomId) continue;
        }
        if (!room.active) continue;
        if (config.facilityCheckEnabled && ((req.facilities & ~room.facilities) != 0)) continue;

        // Check overlap with fixed bookings
        bool conflict = false;
        for (const auto& fb : fixedBookings) {
            if (fb.roomId == room.id && overlaps(reqInt, Interval{fb.start, fb.end})) {
                conflict = true;
                break;
            }
        }
        if (conflict) continue;

        // Check overlap with current assignments
        for (const auto& asgn : currentAssignments) {
            if (asgn.roomId == room.id) {
                for (const auto& otherReq : validRequests) {
                    if (otherReq.id == asgn.requestId) {
                        if (overlaps(reqInt, Interval{otherReq.getAbsoluteStart(), otherReq.getAbsoluteEnd()})) {
                            conflict = true;
                        }
                        break;
                    }
                }
            }
            if (conflict) break;
        }
        if (conflict) continue;

        currentAssignments.push_back({req.id, room.id});
        naiveSearch(reqIdx + 1, validRequests, sortedRooms, fixedBookings, config, currentAssignments, best);
        currentAssignments.pop_back();
    }

    // Unallocated branch
    naiveSearch(reqIdx + 1, validRequests, sortedRooms, fixedBookings, config, currentAssignments, best);
}

bool test10_exhaustiveEqualsNaiveBruteForce() {
    std::mt19937 rng(42); // Deterministic seed

    Config config;
    config.facilityCheckEnabled = true;

    for (int iteration = 0; iteration < 35; ++iteration) {
        // Generate 2..3 rooms
        int numRooms = 2 + (rng() % 2);
        std::vector<Room> rooms;
        for (int r = 0; r < numRooms; ++r) {
            Room rm;
            rm.id = 101 + r;
            rm.name = "Room " + std::to_string(rm.id);
            rm.capacity = 50 + static_cast<int>(rng() % 100);
            rm.facilities = static_cast<uint32_t>(rng() % 4); // bit 0 or 1
            rm.active = (rng() % 10 != 0); // 90% active
            rooms.push_back(rm);
        }

        // Generate 0..2 fixed bookings
        int numBookings = static_cast<int>(rng() % 2);
        std::vector<Booking> bookings;
        for (int b = 0; b < numBookings; ++b) {
            Booking bk;
            bk.id = b + 1;
            bk.roomId = 101 + static_cast<int>(rng() % numRooms);
            int startHour = 9 + static_cast<int>(rng() % 4);
            bk.start = startHour * 60;
            bk.end = bk.start + 60;
            bookings.push_back(bk);
        }

        // Generate 4..6 requests
        int numReqs = 4 + static_cast<int>(rng() % 3);
        std::vector<Request> requests;
        for (int i = 0; i < numReqs; ++i) {
            Request req;
            req.id = i + 1;
            req.clubId = "Club" + std::to_string(i + 1);
            req.title = "Event " + std::to_string(i + 1);
            req.date = 0;
            int startHour = 9 + static_cast<int>(rng() % 4);
            int duration = 1 + static_cast<int>(rng() % 2);
            req.start = startHour * 60;
            req.end = req.start + duration * 60;
            req.attendance = 20;
            req.facilities = static_cast<uint32_t>(rng() % 3);
            req.tier = 1 + static_cast<int>(rng() % 4);
            req.submittedAt = 100 + static_cast<int64_t>(rng() % 500);

            // Preference: 0 (none), or room id
            int prefChoice = static_cast<int>(rng() % (numRooms + 1));
            if (prefChoice == 0) {
                req.preferredRoomId = 0;
                req.roomMode = PreferenceMode::Flexible;
            } else {
                req.preferredRoomId = 100 + prefChoice;
                req.roomMode = (rng() % 2 == 0) ? PreferenceMode::Flexible : PreferenceMode::Strict;
            }
            requests.push_back(req);
        }

        // Run Exhaustive
        auto resC = exhaustive::run(requests, rooms, bookings, config);

        // Run Naive Brute Force
        // First sort requests in Algorithm A order to match exhaustive search processing order
        std::vector<Request> sortedRequests = requests;
        std::sort(sortedRequests.begin(), sortedRequests.end(), [](const Request& a, const Request& b) {
            if (a.tier != b.tier) return a.tier > b.tier;
            if (a.date != b.date) return a.date < b.date;
            if (a.submittedAt != b.submittedAt) return a.submittedAt < b.submittedAt;
            return a.id < b.id;
        });

        std::vector<Room> sortedRooms = rooms;
        std::sort(sortedRooms.begin(), sortedRooms.end(), [](const Room& a, const Room& b) {
            return a.id < b.id;
        });

        NaiveBest naiveBest;
        std::vector<Assignment> cur;
        naiveSearch(0, sortedRequests, sortedRooms, bookings, config, cur, naiveBest);
        if (naiveBest.weight < 0) {
            naiveBest.weight = 0;
            naiveBest.count = 0;
            naiveBest.satisfied = 0;
        }

        if (resC.totalWeight != naiveBest.weight) return false;
        if (resC.allocatedCount != naiveBest.count) return false;
        if (resC.satisfiedPreferences != naiveBest.satisfied) return false;
        if (resC.assignments.size() != naiveBest.assignments.size()) return false;
    }

    return true;
}

// ----------------------------------------------------------------------
// 11. Validator reports violation for Strict in other room, none for Flexible; capacity never a violation
// ----------------------------------------------------------------------
bool test11_validatorReportsStrictViolationCapacityNot() {
    std::vector<Room> rooms = {
        {101, "Small Room", 20, 0, true},
        {102, "Large Room", 100, 0, true}
    };
    std::vector<Booking> bookings = {};
    Config cfg;

    // Request with Strict preference for 101, attendance 50 (> 20)
    std::vector<Request> requests = {
        {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10, 101, PreferenceMode::Strict},
        {2, "Club2", "Event 2", 0, 720, 840, 50, 0, 1, 20, 101, PreferenceMode::Flexible}
    };

    // Case 1: Strict request 1 placed in Room 102 (violation!)
    std::vector<Assignment> badAssign = {{1, 102}};
    auto viols1 = validate(requests, rooms, bookings, badAssign, cfg);
    if (viols1.empty()) return false;
    bool foundStrictMsg = false;
    for (const auto& v : viols1) {
        if (v.find("strict room preference violated") != std::string::npos) {
            foundStrictMsg = true;
            break;
        }
    }
    if (!foundStrictMsg) return false;

    // Case 2: Strict request 1 placed in Room 101 (attendance 50 > capacity 20, but capacity is informational)
    std::vector<Assignment> goodAssign = {{1, 101}};
    auto viols2 = validate(requests, rooms, bookings, goodAssign, cfg);
    if (!viols2.empty()) return false; // Capacity is NEVER a violation

    // Case 3: Flexible request 2 placed in Room 102 (different from preferred 101)
    std::vector<Assignment> flexAssign = {{2, 102}};
    auto viols3 = validate(requests, rooms, bookings, flexAssign, cfg);
    if (!viols3.empty()) return false; // Flexible is NEVER a violation

    return true;
}

// ----------------------------------------------------------------------
// 12. CLI: PREF changes result; unknown request id returns ok:false; no PREF produces same output
// ----------------------------------------------------------------------
bool test12_cliPrefRecordBehaviour() {
    // 12a: PREF changes result
    std::string fileWithPref = "test12_with_pref.txt";
    writeTestFile(fileWithPref,
        "ROOM|101|Hall 1|100|0|1\n"
        "ROOM|102|Hall 2|100|0|1\n"
        "REQ|1|ClubA|0|600|720|30|0|2|100\n"
        "PREF|1|102|0\n" // Flexible for Room 102
    );

    int codeA = 0;
    std::string jsonA = runSolverCapture({"allocate", "A", TEST_DIR + "/" + fileWithPref}, codeA);
    if (codeA != 0) return false;
    if (jsonA.find("\"roomId\":102") == std::string::npos) return false;
    if (jsonA.find("\"preferenceSatisfied\":true") == std::string::npos) return false;
    if (jsonA.find("\"satisfiedPreferences\":1") == std::string::npos) return false;

    // 12b: Unknown request id in PREF returns ok:false and non-zero exit code
    std::string fileBadPref = "test12_bad_pref.txt";
    writeTestFile(fileBadPref,
        "ROOM|101|Hall 1|100|0|1\n"
        "REQ|1|ClubA|0|600|720|30|0|2|100\n"
        "PREF|999|101|0\n" // Unknown request id 999
    );

    int codeBad = 0;
    std::string jsonBad = runSolverCapture({"allocate", "A", TEST_DIR + "/" + fileBadPref}, codeBad);
    if (codeBad == 0) return false; // Must return non-zero exit code
    if (jsonBad.find("\"ok\":false") == std::string::npos) return false;
    if (jsonBad.find("Unknown request id in PREF: 999") == std::string::npos) return false;

    // 12c: File without PREF produces null for preferenceSatisfied and 0 for satisfiedPreferences
    std::string fileNoPref = "test12_no_pref.txt";
    writeTestFile(fileNoPref,
        "ROOM|101|Hall 1|100|0|1\n"
        "REQ|1|ClubA|0|600|720|30|0|2|100\n"
    );

    int codeNoPref = 0;
    std::string jsonNoPref = runSolverCapture({"allocate", "A", TEST_DIR + "/" + fileNoPref}, codeNoPref);
    if (codeNoPref != 0) return false;
    if (jsonNoPref.find("\"roomId\":101") == std::string::npos) return false;
    if (jsonNoPref.find("\"preferenceSatisfied\":null") == std::string::npos) return false;
    if (jsonNoPref.find("\"satisfiedPreferences\":0") == std::string::npos) return false;

    return true;
}

} // anonymous namespace

int main() {
    std::cout << "=======================================================\n";
    std::cout << "     Phase 3d Room Preference Engine Test Suite        \n";
    std::cout << "=======================================================\n";

    runTest("1. No preference: results identical to before", test1_noPreferenceIdentical());
    runTest("2. Flexible, preferred room free: chosen and preferenceSatisfied is true", test2_flexiblePreferredFree());
    runTest("3. Flexible, preferred room busy: falls back to another eligible room", test3_flexiblePreferredBusyFallback());
    runTest("4. Strict, preferred room busy: unallocated, TimeConflict", test4_strictPreferredBusyUnallocated());
    runTest("5. Strict, preferred room lacks required facility: unallocated, MissingFacility", test5_strictLacksFacilityUnallocated());
    runTest("6. Strict, preferred room inactive: unallocated, NoActiveRoom", test6_strictInactiveRoomUnallocated());
    runTest("7. Two clubs strictly want same room at same time: higher tier wins; earlier submission wins", test7_twoClubsStrictContested());
    runTest("8. Same two clubs, one flexible: both allocated", test8_twoClubsOneFlexibleBothAllocated());
    runTest("9. Satisfied-preferences tie-break: more preferences wins", test9_satisfiedPreferencesTieBreak());
    runTest("10. Exhaustive search equals naive brute force on >= 30 seeded random cases", test10_exhaustiveEqualsNaiveBruteForce());
    runTest("11. Validator reports violation for Strict in other room, none for Flexible; capacity never a violation", test11_validatorReportsStrictViolationCapacityNot());
    runTest("12. CLI: PREF changes result; unknown request id returns ok:false; no PREF produces same output", test12_cliPrefRecordBehaviour());

    std::cout << "=======================================================\n";
    std::cout << "Test Summary: " << g_passCount << " PASSED, " << g_failCount << " FAILED\n";
    std::cout << "=======================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
