#pragma once

#include "store.hpp"
#include <string>
#include <vector>

struct OperationResult {
    bool success = false;
    std::string message;
};

struct ComparisonRow {
    std::string algorithmName;
    int allocatedCount = 0;
    int totalWeight = 0;
    double runtimeMs = 0.0;
    int violations = 0;
};

class WorkflowEngine {
public:
    explicit WorkflowEngine(DataStore& store);

    // 1. Submit Request
    OperationResult submitRequest(Role role, StoredRequest request);

    // 2. Run Allocation (Admin only)
    OperationResult runAllocation(Role role, const std::string& algorithmName);

    // 3. Compare Algorithms (Admin only)
    std::pair<OperationResult, std::vector<ComparisonRow>> compareAlgorithms(Role role);

    // 4. Approve Recommendation (Admin only)
    OperationResult approve(Role role, const std::string& adminName, int recommendationId);

    // 5. Reject Request (Admin only)
    OperationResult reject(Role role, const std::string& adminName, int requestId, const std::string& reason);

    // 6. Modify Room and Approve (Admin only)
    OperationResult modifyAndApprove(Role role, const std::string& adminName, int recommendationId, int newRoomId);

    // Validation safety net on full confirmed schedule
    bool validateConfirmedSchedule(std::vector<std::string>& outViolations);

    // Query helpers
    std::vector<StoredRequest> getRequestsForClub(const std::string& clubId) const;
    std::vector<Room> getFeasibleRoomsForRequest(int requestId) const;
    const StoredBooking* getBookingForRequest(int requestId) const;

    DataStore& getStore() { return store_; }
    const DataStore& getStore() const { return store_; }

private:
    DataStore& store_;
};
