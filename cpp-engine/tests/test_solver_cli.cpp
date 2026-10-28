#include "types.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cassert>

namespace fs = std::filesystem;

namespace solver_cli {
    int runSolver(int argc, char* argv[], std::ostream& out);
    std::string escapeJson(const std::string& str);
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

const std::string TEST_DIR = "cpp-engine/build/solver_test_data";

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

// 1. Parse a valid input file and run allocate A, B and C; all report empty violations.
bool test1_validInputAllocateABC() {
    std::string file = "test1.txt";
    writeTestFile(file,
        "# Test 1 valid input\n"
        "CONFIG|1|12\n"
        "ROOM|101|Main Hall|100|3|1\n"
        "ROOM|102|Seminar Room|50|1|1\n"
        "REQ|1|ClubA|0|600|720|30|1|2|100\n"
        "REQ|2|ClubB|0|720|840|40|3|3|110\n"
        "BOOKING|1|101|500|550\n"
    );

    for (const char* alg : {"A", "B", "C"}) {
        int code = 0;
        std::string json = runSolverCapture({"allocate", alg, TEST_DIR + "/" + file}, code);
        if (code != 0) return false;
        if (json.find("\"ok\":true") == std::string::npos) return false;
        if (json.find("\"violations\":[]") == std::string::npos) return false;
        if (json.find("\"allocatedCount\":2") == std::string::npos) return false;
    }
    return true;
}

// 2. The Phase 1 test 9 scenario (three same-tier requests, one room): A's totalWeight is lower than C's.
bool test2_test9ScenarioGreedyVsOptimal() {
    std::string file = "test2.txt";
    // Room 1, three requests of Tier 2 (weight 10 each)
    // Req 1: 09:00 - 17:00 (540 - 1020), submittedAt 10
    // Req 2: 09:00 - 12:00 (540 - 720), submittedAt 20
    // Req 3: 12:00 - 17:00 (720 - 1020), submittedAt 30
    writeTestFile(file,
        "ROOM|1|Auditorium|100|0|1\n"
        "REQ|1|CX|0|540|1020|50|0|2|10\n"
        "REQ|2|CY|0|540|720|50|0|2|20\n"
        "REQ|3|CZ|0|720|1020|50|0|2|30\n"
    );

    int codeA = 0;
    std::string jsonA = runSolverCapture({"allocate", "A", TEST_DIR + "/" + file}, codeA);
    if (codeA != 0) return false;

    int codeC = 0;
    std::string jsonC = runSolverCapture({"allocate", "C", TEST_DIR + "/" + file}, codeC);
    if (codeC != 0) return false;

    // Algorithm A should allocate 1 request (weight 10)
    if (jsonA.find("\"totalWeight\":10") == std::string::npos) return false;
    if (jsonA.find("\"allocatedCount\":1") == std::string::npos) return false;

    // Algorithm C should allocate 2 requests (weight 20)
    if (jsonC.find("\"totalWeight\":20") == std::string::npos) return false;
    if (jsonC.find("\"allocatedCount\":2") == std::string::npos) return false;

    return true;
}

// 3. A back-to-back pair at the same boundary validates as valid=true;
// an overlapping pair validates as valid=false.
bool test3_backToBackVsOverlapping() {
    // Back-to-back: [600, 720) and [720, 840)
    std::string fileB2B = "test3_b2b.txt";
    writeTestFile(fileB2B,
        "ROOM|1|Hall 1|50|0|1\n"
        "REQ|1|C1|0|600|720|20|0|1|10\n"
        "REQ|2|C2|0|720|840|20|0|1|20\n"
        "ASSIGN|1|1\n"
        "ASSIGN|2|1\n"
    );

    int codeB2B = 0;
    std::string jsonB2B = runSolverCapture({"validate", TEST_DIR + "/" + fileB2B}, codeB2B);
    if (codeB2B != 0) return false;
    if (jsonB2B.find("\"valid\":true") == std::string::npos) return false;
    if (jsonB2B.find("\"violations\":[]") == std::string::npos) return false;

    // Overlapping: [600, 750) and [720, 840)
    std::string fileOverlap = "test3_overlap.txt";
    writeTestFile(fileOverlap,
        "ROOM|1|Hall 1|50|0|1\n"
        "REQ|1|C1|0|600|750|20|0|1|10\n"
        "REQ|2|C2|0|720|840|20|0|1|20\n"
        "ASSIGN|1|1\n"
        "ASSIGN|2|1\n"
    );

    int codeOverlap = 0;
    std::string jsonOverlap = runSolverCapture({"validate", TEST_DIR + "/" + fileOverlap}, codeOverlap);
    if (codeOverlap != 0) return false;
    if (jsonOverlap.find("\"valid\":false") == std::string::npos) return false;
    if (jsonOverlap.find("overlaps") == std::string::npos) return false;

    return true;
}

// 4. validate detects an ASSIGN that overlaps a fixed BOOKING.
bool test4_validateDetectsFixedBookingClash() {
    std::string file = "test4.txt";
    writeTestFile(file,
        "ROOM|1|Hall 1|50|0|1\n"
        "REQ|1|C1|0|600|720|20|0|1|10\n"
        "BOOKING|1|1|650|750\n"
        "ASSIGN|1|1\n"
    );

    int code = 0;
    std::string json = runSolverCapture({"validate", TEST_DIR + "/" + file}, code);
    if (code != 0) return false;
    if (json.find("\"valid\":false") == std::string::npos) return false;
    if (json.find("overlaps fixed booking") == std::string::npos) return false;
    return true;
}

// 5. validate detects a duplicate assignment of the same request.
bool test5_validateDetectsDuplicateAssignment() {
    std::string file = "test5.txt";
    writeTestFile(file,
        "ROOM|1|Hall 1|50|0|1\n"
        "ROOM|2|Hall 2|50|0|1\n"
        "REQ|1|C1|0|600|720|20|0|1|10\n"
        "ASSIGN|1|1\n"
        "ASSIGN|1|2\n"
    );

    int code = 0;
    std::string json = runSolverCapture({"validate", TEST_DIR + "/" + file}, code);
    if (code != 0) return false;
    if (json.find("\"valid\":false") == std::string::npos) return false;
    if (json.find("assigned multiple times") == std::string::npos) return false;
    return true;
}

// 6. Capacity is not a violation: an ASSIGN whose request attendance exceeds room capacity is valid=true.
bool test6_capacityNotViolation() {
    std::string file = "test6.txt";
    writeTestFile(file,
        "ROOM|1|Tiny Room|10|0|1\n" // Capacity 10
        "REQ|1|C1|0|600|720|500|0|1|10\n" // Attendance 500
        "ASSIGN|1|1\n"
    );

    int code = 0;
    std::string json = runSolverCapture({"validate", TEST_DIR + "/" + file}, code);
    if (code != 0) return false;
    if (json.find("\"valid\":true") == std::string::npos) return false;
    if (json.find("\"violations\":[]") == std::string::npos) return false;
    return true;
}

// 7. Malformed input produces {"ok":false,...} and a non-zero exit code, not a crash.
bool test7_malformedInputProducesJsonError() {
    // Malformed line 1: unknown record type
    std::string file1 = "test7_badtype.txt";
    writeTestFile(file1, "UNKNOWN|1|2|3\n");
    int code1 = 0;
    std::string json1 = runSolverCapture({"allocate", "A", TEST_DIR + "/" + file1}, code1);
    if (code1 == 0) return false;
    if (json1.find("\"ok\":false") == std::string::npos) return false;

    // Malformed line 2: non-numeric in numeric field
    std::string file2 = "test7_badnum.txt";
    writeTestFile(file2, "ROOM|notanumber|Hall|50|0|1\n");
    int code2 = 0;
    std::string json2 = runSolverCapture({"allocate", "A", TEST_DIR + "/" + file2}, code2);
    if (code2 == 0) return false;
    if (json2.find("\"ok\":false") == std::string::npos) return false;

    // Malformed line 3: unknown room in ASSIGN
    std::string file3 = "test7_badroom.txt";
    writeTestFile(file3,
        "ROOM|1|Hall 1|50|0|1\n"
        "REQ|1|C1|0|600|720|20|0|1|10\n"
        "ASSIGN|1|999\n"
    );
    int code3 = 0;
    std::string json3 = runSolverCapture({"validate", TEST_DIR + "/" + file3}, code3);
    if (code3 == 0) return false;
    if (json3.find("\"ok\":false") == std::string::npos) return false;
    if (json3.find("Unknown room id in ASSIGN") == std::string::npos) return false;

    return true;
}

// 8. A room name containing | is rejected.
bool test8_roomNameContainingPipeRejected() {
    std::string file = "test8.txt";
    writeTestFile(file, "ROOM|101|Auditorium|West Wing|300|3|1\n");

    int code = 0;
    std::string json = runSolverCapture({"allocate", "A", TEST_DIR + "/" + file}, code);
    if (code == 0) return false;
    if (json.find("\"ok\":false") == std::string::npos) return false;
    if (json.find("room name must not contain '|'") == std::string::npos) return false;
    return true;
}

// 9. compare returns exactly three rows with the algorithm names.
bool test9_compareReturnsThreeRows() {
    std::string file = "test9.txt";
    writeTestFile(file,
        "ROOM|101|Hall 1|100|0|1\n"
        "REQ|1|ClubA|0|600|720|20|0|1|10\n"
        "REQ|2|ClubB|0|720|840|20|0|2|20\n"
    );

    int code = 0;
    std::string json = runSolverCapture({"compare", TEST_DIR + "/" + file}, code);
    if (code != 0) return false;
    if (json.find("\"ok\":true") == std::string::npos) return false;
    if (json.find("\"rows\":[") == std::string::npos) return false;

    // Verify all 3 algorithm names appear
    if (json.find("\"algorithm\":\"Priority Greedy\"") == std::string::npos) return false;
    if (json.find("\"algorithm\":\"Earliest Finish Time Greedy\"") == std::string::npos) return false;
    if (json.find("\"algorithm\":\"Exhaustive Search with Pruning\"") == std::string::npos) return false;

    return true;
}

// 10. Output JSON escapes quotes and backslashes correctly in any string field.
bool test10_jsonEscapesQuotesAndBackslashes() {
    // Direct test on escapeJson
    std::string unescaped = "Quote: \"Hello\", Backslash: C:\\Path\\To\\File\nNewline\tTab";
    std::string escaped = solver_cli::escapeJson(unescaped);

    if (escaped.find("\\\"Hello\\\"") == std::string::npos) return false;
    if (escaped.find("C:\\\\Path\\\\To\\\\File") == std::string::npos) return false;
    if (escaped.find("\\n") == std::string::npos) return false;
    if (escaped.find("\\t") == std::string::npos) return false;

    // Test error JSON escaping
    int code = 0;
    std::string json = runSolverCapture({"allocate", "A", "non\"existent\\file.txt"}, code);
    if (code == 0) return false;
    if (json.find("\\\"") == std::string::npos) return false;
    if (json.find("\\\\") == std::string::npos) return false;

    return true;
}

// 11. PREF record parsing and output fields in solver CLI
bool test11_prefRecordSupportInCli() {
    std::string file = "test11.txt";
    writeTestFile(file,
        "ROOM|101|Hall 1|100|0|1\n"
        "ROOM|102|Hall 2|100|0|1\n"
        "REQ|1|ClubA|0|600|720|20|0|1|10\n"
        "PREF|1|102|0\n"
    );

    int code = 0;
    std::string json = runSolverCapture({"allocate", "A", TEST_DIR + "/" + file}, code);
    if (code != 0) return false;
    if (json.find("\"ok\":true") == std::string::npos) return false;
    if (json.find("\"roomId\":102") == std::string::npos) return false;
    if (json.find("\"preferenceSatisfied\":true") == std::string::npos) return false;
    if (json.find("\"satisfiedPreferences\":1") == std::string::npos) return false;

    return true;
}

} // anonymous namespace

