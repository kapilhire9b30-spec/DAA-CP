package edu.campus.alloc.service;

import edu.campus.alloc.dto.AffectedBookingDto;
import edu.campus.alloc.dto.CreateRoomRequest;
import edu.campus.alloc.dto.RoomDeactivationResponse;
import edu.campus.alloc.entity.Booking;
import edu.campus.alloc.entity.DecisionAction;
import edu.campus.alloc.entity.DecisionLog;
import edu.campus.alloc.entity.EventRequest;
import edu.campus.alloc.entity.Room;
import edu.campus.alloc.repository.BookingRepository;
import edu.campus.alloc.repository.DecisionLogRepository;
import edu.campus.alloc.repository.EventRequestRepository;
import edu.campus.alloc.repository.RoomRepository;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;

@Service
public class RoomService {

    private final RoomRepository roomRepository;
    private final BookingRepository bookingRepository;
    private final EventRequestRepository requestRepository;
    private final DecisionLogRepository decisionLogRepository;

    public RoomService(RoomRepository roomRepository,
                       BookingRepository bookingRepository,
                       EventRequestRepository requestRepository,
                       DecisionLogRepository decisionLogRepository) {
        this.roomRepository = roomRepository;
        this.bookingRepository = bookingRepository;
        this.requestRepository = requestRepository;
        this.decisionLogRepository = decisionLogRepository;
    }

    public List<Room> getAllRooms() {
        return roomRepository.findAll();
    }

    public List<Room> getActiveRooms() {
        return roomRepository.findByActiveTrue();
    }

    @Transactional
    public Room createRoom(CreateRoomRequest req) {
        Long nextId = roomRepository.findMaxId() + 1;
        Room room = new Room(
                nextId,
                req.getName(),
                req.getBuilding(),
                req.getCapacity(),
                req.getFacilityMask(),
                true
        );
        return roomRepository.save(room);
    }

    public List<AffectedBookingDto> getAffectedBookings(Long roomId) {
        Room room = roomRepository.findById(roomId)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Room not found with id: " + roomId));

        List<Booking> bookings = bookingRepository.findByRoomId(roomId);
        bookings.sort(Comparator.comparing(Booking::getDate).thenComparing(Booking::getStartTime));

        List<AffectedBookingDto> result = new ArrayList<>();
        for (Booking b : bookings) {
            String title = "Confirmed Booking";
            String club = b.getApprovedBy() != null ? b.getApprovedBy() : "Admin";
            if (b.getRequestId() != null) {
                Optional<EventRequest> optReq = requestRepository.findById(b.getRequestId());
                if (optReq.isPresent()) {
                    EventRequest req = optReq.get();
                    title = req.getTitle();
                    club = req.getClubName();
                }
            }
            result.add(new AffectedBookingDto(
                    b.getId(),
                    b.getRequestId(),
                    title,
                    club,
                    b.getDate(),
                    b.getStartTime(),
                    b.getEndTime()
            ));
        }
        return result;
    }

    @Transactional
    public Room activateRoom(Long id, String who) {
        Room room = roomRepository.findById(id)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Room not found with id: " + id));

        // Idempotent: activating an already-active room returns success without changes
        if (Boolean.TRUE.equals(room.getActive())) {
            return room;
        }

        room.setActive(true);
        Room saved = roomRepository.save(room);

        // Log ROOM_ACTIVATED in decision log (uses 0L for legacy NOT NULL request_id DB constraint)
        DecisionLog log = new DecisionLog(
                who != null ? who : "admin",
                Instant.now(),
                DecisionAction.ROOM_ACTIVATED,
                0L,
                saved.getId(),
                null,
                "Room activated: " + saved.getName()
        );
        decisionLogRepository.save(log);

        return saved;
    }

    @Transactional
    public RoomDeactivationResponse deactivateRoom(Long id, String who) {
        Room room = roomRepository.findById(id)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Room not found with id: " + id));

        List<AffectedBookingDto> affected = getAffectedBookings(id);

        room.setActive(false);
        Room saved = roomRepository.save(room);

        // Log ROOM_DEACTIVATED in decision log with who, when, room and number of affected bookings
        DecisionLog log = new DecisionLog(
                who != null ? who : "admin",
                Instant.now(),
                DecisionAction.ROOM_DEACTIVATED,
                0L,
                saved.getId(),
                null,
                "Room deactivated: " + saved.getName() + " (" + affected.size() + " affected bookings)"
        );
        decisionLogRepository.save(log);

        return new RoomDeactivationResponse(saved, affected);
    }
}
