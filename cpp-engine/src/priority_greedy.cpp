// Time Complexity: O(n log n + n * m * b)
//   where n = number of requests, m = number of rooms, b = max bookings/assignments per room.
//   Sorting requests takes O(n log n). For each request, scanning m rooms and checking overlap
//   against up to b bookings/assignments takes O(m * b).
// Space Complexity: O(n + m + b) to store sorted requests, room index, and active schedules.

#include "algorithms.hpp"
#include "interval.hpp"
#include <algorithm>
#include <chrono>
#include <unordered_map>

namespace priority_greedy {

Result run(
    const std::vector<Request>& requests,
    const std::vector<Room>& rooms,
    const std::vector<Booking>& fixedBookings,
    const Config& config
) {
    auto startTime = std::chrono::steady_clock::now();

    Result result;
    result.algorithmName = "Priority Greedy";

    // 1. Separate valid and invalid requests
    std::vector<Request> validRequests;
    for (const auto& req : requests) {
        if (req.end <= req.start) {
            result.unallocated.push_back({req.id, UnallocatedReason::InvalidTime});
        } else {
            validRequests.push_back(req);
        }
    }

    // 2. Sort valid requests by: tier desc, date asc, submittedAt asc, id asc
    std::sort(validRequests.begin(), validRequests.end(), [](const Request& a, const Request& b) {
        if (a.tier != b.tier) return a.tier > b.tier;
        if (a.date != b.date) return a.date < b.date;
        if (a.submittedAt != b.submittedAt) return a.submittedAt < b.submittedAt;
        return a.id < b.id;
    });

    // 3. Sort rooms in ascending room id order
    std::vector<Room> sortedRooms = rooms;
    std::sort(sortedRooms.begin(), sortedRooms.end(), [](const Room& a, const Room& b) {
        return a.id < b.id;
    });

    // Organize fixed bookings by room
    std::unordered_map<int, std::vector<Interval>> roomBookings;
    for (const auto& b : fixedBookings) {
        roomBookings[b.roomId].push_back(Interval{b.start, b.end});
    }

    // Schedules of proposed assignments per room
    std::unordered_map<int, std::vector<Interval>> proposedSchedule;

    // 4. Allocate requests
    for (const auto& req : validRequests) {
        Interval reqInterval{req.getAbsoluteStart(), req.getAbsoluteEnd()};

        auto isRoomAvailable = [&](const Room& room) -> bool {
            auto fbIt = roomBookings.find(room.id);
            if (fbIt != roomBookings.end()) {
                for (const auto& fbInt : fbIt->second) {
                    if (overlaps(reqInterval, fbInt)) {
                        return false;
                    }
                }
            }
            auto asgnIt = proposedSchedule.find(room.id);
            if (asgnIt != proposedSchedule.end()) {
                for (const auto& asgnInt : asgnIt->second) {
                    if (overlaps(reqInterval, asgnInt)) {
                        return false;
                    }
                }
            }
            return true;
        };

        bool allocated = false;
        int chosenRoomId = -1;
        bool satisfied = false;

        if (req.preferredRoomId != 0 && req.roomMode == PreferenceMode::Strict) {
            // Strict: the preferred room is the ONLY eligible room.
            auto prefRoomIt = std::find_if(sortedRooms.begin(), sortedRooms.end(), [&](const Room& r) {
                return r.id == req.preferredRoomId;
            });

            if (prefRoomIt == sortedRooms.end() || !prefRoomIt->active) {
                result.unallocated.push_back({req.id, UnallocatedReason::NoActiveRoom});
                continue;
            }
            if (config.facilityCheckEnabled && ((req.facilities & ~prefRoomIt->facilities) != 0)) {
                result.unallocated.push_back({req.id, UnallocatedReason::MissingFacility});
                continue;
            }
            if (isRoomAvailable(*prefRoomIt)) {
                chosenRoomId = prefRoomIt->id;
                allocated = true;
                satisfied = true;
            } else {
                result.unallocated.push_back({req.id, UnallocatedReason::TimeConflict});
                continue;
            }
        } else {
            // Flexible or No Preference:
            // If Flexible: the preferred room is tried FIRST (if it is active, facility-eligible and free)
            if (req.preferredRoomId != 0 && req.roomMode == PreferenceMode::Flexible) {
                auto prefRoomIt = std::find_if(sortedRooms.begin(), sortedRooms.end(), [&](const Room& r) {
                    return r.id == req.preferredRoomId;
                });
                if (prefRoomIt != sortedRooms.end() && prefRoomIt->active) {
                    bool facOk = !config.facilityCheckEnabled || ((req.facilities & ~prefRoomIt->facilities) == 0);
                    if (facOk && isRoomAvailable(*prefRoomIt)) {
                        chosenRoomId = prefRoomIt->id;
                        allocated = true;
                        satisfied = true;
                    }
                }
            }

            // Fall back to existing deterministic room order
            if (!allocated) {
                bool anyActive = false;
                bool anyFacilityEligible = false;

                for (const auto& room : sortedRooms) {
                    if (!room.active) continue;
                    anyActive = true;

                    if (config.facilityCheckEnabled && ((req.facilities & ~room.facilities) != 0)) {
                        continue;
                    }
                    anyFacilityEligible = true;

                    if (isRoomAvailable(room)) {
                        chosenRoomId = room.id;
                        allocated = true;
                        satisfied = (req.preferredRoomId != 0 && room.id == req.preferredRoomId);
                        break;
                    }
                }

                if (!allocated) {
                    UnallocatedReason reason;
                    if (!anyActive) {
                        reason = UnallocatedReason::NoActiveRoom;
                    } else if (!anyFacilityEligible) {
                        reason = UnallocatedReason::MissingFacility;
                    } else {
                        reason = UnallocatedReason::TimeConflict;
                    }
                    result.unallocated.push_back({req.id, reason});
                    continue;
                }
            }
        }

        if (allocated) {
            proposedSchedule[chosenRoomId].push_back(reqInterval);
            result.assignments.push_back({req.id, chosenRoomId});
            result.totalWeight += tierToWeight(req.tier);
            result.allocatedCount++;
            if (satisfied) {
                result.satisfiedPreferences++;
            }
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.runtimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

} // namespace priority_greedy
