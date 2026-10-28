# Campus Resource Allocation Optimizer - Phase 3a Solver CLI

A non-interactive, high-performance command-line solver binary designed to be launched as a child process by a Java Spring Boot backend service.

Built with **C++17** and the standard library only. It reuses the Phase 1 optimization algorithms and independent validator with zero logic re-implementation.

---

## Command Interface

```cmd
# Run a specific allocation algorithm (A: Priority Greedy, B: Earliest Finish Time Greedy, C: Exhaustive Search)
solver.exe allocate <A|B|C> <inputFile>

# Run all three algorithms on the same input for comparison
solver.exe compare <inputFile>

# Validate an assignment schedule against rooms and fixed bookings
solver.exe validate <inputFile>
```

- **Exit Code**: `0` on success, non-zero on error or validation failure.
- **Output**: Pure JSON to `stdout`. Nothing is ever printed to `stderr`.

---

## Input File Format Specification

The input file is UTF-8 encoded with one record per line. Fields are delimited by the pipe character `|`.
Blank lines and lines beginning with `#` are ignored.

```
CONFIG|facilityCheck(0 or 1)|maxExhaustiveRequests
ROOM|id|name|capacity|facilityMask|active(0 or 1)
REQ|id|clubId|date|start|end|attendance|facilityMask|tier|submittedAt
BOOKING|id|roomId|start|end
ASSIGN|requestId|roomId
PREF|requestId|roomId|mode(0 or 1)
```

### Date and Time Field Representation

The numeric time fields strictly mirror Phase 1's integer representation (`types.hpp`):
- **Base Unit**: Time is measured in integer minutes since start of Day 0.
- **Half-Open Intervals**: Intervals are half-open `[start, end)`. Consecutive intervals `[540, 660)` (09:00 - 11:00) and `[660, 780)` (11:00 - 13:00) do not overlap.
- **REQ Record**:
  - `date`: Integer day index ($0 = \text{Day 0}, 1 = \text{Day 1}, \dots$).
  - `start` / `end`: Minute of the day ($0 \dots 1439$).
  - *Example*: An event on Day 0 from 09:00 to 11:30 is specified as `date = 0`, `start = 540` ($9 \times 60$), `end = 690` ($11 \times 60 + 30$). Absolute range: $[0 \times 1440 + 540, 0 \times 1440 + 690) = [540, 690)$.
- **BOOKING Record**:
  - `start` / `end`: Absolute minutes since the start of Day 0 (`date * 1440 + minute_of_day`).
  - *Example*: A fixed booking on Day 1 from 09:00 to 10:30 is specified as `start = 1980` ($1 \times 1440 + 540$) and `end = 2070` ($1 \times 1440 + 630$).

### Record Format Details

| Record | Field 1 | Field 2 | Field 3 | Field 4 | Field 5 | Field 6 | Field 7 | Field 8 | Field 9 | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `CONFIG` | `facilityCheck` (0 or 1) | `maxExhaustiveRequests` (int) | | | | | | | | Optional. Defaults: `1\|12` |
| `ROOM` | `id` (int) | `name` (string) | `capacity` (int) | `facilityMask` (uint) | `active` (0 or 1) | | | | | `name` must not contain `\|` |
| `REQ` | `id` (int) | `clubId` (string) | `date` (int) | `start` (min) | `end` (min) | `attendance` (int) | `facilityMask` (uint) | `tier` (1..4) | `submittedAt` (int64) | Tiers 1..4 map to weights 1, 10, 100, 1000 |
| `BOOKING`| `id` (int) | `roomId` (int) | `start` (abs min)| `end` (abs min) | | | | | | Immutable confirmed bookings |
| `ASSIGN` | `requestId` (int)| `roomId` (int) | | | | | | | | Used with `validate` subcommand |
| `PREF`   | `requestId` (int)| `roomId` (int) | `mode` (0 or 1) | | | | | | | Optional room preference (0 = Flexible, 1 = Strict) |

---

## Optimization Objective

The optimization algorithms optimize a three-part **lexicographic objective**:
1. **Total Weight**: Maximize the sum of priority tier weights of allocated requests ($\text{Tier 4}=1000, \text{Tier 3}=100, \text{Tier 2}=10, \text{Tier 1}=1$).
2. **Allocated Count**: In case of equal weight, maximize the number of allocated requests.
3. **Satisfied Preferences**: In case of equal weight and count, maximize the number of satisfied room preferences (`satisfiedPreferences`).
4. **Deterministic Tie-Breaking**: Any remaining ties fall back to the fixed deterministic search/order.

