#include "workflow.hpp"
#include "validator.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

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

void cleanDir(const std::string& path) {
    if (fs::exists(path)) {
        fs::remove_all(path);
    }
    fs::create_directories(path);
}

// 1. runAllocation creates Recommendations and creates ZERO bookings.
bool test1_runAllocationCreatesRecsZeroBookings() {
    std::string testDir = "cpp-engine/build/test_data/test1";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {
        {101, "Hall 1", 100, 0, true}
    };
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Pending, "" },
        { {2, "Club2", "Event 2", 0, 720, 840, 50, 0, 1, 20}, RequestStatus::Pending, "" }
    };
    store.save();

    WorkflowEngine engine(store);
    auto res = engine.runAllocation(Role::Admin, "Priority Greedy");
    if (!res.success) return false;

    if (store.getRecommendations().empty()) return false;
    if (!store.getBookings().empty()) return false; // MUST be zero bookings

    for (const auto& sr : store.getRequests()) {
        if (sr.status != RequestStatus::Recommended) return false;
    }
    return true;
}

// 2. approve on a valid recommendation creates exactly one booking and a decision log entry.
bool test2_approveCreatesOneBookingAndDecision() {
    std::string testDir = "cpp-engine/build/test_data/test2";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {{101, "Hall 1", 100, 0, true}};
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Recommended, "" }
    };
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    store.save();

    WorkflowEngine engine(store);
    size_t initialBookings = store.getBookings().size();
    size_t initialDecisions = store.getDecisions().size();

    auto res = engine.approve(Role::Admin, "AdminUser", 1);
    if (!res.success) return false;

    if (store.getBookings().size() != initialBookings + 1) return false;
    if (store.getDecisions().size() != initialDecisions + 1) return false;

    const auto& b = store.getBookings().back();
    if (b.roomId != 101 || b.requestId != 1 || b.start != 600 || b.end != 720) return false;

    const auto& d = store.getDecisions().back();
    if (d.who != "AdminUser" || d.action != "Approve" || d.requestId != 1 || d.roomId != 101) return false;

    if (store.getRequests()[0].status != RequestStatus::Confirmed) return false;
    return true;
}

// 3. approve is blocked when the room now conflicts with a confirmed booking; nothing changes.
bool test3_approveBlockedOnConflict() {
    std::string testDir = "cpp-engine/build/test_data/test3";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {{101, "Hall 1", 100, 0, true}};
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Recommended, "" }
    };
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    // Prior confirmed booking in room 101 overlapping 600-720
    store.getBookings() = {
        {99, 101, 650, 750, 0}
    };
    store.save();

    WorkflowEngine engine(store);
    auto res = engine.approve(Role::Admin, "AdminUser", 1);
    // Must be blocked!
    if (res.success) return false;

    // Nothing changes
    if (store.getBookings().size() != 1) return false;
    if (store.getRequests()[0].status != RequestStatus::Recommended) return false;
    if (store.getRecommendations().size() != 1) return false;

    return true;
}

// 4. approving the same recommendation twice fails.
bool test4_approvingTwiceFails() {
    std::string testDir = "cpp-engine/build/test_data/test4";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {{101, "Hall 1", 100, 0, true}};
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Recommended, "" }
    };
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    store.save();

    WorkflowEngine engine(store);
    auto res1 = engine.approve(Role::Admin, "AdminUser", 1);
    if (!res1.success) return false;

    // Second approval attempt on same recommendation
    auto res2 = engine.approve(Role::Admin, "AdminUser", 1);
    if (res2.success) return false; // MUST fail

    return true;
}

// 5. modifyAndApprove into a free, feasible room succeeds.
bool test5_modifyAndApproveFreeRoomSucceeds() {
    std::string testDir = "cpp-engine/build/test_data/test5";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {
        {101, "Hall 1", 100, 0, true},
        {102, "Hall 2", 100, 0, true}
    };
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Recommended, "" }
    };
    // Recommended for Room 101
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    store.save();

    WorkflowEngine engine(store);
    auto res = engine.modifyAndApprove(Role::Admin, "AdminUser", 1, 102);
    if (!res.success) return false;

    if (store.getBookings().size() != 1) return false;
    if (store.getBookings()[0].roomId != 102) return false;
    if (store.getRequests()[0].status != RequestStatus::Confirmed) return false;
    if (store.getDecisions().back().action != "ModifyApprove") return false;

    return true;
}

// 6. modifyAndApprove into a room that conflicts with a confirmed booking is blocked.
bool test6_modifyAndApproveConflictingRoomBlocked() {
    std::string testDir = "cpp-engine/build/test_data/test6";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {
        {101, "Hall 1", 100, 0, true},
        {102, "Hall 2", 100, 0, true}
    };
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Recommended, "" }
    };
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    // Room 102 has conflicting booking
    store.getBookings() = {
        {99, 102, 600, 720, 0}
    };
    store.save();

    WorkflowEngine engine(store);
    auto res = engine.modifyAndApprove(Role::Admin, "AdminUser", 1, 102);
    if (res.success) return false; // Must be blocked!

    // Verify explanation contains conflicting room or booking
    if (res.message.find("Conflict with confirmed booking") == std::string::npos) return false;

    // Status remains unchanged
    if (store.getRequests()[0].status != RequestStatus::Recommended) return false;

    return true;
}

