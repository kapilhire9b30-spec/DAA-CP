#include "store.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <ctime>

namespace fs = std::filesystem;

// -------------------------------------------------------------
// Date & Time Helpers
// -------------------------------------------------------------

int daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

void civilFromDays(int z, int& y, int& m, int& d) {
    // Offset by daysFromCivil(2026, 1, 1) relative to 1970-01-01
    static const int baseOffset = daysFromCivil(2026, 1, 1);
    z += 719468 + baseOffset;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int y_approx = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = y_approx + (m <= 2);
}

bool parseDate(const std::string& str, int& outDayIndex) {
    if (str.size() != 10 || str[4] != '-' || str[7] != '-') {
        return false;
    }
    int y = 0, m = 0, d = 0;
    try {
        y = std::stoi(str.substr(0, 4));
        m = std::stoi(str.substr(5, 2));
        d = std::stoi(str.substr(8, 2));
    } catch (...) {
        return false;
    }
    if (y < 2000 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) {
        return false;
    }
    // Days in month validation
    static const int daysInMon[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int maxDays = daysInMon[m - 1];
    if (m == 2) {
        bool isLeap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        if (isLeap) maxDays = 29;
    }
    if (d > maxDays) {
        return false;
    }

    static const int baseOffset = daysFromCivil(2026, 1, 1);
    outDayIndex = daysFromCivil(y, m, d) - baseOffset;
    return true;
}

std::string formatDate(int dayIndex) {
    int y = 0, m = 0, d = 0;
    civilFromDays(dayIndex, y, m, d);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << y << "-"
        << std::setw(2) << m << "-"
        << std::setw(2) << d;
    return oss.str();
}

bool parseTime(const std::string& str, int& outMinuteOfDay) {
    if (str.size() != 5 || str[2] != ':') {
        return false;
    }
    int h = 0, m = 0;
    try {
        h = std::stoi(str.substr(0, 2));
        m = std::stoi(str.substr(3, 2));
    } catch (...) {
        return false;
    }
    if (h < 0 || h > 23 || m < 0 || m > 59) {
        return false;
    }
    outMinuteOfDay = h * 60 + m;
    return true;
}

std::string formatTime(int minuteOfDay) {
    int rem = minuteOfDay % 1440;
    if (rem < 0) rem += 1440;
    int h = rem / 60;
    int m = rem % 60;
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << h << ":"
        << std::setw(2) << m;
    return oss.str();
}

std::string formatDateTime(int dayIndex, int minuteOfDay) {
    return formatDate(dayIndex) + " " + formatTime(minuteOfDay);
}

std::string currentTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmVal{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmVal, &tt);
#else
    localtime_r(&tt, &tmVal);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmVal);
    return std::string(buf);
}

// -------------------------------------------------------------
// CSV Escaping & Parsing
// -------------------------------------------------------------

std::string escapeCsv(const std::string& field) {
    bool needsQuotes = false;
    if (field.find(',') != std::string::npos ||
        field.find('"') != std::string::npos ||
        field.find('\n') != std::string::npos ||
        field.find('\r') != std::string::npos) {
        needsQuotes = true;
    }
    if (!needsQuotes) return field;

    std::string out = "\"";
    for (char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    size_t i = 0;
    while (i < line.size()) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    i += 2;
                    continue;
                } else {
                    inQuotes = false;
                    i++;
                    continue;
                }
            } else {
                current += c;
                i++;
                continue;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
                i++;
                continue;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
                i++;
                continue;
            } else if (c == '\r') {
                i++;
                continue;
            } else {
                current += c;
                i++;
                continue;
            }
        }
    }
    fields.push_back(current);
    return fields;
}

// -------------------------------------------------------------
// DataStore Implementation
// -------------------------------------------------------------

DataStore::DataStore(std::string dataDir)
    : dataDir_(std::move(dataDir)) {
}

int DataStore::nextRequestId() {
    int maxId = 0;
    for (const auto& r : requests_) {
        if (r.req.id > maxId) maxId = r.req.id;
    }
    return maxId + 1;
}

int DataStore::nextBookingId() {
    int maxId = 0;
    for (const auto& b : bookings_) {
        if (b.id > maxId) maxId = b.id;
    }
    return maxId + 1;
}

int DataStore::nextRecommendationId() {
    int maxId = 0;
    for (const auto& rec : recommendations_) {
        if (rec.id > maxId) maxId = rec.id;
    }
    return maxId + 1;
}

int DataStore::nextRoomId() {
    int maxId = 0;
    for (const auto& r : rooms_) {
        if (r.id > maxId) maxId = r.id;
    }
    return maxId + 1;
}

