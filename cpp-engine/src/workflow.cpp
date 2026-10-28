#include "workflow.hpp"
#include "algorithms.hpp"
#include "validator.hpp"
#include "interval.hpp"
#include <algorithm>
#include <sstream>

WorkflowEngine::WorkflowEngine(DataStore& store)
    : store_(store) {
}

// -------------------------------------------------------------
// 1. Submit Request
// -------------------------------------------------------------
OperationResult WorkflowEngine::submitRequest(Role /*role*/, StoredRequest request) {
    // Both Club and Admin can submit requests
    if (request.req.title.empty()) {
        return {false, "Title cannot be empty."};
    }
    if (request.req.end <= request.req.start) {
        return {false, "Invalid time interval: end time must be strictly greater than start time."};
    }
    if (request.req.attendance <= 0) {
        return {false, "Attendance must be greater than zero."};
    }

    if (request.req.id <= 0) {
        request.req.id = store_.nextRequestId();
    }
    request.status = RequestStatus::Pending;
    request.unallocatedReason.clear();

    store_.getRequests().push_back(request);
    store_.save();

    return {true, "Request #" + std::to_string(request.req.id) + " submitted successfully as Pending."};
}

// -------------------------------------------------------------
// 2. Run Allocation (Admin only)
// -------------------------------------------------------------
OperationResult WorkflowEngine::runAllocation(Role role, const std::string& algorithmName) {
    if (role != Role::Admin) {
        return {false, "Access denied: Admin role required to run allocation algorithms."};
    }

    // 1. Collect pending requests
    std::vector<Request> pendingRequests;
    for (const auto& sr : store_.getRequests()) {
        if (sr.status == RequestStatus::Pending) {
            pendingRequests.push_back(sr.req);
        }
    }

    if (pendingRequests.empty()) {
        return {true, "No Pending requests found to allocate."};
    }

    // 2. Prepare confirmed bookings as immutable fixed inputs
    std::vector<Booking> fixedBookings;
    for (const auto& b : store_.getBookings()) {
        fixedBookings.push_back(Booking{b.id, b.roomId, b.start, b.end});
    }

    // 3. Prepare rooms and config
    const auto& rooms = store_.getRooms();
    Config config;
    config.facilityCheckEnabled = true;

    // 4. Run chosen algorithm
    Result res;
    if (algorithmName == "Priority Greedy" || algorithmName == "A") {
        res = priority_greedy::run(pendingRequests, rooms, fixedBookings, config);
    } else if (algorithmName == "Earliest Finish Time Greedy" || algorithmName == "B") {
        res = earliest_finish::run(pendingRequests, rooms, fixedBookings, config);
    } else if (algorithmName == "Exhaustive Search with Pruning" || algorithmName == "C") {
        res = exhaustive::run(pendingRequests, rooms, fixedBookings, config);
    } else {
        return {false, "Unknown algorithm: " + algorithmName + " (valid choices: A, B, C)."};
    }

    // 5. Store results as Recommendations (NEVER bookings)
    int runId = store_.nextRunId();

    // Map assignments
    for (const auto& asgn : res.assignments) {
        // Find stored request
        for (auto& sr : store_.getRequests()) {
            if (sr.req.id == asgn.requestId) {
                sr.status = RequestStatus::Recommended;
                sr.unallocatedReason.clear();

                // Add recommendation
                Recommendation rec;
                rec.id = store_.nextRecommendationId();
                rec.requestId = asgn.requestId;
                rec.roomId = asgn.roomId;
                rec.algorithmName = res.algorithmName;
                rec.runId = runId;
                store_.getRecommendations().push_back(rec);
                break;
            }
        }
    }

    // Unallocated requests stay Pending with reason recorded
    for (const auto& un : res.unallocated) {
        for (auto& sr : store_.getRequests()) {
            if (sr.req.id == un.requestId) {
                sr.status = RequestStatus::Pending;
                sr.unallocatedReason = unallocatedReasonToString(un.reason);
                break;
            }
        }
    }

    store_.save();

    std::ostringstream oss;
    oss << "Algorithm \"" << res.algorithmName << "\" ran successfully: "
        << res.allocatedCount << " request(s) recommended, "
        << res.unallocated.size() << " remained pending. Zero bookings created.";
    return {true, oss.str()};
}

