# Campus Resource Allocation Optimizer - Spring Boot Backend (Phase 3b)

A robust Spring Boot REST backend service that manages campus rooms, requests, recommendations, bookings, decision logs, and enforces role-based human-approval workflows.

All schedule allocation, multi-algorithm comparison, and schedule validation logic are delegated strictly to the tested C++ solver executable (`cpp-engine/build/solver.exe`). Java never implements overlap detection or schedule algorithms.

---

## Tech Stack & Architecture

- **Java**: 17+
- **Framework**: Spring Boot 3.2.5
- **Build Tool**: Maven (pure hand-written `pom.xml`)
- **Web & Validation**: `spring-boot-starter-web`, `spring-boot-starter-validation`
- **Persistence**: `spring-boot-starter-data-jpa`, Hibernate, Jackson
- **Database**: H2 in File Mode (`backend/data/campus_alloc.mv.db`)
- **Security / Crypto**: `spring-security-crypto` (`BCryptPasswordEncoder` ONLY, no full Spring Security)
- **Concurrency**: Service-level `ReentrantLock` + `@Transactional` database isolation
- **Testing**: JUnit 5, `@SpringBootTest`, `MockMvc`
- **Solver Bridge**: `ProcessBuilder` with argument array, 15-second timeout, OS temp file exchange, and Jackson JSON parsing.

---

## Build and Run Commands

### 1. Run All Automated Tests
```cmd
mvn -f backend/pom.xml test
```

### 2. Run Spring Boot Backend Server (Port 8080)
```cmd
mvn -f backend/pom.xml spring-boot:run
```

The server binds to port `8080`. API base URL: `http://localhost:8080/api`.

---

## Seed Data (Initialized on First Startup)

On the initial clean startup, the backend automatically seeds:
- **Admin User**: `admin` / `admin123` (role `ADMIN`, password configurable via `app.admin.password`)
- **Club Users**:
  - `robotics` / `robotics123` (role `CLUB`, clubName `Robotics`)
  - `coding` / `coding123` (role `CLUB`, clubName `CodingClub`)
  - `literary` / `literary123` (role `CLUB`, clubName `Literary`)
- **Rooms**:
  - Room 101: `Main Auditorium` (capacity 300, facilities 3 [Projector|Audio], active `true`)
  - Room 102: `Seminar Hall A` (capacity 80, facilities 1 [Projector], active `true`)
  - Room 103: `Computer Lab 1` (capacity 40, facilities 4 [Workstations], active `true`)
  - Room 104: `Conference Room B` (capacity 20, facilities 3, active `false`)
- **Confirmed Booking**:
  - Room 101 on `2026-10-26`, 09:00 - 10:30 (Fixed booking approved by `admin`)
- **Pending Requests**:
  - Request 1: `Robotics Workshop` on `2026-10-26`, 10:30 - 12:30 (tier 3, facilities 3)
  - Request 2: `Coding Hackathon` on `2026-10-26`, 10:00 - 14:00 (tier 2, facilities 4)
  - Request 3: `Poetry Slam` on `2026-10-26`, 11:00 - 13:00 (tier 1, facilities 1)

---

## Workflow Rules & State Machine

1. **Request Submission**:
   - Status starts as `PENDING`.
   - Early validation ensures required fields and `endTime > startTime`.
2. **Allocation (`POST /api/allocate`)**:
   - Resets any unreviewed recommendations and returns their requests to `PENDING`.
   - Sends pending requests, active rooms, and confirmed bookings to `solver.exe allocate <A|B|C>`.
   - Placed requests become `RECOMMENDED`; unplaced requests stay `PENDING` with reasons.
   - **Zero bookings are created.**
3. **Approval (`POST /api/recommendations/{id}/approve`)**:
   - Runs inside a `ReentrantLock` and database transaction.
   - Validates assignment with confirmed bookings using `solver.exe validate`.
   - On success: creates `Booking`, sets request to `CONFIRMED`, deactivates recommendations, logs `APPROVE` action in `DecisionLog`.
   - Safety Net: validates entire confirmed schedule using `solver.exe validate`.
4. **Modify & Approve (`POST /api/recommendations/{id}/modify`)**:
   - Re-validates assignment with the new room using `solver.exe validate`.
   - Blocks any conflict. If capacity < attendance, accepts with an informational warning flag.
5. **Rejection (`POST /api/requests/{id}/reject`)**:
   - Requires a non-blank reason. Sets status to `REJECTED`, deactivates recommendations, logs `REJECT` action.
