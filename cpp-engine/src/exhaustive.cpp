// Time Complexity: O((m + 1)^n) worst case
//   where n = number of requests, m = number of rooms.
//   Each request can branch to any of the m rooms or remain unallocated (m + 1 choices).
//   Pruning via branch-and-bound significantly reduces the search space in practice.
// Space Complexity: O(n + m + b) for recursion stack and schedule tracking.

#include "algorithms.hpp"
#include "interval.hpp"
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace exhaustive {

namespace {

void search(
    size_t reqIdx,
    int currentWeight,
    int currentCount,
    int currentSatisfied,
    const std::vector<Request>& validRequests,
    const std::vector<Room>& sortedRooms,
    const std::vector<int>& reqWeights,
    const std::vector<int>& remainingWeights,
    const std::vector<int>& remainingPrefs,
    const Config& config,
    const std::unordered_map<int, std::vector<Interval>>& roomBookings,
    std::unordered_map<int, std::vector<Interval>>& roomAssignments,
    std::vector<Assignment>& currentAssignments,
    int& bestWeight,
    int& bestCount,
    int& bestSatisfied,
    std::vector<Assignment>& bestAssignments
) {
    size_t n = validRequests.size();

    // Base case: all requests considered
    if (reqIdx == n) {
        bool isBetter = false;
        if (currentWeight > bestWeight) {
            isBetter = true;
        } else if (currentWeight == bestWeight) {
            if (currentCount > bestCount) {
                isBetter = true;
            } else if (currentCount == bestCount) {
                if (currentSatisfied > bestSatisfied) {
                    isBetter = true;
                }
            }
        }
        if (isBetter) {
            bestWeight = currentWeight;
            bestCount = currentCount;
            bestSatisfied = currentSatisfied;
            bestAssignments = currentAssignments;
        }
        return;
    }

    // Pruning Bound Explanation:
    // The objective is lexicographic:
    //   1. totalWeight (maximize)
    //   2. allocatedCount (maximize)
    //   3. satisfiedPreferences (maximize)
    //   4. Fixed tie order (first branch visited in deterministic search order).
    //
    // For any completion of the current partial schedule starting from reqIdx:
    // - Maximum possible weight: maxPossibleWeight = currentWeight + remainingWeights[reqIdx]
    // - Maximum possible count:  maxPossibleCount  = currentCount + (n - reqIdx)
    // - Maximum possible satisfied preferences: maxPossibleSatisfied = currentSatisfied + remainingPrefs[reqIdx]
    //
    // A branch can be safely pruned if and only if no completion can strictly beat (bestWeight, bestCount, bestSatisfied):
    // 1. If maxPossibleWeight < bestWeight:
    //    Even if all remaining requests are allocated, total weight is strictly below bestWeight.
    // 2. If maxPossibleWeight == bestWeight and maxPossibleCount < bestCount:
    //    Weight can at best tie bestWeight, but allocated count cannot reach bestCount.
    // 3. If maxPossibleWeight == bestWeight and maxPossibleCount == bestCount and maxPossibleSatisfied <= bestSatisfied:
    //    Weight and count can at best tie, and satisfied preferences cannot exceed bestSatisfied.
    //    Because ties are awarded to the incumbent best schedule, no completion can beat best.
    // Any completion with maxPossibleWeight > bestWeight, or equal weight with strictly higher potential count,
    // or equal weight and equal count with strictly higher potential satisfied preferences, could strictly beat
    // the current best and is preserved. Hence, this pruning rule is exact and never eliminates an optimal solution.
    if (bestWeight >= 0) {
        int maxPossibleWeight = currentWeight + remainingWeights[reqIdx];
        if (maxPossibleWeight < bestWeight) {
            return;
        }
        if (maxPossibleWeight == bestWeight) {
            int remainingCount = static_cast<int>(n - reqIdx);
            int maxPossibleCount = currentCount + remainingCount;
            if (maxPossibleCount < bestCount) {
                return;
            }
            if (maxPossibleCount == bestCount) {
                int maxPossibleSatisfied = currentSatisfied + remainingPrefs[reqIdx];
                if (maxPossibleSatisfied <= bestSatisfied) {
                    return;
                }
            }
        }
    }

    const auto& req = validRequests[reqIdx];
    Interval reqInterval{req.getAbsoluteStart(), req.getAbsoluteEnd()};
    int weight = reqWeights[reqIdx];

    // Branch 1..m: Assign to each feasible room (in ascending room id order)
    // For Strict preference, the search branches ONLY over the room the request is allowed to use.
    for (const auto& room : sortedRooms) {
        if (req.preferredRoomId != 0 && req.roomMode == PreferenceMode::Strict) {
            if (room.id != req.preferredRoomId) {
                continue; // Strict: branch only over the allowed preferred room
            }
        }

        if (!room.active) {
            continue;
        }

        if (config.facilityCheckEnabled && ((req.facilities & ~room.facilities) != 0)) {
            continue;
        }

        // Check overlap with fixed bookings
        bool conflict = false;
        auto fbIt = roomBookings.find(room.id);
        if (fbIt != roomBookings.end()) {
            for (const auto& fbInt : fbIt->second) {
                if (overlaps(reqInterval, fbInt)) {
                    conflict = true;
                    break;
                }
            }
        }
        if (conflict) {
            continue;
        }

        // Check overlap with current assignments in this room
        auto& asgns = roomAssignments[room.id];
        for (const auto& asgnInt : asgns) {
            if (overlaps(reqInterval, asgnInt)) {
                conflict = true;
                break;
            }
        }
        if (conflict) {
            continue;
        }

        // Feasible branch: assign to room
        int addedSatisfied = (req.preferredRoomId != 0 && room.id == req.preferredRoomId) ? 1 : 0;
        asgns.push_back(reqInterval);
        currentAssignments.push_back({req.id, room.id});

        search(
            reqIdx + 1,
            currentWeight + weight,
            currentCount + 1,
            currentSatisfied + addedSatisfied,
            validRequests,
            sortedRooms,
            reqWeights,
            remainingWeights,
            remainingPrefs,
            config,
            roomBookings,
            roomAssignments,
            currentAssignments,
            bestWeight,
            bestCount,
            bestSatisfied,
            bestAssignments
        );

        // Backtrack
        currentAssignments.pop_back();
        asgns.pop_back();
    }

    // Branch m+1: Leave unallocated
    search(
        reqIdx + 1,
        currentWeight,
        currentCount,
        currentSatisfied,
        validRequests,
        sortedRooms,
        reqWeights,
        remainingWeights,
        remainingPrefs,
        config,
        roomBookings,
        roomAssignments,
        currentAssignments,
        bestWeight,
        bestCount,
        bestSatisfied,
        bestAssignments
    );
}

} // anonymous namespace

Result run(
    const std::vector<Request>& requests,
    const std::vector<Room>& rooms,
    const std::vector<Booking>& fixedBookings,
    const Config& config
) {
    auto startTime = std::chrono::steady_clock::now();

    Result result;
    result.algorithmName = "Exhaustive Search with Pruning";

    // 1. Separate valid and invalid requests
    std::vector<Request> validRequests;
    for (const auto& req : requests) {
        if (req.end <= req.start) {
            result.unallocated.push_back({req.id, UnallocatedReason::InvalidTime});
        } else {
            validRequests.push_back(req);
        }
    }

    // 2. Guard against too large search space
    if (static_cast<int>(validRequests.size()) > config.maxExhaustiveRequests) {
        result.warnings.push_back(
            "Valid request count (" + std::to_string(validRequests.size()) +
            ") exceeds maxExhaustiveRequests (" + std::to_string(config.maxExhaustiveRequests) +
            "). Exhaustive search aborted."
        );
        for (const auto& req : validRequests) {
            result.unallocated.push_back({req.id, UnallocatedReason::TooLargeForSearch});
        }
        auto endTime = std::chrono::steady_clock::now();
        result.runtimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return result;
    }

    // 3. Process requests in Algorithm A's sorted order: tier desc, date asc, submittedAt asc, id asc
    std::sort(validRequests.begin(), validRequests.end(), [](const Request& a, const Request& b) {
        if (a.tier != b.tier) return a.tier > b.tier;
        if (a.date != b.date) return a.date < b.date;
        if (a.submittedAt != b.submittedAt) return a.submittedAt < b.submittedAt;
        return a.id < b.id;
    });

    // 4. Sort rooms in ascending room id order
    std::vector<Room> sortedRooms = rooms;
    std::sort(sortedRooms.begin(), sortedRooms.end(), [](const Room& a, const Room& b) {
        return a.id < b.id;
    });

    // Organize fixed bookings by room
    std::unordered_map<int, std::vector<Interval>> roomBookings;
    for (const auto& b : fixedBookings) {
        roomBookings[b.roomId].push_back(Interval{b.start, b.end});
    }

    // Precompute weights and suffix sums of weights for pruning
    size_t n = validRequests.size();
    std::vector<int> reqWeights(n);
    for (size_t i = 0; i < n; ++i) {
        reqWeights[i] = tierToWeight(validRequests[i].tier);
    }

    std::vector<int> remainingWeights(n + 1, 0);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        remainingWeights[i] = remainingWeights[i + 1] + reqWeights[i];
    }

    std::vector<int> remainingPrefs(n + 1, 0);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        int hasPref = (validRequests[i].preferredRoomId != 0) ? 1 : 0;
        remainingPrefs[i] = remainingPrefs[i + 1] + hasPref;
    }

    int bestWeight = -1;
    int bestCount = -1;
    int bestSatisfied = -1;
    std::vector<Assignment> bestAssignments;
    std::vector<Assignment> currentAssignments;
    std::unordered_map<int, std::vector<Interval>> roomAssignments;

    search(
        0,
        0,
        0,
        0,
        validRequests,
        sortedRooms,
        reqWeights,
        remainingWeights,
        remainingPrefs,
        config,
        roomBookings,
        roomAssignments,
        currentAssignments,
        bestWeight,
        bestCount,
        bestSatisfied,
        bestAssignments
    );

    if (bestWeight < 0) {
        bestWeight = 0;
        bestCount = 0;
        bestSatisfied = 0;
    }

    result.assignments = bestAssignments;
    result.totalWeight = bestWeight;
    result.allocatedCount = bestCount;
    result.satisfiedPreferences = bestSatisfied;

    // Identify unallocated requests and assign most specific reason
    std::unordered_set<int> allocatedRequestIds;
    for (const auto& asgn : bestAssignments) {
        allocatedRequestIds.insert(asgn.requestId);
    }

    for (const auto& req : validRequests) {
        if (allocatedRequestIds.find(req.id) == allocatedRequestIds.end()) {
            if (req.preferredRoomId != 0 && req.roomMode == PreferenceMode::Strict) {
                auto prefIt = std::find_if(sortedRooms.begin(), sortedRooms.end(), [&](const Room& r) {
                    return r.id == req.preferredRoomId;
                });
                if (prefIt == sortedRooms.end() || !prefIt->active) {
                    result.unallocated.push_back({req.id, UnallocatedReason::NoActiveRoom});
                } else if (config.facilityCheckEnabled && ((req.facilities & ~prefIt->facilities) != 0)) {
                    result.unallocated.push_back({req.id, UnallocatedReason::MissingFacility});
                } else {
                    result.unallocated.push_back({req.id, UnallocatedReason::TimeConflict});
                }
            } else {
                bool anyActive = false;
                bool anyFacilityEligible = false;

                for (const auto& room : sortedRooms) {
                    if (!room.active) continue;
                    anyActive = true;

                    if (config.facilityCheckEnabled && ((req.facilities & ~room.facilities) != 0)) {
                        continue;
                    }
                    anyFacilityEligible = true;
                }

                UnallocatedReason reason;
                if (!anyActive) {
                    reason = UnallocatedReason::NoActiveRoom;
                } else if (!anyFacilityEligible) {
                    reason = UnallocatedReason::MissingFacility;
                } else {
                    reason = UnallocatedReason::TimeConflict;
                }
                result.unallocated.push_back({req.id, reason});
            }
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.runtimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

} // namespace exhaustive
