# Campus Resource Allocation Optimizer - C++ Engine (Phase 1)

Phase 1 implementation of the **Campus Resource Allocation Optimizer** engine, containing core optimization algorithms, an independent schedule validator, comprehensive automated test suites, and an interactive console demonstration.

Built with **C++17** and the standard library only. No external dependencies, networking, or database.

---

## Architecture and Scope

The project strictly resides in `cpp-engine/` and is partitioned as follows:
- `include/types.hpp`: Domain data models (`Request`, `Room`, `Booking`, `Assignment`, `Unallocated`, `Result`, `Config`, `UnallocatedReason` enum).
- `include/interval.hpp`: Half-open time intervals `[start, end)` with boundary-safe non-overlapping intersection tests.
- `include/validator.hpp` & `src/validator.cpp`: Independent audit validator inspecting room activity, collisions, fixed booking conflicts, and facility bitmask requirements.
- `include/algorithms.hpp`: Common interface `Result run(requests, rooms, fixedBookings, config)` shared across all algorithms.
- `src/priority_greedy.cpp`: Priority Greedy algorithm (`Algorithm A`).
- `src/earliest_finish.cpp`: Earliest Finish Time Greedy algorithm (`Algorithm B`).
- `src/exhaustive.cpp`: Branch-and-bound exhaustive search with aggressive prefix-sum pruning (`Algorithm C`).
- `src/main.cpp`: Seed demonstration and comparative benchmarking matrix.
- `tests/test_engine.cpp`: Assert test suite covering 11 critical operational specifications.

---

## Domain Rules

1. **Integer Time Representation**: Time is represented in minutes from the start of Day 0: `time = date * 1440 + minute_of_day`.
2. **Half-Open Intervals**: All time ranges are half-open `[start, end)`. Two intervals `a` and `b` overlap if and only if `a.start < b.end && b.start < a.end`. Consecutive back-to-back intervals (such as `[10:00, 12:00)` and `[12:00, 14:00)`) do not overlap.
3. **Informational Capacity**: Room capacity is purely informational and never invalidates an assignment or blocks feasibility.
4. **Facility Bitmasks**: Facilities are represented as bitwise flags. When facility checking is enabled, a room is eligible if `(request.facilities & ~room.facilities) == 0`.
5. **Active Status**: Inactive rooms are strictly excluded from allocation.
6. **Immutable Fixed Bookings**: Confirmed existing bookings cannot be moved, overridden, or deleted.
7. **Priority Tiers & Weights**: Tiers 1 through 4 map strictly to weights 1, 10, 100, 1000 respectively, ensuring higher tiers outweigh any combination of lower tiers.
8. **Lexicographic Optimization Objective**: Maximize total weight $\rightarrow$ maximize total allocated count $\rightarrow$ deterministic tie-break.

---

## Algorithms and Complexity

| Algorithm | Time Complexity | Space Complexity | Notes |
| :--- | :--- | :--- | :--- |
| **A: Priority Greedy** | $\mathcal{O}(n \log n + n \cdot m \cdot b)$ | $\mathcal{O}(n + m + b)$ | Sorts by `tier desc, date asc, submittedAt asc, id asc`. Allocates greedily to first feasible room. |
| **B: Earliest Finish Time Greedy** | $\mathcal{O}(n \log n + n \cdot m \cdot b)$ | $\mathcal{O}(n + m + b)$ | Sorts by `end asc, tier desc, id asc`. Optimal for max count only in single-room unweighted scenarios. |
| **C: Exhaustive Search with Pruning** | $\mathcal{O}((m + 1)^n)$ (worst case) | $\mathcal{O}(n + m + b)$ | Branch-and-bound search with suffix-sum pruning. Bounded by `config.maxExhaustiveRequests` (default: 12). |

*Parameters: $n$ = requests, $m$ = rooms, $b$ = maximum bookings/assignments per room.*

---

## Build Instructions

Outputs are placed into `cpp-engine/build/`. Ensure the directory exists:

```bash
mkdir -p cpp-engine/build
```

### Windows (MinGW g++ / PowerShell or CMD)

**Console Demo:**
```cmd
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/main.cpp -o cpp-engine/build/engine.exe
```

**Test Suite:**
```cmd
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/tests/test_engine.cpp -o cpp-engine/build/test_engine.exe
```

### Linux / macOS / Bash Wildcards

**Console Demo:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/*.cpp -o cpp-engine/build/engine
```

**Test Suite:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/tests/test_engine.cpp -o cpp-engine/build/test_engine
```

---

## Running

### Run the Demo
```bash
# Windows
.\cpp-engine\build\engine.exe

# Linux / macOS
./cpp-engine/build/engine
```

### Run the Tests
```bash
# Windows
.\cpp-engine\build\test_engine.exe

# Linux / macOS
./cpp-engine/build/test_engine
```
