package edu.campus.alloc.service;

import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.RecommendationResponse;
import edu.campus.alloc.entity.*;
import edu.campus.alloc.repository.*;
import edu.campus.alloc.solver.SolverClient;
import edu.campus.alloc.solver.dto.*;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalTime;
import java.time.temporal.ChronoUnit;
import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

@Service
public class WorkflowService {

    private static final LocalDate BASE_DATE = LocalDate.of(2026, 1, 1);

    private final ReentrantLock lock = new ReentrantLock();

    private final SolverClient solverClient;
    private final EventRequestRepository requestRepository;
    private final RoomRepository roomRepository;
    private final RecommendationRepository recommendationRepository;
    private final AlgorithmRunRepository algorithmRunRepository;
    private final BookingRepository bookingRepository;
    private final DecisionLogRepository decisionLogRepository;

    public WorkflowService(SolverClient solverClient,
                           EventRequestRepository requestRepository,
                           RoomRepository roomRepository,
                           RecommendationRepository recommendationRepository,
                           AlgorithmRunRepository algorithmRunRepository,
                           BookingRepository bookingRepository,
                           DecisionLogRepository decisionLogRepository) {
        this.solverClient = solverClient;
        this.requestRepository = requestRepository;
        this.roomRepository = roomRepository;
        this.recommendationRepository = recommendationRepository;
        this.algorithmRunRepository = algorithmRunRepository;
        this.bookingRepository = bookingRepository;
        this.decisionLogRepository = decisionLogRepository;
    }

    public static int toDayIndex(LocalDate date) {
        return (int) ChronoUnit.DAYS.between(BASE_DATE, date);
    }

    public static int toMinuteOfDay(LocalTime time) {
        return time.getHour() * 60 + time.getMinute();
    }

    public static int toAbsoluteMinutes(LocalDate date, LocalTime time) {
        return toDayIndex(date) * 1440 + toMinuteOfDay(time);
    }

    @Transactional
    public SolverAllocateResponse allocate(String algorithm) {
        lock.lock();
        try {
            // 1. Deactivate any unreviewed recommendations and reset their requests to PENDING
            List<Recommendation> activeRecs = recommendationRepository.findByActiveTrue();
            for (Recommendation r : activeRecs) {
                r.setActive(false);
                recommendationRepository.save(r);
            }

            List<EventRequest> recRequests = requestRepository.findByStatus(RequestStatus.RECOMMENDED);
            for (EventRequest req : recRequests) {
                req.setStatus(RequestStatus.PENDING);
                requestRepository.save(req);
            }

            // 2. Fetch fresh pending requests, active rooms, and confirmed bookings
            List<EventRequest> pending = requestRepository.findByStatus(RequestStatus.PENDING);
            List<Room> activeRooms = roomRepository.findByActiveTrue();
            List<Booking> confirmedBookings = bookingRepository.findAll();

            // 3. Build solver input file content
            String inputContent = buildSolverInput(activeRooms, pending, confirmedBookings, Collections.emptyList());

            // 4. Call solver allocate
            SolverAllocateResponse response = solverClient.allocate(algorithm, inputContent);

            // 5. Store AlgorithmRun
            AlgorithmRun run = new AlgorithmRun(
                    response.getAlgorithm(),
                    Instant.now(),
                    response.getAllocatedCount(),
                    response.getTotalWeight(),
                    response.getRuntimeMs()
            );
            AlgorithmRun savedRun = algorithmRunRepository.save(run);

            // 6. Store recommendations and update request statuses
            for (AssignmentDto asgn : response.getAssignments()) {
                Recommendation rec = new Recommendation(
                        asgn.getRequestId(),
                        asgn.getRoomId(),
                        response.getAlgorithm(),
                        savedRun.getId(),
                        Instant.now(),
                        true
                );
                recommendationRepository.save(rec);

                requestRepository.findById(asgn.getRequestId()).ifPresent(req -> {
                    req.setStatus(RequestStatus.RECOMMENDED);
                    req.setLastUnallocatedReason(null);
                    requestRepository.save(req);
                });
            }

            for (UnallocatedDto unalloc : response.getUnallocated()) {
                requestRepository.findById(unalloc.getRequestId()).ifPresent(req -> {
                    req.setStatus(RequestStatus.PENDING);
                    req.setLastUnallocatedReason(unalloc.getReason());
                    requestRepository.save(req);
                });
            }

            return response;

        } finally {
            lock.unlock();
        }
    }

