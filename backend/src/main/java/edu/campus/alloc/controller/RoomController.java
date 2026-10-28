package edu.campus.alloc.controller;

import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.AffectedBookingDto;
import edu.campus.alloc.dto.ApiResponse;
import edu.campus.alloc.dto.CreateRoomRequest;
import edu.campus.alloc.dto.RoomDeactivationResponse;
import edu.campus.alloc.entity.Room;
import edu.campus.alloc.service.RoomService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/rooms")
public class RoomController {

    private final RoomService roomService;

    public RoomController(RoomService roomService) {
        this.roomService = roomService;
    }

    @GetMapping
    public ResponseEntity<List<Room>> getAllRooms() {
        return ResponseEntity.ok(roomService.getAllRooms());
    }

    @PostMapping
    public ResponseEntity<ApiResponse<Room>> createRoom(@Valid @RequestBody CreateRoomRequest request) {
        Room created = roomService.createRoom(request);
        return ResponseEntity.status(HttpStatus.CREATED).body(ApiResponse.success(created));
    }

    @PostMapping("/{id}/activate")
    public ResponseEntity<ApiResponse<Room>> activateRoom(@PathVariable Long id, HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        String who = session != null ? session.getUsername() : "admin";
        Room updated = roomService.activateRoom(id, who);
        return ResponseEntity.ok(ApiResponse.success(updated));
    }

    @PostMapping("/{id}/deactivate")
    public ResponseEntity<ApiResponse<RoomDeactivationResponse>> deactivateRoom(@PathVariable Long id, HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        String who = session != null ? session.getUsername() : "admin";
        RoomDeactivationResponse response = roomService.deactivateRoom(id, who);
        return ResponseEntity.ok(ApiResponse.success(response));
    }

    @GetMapping("/{id}/affected-bookings")
    public ResponseEntity<List<AffectedBookingDto>> getAffectedBookings(@PathVariable Long id) {
        List<AffectedBookingDto> list = roomService.getAffectedBookings(id);
        return ResponseEntity.ok(list);
    }
}