// 7. modifyAndApprove into a room whose capacity is smaller than attendance still succeeds (capacity is informational).
bool test7_modifyAndApproveCapacityInformationalOnly() {
    std::string testDir = "cpp-engine/build/test_data/test7";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {
        {101, "Hall 1", 500, 0, true},
        {102, "Tiny Seminar Room", 20, 0, true} // Capacity 20
    };
    store.getRequests() = {
        { {1, "Club1", "Huge Event", 0, 600, 720, 400, 0, 1, 10}, RequestStatus::Recommended, "" } // Attendance 400
    };
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    store.save();

    WorkflowEngine engine(store);
    auto res = engine.modifyAndApprove(Role::Admin, "AdminUser", 1, 102);
    // MUST succeed because capacity is informational ONLY
    if (!res.success) return false;

    if (store.getBookings().size() != 1) return false;
    if (store.getBookings()[0].roomId != 102) return false;
    if (store.getRequests()[0].status != RequestStatus::Confirmed) return false;

    return true;
}

// 8. reject requires a reason and records it.
bool test8_rejectRequiresReasonAndRecordsIt() {
    std::string testDir = "cpp-engine/build/test_data/test8";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {{101, "Hall 1", 100, 0, true}};
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Pending, "" }
    };
    store.save();

    WorkflowEngine engine(store);

    // Empty reason must fail
    auto failRes = engine.reject(Role::Admin, "AdminUser", 1, "   ");
    if (failRes.success) return false;

    // Valid reason must succeed
    auto okRes = engine.reject(Role::Admin, "AdminUser", 1, "Campus maintenance scheduled");
    if (!okRes.success) return false;

    if (store.getRequests()[0].status != RequestStatus::Rejected) return false;
    if (store.getRequests()[0].unallocatedReason != "Campus maintenance scheduled") return false;

    if (store.getDecisions().empty()) return false;
    const auto& d = store.getDecisions().back();
    if (d.action != "Reject" || d.reason != "Campus maintenance scheduled") return false;

    return true;
}

// 9. A Club role is refused for runAllocation, approve, reject and modifyAndApprove.
bool test9_clubRoleRefusedAdminOperations() {
    std::string testDir = "cpp-engine/build/test_data/test9";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {
        {101, "Hall 1", 100, 0, true},
        {102, "Hall 2", 100, 0, true}
    };
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Recommended, "" }
    };
    store.getRecommendations() = {
        {1, 1, 101, "Priority Greedy", 1}
    };
    store.save();

    WorkflowEngine engine(store);

    if (engine.runAllocation(Role::Club, "A").success) return false;
    if (engine.approve(Role::Club, "ClubUser", 1).success) return false;
    if (engine.reject(Role::Club, "ClubUser", 1, "Reason").success) return false;
    if (engine.modifyAndApprove(Role::Club, "ClubUser", 1, 102).success) return false;

    return true;
}

// 10. compareAlgorithms changes no request status and creates no recommendations or bookings.
bool test10_compareAlgorithmsIsSideEffectFree() {
    std::string testDir = "cpp-engine/build/test_data/test10";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {{101, "Hall 1", 100, 0, true}};
    store.getRequests() = {
        { {1, "Club1", "Event 1", 0, 600, 720, 50, 0, 1, 10}, RequestStatus::Pending, "" }
    };
    store.save();

    WorkflowEngine engine(store);
    auto [res, rows] = engine.compareAlgorithms(Role::Admin);
    if (!res.success) return false;
    if (rows.size() != 3) return false;

    // Assert side-effect free:
    if (store.getRequests()[0].status != RequestStatus::Pending) return false;
    if (!store.getRecommendations().empty()) return false;
    if (!store.getBookings().empty()) return false;

    return true;
}

