# Campus Resource Allocation Optimizer - Phase 2 Console Application

Phase 2 builds an interactive console application featuring a human-in-the-loop approval workflow, role-based access control, decision audit logging, and file-based CSV persistence.

Built with **C++17** and the standard library only. Reuses Phase 1 optimization algorithms and independent validator without modification.

---

## Architectural Additions (Phase 2)

- `include/store.hpp` & `src/store.cpp`: CSV persistence engine for rooms, requests, confirmed bookings, proposed recommendations, audit decisions, and admin authentication.
- `include/workflow.hpp` & `src/workflow.cpp`: Testable, role-enforced business workflow functions implementing recommendation proposals, admin reviews, room modifications, approval safety nets, and rejections.
- `src/app.cpp`: Interactive terminal interface providing Club and Administrator portals.
- `tests/test_workflow.cpp`: Assert test suite covering all 12 operational workflow constraints and persistence round-trips.

---

## Workflow and Security Architecture

### Role-Based Access Control (RBAC)
- **Club Role**:
  - Can submit new requests.
  - Can view only their own club's requests, statuses, and confirmed room/time assignments.
  - Cannot approve, modify, reject, trigger algorithm allocations, or edit room inventory.
- **Admin Role**:
  - Full operational access: add/deactivate rooms, run allocation algorithms, compare algorithm performance, review recommendations, approve or modify room assignments, reject requests, and view decision audit logs.
  - Authenticated via password stored in `cpp-engine/data/admin.txt` (default demo password: `admin123`).
  - *Security Note*: The plaintext `admin.txt` file is designed strictly for local demonstration and testing purposes.

### Request Lifecycle
```
[Club Submits] --> Pending
                     |
         [Admin Runs Algorithm]
                     |
                     v
                Recommended  ----(Admin Rejects with Reason)----> Rejected
                     |
       (Admin Approves / Modifies)
                     |
                     v
                 Confirmed (Immutable Booking Created)
```

### Safety Net Validator
Every approval and room modification runs Phase 1's independent `validate()` function across the entire confirmed schedule before persisting. If any collision, inactive room reference, or facility violation occurs, the transaction is automatically rolled back.

---

## Data Persistence (`cpp-engine/data/`)

On initial launch, if missing, the directory and seed files are automatically created:
- `rooms.csv`: Room inventory, capacities, facility bitmasks, active flags.
- `requests.csv`: Club requests, date, time intervals, attendance, facility requirements, tiers, statuses, and unallocated reasons.
- `bookings.csv`: Confirmed immutable room bookings.
- `recommendations.csv`: Algorithm-generated proposals awaiting admin review.
- `decisions.csv`: Immutable audit log recording operator name, timestamp, action (`Approve`, `Reject`, `ModifyApprove`), IDs, reasons, and details.
- `admin.txt`: Administrator access password.

*Note: Automated tests store data in `cpp-engine/build/test_data/` and never touch production seed data in `cpp-engine/data/`.*

---

## Build Instructions

Outputs are placed into `cpp-engine/build/`.

### Windows (MinGW g++)

**1. Build Interactive Console Application:**
```cmd
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/store.cpp cpp-engine/src/workflow.cpp cpp-engine/src/app.cpp -o cpp-engine/build/app.exe
```

**2. Build Workflow Test Suite:**
```cmd
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/store.cpp cpp-engine/src/workflow.cpp cpp-engine/tests/test_workflow.cpp -o cpp-engine/build/test_workflow.exe
```

### Linux / macOS / Bash

**1. Build Interactive Console Application:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/store.cpp cpp-engine/src/workflow.cpp cpp-engine/src/app.cpp -o cpp-engine/build/app
```

**2. Build Workflow Test Suite:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Icpp-engine/include cpp-engine/src/validator.cpp cpp-engine/src/priority_greedy.cpp cpp-engine/src/earliest_finish.cpp cpp-engine/src/exhaustive.cpp cpp-engine/src/store.cpp cpp-engine/src/workflow.cpp cpp-engine/tests/test_workflow.cpp -o cpp-engine/build/test_workflow
```

---

## Running

### Run the Phase 2 Console App
```cmd
# Windows
.\cpp-engine\build\app.exe

# Linux / macOS
./cpp-engine/build/app
```

### Run the Phase 2 Test Suite
```cmd
# Windows
.\cpp-engine\build\test_workflow.exe

# Linux / macOS
./cpp-engine/build/test_workflow
```