    public SolverCompareResponse compare() {
        // Read-only side-effect free compare call
        List<EventRequest> pending = requestRepository.findByStatus(RequestStatus.PENDING);
        List<Room> activeRooms = roomRepository.findByActiveTrue();
        List<Booking> confirmedBookings = bookingRepository.findAll();

        String inputContent = buildSolverInput(activeRooms, pending, confirmedBookings, Collections.emptyList());
        return solverClient.compare(inputContent);
    }

    public SolverCompareResponse demo() {
        // Build in-memory test 9 scenario: Room 1, three same-tier requests
        StringBuilder sb = new StringBuilder();
        sb.append("CONFIG|1|12\n");
        sb.append("ROOM|1|Auditorium|100|0|1\n");
        sb.append("REQ|1|CX|0|540|1020|50|0|2|10\n");
        sb.append("REQ|2|CY|0|540|720|50|0|2|20\n");
        sb.append("REQ|3|CZ|0|720|1020|50|0|2|30\n");

        return solverClient.compare(sb.toString());
    }

    public List<RecommendationResponse> getRecommendations() {
        List<Recommendation> activeRecs = recommendationRepository.findByActiveTrue();
        List<Room> activeRooms = roomRepository.findByActiveTrue();
        List<Booking> confirmedBookings = bookingRepository.findAll();

        List<RecommendationResponse> result = new ArrayList<>();
        for (Recommendation r : activeRecs) {
            EventRequest req = requestRepository.findById(r.getRequestId()).orElse(null);
            Room proposedRoom = roomRepository.findById(r.getRoomId()).orElse(null);
            if (req == null || proposedRoom == null) {
                continue;
            }

            boolean capacityWarning = proposedRoom.getCapacity() < req.getAttendance();

            // Find other feasible rooms strictly via solver.exe validate
            List<Room> otherFeasibleRooms = new ArrayList<>();
            for (Room candidate : activeRooms) {
                if (candidate.getId().equals(proposedRoom.getId())) {
                    continue;
                }
                // Validate request against candidate room using solver
                String validateInput = buildValidateSingleAssignmentInput(activeRooms, confirmedBookings, req, candidate.getId());
                try {
                    SolverValidateResponse valRes = solverClient.validate(validateInput);
                    if (valRes.isValid()) {
                        otherFeasibleRooms.add(candidate);
                    }
                } catch (Exception ignored) {
                }
            }

            result.add(new RecommendationResponse(
                    r.getId(),
                    req,
                    proposedRoom,
                    r.getAlgorithm(),
                    capacityWarning,
                    otherFeasibleRooms,
                    r.getCreatedAt(),
                    r.getActive()
            ));
        }

        return result;
    }

    @Transactional
    public Booking approve(Long recommendationId, UserSession session) {
        lock.lock();
        try {
            Recommendation rec = recommendationRepository.findById(recommendationId)
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Recommendation not found: " + recommendationId));

            if (!Boolean.TRUE.equals(rec.getActive())) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Recommendation is no longer active");
            }

            EventRequest request = requestRepository.findById(rec.getRequestId())
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Request not found: " + rec.getRequestId()));

            if (request.getStatus() != RequestStatus.RECOMMENDED) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Request is not in RECOMMENDED status");
            }