// 11. Persistence round trip: save then load gives identical rooms, requests, bookings,
// recommendations and decisions (test with a title containing a comma).
bool test11_persistenceRoundTripWithCommas() {
    std::string testDir = "cpp-engine/build/test_data/test11";
    cleanDir(testDir);
    DataStore store1(testDir);

    store1.getRooms() = {
        {101, "Auditorium, Hall A", 300, 3, true},
        {102, "Lab 2, West Wing", 50, 4, false}
    };
    store1.getRequests() = {
        { {1, "ArtClub", "Art, Music & Culture Symposium", 0, 600, 800, 150, 3, 3, 100}, RequestStatus::Pending, "" },
        { {2, "Drama", "Theatre, Drama and Dance", 1, 700, 900, 80, 1, 2, 200}, RequestStatus::Recommended, "" }
    };
    store1.getBookings() = {
        {1, 101, 600, 800, 1}
    };
    store1.getRecommendations() = {
        {1, 2, 101, "Priority Greedy", 5}
    };
    store1.getDecisions() = {
        {"AdminUser", "2026-10-26 14:00:00", "Approve", 1, 101, "Fits perfectly, approved", "Recommendation #1, no issues"}
    };

    if (!store1.save()) return false;

    // Load in fresh store
    DataStore store2(testDir);
    if (!store2.load()) return false;

    // Verify rooms
    if (store2.getRooms().size() != 2) return false;
    if (store2.getRooms()[0].name != "Auditorium, Hall A") return false;
    if (store2.getRooms()[1].name != "Lab 2, West Wing") return false;

    // Verify requests with commas in titles
    if (store2.getRequests().size() != 2) return false;
    if (store2.getRequests()[0].req.title != "Art, Music & Culture Symposium") return false;
    if (store2.getRequests()[1].req.title != "Theatre, Drama and Dance") return false;
    if (store2.getRequests()[0].status != RequestStatus::Pending) return false;
    if (store2.getRequests()[1].status != RequestStatus::Recommended) return false;

    // Verify bookings
    if (store2.getBookings().size() != 1) return false;
    if (store2.getBookings()[0].id != 1 || store2.getBookings()[0].roomId != 101) return false;

    // Verify recommendations
    if (store2.getRecommendations().size() != 1) return false;
    if (store2.getRecommendations()[0].algorithmName != "Priority Greedy") return false;

    // Verify decisions with commas in reasons & details
    if (store2.getDecisions().size() != 1) return false;
    if (store2.getDecisions()[0].reason != "Fits perfectly, approved") return false;
    if (store2.getDecisions()[0].details != "Recommendation #1, no issues") return false;

    return true;
}

// 12. After a sequence of approvals, the Phase 1 validator reports 0 violations on the confirmed schedule.
bool test12_sequenceOfApprovalsProducesZeroViolations() {
    std::string testDir = "cpp-engine/build/test_data/test12";
    cleanDir(testDir);
    DataStore store(testDir);

    store.getRooms() = {
        {101, "Auditorium", 300, 3, true},
        {102, "Seminar Hall", 100, 1, true}
    };
    store.getRequests() = {
        { {1, "C1", "Morning Keynote", 0, 540, 660, 200, 3, 4, 10}, RequestStatus::Pending, "" },
        { {2, "C2", "Afternoon Talk", 0, 700, 800, 150, 3, 3, 20}, RequestStatus::Pending, "" },
        { {3, "C3", "Seminar Event", 0, 540, 660, 50, 1, 2, 30}, RequestStatus::Pending, "" }
    };
    store.save();

    WorkflowEngine engine(store);

    // 1. Run allocation
    auto allocRes = engine.runAllocation(Role::Admin, "A");
    if (!allocRes.success) return false;

    // 2. Sequentially approve all generated recommendations
    auto recs = store.getRecommendations();
    for (const auto& rec : recs) {
        auto appRes = engine.approve(Role::Admin, "AdminUser", rec.id);
        if (!appRes.success) return false;
    }

    // 3. Safety net check
    std::vector<std::string> violations;
    bool valid = engine.validateConfirmedSchedule(violations);
    if (!valid || !violations.empty()) return false;

    return true;
}

} // anonymous namespace

int main() {
    std::cout << "=======================================================\n";
    std::cout << "       Phase 2 Workflow and Persistence Tests          \n";
    std::cout << "=======================================================\n";

    runTest("1. runAllocation creates Recommendations and creates ZERO bookings", test1_runAllocationCreatesRecsZeroBookings());
    runTest("2. approve creates exactly one booking and decision log", test2_approveCreatesOneBookingAndDecision());
    runTest("3. approve is blocked when room conflicts with confirmed booking", test3_approveBlockedOnConflict());
    runTest("4. approving the same recommendation twice fails", test4_approvingTwiceFails());
    runTest("5. modifyAndApprove into free, feasible room succeeds", test5_modifyAndApproveFreeRoomSucceeds());
    runTest("6. modifyAndApprove into conflicting room is blocked", test6_modifyAndApproveConflictingRoomBlocked());
    runTest("7. modifyAndApprove capacity smaller than attendance succeeds (informational)", test7_modifyAndApproveCapacityInformationalOnly());
    runTest("8. reject requires a reason and records it", test8_rejectRequiresReasonAndRecordsIt());
    runTest("9. Club role is refused for admin operations", test9_clubRoleRefusedAdminOperations());
    runTest("10. compareAlgorithms changes no status and creates no recommendations/bookings", test10_compareAlgorithmsIsSideEffectFree());
    runTest("11. Persistence round trip with commas in titles and text fields", test11_persistenceRoundTripWithCommas());
    runTest("12. Phase 1 validator reports 0 violations after sequence of approvals", test12_sequenceOfApprovalsProducesZeroViolations());

    std::cout << "=======================================================\n";
    std::cout << "Test Summary: " << g_passCount << " PASSED, " << g_failCount << " FAILED\n";
    std::cout << "=======================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
