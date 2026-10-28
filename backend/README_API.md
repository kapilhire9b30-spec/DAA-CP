# Campus Resource Allocation Optimizer - API Documentation (Phase 3b)

This document specifies all REST API endpoints provided by the Spring Boot backend service.

- **Base URL**: `http://localhost:8080/api`
- **Data Formats**: JSON (`Content-Type: application/json; charset=UTF-8`)
- **Date Format**: ISO 8601 `yyyy-MM-dd`
- **Time Format**: 24-hour `HH:mm`
- **Authentication**: Bearer Token via header `Authorization: Bearer <token>` on all endpoints except `POST /api/auth/login`.
- **CORS**: Enabled for `http://localhost:5173`.

---

## 1. Authentication Endpoints

### 1.1 Login
- **Endpoint**: `POST /api/auth/login`
- **Access**: Public
- **Request Body**:
```json
{
  "username": "admin",
  "password": "admin123"
}
```
- **Response** (`200 OK`):
```json
{
  "token": "7b049688-6628-48b2-b13c-449e79feef86",
  "role": "ADMIN",
  "username": "admin",
  "clubName": null
}
```

### 1.2 Logout
- **Endpoint**: `POST /api/auth/logout`
- **Access**: CLUB, ADMIN
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
{
  "ok": true,
  "data": "Logged out successfully"
}
```

### 1.3 Get Current User Session
- **Endpoint**: `GET /api/me`
- **Access**: CLUB, ADMIN
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
{
  "userId": 1,
  "username": "admin",
  "role": "ADMIN",
  "clubName": ""
}
```

---

## 2. Room Management Endpoints

### 2.1 Get All Rooms
- **Endpoint**: `GET /api/rooms`
- **Access**: CLUB, ADMIN
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
[
  {
    "id": 101,
    "name": "Main Auditorium",
    "building": "Campus Center",
    "capacity": 300,
    "facilityMask": 3,
    "active": true
  },
  {
    "id": 102,
    "name": "Seminar Hall A",
    "building": "North Block",
    "capacity": 80,
    "facilityMask": 1,
    "active": true
  },
  {
    "id": 103,
    "name": "Computer Lab 1",
    "building": "Tech Wing",
    "capacity": 40,
    "facilityMask": 4,
    "active": true
  },
  {
    "id": 104,
    "name": "Conference Room B",
    "building": "Admin Block",
    "capacity": 20,
    "facilityMask": 3,
    "active": false
  }
]
```

### 2.2 Create Room
- **Endpoint**: `POST /api/rooms`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Request Body**:
```json
{
  "name": "Lecture Hall C",
  "building": "Science Block",
  "capacity": 120,
  "facilityMask": 1
}
```
- **Response** (`201 Created`):
```json
{
  "ok": true,
  "data": {
    "id": 105,
    "name": "Lecture Hall C",
    "building": "Science Block",
    "capacity": 120,
    "facilityMask": 1,
    "active": true
  }
}
```

### 2.3 Deactivate Room
- **Endpoint**: `POST /api/rooms/{id}/deactivate`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
{
  "ok": true,
  "data": {
    "id": 102,
    "name": "Seminar Hall A",
    "building": "North Block",
    "capacity": 80,
    "facilityMask": 1,
    "active": false,
    "room": {
      "id": 102,
      "name": "Seminar Hall A",
      "building": "North Block",
      "capacity": 80,
      "facilityMask": 1,
      "active": false
    },
    "affectedBookings": [
      {
        "id": 1,
        "requestId": 1,
        "title": "Robotics Workshop",
        "club": "Robotics",
        "date": "2026-10-26",
        "start": "09:00",
        "end": "10:30"
      }
    ]
  }
}
```

### 2.4 Activate Room
- **Endpoint**: `POST /api/rooms/{id}/activate`
- **Access**: ADMIN only (idempotent: activating an already-active room returns 200 OK without modifications)
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
{
  "ok": true,
  "data": {
    "id": 104,
    "name": "Conference Room B",
    "building": "Admin Block",
    "capacity": 20,
    "facilityMask": 3,
    "active": true
  }
}
```

### 2.5 Get Affected Bookings Before Deactivation
- **Endpoint**: `GET /api/rooms/{id}/affected-bookings`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
[
  {
    "id": 1,
    "requestId": 1,
    "title": "Robotics Workshop",
    "club": "Robotics",
    "date": "2026-10-26",
    "start": "09:00",
    "end": "10:30"
  }
]
```