// -------------------------------------------------------------
// 3. Compare Algorithms (Admin only)
// -------------------------------------------------------------
std::pair<OperationResult, std::vector<ComparisonRow>> WorkflowEngine::compareAlgorithms(Role role) {
    if (role != Role::Admin) {
        return {{false, "Access denied: Admin role required to compare algorithms."}, {}};
    }

    // Gather pending requests
    std::vector<Request> pendingRequests;
    for (const auto& sr : store_.getRequests()) {
        if (sr.status == RequestStatus::Pending) {
            pendingRequests.push_back(sr.req);
        }
    }

    std::vector<Booking> fixedBookings;
    for (const auto& b : store_.getBookings()) {
        fixedBookings.push_back(Booking{b.id, b.roomId, b.start, b.end});
    }

    const auto& rooms = store_.getRooms();
    Config config;
    config.facilityCheckEnabled = true;

    // Run A, B, and C without modifying store or statuses
    auto resA = priority_greedy::run(pendingRequests, rooms, fixedBookings, config);
    auto resB = earliest_finish::run(pendingRequests, rooms, fixedBookings, config);
    auto resC = exhaustive::run(pendingRequests, rooms, fixedBookings, config);

    auto violsA = validate(pendingRequests, rooms, fixedBookings, resA.assignments, config);
    auto violsB = validate(pendingRequests, rooms, fixedBookings, resB.assignments, config);
    auto violsC = validate(pendingRequests, rooms, fixedBookings, resC.assignments, config);

    std::vector<ComparisonRow> rows = {
        {resA.algorithmName, resA.allocatedCount, resA.totalWeight, resA.runtimeMs, static_cast<int>(violsA.size())},
        {resB.algorithmName, resB.allocatedCount, resB.totalWeight, resB.runtimeMs, static_cast<int>(violsB.size())},
        {resC.algorithmName, resC.allocatedCount, resC.totalWeight, resC.runtimeMs, static_cast<int>(violsC.size())}
    };

    return {{true, "Comparison completed successfully."}, rows};
}