### Asymptotic Complexity Analysis
Per-request room preferences do not change the asymptotic time or space complexity of the algorithms:
- **Priority Greedy & Earliest Finish Time Greedy**: Time complexity remains $\mathcal{O}(n \log n + n \cdot m \cdot b)$, where $n$ is the number of requests, $m$ is the number of rooms, and $b$ is the maximum number of intervals per room. For Flexible requests, trying the preferred room first performs one $\mathcal{O}(b)$ overlap check prior to scanning the remaining rooms, bounded by $\mathcal{O}(m \cdot b)$. For Strict requests, at most one room is tested, reducing practical work. Space complexity remains $\mathcal{O}(n + m + b)$.
- **Exhaustive Search with Pruning**: Worst-case time complexity remains $\mathcal{O}((m + 1)^n)$. Precomputing suffix sums of weights and preference counts takes $\mathcal{O}(n)$ time and space. For Strict requests, the branching factor drops from $m+1$ to at most $2$ (preferred room or unallocated), reducing the search tree size. The lexicographic branch-and-bound pruning rule preserves exact optimality while safely eliminating non-competitive subtrees.

---

## JSON Output Schemas

### 1. `allocate` Subcommand
```json
{
  "ok": true,
  "algorithm": "Priority Greedy",
  "assignments": [
    {"requestId": 1, "roomId": 101, "preferenceSatisfied": true}
  ],
  "unallocated": [
    {"requestId": 2, "reason": "TimeConflict"}
  ],
  "totalWeight": 1000,
  "allocatedCount": 1,
  "satisfiedPreferences": 1,
  "runtimeMs": 0.05,
  "violations": [],
  "warnings": []
}
```

### 2. `compare` Subcommand
```json
{
  "ok": true,
  "rows": [
    {
      "algorithm": "Priority Greedy",
      "assignments": [
        {"requestId": 1, "roomId": 101, "preferenceSatisfied": null}
      ],
      "unallocated": [...],
      "totalWeight": 10,
      "allocatedCount": 1,
      "satisfiedPreferences": 0,
      "runtimeMs": 0.03,
      "violations": [],
      "warnings": []
    },
    {
      "algorithm": "Earliest Finish Time Greedy",
      ...
    },
    {
      "algorithm": "Exhaustive Search with Pruning",
      ...
    }
  ]
}
```

### 3. `validate` Subcommand
```json
{
  "ok": true,
  "valid": true,
  "violations": []
}
```
*(If violations occur, `"valid": false` and `"violations": ["Violation message 1", ...]`)*

### 4. Error Output (Exit code $\ne 0$)
```json
{
  "ok": false,
  "error": "Line 4: ROOM capacity must be a non-negative integer"
}
```

---

## Build Instructions

Outputs are placed into `cpp-engine/build/`.

### Windows (MinGW g++)

**1. Build Solver CLI Binary:**
```cmd
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/solver_cli.cpp -o cpp-engine/build/solver.exe
```

**2. Build Solver Test Suite:**
```cmd
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include -DSOLVER_CLI_NO_MAIN cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/solver_cli.cpp cpp-engine/tests/test_solver_cli.cpp -o cpp-engine/build/test_solver_cli.exe
```

### Linux / macOS / Bash

**1. Build Solver CLI Binary:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/solver_cli.cpp -o cpp-engine/build/solver
```

**2. Build Solver Test Suite:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include -DSOLVER_CLI_NO_MAIN cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/solver_cli.cpp cpp-engine/tests/test_solver_cli.cpp -o cpp-engine/build/test_solver_cli
```

---

## Execution Examples

### Run Algorithm A (Priority Greedy)
```cmd
.\cpp-engine\build\solver.exe allocate A input.txt
```

### Run Comparison Across All Algorithms
```cmd
.\cpp-engine\build\solver.exe compare input.txt
```

### Validate Schedules
```cmd
.\cpp-engine\build\solver.exe validate input.txt
```

### Run Phase 3a Test Suite
```cmd
.\cpp-engine\build\test_solver_cli.exe
```
