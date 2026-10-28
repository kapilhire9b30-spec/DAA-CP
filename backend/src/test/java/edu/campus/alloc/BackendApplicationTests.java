package edu.campus.alloc;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import edu.campus.alloc.dto.LoginRequest;
import edu.campus.alloc.dto.RejectRequest;
import edu.campus.alloc.entity.*;
import edu.campus.alloc.repository.*;
import edu.campus.alloc.solver.SolverClient;
import edu.campus.alloc.solver.SolverException;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.MvcResult;

import java.io.File;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalTime;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.jupiter.api.Assertions.*;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;

@SpringBootTest
@AutoConfigureMockMvc
public class BackendApplicationTests {

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @Autowired
    private UserRepository userRepository;

    @Autowired
    private RoomRepository roomRepository;

    @Autowired
    private EventRequestRepository requestRepository;

    @Autowired
    private RecommendationRepository recommendationRepository;

    @Autowired
    private AlgorithmRunRepository algorithmRunRepository;

    @Autowired
    private BookingRepository bookingRepository;

    @Autowired
    private DecisionLogRepository decisionLogRepository;

    @Autowired
    private SolverClient solverClient;

    @BeforeAll
    static void checkSolverExists() {
        File solver1 = new File("../cpp-engine/build/solver.exe");
        File solver2 = new File("cpp-engine/build/solver.exe");
        assertTrue(solver1.exists() || solver2.exists(),
                "cpp-engine/build/solver.exe must exist! Please build the C++ solver first.");
    }