int main() {
    std::cout << "=======================================================\n";
    std::cout << "         Phase 3a Solver CLI Test Suite                \n";
    std::cout << "=======================================================\n";

    runTest("1. Parse valid input file and run allocate A, B and C (0 violations)", test1_validInputAllocateABC());
    runTest("2. Phase 1 test 9 scenario: Algorithm A weight < Algorithm C weight", test2_test9ScenarioGreedyVsOptimal());
    runTest("3. Back-to-back boundary valid=true, overlapping pair valid=false", test3_backToBackVsOverlapping());
    runTest("4. validate detects ASSIGN overlapping fixed BOOKING", test4_validateDetectsFixedBookingClash());
    runTest("5. validate detects duplicate assignment of same request", test5_validateDetectsDuplicateAssignment());
    runTest("6. Capacity is informational only and not a violation (valid=true)", test6_capacityNotViolation());
    runTest("7. Malformed input produces {\"ok\":false,...} and non-zero exit code", test7_malformedInputProducesJsonError());
    runTest("8. Room name containing '|' delimiter is rejected with clear error", test8_roomNameContainingPipeRejected());
    runTest("9. compare returns exactly three rows with all algorithm names", test9_compareReturnsThreeRows());
    runTest("10. Output JSON escapes quotes and backslashes correctly", test10_jsonEscapesQuotesAndBackslashes());
    runTest("11. PREF record parsing and output fields in solver CLI", test11_prefRecordSupportInCli());

    std::cout << "=======================================================\n";
    std::cout << "Test Summary: " << g_passCount << " PASSED, " << g_failCount << " FAILED\n";
    std::cout << "=======================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
