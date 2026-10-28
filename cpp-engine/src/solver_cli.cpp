#include "types.hpp"
#include "algorithms.hpp"
#include "validator.hpp"
#include "interval.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <cctype>
#include <iomanip>

namespace solver_cli {

// -------------------------------------------------------------
// JSON String Escaping (RFC 8259 compliant)
// -------------------------------------------------------------
std::string escapeJson(const std::string& str) {
    std::ostringstream oss;
    for (unsigned char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

std::string formatErrorJson(const std::string& errorMsg) {
    return "{\"ok\":false,\"error\":\"" + escapeJson(errorMsg) + "\"}";
}

// -------------------------------------------------------------
// String splitting & numeric parsing helpers
// -------------------------------------------------------------
std::vector<std::string> splitPipes(const std::string& line) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (true) {
        size_t pos = line.find('|', start);
        if (pos == std::string::npos) {
            tokens.push_back(line.substr(start));
            break;
        }
        tokens.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return tokens;
}

std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

bool parseInt(const std::string& s, int& outVal) {
    std::string t = trim(s);
    if (t.empty()) return false;
    size_t idx = 0;
    if (t[0] == '-' || t[0] == '+') idx = 1;
    if (idx >= t.size()) return false;
    for (; idx < t.size(); ++idx) {
        if (!std::isdigit(static_cast<unsigned char>(t[idx]))) return false;
    }
    try {
        outVal = std::stoi(t);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseUInt32(const std::string& s, uint32_t& outVal) {
    std::string t = trim(s);
    if (t.empty()) return false;
    for (char c : t) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    try {
        unsigned long val = std::stoul(t);
        outVal = static_cast<uint32_t>(val);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseInt64(const std::string& s, int64_t& outVal) {
    std::string t = trim(s);
    if (t.empty()) return false;
    size_t idx = 0;
    if (t[0] == '-' || t[0] == '+') idx = 1;
    if (idx >= t.size()) return false;
    for (; idx < t.size(); ++idx) {
        if (!std::isdigit(static_cast<unsigned char>(t[idx]))) return false;
    }
    try {
        outVal = std::stoll(t);
        return true;
    } catch (...) {
        return false;
    }
}

// -------------------------------------------------------------
// Input File Data Representation
// -------------------------------------------------------------
struct SolverInput {
    Config config;
    std::vector<Room> rooms;
    std::vector<Request> requests;
    std::vector<Booking> fixedBookings;
    std::vector<Assignment> assignments;
};

// Returns empty string on success, or error message on failure
std::string parseInputFile(const std::string& filePath, SolverInput& out) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return "Cannot open input file: " + filePath;
    }

    std::string rawLine;
    int lineNum = 0;

    // Use default Phase 1 config
    out.config.facilityCheckEnabled = true;
    out.config.maxExhaustiveRequests = 12;

    struct PrefRecord {
        int requestId = 0;
        int roomId = 0;
        int mode = 0;
    };
    std::vector<PrefRecord> parsedPrefs;

    while (std::getline(ifs, rawLine)) {
        lineNum++;
        std::string line = trim(rawLine);
        if (lineNum == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = trim(line.substr(3));
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto tokens = splitPipes(line);
        if (tokens.empty()) continue;

        const std::string& recordType = tokens[0];

        if (recordType == "CONFIG") {
            if (tokens.size() != 3) {
                return "Line " + std::to_string(lineNum) + ": CONFIG record must have exactly 3 fields (CONFIG|facilityCheck|maxExhaustiveRequests)";
            }
            int facCheck = 0;
            if (!parseInt(tokens[1], facCheck) || (facCheck != 0 && facCheck != 1)) {
                return "Line " + std::to_string(lineNum) + ": facilityCheck must be 0 or 1";
            }
            int maxReqs = 0;
            if (!parseInt(tokens[2], maxReqs) || maxReqs < 0) {
                return "Line " + std::to_string(lineNum) + ": maxExhaustiveRequests must be a non-negative integer";
            }
            out.config.facilityCheckEnabled = (facCheck == 1);
            out.config.maxExhaustiveRequests = maxReqs;

        } else if (recordType == "ROOM") {
            if (tokens.size() > 6) {
                return "Line " + std::to_string(lineNum) + ": ROOM record has too many fields (room name must not contain '|')";
            }
            if (tokens.size() < 6) {
                return "Line " + std::to_string(lineNum) + ": ROOM record must have exactly 6 fields (ROOM|id|name|capacity|facilityMask|active)";
            }
            int id = 0;
            if (!parseInt(tokens[1], id)) {
                return "Line " + std::to_string(lineNum) + ": ROOM id must be an integer";
            }
            std::string name = tokens[2];
            if (name.empty()) {
                return "Line " + std::to_string(lineNum) + ": ROOM name cannot be empty";
            }
            int cap = 0;
            if (!parseInt(tokens[3], cap) || cap < 0) {
                return "Line " + std::to_string(lineNum) + ": ROOM capacity must be a non-negative integer";
            }
            uint32_t fac = 0;
            if (!parseUInt32(tokens[4], fac)) {
                return "Line " + std::to_string(lineNum) + ": ROOM facilityMask must be a non-negative integer";
            }
            int active = 0;
            if (!parseInt(tokens[5], active) || (active != 0 && active != 1)) {
                return "Line " + std::to_string(lineNum) + ": ROOM active must be 0 or 1";
            }

            Room r;
            r.id = id;
            r.name = name;
            r.capacity = cap;
            r.facilities = fac;
            r.active = (active == 1);
            out.rooms.push_back(r);

        } else if (recordType == "REQ") {
            if (tokens.size() != 10) {
                return "Line " + std::to_string(lineNum) + ": REQ record must have exactly 10 fields (REQ|id|clubId|date|start|end|attendance|facilityMask|tier|submittedAt)";
            }
            int id = 0;
            if (!parseInt(tokens[1], id)) {
                return "Line " + std::to_string(lineNum) + ": REQ id must be an integer";
            }
            std::string clubId = tokens[2];
            int date = 0;
            if (!parseInt(tokens[3], date) || date < 0) {
                return "Line " + std::to_string(lineNum) + ": REQ date must be a non-negative integer";
            }
            int start = 0;
            if (!parseInt(tokens[4], start)) {
                return "Line " + std::to_string(lineNum) + ": REQ start must be an integer";
            }
            int end = 0;
            if (!parseInt(tokens[5], end)) {
                return "Line " + std::to_string(lineNum) + ": REQ end must be an integer";
            }
            int attendance = 0;
            if (!parseInt(tokens[6], attendance) || attendance < 0) {
                return "Line " + std::to_string(lineNum) + ": REQ attendance must be a non-negative integer";
            }
            uint32_t fac = 0;
            if (!parseUInt32(tokens[7], fac)) {
                return "Line " + std::to_string(lineNum) + ": REQ facilityMask must be a non-negative integer";
            }
            int tier = 0;
            if (!parseInt(tokens[8], tier) || tier < 1 || tier > 4) {
                return "Line " + std::to_string(lineNum) + ": REQ tier must be in range 1..4";
            }
            int64_t submittedAt = 0;
            if (!parseInt64(tokens[9], submittedAt)) {
                return "Line " + std::to_string(lineNum) + ": REQ submittedAt must be an integer timestamp";
            }

            Request req;
            req.id = id;
            req.clubId = clubId;
            req.title = "Req " + std::to_string(id);
            req.date = date;
            req.start = start;
            req.end = end;
            req.attendance = attendance;
            req.facilities = fac;
            req.tier = tier;
            req.submittedAt = submittedAt;
            out.requests.push_back(req);

        } else if (recordType == "BOOKING") {
            if (tokens.size() != 5) {
                return "Line " + std::to_string(lineNum) + ": BOOKING record must have exactly 5 fields (BOOKING|id|roomId|start|end)";
            }
            int id = 0;
            if (!parseInt(tokens[1], id)) {
                return "Line " + std::to_string(lineNum) + ": BOOKING id must be an integer";
            }
            int roomId = 0;
            if (!parseInt(tokens[2], roomId)) {
                return "Line " + std::to_string(lineNum) + ": BOOKING roomId must be an integer";
            }
            int start = 0;
            if (!parseInt(tokens[3], start)) {
                return "Line " + std::to_string(lineNum) + ": BOOKING start must be an integer";
            }
            int end = 0;
            if (!parseInt(tokens[4], end)) {
                return "Line " + std::to_string(lineNum) + ": BOOKING end must be an integer";
            }

            Booking b;
            b.id = id;
            b.roomId = roomId;
            b.start = start;
            b.end = end;
            out.fixedBookings.push_back(b);

        } else if (recordType == "ASSIGN") {
            if (tokens.size() != 3) {
                return "Line " + std::to_string(lineNum) + ": ASSIGN record must have exactly 3 fields (ASSIGN|requestId|roomId)";
            }
            int reqId = 0;
            if (!parseInt(tokens[1], reqId)) {
                return "Line " + std::to_string(lineNum) + ": ASSIGN requestId must be an integer";
            }
            int roomId = 0;
            if (!parseInt(tokens[2], roomId)) {
                return "Line " + std::to_string(lineNum) + ": ASSIGN roomId must be an integer";
            }
            out.assignments.push_back(Assignment{reqId, roomId});

        } else if (recordType == "PREF") {
            if (tokens.size() != 4) {
                return "Line " + std::to_string(lineNum) + ": PREF record must have exactly 4 fields (PREF|requestId|roomId|mode)";
            }
            int reqId = 0;
            if (!parseInt(tokens[1], reqId)) {
                return "Line " + std::to_string(lineNum) + ": PREF requestId must be an integer";
            }
            int roomId = 0;
            if (!parseInt(tokens[2], roomId)) {
                return "Line " + std::to_string(lineNum) + ": PREF roomId must be an integer";
            }
            int mode = 0;
            if (!parseInt(tokens[3], mode) || (mode != 0 && mode != 1)) {
                return "Line " + std::to_string(lineNum) + ": PREF mode must be 0 (flexible) or 1 (strict)";
            }
            parsedPrefs.push_back({reqId, roomId, mode});

        } else {
            return "Line " + std::to_string(lineNum) + ": Unknown record type '" + recordType + "'";
        }
    }

    // Apply and validate PREF records
    if (!parsedPrefs.empty()) {
        std::unordered_map<int, size_t> reqMap;
        for (size_t i = 0; i < out.requests.size(); ++i) {
            reqMap[out.requests[i].id] = i;
        }
        for (const auto& pref : parsedPrefs) {
            auto it = reqMap.find(pref.requestId);
            if (it == reqMap.end()) {
                return "Unknown request id in PREF: " + std::to_string(pref.requestId);
            }
            out.requests[it->second].preferredRoomId = pref.roomId;
            out.requests[it->second].roomMode = (pref.mode == 1) ? PreferenceMode::Strict : PreferenceMode::Flexible;
        }
    }

    // Verify room IDs referenced in ASSIGN lines exist in ROOM records
    if (!out.assignments.empty()) {
        std::unordered_set<int> roomIds;
        for (const auto& r : out.rooms) {
            roomIds.insert(r.id);
        }
        for (const auto& asgn : out.assignments) {
            if (roomIds.find(asgn.roomId) == roomIds.end()) {
                return "Unknown room id in ASSIGN: " + std::to_string(asgn.roomId);
            }
        }
    }

    return "";
}

// -------------------------------------------------------------
// JSON Builders
// -------------------------------------------------------------
std::string buildResultJsonObject(
    const Result& res,
    const std::vector<std::string>& violations,
    const std::vector<Request>& requests
) {
    std::unordered_map<int, const Request*> reqLookup;
    for (const auto& r : requests) {
        reqLookup[r.id] = &r;
    }

    std::ostringstream oss;
    oss << "{\"algorithm\":\"" << escapeJson(res.algorithmName) << "\",\"assignments\":[";
    for (size_t i = 0; i < res.assignments.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& asgn = res.assignments[i];
        oss << "{\"requestId\":" << asgn.requestId
            << ",\"roomId\":" << asgn.roomId;
        auto it = reqLookup.find(asgn.requestId);
        if (it != reqLookup.end() && it->second->preferredRoomId != 0) {
            if (asgn.roomId == it->second->preferredRoomId) {
                oss << ",\"preferenceSatisfied\":true";
            } else {
                oss << ",\"preferenceSatisfied\":false";
            }
        } else {
            oss << ",\"preferenceSatisfied\":null";
        }
        oss << "}";
    }
    oss << "],\"unallocated\":[";
    for (size_t i = 0; i < res.unallocated.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"requestId\":" << res.unallocated[i].requestId
            << ",\"reason\":\"" << escapeJson(unallocatedReasonToString(res.unallocated[i].reason)) << "\"}";
    }
    oss << "],\"totalWeight\":" << res.totalWeight
        << ",\"allocatedCount\":" << res.allocatedCount
        << ",\"satisfiedPreferences\":" << res.satisfiedPreferences
        << ",\"runtimeMs\":" << std::fixed << std::setprecision(2) << res.runtimeMs
        << ",\"violations\":[";
    for (size_t i = 0; i < violations.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << escapeJson(violations[i]) << "\"";
    }
    oss << "],\"warnings\":[";
    for (size_t i = 0; i < res.warnings.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << escapeJson(res.warnings[i]) << "\"";
    }
    oss << "]}";
    return oss.str();
}

std::string buildAllocateJson(
    const Result& res,
    const std::vector<std::string>& violations,
    const std::vector<Request>& requests
) {
    std::string inner = buildResultJsonObject(res, violations, requests);
    // Replace opening '{' with '{"ok":true,'
    if (!inner.empty() && inner.front() == '{') {
        return "{\"ok\":true," + inner.substr(1);
    }
    return "{\"ok\":true}";
}

std::string buildCompareJson(
    const std::vector<std::pair<Result, std::vector<std::string>>>& results,
    const std::vector<Request>& requests
) {
    std::ostringstream oss;
    oss << "{\"ok\":true,\"rows\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i > 0) oss << ",";
        oss << buildResultJsonObject(results[i].first, results[i].second, requests);
    }
    oss << "]}";
    return oss.str();
}

std::string buildValidateJson(const std::vector<std::string>& violations) {
    std::ostringstream oss;
    bool valid = violations.empty();
    oss << "{\"ok\":true,\"valid\":" << (valid ? "true" : "false") << ",\"violations\":[";
    for (size_t i = 0; i < violations.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << escapeJson(violations[i]) << "\"";
    }
    oss << "]}";
    return oss.str();
}

// -------------------------------------------------------------
// Core Command Execution Engine
// -------------------------------------------------------------
int runSolver(int argc, char* argv[], std::ostream& out) {
    if (argc < 2) {
        out << formatErrorJson("Usage: solver.exe <allocate <A|B|C>|compare|validate> <inputFile>") << "\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "allocate") {
        if (argc < 4) {
            out << formatErrorJson("Usage: solver.exe allocate <A|B|C> <inputFile>") << "\n";
            return 1;
        }
        std::string algChoice = argv[2];
        std::string inputFile = argv[3];

        if (algChoice != "A" && algChoice != "B" && algChoice != "C" && algChoice != "auto") {
            out << formatErrorJson("Unknown algorithm: " + algChoice + ". Valid choices: A, B, C, auto") << "\n";
            return 1;
        }

        SolverInput input;
        std::string err = parseInputFile(inputFile, input);
        if (!err.empty()) {
            out << formatErrorJson(err) << "\n";
            return 1;
        }

        Result res;
        if (algChoice == "A") {
            res = priority_greedy::run(input.requests, input.rooms, input.fixedBookings, input.config);
        } else if (algChoice == "B") {
            res = earliest_finish::run(input.requests, input.rooms, input.fixedBookings, input.config);
        } else if (algChoice == "C") {
            res = exhaustive::run(input.requests, input.rooms, input.fixedBookings, input.config);
        } else {
            auto resA = priority_greedy::run(input.requests, input.rooms, input.fixedBookings, input.config);
            auto resB = earliest_finish::run(input.requests, input.rooms, input.fixedBookings, input.config);
            std::vector<Result> candidates = {resA, resB};
            if (static_cast<int>(input.requests.size()) <= input.config.maxExhaustiveRequests) {
                candidates.push_back(exhaustive::run(input.requests, input.rooms, input.fixedBookings, input.config));
            }
            res = candidates[0];
            for (size_t i = 1; i < candidates.size(); ++i) {
                const auto& c = candidates[i];
                if (c.totalWeight > res.totalWeight) {
                    res = c;
                } else if (c.totalWeight == res.totalWeight) {
                    if (c.allocatedCount > res.allocatedCount) {
                        res = c;
                    } else if (c.allocatedCount == res.allocatedCount) {
                        if (c.satisfiedPreferences > res.satisfiedPreferences) {
                            res = c;
                        }
                    }
                }
            }
        }

        // Run independent validator on the algorithm's output
        auto violations = validate(input.requests, input.rooms, input.fixedBookings, res.assignments, input.config);

        out << buildAllocateJson(res, violations, input.requests) << "\n";
        return 0;

    } else if (command == "auto") {
        if (argc < 3) {
            out << formatErrorJson("Usage: solver.exe auto <inputFile>") << "\n";
            return 1;
        }
        std::string inputFile = argv[2];

        SolverInput input;
        std::string err = parseInputFile(inputFile, input);
        if (!err.empty()) {
            out << formatErrorJson(err) << "\n";
            return 1;
        }

        auto resA = priority_greedy::run(input.requests, input.rooms, input.fixedBookings, input.config);
        auto resB = earliest_finish::run(input.requests, input.rooms, input.fixedBookings, input.config);
        std::vector<Result> candidates = {resA, resB};
        if (static_cast<int>(input.requests.size()) <= input.config.maxExhaustiveRequests) {
            candidates.push_back(exhaustive::run(input.requests, input.rooms, input.fixedBookings, input.config));
        }
        Result res = candidates[0];
        for (size_t i = 1; i < candidates.size(); ++i) {
            const auto& c = candidates[i];
            if (c.totalWeight > res.totalWeight) {
                res = c;
            } else if (c.totalWeight == res.totalWeight) {
                if (c.allocatedCount > res.allocatedCount) {
                    res = c;
                } else if (c.allocatedCount == res.allocatedCount) {
                    if (c.satisfiedPreferences > res.satisfiedPreferences) {
                        res = c;
                    }
                }
            }
        }

        auto violations = validate(input.requests, input.rooms, input.fixedBookings, res.assignments, input.config);
        out << buildAllocateJson(res, violations, input.requests) << "\n";
        return 0;

    } else if (command == "compare") {
        if (argc < 3) {
            out << formatErrorJson("Usage: solver.exe compare <inputFile>") << "\n";
            return 1;
        }
        std::string inputFile = argv[2];

        SolverInput input;
        std::string err = parseInputFile(inputFile, input);
        if (!err.empty()) {
            out << formatErrorJson(err) << "\n";
            return 1;
        }

        auto resA = priority_greedy::run(input.requests, input.rooms, input.fixedBookings, input.config);
        auto violsA = validate(input.requests, input.rooms, input.fixedBookings, resA.assignments, input.config);

        auto resB = earliest_finish::run(input.requests, input.rooms, input.fixedBookings, input.config);
        auto violsB = validate(input.requests, input.rooms, input.fixedBookings, resB.assignments, input.config);

        auto resC = exhaustive::run(input.requests, input.rooms, input.fixedBookings, input.config);
        auto violsC = validate(input.requests, input.rooms, input.fixedBookings, resC.assignments, input.config);

        std::vector<std::pair<Result, std::vector<std::string>>> results = {
            {resA, violsA},
            {resB, violsB},
            {resC, violsC}
        };

        out << buildCompareJson(results, input.requests) << "\n";
        return 0;

    } else if (command == "validate") {
        if (argc < 3) {
            out << formatErrorJson("Usage: solver.exe validate <inputFile>") << "\n";
            return 1;
        }
        std::string inputFile = argv[2];

        SolverInput input;
        std::string err = parseInputFile(inputFile, input);
        if (!err.empty()) {
            out << formatErrorJson(err) << "\n";
            return 1;
        }

        auto violations = validate(input.requests, input.rooms, input.fixedBookings, input.assignments, input.config);
        out << buildValidateJson(violations) << "\n";
        return 0;

    } else {
        out << formatErrorJson("Unknown command: " + command + ". Valid commands: allocate <A|B|C> <file>, compare <file>, validate <file>") << "\n";
        return 1;
    }
}

} // namespace solver_cli

#ifndef SOLVER_CLI_NO_MAIN
int main(int argc, char* argv[]) {
    return solver_cli::runSolver(argc, argv, std::cout);
}
#endif