    private String login(String username, String password) throws Exception {
        MvcResult result = mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content(objectMapper.writeValueAsString(new LoginRequest(username, password))))
                .andExpect(status().isOk())
                .andReturn();
        JsonNode node = objectMapper.readTree(result.getResponse().getContentAsString());
        return node.get("token").asText();
    }

    // 1. No token -> 401. Club token on an admin endpoint -> 403.
    @Test
    void test1_noTokenAndClubForbidden() throws Exception {
        // No token -> 401
        mockMvc.perform(get("/api/rooms"))
                .andExpect(status().isUnauthorized())
                .andExpect(jsonPath("$.ok").value(false))
                .andExpect(jsonPath("$.error").exists());

        // Club token on an admin endpoint -> 403
        String clubToken = login("robotics", "robotics123");
        mockMvc.perform(post("/api/allocate")
                        .header("Authorization", "Bearer " + clubToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"algorithm\":\"A\"}"))
                .andExpect(status().isForbidden())
                .andExpect(jsonPath("$.ok").value(false))
                .andExpect(jsonPath("$.error").value("Forbidden: insufficient permissions"));
    }

    // 2. A club sees only its own requests.
    @Test
    void test2_clubSeesOnlyItsOwnRequests() throws Exception {
        String roboticsToken = login("robotics", "robotics123");
        MvcResult resRobotics = mockMvc.perform(get("/api/requests")
                        .header("Authorization", "Bearer " + roboticsToken))
                .andExpect(status().isOk())
                .andReturn();
        JsonNode listRobotics = objectMapper.readTree(resRobotics.getResponse().getContentAsString());
        assertTrue(listRobotics.size() > 0);
        for (JsonNode req : listRobotics) {
            assertEquals("Robotics", req.get("clubName").asText());
        }

        String codingToken = login("coding", "coding123");
        MvcResult resCoding = mockMvc.perform(get("/api/requests")
                        .header("Authorization", "Bearer " + codingToken))
                .andExpect(status().isOk())
                .andReturn();
        JsonNode listCoding = objectMapper.readTree(resCoding.getResponse().getContentAsString());
        assertTrue(listCoding.size() > 0);
        for (JsonNode req : listCoding) {
            assertEquals("CodingClub", req.get("clubName").asText());
        }
    }

    // 3. allocate creates recommendations and ZERO bookings.
    @Test
    void test3_allocateCreatesRecommendationsAndZeroBookings() throws Exception {
        String adminToken = login("admin", "admin123");
        long bookingCountBefore = bookingRepository.count();

        MvcResult res = mockMvc.perform(post("/api/allocate")
                        .header("Authorization", "Bearer " + adminToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"algorithm\":\"A\"}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andReturn();

        JsonNode allocRes = objectMapper.readTree(res.getResponse().getContentAsString());
        assertTrue(allocRes.get("assignments").size() > 0);

        List<Recommendation> activeRecs = recommendationRepository.findByActiveTrue();
        assertFalse(activeRecs.isEmpty());

        long bookingCountAfter = bookingRepository.count();
        assertEquals(bookingCountBefore, bookingCountAfter, "allocate must create ZERO bookings!");
    }

    // 4. approve creates exactly one booking and one decision log entry.
    @Test
    void test4_approveCreatesExactlyOneBookingAndOneDecisionLog() throws Exception {
        String adminToken = login("admin", "admin123");

        // Ensure allocate has been executed
        mockMvc.perform(post("/api/allocate")
                        .header("Authorization", "Bearer " + adminToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"algorithm\":\"A\"}"))
                .andExpect(status().isOk());

        List<Recommendation> activeRecs = recommendationRepository.findByActiveTrue();
        assertFalse(activeRecs.isEmpty());
        Recommendation recToApprove = activeRecs.get(0);

        long bookingCountBefore = bookingRepository.count();
        long decisionCountBefore = decisionLogRepository.count();

        mockMvc.perform(post("/api/recommendations/" + recToApprove.getId() + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.requestId").value(recToApprove.getRequestId()))
                .andExpect(jsonPath("$.data.roomId").value(recToApprove.getRoomId()));

        assertEquals(bookingCountBefore + 1, bookingRepository.count(), "approve must create exactly one booking!");
        assertEquals(decisionCountBefore + 1, decisionLogRepository.count(), "approve must create exactly one decision log entry!");

        DecisionLog log = decisionLogRepository.findAllByOrderByAtDesc().get(0);
        assertEquals(DecisionAction.APPROVE, log.getAction());
        assertEquals("admin", log.getWho());
        assertEquals(recToApprove.getRequestId(), log.getRequestId());
    }

    // 5. approving the same recommendation twice fails.
    @Test
    void test5_approvingSameRecommendationTwiceFails() throws Exception {
        String adminToken = login("admin", "admin123");

        mockMvc.perform(post("/api/allocate")
                        .header("Authorization", "Bearer " + adminToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"algorithm\":\"A\"}"))
                .andExpect(status().isOk());

        List<Recommendation> activeRecs = recommendationRepository.findByActiveTrue();
        assertFalse(activeRecs.isEmpty());
        Long recId = activeRecs.get(0).getId();

        // First approval succeeds
        mockMvc.perform(post("/api/recommendations/" + recId + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk());

        // Second approval must fail
        mockMvc.perform(post("/api/recommendations/" + recId + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.ok").value(false))
                .andExpect(jsonPath("$.error").exists());
    }

    // 6. approve or modify into a room that conflicts with a confirmed booking is blocked and nothing changes.
    @Test
    void test6_approveOrModifyConflictingRoomBlocked() throws Exception {
        String adminToken = login("admin", "admin123");

        // Room 101 has confirmed seed booking 09:00 - 10:30 on 2026-10-26
        // Create request conflicting with room 101: 09:30 - 11:00 on 2026-10-26
        EventRequest conflictReq = new EventRequest(
                1L,
                "Robotics",
                "Conflict Request",
                "Overlaps seed booking",
                LocalDate.of(2026, 10, 26),
                LocalTime.of(9, 30),
                LocalTime.of(11, 0),
                50,
                3,
                2,
                System.currentTimeMillis(),
                RequestStatus.RECOMMENDED
        );
        conflictReq = requestRepository.save(conflictReq);

        Recommendation rec = new Recommendation(
                conflictReq.getId(),
                101L,
                "Priority Greedy",
                1L,
                Instant.now(),
                true
        );
        rec = recommendationRepository.save(rec);

        long bookingCountBefore = bookingRepository.count();

        // Approve should be blocked due to conflict
        mockMvc.perform(post("/api/recommendations/" + rec.getId() + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isConflict())
                .andExpect(jsonPath("$.ok").value(false))
                .andExpect(jsonPath("$.error").value(org.hamcrest.Matchers.containsString("conflict")));

        // Verify nothing changed
        assertEquals(bookingCountBefore, bookingRepository.count());
        EventRequest reqAfter = requestRepository.findById(conflictReq.getId()).orElseThrow();
        assertEquals(RequestStatus.RECOMMENDED, reqAfter.getStatus());
    }

    // 7. modify into a room whose capacity is below the attendance still succeeds.
    @Test
    void test7_modifyIntoRoomWithCapacityBelowAttendanceSucceeds() throws Exception {
        String adminToken = login("admin", "admin123");

        // Create a request with attendance 200, facilityMask 1, time 15:00 - 17:00 on 2026-10-26
        EventRequest bigReq = new EventRequest(
                1L,
                "Robotics",
                "Big Attendance Workshop",
                "Attendance 200",
                LocalDate.of(2026, 10, 26),
                LocalTime.of(15, 0),
                LocalTime.of(17, 0),
                200,
                1,
                2,
                System.currentTimeMillis(),
                RequestStatus.RECOMMENDED
        );
        bigReq = requestRepository.save(bigReq);

        Recommendation rec = new Recommendation(
                bigReq.getId(),
                101L,
                "Priority Greedy",
                1L,
                Instant.now(),
                true
        );
        rec = recommendationRepository.save(rec);

        // Modify to Room 102 (capacity 80 < attendance 200, facility 1, active true)
        mockMvc.perform(post("/api/recommendations/" + rec.getId() + "/modify")
                        .header("Authorization", "Bearer " + adminToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"roomId\":102}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.booking.roomId").value(102))
                .andExpect(jsonPath("$.warning").exists());

        EventRequest updated = requestRepository.findById(bigReq.getId()).orElseThrow();
        assertEquals(RequestStatus.CONFIRMED, updated.getStatus());
    }

    // 8. reject without a reason fails; with a reason it is recorded.
    @Test
    void test8_rejectWithoutReasonFailsAndWithReasonRecorded() throws Exception {
        String adminToken = login("admin", "admin123");

        EventRequest req = new EventRequest(
                1L,
                "Robotics",
                "Request To Reject",
                "Description",
                LocalDate.of(2026, 10, 26),
                LocalTime.of(18, 0),
                LocalTime.of(19, 0),
                20,
                0,
                1,
                System.currentTimeMillis(),
                RequestStatus.PENDING
        );
        req = requestRepository.save(req);

        // Reject without reason fails
        mockMvc.perform(post("/api/requests/" + req.getId() + "/reject")
                        .header("Authorization", "Bearer " + adminToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"reason\":\"\"}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.ok").value(false));

        // Reject with reason succeeds
        mockMvc.perform(post("/api/requests/" + req.getId() + "/reject")
                        .header("Authorization", "Bearer " + adminToken)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"reason\":\"Event cancelled by college administration\"}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.status").value("REJECTED"))
                .andExpect(jsonPath("$.data.rejectionReason").value("Event cancelled by college administration"));

        DecisionLog log = decisionLogRepository.findAllByOrderByAtDesc().get(0);
        assertEquals(DecisionAction.REJECT, log.getAction());
        assertEquals("Event cancelled by college administration", log.getReason());
    }

    // 9. compare and demo change no data.
    @Test
    void test9_compareAndDemoChangeNoData() throws Exception {
        String adminToken = login("admin", "admin123");

        long reqCount = requestRepository.count();
        long roomCount = roomRepository.count();
        long bookCount = bookingRepository.count();
        long recCount = recommendationRepository.count();
        long runCount = algorithmRunRepository.count();
        long decCount = decisionLogRepository.count();

        // Call compare
        mockMvc.perform(get("/api/compare")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.rows").isArray());

        // Call demo
        mockMvc.perform(get("/api/demo")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.rows").isArray());

        // Verify side-effect free
        assertEquals(reqCount, requestRepository.count());
        assertEquals(roomCount, roomRepository.count());
        assertEquals(bookCount, bookingRepository.count());
        assertEquals(recCount, recommendationRepository.count());
        assertEquals(runCount, algorithmRunRepository.count());
        assertEquals(decCount, decisionLogRepository.count());
    }

    // 10. A solver failure (bad solver.path) returns 502 JSON, not a stack trace.
    @Test
    void test10_solverFailureReturns502JsonNotStackTrace() throws Exception {
        // Test that SolverClient throwing SolverException results in HTTP 502 JSON {"ok":false,"error":...}
        SolverClient badClient = new SolverClient("non_existent_solver_executable.exe", objectMapper);
        SolverException thrown = assertThrows(SolverException.class, () -> {
            badClient.allocate("A", "CONFIG|1|12\n");
        });

        // Test with MockMvc through GlobalExceptionHandler
        // Simulating a request where SolverException is thrown
        edu.campus.alloc.controller.GlobalExceptionHandler handler = new edu.campus.alloc.controller.GlobalExceptionHandler();
        var response = handler.handleSolverException(thrown);
        assertEquals(502, response.getStatusCode().value());
        assertNotNull(response.getBody());
        assertEquals(false, response.getBody().get("ok"));
        assertTrue(response.getBody().containsKey("error"));
        assertFalse(response.getBody().toString().contains("Exception in thread"));
    }

    // 11. Concurrency: two threads approve two conflicting recommendations at once; exactly one succeeds.
    @Test
    void test11_concurrencyTwoThreadsApproveConflictingRecommendations() throws Exception {
        String adminToken = login("admin", "admin123");

        // Create two overlapping requests for date 2026-11-15
        LocalDate testDate = LocalDate.of(2026, 11, 15);
        EventRequest reqA = new EventRequest(
                1L, "Robotics", "Concurrent A", "Req A",
                testDate, LocalTime.of(10, 0), LocalTime.of(12, 0),
                30, 0, 2, System.currentTimeMillis(), RequestStatus.RECOMMENDED
        );
        EventRequest reqB = new EventRequest(
                2L, "CodingClub", "Concurrent B", "Req B",
                testDate, LocalTime.of(11, 0), LocalTime.of(13, 0),
                30, 0, 2, System.currentTimeMillis() + 1, RequestStatus.RECOMMENDED
        );
        reqA = requestRepository.save(reqA);
        reqB = requestRepository.save(reqB);

        // Both recommendations propose Room 102
        Recommendation recA = new Recommendation(reqA.getId(), 102L, "A", 99L, Instant.now(), true);
        Recommendation recB = new Recommendation(reqB.getId(), 102L, "A", 99L, Instant.now(), true);
        recA = recommendationRepository.save(recA);
        recB = recommendationRepository.save(recB);

        Long recAId = recA.getId();
        Long recBId = recB.getId();

        ExecutorService executor = Executors.newFixedThreadPool(2);
        CountDownLatch startLatch = new CountDownLatch(1);
        CountDownLatch endLatch = new CountDownLatch(2);

        AtomicInteger successCount = new AtomicInteger(0);
        AtomicInteger failureCount = new AtomicInteger(0);

        Runnable taskA = () -> {
            try {
                startLatch.await();
                MvcResult res = mockMvc.perform(post("/api/recommendations/" + recAId + "/approve")
                                .header("Authorization", "Bearer " + adminToken))
                        .andReturn();
                if (res.getResponse().getStatus() == 200) {
                    successCount.incrementAndGet();
                } else {
                    failureCount.incrementAndGet();
                }
            } catch (Exception e) {
                failureCount.incrementAndGet();
            } finally {
                endLatch.countDown();
            }
        };

        Runnable taskB = () -> {
            try {
                startLatch.await();
                MvcResult res = mockMvc.perform(post("/api/recommendations/" + recBId + "/approve")
                                .header("Authorization", "Bearer " + adminToken))
                        .andReturn();
                if (res.getResponse().getStatus() == 200) {
                    successCount.incrementAndGet();
                } else {
                    failureCount.incrementAndGet();
                }
            } catch (Exception e) {
                failureCount.incrementAndGet();
            } finally {
                endLatch.countDown();
            }
        };

        executor.submit(taskA);
        executor.submit(taskB);

        // Fire both threads simultaneously
        startLatch.countDown();
        boolean completed = endLatch.await(10, TimeUnit.SECONDS);
        assertTrue(completed, "Concurrent approval tasks timed out!");
        executor.shutdown();

        assertEquals(1, successCount.get(), "Exactly one approval must succeed!");
        assertEquals(1, failureCount.get(), "The conflicting approval must be rejected!");
    }

    // 12. activate works and is admin-only (club 403, no token 401).
    @Test
    void test12_activateWorksAndIsAdminOnly() throws Exception {
        // Ensure room 104 is inactive initially
        Room r104 = roomRepository.findById(104L).orElseThrow();
        r104.setActive(false);
        roomRepository.save(r104);

        // No token -> 401
        mockMvc.perform(post("/api/rooms/104/activate"))
                .andExpect(status().isUnauthorized())
                .andExpect(jsonPath("$.ok").value(false));

        // Club token -> 403
        String clubToken = login("robotics", "robotics123");
        mockMvc.perform(post("/api/rooms/104/activate")
                        .header("Authorization", "Bearer " + clubToken))
                .andExpect(status().isForbidden())
                .andExpect(jsonPath("$.ok").value(false));

        // Admin token -> 200 OK
        String adminToken = login("admin", "admin123");
        mockMvc.perform(post("/api/rooms/104/activate")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.active").value(true));

        // Verify in DB
        assertTrue(roomRepository.findById(104L).orElseThrow().getActive());

        // Verify decision log has ROOM_ACTIVATED
        List<DecisionLog> logs = decisionLogRepository.findAllByOrderByAtDesc();
        assertFalse(logs.isEmpty());
        assertEquals(DecisionAction.ROOM_ACTIVATED, logs.get(0).getAction());
        assertEquals("admin", logs.get(0).getWho());
        assertEquals(104L, logs.get(0).getRoomId());
    }

    // 13. activate is idempotent (calling it twice succeeds both times).
    @Test
    void test13_activateIsIdempotent() throws Exception {
        String adminToken = login("admin", "admin123");

        // First activate
        mockMvc.perform(post("/api/rooms/104/activate")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.active").value(true));

        long logCountAfterFirst = decisionLogRepository.count();

        // Second activate on already active room -> must succeed idempotently
        mockMvc.perform(post("/api/rooms/104/activate")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.active").value(true));

        assertEquals(logCountAfterFirst, decisionLogRepository.count(), "Idempotent call should not write duplicate log");
    }

    // 14. affected-bookings returns the right list (and is admin-only).
    @Test
    void test14_affectedBookingsReturnsRightList() throws Exception {
        // No token -> 401
        mockMvc.perform(get("/api/rooms/101/affected-bookings"))
                .andExpect(status().isUnauthorized());

        // Club token -> 403
        String clubToken = login("robotics", "robotics123");
        mockMvc.perform(get("/api/rooms/101/affected-bookings")
                        .header("Authorization", "Bearer " + clubToken))
                .andExpect(status().isForbidden());

        // Admin token -> 200 OK with affected list
        String adminToken = login("admin", "admin123");
        MvcResult res = mockMvc.perform(get("/api/rooms/101/affected-bookings")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andReturn();

        JsonNode array = objectMapper.readTree(res.getResponse().getContentAsString());
        assertTrue(array.isArray());
        assertTrue(array.size() >= 1, "Room 101 should have at least the seed booking");
        JsonNode firstBooking = array.get(0);
        assertNotNull(firstBooking.get("id"));
        assertNotNull(firstBooking.get("date"));
        assertNotNull(firstBooking.get("start"));
        assertNotNull(firstBooking.get("end"));
    }

    // 15. deactivating writes the decision log and returns affected bookings.
    @Test
    void test15_deactivateWritesDecisionLogAndReturnsAffectedBookings() throws Exception {
        String adminToken = login("admin", "admin123");

        long logsBefore = decisionLogRepository.count();

        MvcResult res = mockMvc.perform(post("/api/rooms/101/deactivate")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.active").value(false))
                .andExpect(jsonPath("$.data.affectedBookings").isArray())
                .andReturn();

        JsonNode data = objectMapper.readTree(res.getResponse().getContentAsString()).get("data");
        assertTrue(data.get("affectedBookings").size() >= 1);

        assertEquals(logsBefore + 1, decisionLogRepository.count());
        DecisionLog log = decisionLogRepository.findAllByOrderByAtDesc().get(0);
        assertEquals(DecisionAction.ROOM_DEACTIVATED, log.getAction());
        assertEquals("admin", log.getWho());
        assertEquals(101L, log.getRoomId());
        assertTrue(log.getDetails().contains("affected bookings"));
    }

    // 16. The bug: with an older confirmed booking in an inactive room, approving an unrelated feasible recommendation succeeds.
    @Test
    void test16_olderBookingInInactiveRoomDoesNotBlockUnrelatedApproval() throws Exception {
        String adminToken = login("admin", "admin123");

        // Ensure Room 101 is deactivated and still has an older confirmed booking
        Room r101 = roomRepository.findById(101L).orElseThrow();
        r101.setActive(false);
        roomRepository.save(r101);
        assertFalse(bookingRepository.findByRoomId(101L).isEmpty(), "Room 101 must have an existing booking");

        // Create an unrelated request for Room 102 on a future date
        EventRequest req = new EventRequest(
                1L, "Robotics", "Unrelated Future Workshop", "Test bug fix",
                LocalDate.of(2026, 12, 1), LocalTime.of(10, 0), LocalTime.of(12, 0),
                40, 1, 2, System.currentTimeMillis(), RequestStatus.RECOMMENDED
        );
        req = requestRepository.save(req);

        // Recommend Room 102 (active hall)
        Recommendation rec = new Recommendation(req.getId(), 102L, "A", 100L, Instant.now(), true);
        rec = recommendationRepository.save(rec);

        // Approve recommendation: previously this threw "Assignment for request N uses inactive room 101" during full-schedule check
        mockMvc.perform(post("/api/recommendations/" + rec.getId() + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.ok").value(true))
                .andExpect(jsonPath("$.data.roomId").value(102L));

        // Confirm booking was created
        assertTrue(bookingRepository.findByRequestId(req.getId()).isPresent());
    }

    // 17. A new booking into an inactive room is still refused.
    @Test
    void test17_newBookingIntoInactiveRoomIsRefused() throws Exception {
        String adminToken = login("admin", "admin123");

        // Ensure Room 104 is inactive
        Room r104 = roomRepository.findById(104L).orElseThrow();
        r104.setActive(false);
        roomRepository.save(r104);

        // Request proposing inactive Room 104
        EventRequest req = new EventRequest(
                1L, "Robotics", "Attempt Inactive Room", "Should fail",
                LocalDate.of(2026, 12, 2), LocalTime.of(14, 0), LocalTime.of(16, 0),
                15, 0, 2, System.currentTimeMillis(), RequestStatus.RECOMMENDED
        );
        req = requestRepository.save(req);

        Recommendation rec = new Recommendation(req.getId(), 104L, "A", 100L, Instant.now(), true);
        rec = recommendationRepository.save(rec);

        // Approve must be refused because Room 104 is inactive
        mockMvc.perform(post("/api/recommendations/" + rec.getId() + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isConflict())
                .andExpect(jsonPath("$.ok").value(false))
                .andExpect(jsonPath("$.error").value(org.hamcrest.Matchers.containsString("inactive room")));

        // Verify booking was NOT created
        assertTrue(bookingRepository.findByRequestId(req.getId()).isEmpty());
    }

    // 18. Genuine overlap in the confirmed schedule is still detected and fails.
    @Test
    void test18_genuineOverlapInConfirmedScheduleStillDetected() throws Exception {
        String adminToken = login("admin", "admin123");

        LocalDate date = LocalDate.of(2026, 12, 10);

        // Create booking 1 directly in Room 102
        Booking b1 = new Booking(null, 102L, date, LocalTime.of(10, 0), LocalTime.of(12, 0), "admin", Instant.now());
        bookingRepository.save(b1);

        // Create overlapping request for Room 102
        EventRequest overlapReq = new EventRequest(
                1L, "Robotics", "Overlapping Request", "Overlap",
                date, LocalTime.of(11, 0), LocalTime.of(13, 0),
                20, 0, 2, System.currentTimeMillis(), RequestStatus.RECOMMENDED
        );
        overlapReq = requestRepository.save(overlapReq);

        Recommendation rec = new Recommendation(overlapReq.getId(), 102L, "A", 100L, Instant.now(), true);
        rec = recommendationRepository.save(rec);

        // Approve should fail with conflict due to genuine overlap
        mockMvc.perform(post("/api/recommendations/" + rec.getId() + "/approve")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isConflict())
                .andExpect(jsonPath("$.ok").value(false))
                .andExpect(jsonPath("$.error").value(org.hamcrest.Matchers.containsString("overlaps")));
    }

    // 19. GET /api/bookings includes roomActive flag.
    @Test
    void test19_getBookingsIncludesRoomActiveFlag() throws Exception {
        String adminToken = login("admin", "admin123");

        MvcResult res = mockMvc.perform(get("/api/bookings")
                        .header("Authorization", "Bearer " + adminToken))
                .andExpect(status().isOk())
                .andReturn();

        JsonNode bookings = objectMapper.readTree(res.getResponse().getContentAsString());
        assertTrue(bookings.size() > 0);
        for (JsonNode b : bookings) {
            assertTrue(b.has("roomActive"), "Each booking must have roomActive flag");
            assertTrue(b.get("roomActive").isBoolean());
        }
    }
}