bool DataStore::initializeSeedIfMissing() {
    fs::path dir(dataDir_);
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    fs::path roomsPath = dir / "rooms.csv";
    fs::path requestsPath = dir / "requests.csv";
    fs::path bookingsPath = dir / "bookings.csv";
    fs::path adminPath = dir / "admin.txt";

    // If core files don't exist, populate initial seed
    if (!fs::exists(roomsPath)) {
        rooms_ = {
            {101, "Main Auditorium", 300, 1 | 2, true},   // Projector + Audio
            {102, "Seminar Hall A", 80, 1, true},         // Projector
            {103, "Computer Lab 1", 40, 4, true},         // Workstations
            {104, "Conference Room B", 20, 1 | 2, false}  // INACTIVE
        };
    }

    if (!fs::exists(bookingsPath)) {
        // 1 fixed booking (start: 09:00 = 540, end: 10:30 = 630 on Day 0)
        bookings_ = {
            {1, 101, 540, 630, 0}
        };
    }

    if (!fs::exists(requestsPath)) {
        // A few initial seed requests
        requests_ = {
            { {1, "Robotics", "Robotics Workshop", 0, 630, 750, 60, 1 | 2, 3, 100}, RequestStatus::Pending, "" },
            { {2, "CodingClub", "Coding Hackathon", 0, 600, 840, 35, 4, 2, 110}, RequestStatus::Pending, "" },
            { {3, "Literary", "Poetry Slam", 0, 660, 780, 45, 1, 1, 120}, RequestStatus::Pending, "" }
        };
    }

    if (!fs::exists(adminPath)) {
        adminPassword_ = "admin123";
        std::ofstream ofs(adminPath);
        ofs << "admin123\n";
    }

    return save();
}

bool DataStore::load() {
    fs::path dir(dataDir_);
    if (!fs::exists(dir)) {
        return initializeSeedIfMissing();
    }

    // 1. Load admin password
    fs::path adminPath = dir / "admin.txt";
    if (fs::exists(adminPath)) {
        std::ifstream ifs(adminPath);
        std::string line;
        if (std::getline(ifs, line)) {
            // Trim whitespace
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            if (!line.empty()) adminPassword_ = line;
        }
    } else {
        adminPassword_ = "admin123";
        std::ofstream ofs(adminPath);
        ofs << "admin123\n";
    }

    // 2. Load Rooms
    fs::path roomsPath = dir / "rooms.csv";
    if (fs::exists(roomsPath)) {
        rooms_.clear();
        std::ifstream ifs(roomsPath);
        std::string line;
        bool header = true;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            if (header) { header = false; continue; }
            auto fields = parseCsvLine(line);
            if (fields.size() >= 5) {
                Room r;
                r.id = std::stoi(fields[0]);
                r.name = fields[1];
                r.capacity = std::stoi(fields[2]);
                r.facilities = static_cast<uint32_t>(std::stoul(fields[3]));
                r.active = (fields[4] == "1" || fields[4] == "true");
                rooms_.push_back(r);
            }
        }
    } else {
        initializeSeedIfMissing();
        return true;
    }

    // 3. Load Bookings
    fs::path bookingsPath = dir / "bookings.csv";
    if (fs::exists(bookingsPath)) {
        bookings_.clear();
        std::ifstream ifs(bookingsPath);
        std::string line;
        bool header = true;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            if (header) { header = false; continue; }
            auto fields = parseCsvLine(line);
            if (fields.size() >= 4) {
                StoredBooking b;
                b.id = std::stoi(fields[0]);
                b.roomId = std::stoi(fields[1]);
                b.start = std::stoi(fields[2]);
                b.end = std::stoi(fields[3]);
                if (fields.size() >= 5) {
                    b.requestId = std::stoi(fields[4]);
                }
                bookings_.push_back(b);
            }
        }
    }

    // 4. Load Requests
    fs::path requestsPath = dir / "requests.csv";
    if (fs::exists(requestsPath)) {
        requests_.clear();
        std::ifstream ifs(requestsPath);
        std::string line;
        bool header = true;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            if (header) { header = false; continue; }
            auto fields = parseCsvLine(line);
            if (fields.size() >= 11) {
                StoredRequest sr;
                sr.req.id = std::stoi(fields[0]);
                sr.req.clubId = fields[1];
                sr.req.title = fields[2];
                sr.req.date = std::stoi(fields[3]);
                sr.req.start = std::stoi(fields[4]);
                sr.req.end = std::stoi(fields[5]);
                sr.req.attendance = std::stoi(fields[6]);
                sr.req.facilities = static_cast<uint32_t>(std::stoul(fields[7]));
                sr.req.tier = std::stoi(fields[8]);
                sr.req.submittedAt = std::stoll(fields[9]);
                sr.status = stringToRequestStatus(fields[10]);
                if (fields.size() >= 12) {
                    sr.unallocatedReason = fields[11];
                }
                requests_.push_back(sr);
            }
        }
    }

    // 5. Load Recommendations
    fs::path recsPath = dir / "recommendations.csv";
    if (fs::exists(recsPath)) {
        recommendations_.clear();
        std::ifstream ifs(recsPath);
        std::string line;
        bool header = true;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            if (header) { header = false; continue; }
            auto fields = parseCsvLine(line);
            if (fields.size() >= 5) {
                Recommendation rec;
                rec.id = std::stoi(fields[0]);
                rec.requestId = std::stoi(fields[1]);
                rec.roomId = std::stoi(fields[2]);
                rec.algorithmName = fields[3];
                rec.runId = std::stoi(fields[4]);
                if (rec.runId > lastRunId_) lastRunId_ = rec.runId;
                recommendations_.push_back(rec);
            }
        }
    }

    // 6. Load Decisions
    fs::path decsPath = dir / "decisions.csv";
    if (fs::exists(decsPath)) {
        decisions_.clear();
        std::ifstream ifs(decsPath);
        std::string line;
        bool header = true;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            if (header) { header = false; continue; }
            auto fields = parseCsvLine(line);
            if (fields.size() >= 7) {
                DecisionLog d;
                d.who = fields[0];
                d.when = fields[1];
                d.action = fields[2];
                d.requestId = std::stoi(fields[3]);
                d.roomId = std::stoi(fields[4]);
                d.reason = fields[5];
                d.details = fields[6];
                decisions_.push_back(d);
            }
        }
    }

    return true;
}

