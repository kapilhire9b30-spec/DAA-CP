#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <memory>

enum class Role {
    Club,
    Admin
};

enum class RequestStatus {
    Pending,
    Recommended,
    Confirmed,
    Rejected
};

inline std::string requestStatusToString(RequestStatus status) {
    switch (status) {
        case RequestStatus::Pending: return "Pending";
        case RequestStatus::Recommended: return "Recommended";
        case RequestStatus::Confirmed: return "Confirmed";
        case RequestStatus::Rejected: return "Rejected";
    }
    return "Unknown";
}

inline RequestStatus stringToRequestStatus(const std::string& str) {
    if (str == "Recommended") return RequestStatus::Recommended;
    if (str == "Confirmed") return RequestStatus::Confirmed;
    if (str == "Rejected") return RequestStatus::Rejected;
    return RequestStatus::Pending;
}

struct StoredRequest {
    Request req;
    RequestStatus status = RequestStatus::Pending;
    std::string unallocatedReason;
};

struct StoredBooking {
    int id = 0;
    int roomId = 0;
    int start = 0; // Absolute minutes from day 0
    int end = 0;   // Absolute minutes from day 0
    int requestId = 0;
};

struct Recommendation {
    int id = 0;
    int requestId = 0;
    int roomId = 0;
    std::string algorithmName;
    int runId = 0;
};

struct DecisionLog {
    std::string who;
    std::string when;   // Formatted timestamp
    std::string action; // "Approve", "Reject", "ModifyApprove"
    int requestId = 0;
    int roomId = 0;
    std::string reason;
    std::string details;
};

// Date & Time Helpers
int daysFromCivil(int y, int m, int d);
void civilFromDays(int z, int& y, int& m, int& d);
bool parseDate(const std::string& str, int& outDayIndex);
std::string formatDate(int dayIndex);
bool parseTime(const std::string& str, int& outMinuteOfDay);
std::string formatTime(int minuteOfDay);
std::string formatDateTime(int dayIndex, int minuteOfDay);
std::string currentTimestampString();

// CSV helpers
std::string escapeCsv(const std::string& field);
std::vector<std::string> parseCsvLine(const std::string& line);

class DataStore {
public:
    explicit DataStore(std::string dataDir = "cpp-engine/data");

    bool load();
    bool save();
    bool initializeSeedIfMissing();

    const std::string& getDataDir() const { return dataDir_; }
    void setDataDir(const std::string& dir) { dataDir_ = dir; }

    // Repositories
    std::vector<Room>& getRooms() { return rooms_; }
    const std::vector<Room>& getRooms() const { return rooms_; }

    std::vector<StoredRequest>& getRequests() { return requests_; }
    const std::vector<StoredRequest>& getRequests() const { return requests_; }

    std::vector<StoredBooking>& getBookings() { return bookings_; }
    const std::vector<StoredBooking>& getBookings() const { return bookings_; }

    std::vector<Recommendation>& getRecommendations() { return recommendations_; }
    const std::vector<Recommendation>& getRecommendations() const { return recommendations_; }

    std::vector<DecisionLog>& getDecisions() { return decisions_; }
    const std::vector<DecisionLog>& getDecisions() const { return decisions_; }

    const std::string& getAdminPassword() const { return adminPassword_; }
    bool verifyAdminPassword(const std::string& pwd) const { return pwd == adminPassword_; }

    // ID generators
    int nextRequestId();
    int nextBookingId();
    int nextRecommendationId();
    int nextRoomId();
    int nextRunId() { return ++lastRunId_; }

private:
    std::string dataDir_;
    std::string adminPassword_ = "admin123";
    int lastRunId_ = 0;

    std::vector<Room> rooms_;
    std::vector<StoredRequest> requests_;
    std::vector<StoredBooking> bookings_;
    std::vector<Recommendation> recommendations_;
    std::vector<DecisionLog> decisions_;
};