            if (bookingRepository.findByRequestId(request.getId()).isPresent()) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "A booking already exists for this request");
            }

            Room room = roomRepository.findById(rec.getRoomId())
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Room not found: " + rec.getRoomId()));

            // Validate proposed assignment with solver.exe using real room status
            List<Room> allRooms = roomRepository.findAll();
            List<Booking> confirmedBookings = bookingRepository.findAll();
            String validateInput = buildValidateSingleAssignmentInput(allRooms, confirmedBookings, request, room.getId());

            SolverValidateResponse valRes = solverClient.validate(validateInput);
            if (!valRes.isValid()) {
                throw new ResponseStatusException(HttpStatus.CONFLICT, "Schedule conflict detected: " + String.join("; ", valRes.getViolations()));
            }

            // Create Booking
            Booking booking = new Booking(
                    request.getId(),
                    room.getId(),
                    request.getDate(),
                    request.getStartTime(),
                    request.getEndTime(),
                    session.getUsername(),
                    Instant.now()
            );
            booking.setRoomActive(room.getActive());
            Booking savedBooking = bookingRepository.save(booking);

            // Update request
            request.setStatus(RequestStatus.CONFIRMED);
            requestRepository.save(request);

            // Deactivate all recommendations for this request
            for (Recommendation r : recommendationRepository.findByRequestId(request.getId())) {
                r.setActive(false);
                recommendationRepository.save(r);
            }

            // Log Decision
            DecisionLog decisionLog = new DecisionLog(
                    session.getUsername(),
                    Instant.now(),
                    DecisionAction.APPROVE,
                    request.getId(),
                    room.getId(),
                    null,
                    "Approved recommendation #" + recommendationId + " for Room " + room.getName()
            );
            decisionLogRepository.save(decisionLog);

            // Validate entire confirmed schedule safety net
            validateFullConfirmedSchedule();

            return savedBooking;

        } finally {
            lock.unlock();
        }
    }

    @Transactional
    public Map<String, Object> modify(Long recommendationId, Long newRoomId, UserSession session) {
        lock.lock();
        try {
            Recommendation rec = recommendationRepository.findById(recommendationId)
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Recommendation not found: " + recommendationId));

            if (!Boolean.TRUE.equals(rec.getActive())) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Recommendation is no longer active");
            }

            EventRequest request = requestRepository.findById(rec.getRequestId())
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Request not found: " + rec.getRequestId()));

            if (request.getStatus() != RequestStatus.RECOMMENDED) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Request is not in RECOMMENDED status");
            }

            if (bookingRepository.findByRequestId(request.getId()).isPresent()) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "A booking already exists for this request");
            }

            Room newRoom = roomRepository.findById(newRoomId)
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Target room not found: " + newRoomId));

            if (!Boolean.TRUE.equals(newRoom.getActive())) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Target room is inactive");
            }

            // Validate proposed assignment with new room using solver.exe
            List<Room> allRooms = roomRepository.findAll();
            List<Booking> confirmedBookings = bookingRepository.findAll();
            String validateInput = buildValidateSingleAssignmentInput(allRooms, confirmedBookings, request, newRoom.getId());

            SolverValidateResponse valRes = solverClient.validate(validateInput);
            if (!valRes.isValid()) {
                throw new ResponseStatusException(HttpStatus.CONFLICT, "Schedule conflict detected: " + String.join("; ", valRes.getViolations()));
            }

            // Check capacity (informational warning only)
            String warning = null;
            if (newRoom.getCapacity() < request.getAttendance()) {
                warning = "Room capacity (" + newRoom.getCapacity() + ") is below expected attendance (" + request.getAttendance() + ")";
            }

            // Create Booking
            Booking booking = new Booking(
                    request.getId(),
                    newRoom.getId(),
                    request.getDate(),
                    request.getStartTime(),
                    request.getEndTime(),
                    session.getUsername(),
                    Instant.now()
            );
            booking.setRoomActive(newRoom.getActive());
            Booking savedBooking = bookingRepository.save(booking);

            // Update request
            request.setStatus(RequestStatus.CONFIRMED);
            requestRepository.save(request);

            // Deactivate recommendations for this request
            for (Recommendation r : recommendationRepository.findByRequestId(request.getId())) {
                r.setActive(false);
                recommendationRepository.save(r);
            }

            // Log Decision
            DecisionLog decisionLog = new DecisionLog(
                    session.getUsername(),
                    Instant.now(),
                    DecisionAction.MODIFY_APPROVE,
                    request.getId(),
                    newRoom.getId(),
                    null,
                    "Modified room from " + rec.getRoomId() + " to " + newRoomId + " and approved." + (warning != null ? " Warning: " + warning : "")
            );
            decisionLogRepository.save(decisionLog);

            // Validate entire confirmed schedule safety net
            validateFullConfirmedSchedule();

            Map<String, Object> responseData = new HashMap<>();
            responseData.put("ok", true);
            responseData.put("booking", savedBooking);
            if (warning != null) {
                responseData.put("warning", warning);
            }
            return responseData;

        } finally {
            lock.unlock();
        }
    }

    @Transactional
    public EventRequest reject(Long requestId, String reason, UserSession session) {
        lock.lock();
        try {
            if (reason == null || reason.trim().isEmpty()) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Rejection reason is required");
            }

            EventRequest request = requestRepository.findById(requestId)
                    .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Request not found: " + requestId));

            request.setStatus(RequestStatus.REJECTED);
            request.setRejectionReason(reason.trim());
            EventRequest savedRequest = requestRepository.save(request);

            // Deactivate recommendations
            for (Recommendation r : recommendationRepository.findByRequestId(requestId)) {
                r.setActive(false);
                recommendationRepository.save(r);
            }

            // Log decision
            DecisionLog decisionLog = new DecisionLog(
                    session.getUsername(),
                    Instant.now(),
                    DecisionAction.REJECT,
                    requestId,
                    null,
                    reason.trim(),
                    "Request rejected"
            );
            decisionLogRepository.save(decisionLog);

            return savedRequest;

        } finally {
            lock.unlock();
        }
    }

    public List<Booking> getBookings() {
        List<Booking> bookings = bookingRepository.findAll();
        Map<Long, Boolean> roomActiveMap = roomRepository.findAll().stream()
                .collect(java.util.stream.Collectors.toMap(Room::getId, Room::getActive, (a, b) -> a));
        for (Booking b : bookings) {
            b.setRoomActive(roomActiveMap.getOrDefault(b.getRoomId(), false));
        }
        return bookings;
    }

    public List<DecisionLog> getDecisions() {
        return decisionLogRepository.findAllByOrderByAtDesc();
    }

    private void validateFullConfirmedSchedule() {
        List<Room> rooms = roomRepository.findAll();
        List<Booking> bookings = bookingRepository.findAll();

        Map<Long, Room> roomMap = rooms.stream()
                .collect(java.util.stream.Collectors.toMap(Room::getId, r -> r, (a, b) -> a));
        for (Booking b : bookings) {
            Room r = roomMap.get(b.getRoomId());
            if (r != null && !Boolean.TRUE.equals(r.getActive())) {
                System.out.println("Warning: Confirmed booking #" + b.getId() + " uses inactive room " + r.getName() + " (#" + r.getId() + ")");
            }
        }

        StringBuilder sb = new StringBuilder();
        sb.append("CONFIG|1|12\n");
        for (Room r : rooms) {
            sb.append("ROOM|").append(r.getId()).append("|")
                    .append(r.getName()).append("|")
                    .append(r.getCapacity()).append("|")
                    .append(r.getFacilityMask()).append("|")
                    .append(1).append("\n"); // Send 1 for full schedule check so older confirmed bookings in inactive rooms don't cause failure
        }

        // Express confirmed bookings as REQ + ASSIGN if they have a requestId, or as BOOKING if fixed
        for (Booking b : bookings) {
            if (b.getRequestId() != null) {
                Optional<EventRequest> optReq = requestRepository.findById(b.getRequestId());
                if (optReq.isPresent()) {
                    EventRequest req = optReq.get();
                    sb.append("REQ|").append(req.getId()).append("|")
                            .append(req.getClubName()).append("|")
                            .append(toDayIndex(req.getDate())).append("|")
                            .append(toMinuteOfDay(req.getStartTime())).append("|")
                            .append(toMinuteOfDay(req.getEndTime())).append("|")
                            .append(req.getAttendance()).append("|")
                            .append(req.getFacilityMask()).append("|")
                            .append(req.getTier()).append("|")
                            .append(req.getSubmittedAt()).append("\n");
                    sb.append("ASSIGN|").append(req.getId()).append("|")
                            .append(b.getRoomId()).append("\n");
                    continue;
                }
            }

            // Fallback for seed / fixed booking without a request
            int startAbs = toAbsoluteMinutes(b.getDate(), b.getStartTime());
            int endAbs = toAbsoluteMinutes(b.getDate(), b.getEndTime());
            sb.append("BOOKING|").append(b.getId()).append("|")
                    .append(b.getRoomId()).append("|")
                    .append(startAbs).append("|")
                    .append(endAbs).append("\n");
        }

        SolverValidateResponse valRes = solverClient.validate(sb.toString());
        if (!valRes.isValid()) {
            throw new IllegalStateException("Full confirmed schedule validation failed: " + String.join("; ", valRes.getViolations()));
        }
    }

    private String buildSolverInput(List<Room> rooms, List<EventRequest> requests, List<Booking> bookings, List<AssignmentDto> assignments) {
        StringBuilder sb = new StringBuilder();
        sb.append("CONFIG|1|12\n");

        for (Room r : rooms) {
            sb.append("ROOM|").append(r.getId()).append("|")
                    .append(r.getName()).append("|")
                    .append(r.getCapacity()).append("|")
                    .append(r.getFacilityMask()).append("|")
                    .append(r.getActive() ? 1 : 0).append("\n");
        }

        for (EventRequest req : requests) {
            sb.append("REQ|").append(req.getId()).append("|")
                    .append(req.getClubName()).append("|")
                    .append(toDayIndex(req.getDate())).append("|")
                    .append(toMinuteOfDay(req.getStartTime())).append("|")
                    .append(toMinuteOfDay(req.getEndTime())).append("|")
                    .append(req.getAttendance()).append("|")
                    .append(req.getFacilityMask()).append("|")
                    .append(req.getTier()).append("|")
                    .append(req.getSubmittedAt()).append("\n");
        }

        for (Booking b : bookings) {
            int startAbs = toAbsoluteMinutes(b.getDate(), b.getStartTime());
            int endAbs = toAbsoluteMinutes(b.getDate(), b.getEndTime());
            sb.append("BOOKING|").append(b.getId()).append("|")
                    .append(b.getRoomId()).append("|")
                    .append(startAbs).append("|")
                    .append(endAbs).append("\n");
        }

        for (AssignmentDto asgn : assignments) {
            sb.append("ASSIGN|").append(asgn.getRequestId()).append("|")
                    .append(asgn.getRoomId()).append("\n");
        }

        return sb.toString();
    }

    private String buildValidateSingleAssignmentInput(List<Room> rooms, List<Booking> confirmedBookings, EventRequest request, Long roomId) {
        StringBuilder sb = new StringBuilder();
        sb.append("CONFIG|1|12\n");

        for (Room r : rooms) {
            sb.append("ROOM|").append(r.getId()).append("|")
                    .append(r.getName()).append("|")
                    .append(r.getCapacity()).append("|")
                    .append(r.getFacilityMask()).append("|")
                    .append(r.getActive() ? 1 : 0).append("\n");
        }

        sb.append("REQ|").append(request.getId()).append("|")
                .append(request.getClubName()).append("|")
                .append(toDayIndex(request.getDate())).append("|")
                .append(toMinuteOfDay(request.getStartTime())).append("|")
                .append(toMinuteOfDay(request.getEndTime())).append("|")
                .append(request.getAttendance()).append("|")
                .append(request.getFacilityMask()).append("|")
                .append(request.getTier()).append("|")
                .append(request.getSubmittedAt()).append("\n");

        for (Booking b : confirmedBookings) {
            int startAbs = toAbsoluteMinutes(b.getDate(), b.getStartTime());
            int endAbs = toAbsoluteMinutes(b.getDate(), b.getEndTime());
            sb.append("BOOKING|").append(b.getId()).append("|")
                    .append(b.getRoomId()).append("|")
                    .append(startAbs).append("|")
                    .append(endAbs).append("\n");
        }

        sb.append("ASSIGN|").append(request.getId()).append("|").append(roomId).append("\n");

        return sb.toString();
    }
}