// -------------------------------------------------------------
// 4. Approve Recommendation (Admin only)
// -------------------------------------------------------------
OperationResult WorkflowEngine::approve(Role role, const std::string& adminName, int recommendationId) {
    if (role != Role::Admin) {
        return {false, "Access denied: Admin role required to approve recommendations."};
    }

    // 1. Locate recommendation
    auto& recs = store_.getRecommendations();
    auto recIt = std::find_if(recs.begin(), recs.end(), [recommendationId](const Recommendation& r) {
        return r.id == recommendationId;
    });
    if (recIt == recs.end()) {
        return {false, "Recommendation #" + std::to_string(recommendationId) + " not found."};
    }
    Recommendation rec = *recIt;

    // 2. Locate request
    auto& reqs = store_.getRequests();
    auto reqIt = std::find_if(reqs.begin(), reqs.end(), [rec](const StoredRequest& sr) {
        return sr.req.id == rec.requestId;
    });
    if (reqIt == reqs.end()) {
        return {false, "Request #" + std::to_string(rec.requestId) + " not found."};
    }
    StoredRequest& storedReq = *reqIt;

    // Rule 7 & Check 1: Status must still be Recommended (confirming twice fails)
    if (storedReq.status == RequestStatus::Confirmed) {
        return {false, "Request #" + std::to_string(storedReq.req.id) + " is already Confirmed. Duplicate approval refused."};
    }
    if (storedReq.status != RequestStatus::Recommended) {
        return {false, "Request #" + std::to_string(storedReq.req.id) + " is not in Recommended status."};
    }

    // 3. Locate room
    const auto& rooms = store_.getRooms();
    auto roomIt = std::find_if(rooms.begin(), rooms.end(), [rec](const Room& r) {
        return r.id == rec.roomId;
    });
    if (roomIt == rooms.end()) {
        return {false, "Room #" + std::to_string(rec.roomId) + " not found."};
    }
    const Room& room = *roomIt;

    // Check 2: Room is active
    if (!room.active) {
        return {false, "Room #" + std::to_string(room.id) + " (" + room.name + ") is inactive."};
    }

    // Check 3: Facility requirements satisfied
    if ((storedReq.req.facilities & ~room.facilities) != 0) {
        return {false, "Room #" + std::to_string(room.id) + " lacks required facilities."};
    }

    // Check 4: No overlap against confirmed bookings
    Interval reqInt{storedReq.req.getAbsoluteStart(), storedReq.req.getAbsoluteEnd()};
    for (const auto& b : store_.getBookings()) {
        if (b.roomId == room.id) {
            Interval bInt{b.start, b.end};
            if (overlaps(reqInt, bInt)) {
                std::ostringstream oss;
                oss << "Conflict with confirmed booking #" << b.id
                    << " in room " << room.name << " [" << formatTime(b.start % 1440)
                    << " - " << formatTime(b.end % 1440) << ").";
                return {false, oss.str()};
            }
        }
    }

    // All re-checks passed: Create Confirmed Booking and update status
    storedReq.status = RequestStatus::Confirmed;
    storedReq.unallocatedReason.clear();

    StoredBooking newBooking;
    newBooking.id = store_.nextBookingId();
    newBooking.roomId = room.id;
    newBooking.start = reqInt.start;
    newBooking.end = reqInt.end;
    newBooking.requestId = storedReq.req.id;
    store_.getBookings().push_back(newBooking);

    // Decision log
    std::string details = "Approved proposal from algorithm " + rec.algorithmName;
    if (storedReq.req.attendance > room.capacity) {
        details += " [Informational: attendance " + std::to_string(storedReq.req.attendance) +
                   " exceeds listed room capacity " + std::to_string(room.capacity) + "]";
    }
    DecisionLog decision{
        adminName,
        currentTimestampString(),
        "Approve",
        storedReq.req.id,
        room.id,
        "Admin approval of recommendation #" + std::to_string(rec.id),
        details
    };
    store_.getDecisions().push_back(decision);

    // Remove recommendation
    recs.erase(recIt);

    // Rule 8: Safety net validation on full confirmed schedule
    std::vector<std::string> violations;
    if (!validateConfirmedSchedule(violations)) {
        // Rollback
        store_.getBookings().pop_back();
        store_.getDecisions().pop_back();
        storedReq.status = RequestStatus::Recommended;
        recs.push_back(rec);
        std::ostringstream oss;
        oss << "Approval failed post-validation safety net (" << violations.size() << " violation(s)). Rolled back.";
        return {false, oss.str()};
    }

    store_.save();
    return {true, "Request #" + std::to_string(storedReq.req.id) + " successfully approved and booked in Room " + room.name + "."};
}

// -------------------------------------------------------------
// 5. Reject Request (Admin only)
// -------------------------------------------------------------
OperationResult WorkflowEngine::reject(Role role, const std::string& adminName, int requestId, const std::string& reason) {
    if (role != Role::Admin) {
        return {false, "Access denied: Admin role required to reject requests."};
    }

    // Reason is strictly required
    std::string trimmedReason = reason;
    trimmedReason.erase(0, trimmedReason.find_first_not_of(" \t\n\r"));
    trimmedReason.erase(trimmedReason.find_last_not_of(" \t\n\r") + 1);
    if (trimmedReason.empty()) {
        return {false, "Rejection reason is required."};
    }

    auto& reqs = store_.getRequests();
    auto reqIt = std::find_if(reqs.begin(), reqs.end(), [requestId](const StoredRequest& sr) {
        return sr.req.id == requestId;
    });
    if (reqIt == reqs.end()) {
        return {false, "Request #" + std::to_string(requestId) + " not found."};
    }
    StoredRequest& storedReq = *reqIt;

    storedReq.status = RequestStatus::Rejected;
    storedReq.unallocatedReason = trimmedReason;

    // Remove any active recommendation for this request
    auto& recs = store_.getRecommendations();
    recs.erase(
        std::remove_if(recs.begin(), recs.end(), [requestId](const Recommendation& r) {
            return r.requestId == requestId;
        }),
        recs.end()
    );

    // Log decision
    DecisionLog decision{
        adminName,
        currentTimestampString(),
        "Reject",
        requestId,
        0,
        trimmedReason,
        "Request rejected by administrator"
    };
    store_.getDecisions().push_back(decision);

    store_.save();
    return {true, "Request #" + std::to_string(requestId) + " rejected."};
}

