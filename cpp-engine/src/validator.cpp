#include "validator.hpp"
#include "interval.hpp"
#include <unordered_map>
#include <unordered_set>
#include <sstream>

std::vector<std::string> validate(
    const std::vector<Request>& requests,
    const std::vector<Room>& rooms,
    const std::vector<Booking>& fixedBookings,
    const std::vector<Assignment>& assignments,
    const Config& config
) {
    std::vector<std::string> violations;

    // Fast lookup for requests
    std::unordered_map<int, const Request*> requestMap;
    for (const auto& req : requests) {
        requestMap[req.id] = &req;
    }

    // Fast lookup for rooms
    std::unordered_map<int, const Room*> roomMap;
    for (const auto& room : rooms) {
        roomMap[room.id] = &room;
    }

    // 5. Check: Each request is assigned at most once
    std::unordered_map<int, int> requestAssignmentCount;
    for (const auto& asgn : assignments) {
        requestAssignmentCount[asgn.requestId]++;
    }
    for (const auto& [reqId, count] : requestAssignmentCount) {
        if (count > 1) {
            std::ostringstream oss;
            oss << "Request " << reqId << " is assigned multiple times (" << count << " assignments).";
            violations.push_back(oss.str());
        }
    }

    // 4. Check: Every assignment uses an existing, active room & valid request
    for (const auto& asgn : assignments) {
        auto reqIt = requestMap.find(asgn.requestId);
        if (reqIt == requestMap.end()) {
            std::ostringstream oss;
            oss << "Assignment references non-existent request ID: " << asgn.requestId;
            violations.push_back(oss.str());
            continue;
        }

        auto roomIt = roomMap.find(asgn.roomId);
        if (roomIt == roomMap.end()) {
            std::ostringstream oss;
            oss << "Assignment for request " << asgn.requestId << " references non-existent room ID: " << asgn.roomId;
            violations.push_back(oss.str());
            continue;
        }

        const Room* room = roomIt->second;
        if (!room->active) {
            std::ostringstream oss;
            oss << "Assignment for request " << asgn.requestId << " uses inactive room " << room->id << " (" << room->name << ").";
            violations.push_back(oss.str());
        }

        // 3. Facility requirements satisfied (when enabled)
        if (config.facilityCheckEnabled) {
            const Request* req = reqIt->second;
            if ((req->facilities & ~room->facilities) != 0) {
                std::ostringstream oss;
                oss << "Request " << req->id << " requires facilities (" << req->facilities 
                    << ") not provided by room " << room->id << " (" << room->facilities << ").";
                violations.push_back(oss.str());
            }
        }

        // Strict room preference check (Flexible is never a violation)
        const Request* req = reqIt->second;
        if (req->preferredRoomId != 0 && req->roomMode == PreferenceMode::Strict) {
            if (asgn.roomId != req->preferredRoomId) {
                std::ostringstream oss;
                oss << "Request " << req->id << ": strict room preference violated (assigned to room "
                    << asgn.roomId << " instead of preferred room " << req->preferredRoomId << ").";
                violations.push_back(oss.str());
            }
        }
    }

    // 2. Check: No assignment overlaps a fixed booking
    for (const auto& asgn : assignments) {
        auto reqIt = requestMap.find(asgn.requestId);
        if (reqIt == requestMap.end()) continue;

        Interval reqInterval{reqIt->second->getAbsoluteStart(), reqIt->second->getAbsoluteEnd()};

        for (const auto& fb : fixedBookings) {
            if (fb.roomId == asgn.roomId) {
                Interval fbInterval{fb.start, fb.end};
                if (overlaps(reqInterval, fbInterval)) {
                    std::ostringstream oss;
                    oss << "Assignment for request " << asgn.requestId << " [" << reqInterval.start << ", " << reqInterval.end
                        << ") overlaps fixed booking " << fb.id << " [" << fb.start << ", " << fb.end
                        << ") in room " << asgn.roomId << ".";
                    violations.push_back(oss.str());
                }
            }
        }
    }

    // 1. Check: No two assignments overlap in the same room
    // 6. Adjacent half-open intervals are NOT reported as conflicts (handled by overlaps())
    for (size_t i = 0; i < assignments.size(); ++i) {
        auto reqIt1 = requestMap.find(assignments[i].requestId);
        if (reqIt1 == requestMap.end()) continue;
        Interval int1{reqIt1->second->getAbsoluteStart(), reqIt1->second->getAbsoluteEnd()};

        for (size_t j = i + 1; j < assignments.size(); ++j) {
            if (assignments[i].roomId != assignments[j].roomId) continue;

            auto reqIt2 = requestMap.find(assignments[j].requestId);
            if (reqIt2 == requestMap.end()) continue;
            Interval int2{reqIt2->second->getAbsoluteStart(), reqIt2->second->getAbsoluteEnd()};

            if (overlaps(int1, int2)) {
                std::ostringstream oss;
                oss << "Assignment for request " << assignments[i].requestId << " [" << int1.start << ", " << int1.end
                    << ") overlaps assignment for request " << assignments[j].requestId << " [" << int2.start << ", " << int2.end
                    << ") in room " << assignments[i].roomId << ".";
                violations.push_back(oss.str());
            }
        }
    }

    // Note: Room capacity is strictly informational and is never validated as a violation.
    return violations;
}