bool DataStore::save() {
    fs::path dir(dataDir_);
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    // 1. Save admin.txt
    {
        std::ofstream ofs(dir / "admin.txt");
        if (!ofs.is_open()) return false;
        ofs << adminPassword_ << "\n";
    }

    // 2. Save rooms.csv
    {
        std::ofstream ofs(dir / "rooms.csv");
        if (!ofs.is_open()) return false;
        ofs << "id,name,capacity,facilities,active\n";
        for (const auto& r : rooms_) {
            ofs << r.id << ","
                << escapeCsv(r.name) << ","
                << r.capacity << ","
                << r.facilities << ","
                << (r.active ? "1" : "0") << "\n";
        }
    }

    // 3. Save requests.csv
    {
        std::ofstream ofs(dir / "requests.csv");
        if (!ofs.is_open()) return false;
        ofs << "id,clubId,title,date,start,end,attendance,facilities,tier,submittedAt,status,unallocatedReason\n";
        for (const auto& sr : requests_) {
            ofs << sr.req.id << ","
                << escapeCsv(sr.req.clubId) << ","
                << escapeCsv(sr.req.title) << ","
                << sr.req.date << ","
                << sr.req.start << ","
                << sr.req.end << ","
                << sr.req.attendance << ","
                << sr.req.facilities << ","
                << sr.req.tier << ","
                << sr.req.submittedAt << ","
                << requestStatusToString(sr.status) << ","
                << escapeCsv(sr.unallocatedReason) << "\n";
        }
    }

    // 4. Save bookings.csv
    {
        std::ofstream ofs(dir / "bookings.csv");
        if (!ofs.is_open()) return false;
        ofs << "id,roomId,start,end,requestId\n";
        for (const auto& b : bookings_) {
            ofs << b.id << ","
                << b.roomId << ","
                << b.start << ","
                << b.end << ","
                << b.requestId << "\n";
        }
    }

    // 5. Save recommendations.csv
    {
        std::ofstream ofs(dir / "recommendations.csv");
        if (!ofs.is_open()) return false;
        ofs << "id,requestId,roomId,algorithmName,runId\n";
        for (const auto& rec : recommendations_) {
            ofs << rec.id << ","
                << rec.requestId << ","
                << rec.roomId << ","
                << escapeCsv(rec.algorithmName) << ","
                << rec.runId << "\n";
        }
    }

    // 6. Save decisions.csv
    {
        std::ofstream ofs(dir / "decisions.csv");
        if (!ofs.is_open()) return false;
        ofs << "who,when,action,requestId,roomId,reason,details\n";
        for (const auto& d : decisions_) {
            ofs << escapeCsv(d.who) << ","
                << escapeCsv(d.when) << ","
                << escapeCsv(d.action) << ","
                << d.requestId << ","
                << d.roomId << ","
                << escapeCsv(d.reason) << ","
                << escapeCsv(d.details) << "\n";
        }
    }

    return true;
}