---

## 3. Event Request Endpoints

### 3.1 Submit Event Request
- **Endpoint**: `POST /api/requests`
- **Access**: CLUB, ADMIN
- **Headers**: `Authorization: Bearer <token>`
- **Request Body**:
```json
{
  "title": "Autonomous Robotics Demo",
  "description": "Showcase of robotics lab projects",
  "date": "2026-10-26",
  "startTime": "14:00",
  "endTime": "16:00",
  "attendance": 50,
  "facilityMask": 3,
  "tier": 3
}
```
- **Response** (`201 Created`):
```json
{
  "ok": true,
  "data": {
    "id": 4,
    "userId": 2,
    "clubName": "Robotics",
    "title": "Autonomous Robotics Demo",
    "description": "Showcase of robotics lab projects",
    "date": "2026-10-26",
    "startTime": "14:00:00",
    "endTime": "16:00:00",
    "attendance": 50,
    "facilityMask": 3,
    "tier": 3,
    "submittedAt": 1793085600000,
    "status": "PENDING",
    "rejectionReason": null,
    "lastUnallocatedReason": null
  }
}
```

### 3.2 List Event Requests
- **Endpoint**: `GET /api/requests`
- **Access**: CLUB (returns only caller's requests), ADMIN (returns all requests)
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
[
  {
    "id": 1,
    "userId": 2,
    "clubName": "Robotics",
    "title": "Robotics Workshop",
    "description": "Hands-on robotics hardware workshop",
    "date": "2026-10-26",
    "startTime": "10:30:00",
    "endTime": "12:30:00",
    "attendance": 60,
    "facilityMask": 3,
    "tier": 3,
    "submittedAt": 100,
    "status": "PENDING",
    "rejectionReason": null,
    "lastUnallocatedReason": null
  }
]
```

### 3.3 Reject Event Request
- **Endpoint**: `POST /api/requests/{id}/reject`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Request Body**:
```json
{
  "reason": "Facility maintenance scheduled on that date"
}
```
- **Response** (`200 OK`):
```json
{
  "ok": true,
  "data": {
    "id": 1,
    "userId": 2,
    "clubName": "Robotics",
    "title": "Robotics Workshop",
    "description": "Hands-on robotics hardware workshop",
    "date": "2026-10-26",
    "startTime": "10:30:00",
    "endTime": "12:30:00",
    "attendance": 60,
    "facilityMask": 3,
    "tier": 3,
    "submittedAt": 100,
    "status": "REJECTED",
    "rejectionReason": "Facility maintenance scheduled on that date",
    "lastUnallocatedReason": null
  }
}
```

---

## 4. Scheduling & Workflow Endpoints

### 4.1 Run Optimization Algorithm (Allocate)
- **Endpoint**: `POST /api/allocate`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Request Body**:
```json
{
  "algorithm": "A"
}
```
*(Algorithms: "A" = Priority Greedy, "B" = Earliest Finish Time Greedy, "C" = Exhaustive Search)*
- **Response** (`200 OK`):
```json
{
  "ok": true,
  "error": null,
  "algorithm": "Priority Greedy",
  "assignments": [
    {
      "requestId": 1,
      "roomId": 101
    },
    {
      "requestId": 2,
      "roomId": 103
    },
    {
      "requestId": 3,
      "roomId": 102
    }
  ],
  "unallocated": [],
  "totalWeight": 111,
  "allocatedCount": 3,
  "runtimeMs": 0.08,
  "violations": [],
  "warnings": []
}
```

### 4.2 Compare All Algorithms
- **Endpoint**: `GET /api/compare`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
{
  "ok": true,
  "error": null,
  "rows": [
    {
      "algorithm": "Priority Greedy",
      "assignments": [
        {"requestId": 1, "roomId": 101},
        {"requestId": 2, "roomId": 103},
        {"requestId": 3, "roomId": 102}
      ],
      "unallocated": [],
      "totalWeight": 111,
      "allocatedCount": 3,
      "runtimeMs": 0.07,
      "violations": [],
      "warnings": []
    },
    {
      "algorithm": "Earliest Finish Time Greedy",
      "assignments": [
        {"requestId": 1, "roomId": 101},
        {"requestId": 2, "roomId": 103},
        {"requestId": 3, "roomId": 102}
      ],
      "unallocated": [],
      "totalWeight": 111,
      "allocatedCount": 3,
      "runtimeMs": 0.06,
      "violations": [],
      "warnings": []
    },
    {
      "algorithm": "Exhaustive Search with Pruning",
      "assignments": [
        {"requestId": 1, "roomId": 101},
        {"requestId": 2, "roomId": 103},
        {"requestId": 3, "roomId": 102}
      ],
      "unallocated": [],
      "totalWeight": 111,
      "allocatedCount": 3,
      "runtimeMs": 0.09,
      "violations": [],
      "warnings": []
    }
  ]
}
```

### 4.3 Demo Algorithm Comparison (Side-Effect Free Test 9 Scenario)
- **Endpoint**: `GET /api/demo`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`): Returns comparative results for 3 same-tier requests competing for 1 room.

### 4.4 Get Active Recommendations
- **Endpoint**: `GET /api/recommendations`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
[
  {
    "id": 1,
    "request": {
      "id": 1,
      "userId": 2,
      "clubName": "Robotics",
      "title": "Robotics Workshop",
      "description": "Hands-on robotics hardware workshop",
      "date": "2026-10-26",
      "startTime": "10:30:00",
      "endTime": "12:30:00",
      "attendance": 60,
      "facilityMask": 3,
      "tier": 3,
      "submittedAt": 100,
      "status": "RECOMMENDED",
      "rejectionReason": null,
      "lastUnallocatedReason": null
    },
    "proposedRoom": {
      "id": 101,
      "name": "Main Auditorium",
      "building": "Campus Center",
      "capacity": 300,
      "facilityMask": 3,
      "active": true
    },
    "algorithm": "Priority Greedy",
    "capacityWarning": false,
    "otherFeasibleRooms": [],
    "createdAt": "2026-10-27T04:45:00Z",
    "active": true
  }
]
```

### 4.5 Approve Recommendation
- **Endpoint**: `POST /api/recommendations/{id}/approve`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response (`200 OK` on Success)**:
```json
{
  "ok": true,
  "data": {
    "id": 2,
    "requestId": 1,
    "roomId": 101,
    "date": "2026-10-26",
    "startTime": "10:30:00",
    "endTime": "12:30:00",
    "approvedBy": "admin",
    "createdAt": "2026-10-27T04:46:12Z"
  }
}
```
- **Response (`409 Conflict` on Collision / Violation)**:
```json
{
  "ok": false,
  "error": "Schedule conflict detected: Request #1 overlaps with fixed booking on room 101"
}
```

### 4.6 Modify Proposed Room and Approve
- **Endpoint**: `POST /api/recommendations/{id}/modify`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Request Body**:
```json
{
  "roomId": 102
}
```
- **Response (`200 OK` with Informational Capacity Warning)**:
```json
{
  "ok": true,
  "booking": {
    "id": 3,
    "requestId": 1,
    "roomId": 102,
    "date": "2026-10-26",
    "startTime": "10:30:00",
    "endTime": "12:30:00",
    "approvedBy": "admin",
    "createdAt": "2026-10-27T04:47:00Z"
  },
  "warning": "Room capacity (80) is below expected attendance (100)"
}
```

---

## 5. Query Endpoints

### 5.1 Get Confirmed Bookings
- **Endpoint**: `GET /api/bookings`
- **Access**: CLUB, ADMIN
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
[
  {
    "id": 1,
    "requestId": null,
    "roomId": 101,
    "date": "2026-10-26",
    "startTime": "09:00:00",
    "endTime": "10:30:00",
    "approvedBy": "admin",
    "createdAt": "2026-10-27T04:40:00Z",
    "roomActive": true
  }
]
```

### 5.2 Get Audit Decision Logs
- **Endpoint**: `GET /api/decisions`
- **Access**: ADMIN only
- **Headers**: `Authorization: Bearer <token>`
- **Response** (`200 OK`):
```json
[
  {
    "id": 1,
    "who": "admin",
    "at": "2026-10-27T04:46:12Z",
    "action": "APPROVE",
    "requestId": 1,
    "roomId": 101,
    "reason": null,
    "details": "Approved recommendation #1 for Room Main Auditorium"
  }
]
```