// -------------------------------------------------------------
// 6. Modify Room and Approve (Admin only)
// -------------------------------------------------------------
OperationResult WorkflowEngine::modifyAndApprove(
    Role role,
    const std::string& adminName,
    int recommendationId,
    int newRoomId
) {
    if (role != Role::Admin) {
        return {false, "Access denied: Admin role required to modify and approve recommendations."};
    }

    // 1. Locate recommendation
    auto& recs = store_.getRecommendations();
    auto recIt = std::find_if(recs.begin(), recs.end(), [recommendationId](const Recommendation& r) {
        return r.id == recommendationId;
    });
    if (recIt == recs.end()) {
        return {false, "Recommendation #" + std::to_string(recommendationId) + " not found."};
    }
    Recommendation rec = *recIt;

    // 2. Locate request
    auto& reqs = store_.getRequests();
    auto reqIt = std::find_if(reqs.begin(), reqs.end(), [rec](const StoredRequest& sr) {
        return sr.req.id == rec.requestId;
    });
    if (reqIt == reqs.end()) {
        return {false, "Request #" + std::to_string(rec.requestId) + " not found."};
    }
    StoredRequest& storedReq = *reqIt;

    // Rule 7 & Check 1: Status must still be Recommended
    if (storedReq.status == RequestStatus::Confirmed) {
        return {false, "Request #" + std::to_string(storedReq.req.id) + " is already Confirmed. Duplicate approval refused."};
    }
    if (storedReq.status != RequestStatus::Recommended) {
        return {false, "Request #" + std::to_string(storedReq.req.id) + " is not in Recommended status."};
    }

    // 3. Locate new room
    const auto& rooms = store_.getRooms();
    auto roomIt = std::find_if(rooms.begin(), rooms.end(), [newRoomId](const Room& r) {
        return r.id == newRoomId;
    });
    if (roomIt == rooms.end()) {
        return {false, "Target room #" + std::to_string(newRoomId) + " does not exist."};
    }
    const Room& newRoom = *roomIt;

    // Check 2: New room is active
    if (!newRoom.active) {
        return {false, "Target room #" + std::to_string(newRoom.id) + " (" + newRoom.name + ") is inactive."};
    }

    // Check 3: Facility requirements satisfied
    if ((storedReq.req.facilities & ~newRoom.facilities) != 0) {
        return {false, "Target room " + newRoom.name + " lacks required facilities (required: " +
                       std::to_string(storedReq.req.facilities) + ", offered: " + std::to_string(newRoom.facilities) + ")."};
    }

    // Check 4: Interval does NOT overlap any confirmed booking in target room
    Interval reqInt{storedReq.req.getAbsoluteStart(), storedReq.req.getAbsoluteEnd()};
    for (const auto& b : store_.getBookings()) {
        if (b.roomId == newRoom.id) {
            Interval bInt{b.start, b.end};
            if (overlaps(reqInt, bInt)) {
                std::ostringstream oss;
                oss << "Conflict with confirmed booking #" << b.id
                    << " in room " << newRoom.name << " [" << formatTime(b.start % 1440)
                    << " - " << formatTime(b.end % 1440) << ").";
                return {false, oss.str()};
            }
        }
    }

    // Informational capacity note (never blocks!)
    std::string capacityNotice;
    if (storedReq.req.attendance > newRoom.capacity) {
        capacityNotice = " [Notice: attendance " + std::to_string(storedReq.req.attendance) +
                         " exceeds listed capacity " + std::to_string(newRoom.capacity) + " (informational)]";
    }

    // Update status and create booking
    storedReq.status = RequestStatus::Confirmed;
    storedReq.unallocatedReason.clear();

    StoredBooking newBooking;
    newBooking.id = store_.nextBookingId();
    newBooking.roomId = newRoom.id;
    newBooking.start = reqInt.start;
    newBooking.end = reqInt.end;
    newBooking.requestId = storedReq.req.id;
    store_.getBookings().push_back(newBooking);

    // Decision log
    std::string details = "Modified room from " + std::to_string(rec.roomId) + " to " + std::to_string(newRoom.id) + capacityNotice;
    DecisionLog decision{
        adminName,
        currentTimestampString(),
        "ModifyApprove",
        storedReq.req.id,
        newRoom.id,
        "Admin modified room and approved recommendation #" + std::to_string(rec.id),
        details
    };
    store_.getDecisions().push_back(decision);

    // Remove recommendation
    recs.erase(recIt);

    // Rule 8: Run safety net validator
    std::vector<std::string> violations;
    if (!validateConfirmedSchedule(violations)) {
        // Rollback
        store_.getBookings().pop_back();
        store_.getDecisions().pop_back();
        storedReq.status = RequestStatus::Recommended;
        recs.push_back(rec);
        std::ostringstream oss;
        oss << "Modify & approve failed post-validation safety net (" << violations.size() << " violation(s)). Rolled back.";
        return {false, oss.str()};
    }

    store_.save();
    return {true, "Request #" + std::to_string(storedReq.req.id) + " modified to Room " + newRoom.name + " and approved." + capacityNotice};
}

