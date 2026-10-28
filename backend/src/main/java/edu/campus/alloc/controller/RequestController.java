package edu.campus.alloc.controller;

import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.ApiResponse;
import edu.campus.alloc.dto.CreateEventRequest;
import edu.campus.alloc.dto.RejectRequest;
import edu.campus.alloc.entity.EventRequest;
import edu.campus.alloc.service.RequestService;
import edu.campus.alloc.service.WorkflowService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/requests")
public class RequestController {

    private final RequestService requestService;
    private final WorkflowService workflowService;

    public RequestController(RequestService requestService, WorkflowService workflowService) {
        this.requestService = requestService;
        this.workflowService = workflowService;
    }

    @PostMapping
    public ResponseEntity<ApiResponse<EventRequest>> createRequest(@Valid @RequestBody CreateEventRequest requestDto,
                                                                   HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        EventRequest created = requestService.submitRequest(requestDto, session);
        return ResponseEntity.status(HttpStatus.CREATED).body(ApiResponse.success(created));
    }

    @GetMapping
    public ResponseEntity<List<EventRequest>> getRequests(HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        List<EventRequest> list = requestService.getRequests(session);
        return ResponseEntity.ok(list);
    }

    @PostMapping("/{id}/reject")
    public ResponseEntity<ApiResponse<EventRequest>> rejectRequest(@PathVariable Long id,
                                                                  @Valid @RequestBody RejectRequest rejectDto,
                                                                  HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        EventRequest rejected = workflowService.reject(id, rejectDto.getReason(), session);
        return ResponseEntity.ok(ApiResponse.success(rejected));
    }
}