// -------------------------------------------------------------
// Validation Safety Net
// -------------------------------------------------------------
bool WorkflowEngine::validateConfirmedSchedule(std::vector<std::string>& outViolations) {
    std::vector<Request> requests;
    for (const auto& sr : store_.getRequests()) {
        requests.push_back(sr.req);
    }

    const auto& rooms = store_.getRooms();

    std::vector<Assignment> asgns;
    std::vector<Booking> fixedBookings;

    for (const auto& b : store_.getBookings()) {
        if (b.requestId > 0) {
            asgns.push_back(Assignment{b.requestId, b.roomId});
        } else {
            fixedBookings.push_back(Booking{b.id, b.roomId, b.start, b.end});
        }
    }

    Config cfg;
    cfg.facilityCheckEnabled = true;

    outViolations = validate(requests, rooms, fixedBookings, asgns, cfg);
    return outViolations.empty();
}

// -------------------------------------------------------------
// Query Helpers
// -------------------------------------------------------------
std::vector<StoredRequest> WorkflowEngine::getRequestsForClub(const std::string& clubId) const {
    std::vector<StoredRequest> result;
    for (const auto& sr : store_.getRequests()) {
        if (sr.req.clubId == clubId) {
            result.push_back(sr);
        }
    }
    return result;
}

std::vector<Room> WorkflowEngine::getFeasibleRoomsForRequest(int requestId) const {
    std::vector<Room> feasible;
    auto& reqs = store_.getRequests();
    auto it = std::find_if(reqs.begin(), reqs.end(), [requestId](const StoredRequest& sr) {
        return sr.req.id == requestId;
    });
    if (it == reqs.end()) return feasible;

    const auto& req = it->req;
    Interval reqInt{req.getAbsoluteStart(), req.getAbsoluteEnd()};

    for (const auto& room : store_.getRooms()) {
        if (!room.active) continue;
        if ((req.facilities & ~room.facilities) != 0) continue;

        bool conflict = false;
        for (const auto& b : store_.getBookings()) {
            if (b.roomId == room.id) {
                if (overlaps(reqInt, Interval{b.start, b.end})) {
                    conflict = true;
                    break;
                }
            }
        }
        if (!conflict) {
            feasible.push_back(room);
        }
    }
    return feasible;
}

const StoredBooking* WorkflowEngine::getBookingForRequest(int requestId) const {
    for (const auto& b : store_.getBookings()) {
        if (b.requestId == requestId) {
            return &b;
        }
    }
    return nullptr;
}
