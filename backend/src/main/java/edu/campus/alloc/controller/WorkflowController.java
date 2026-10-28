package edu.campus.alloc.controller;

import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.AllocateRequest;
import edu.campus.alloc.dto.ApiResponse;
import edu.campus.alloc.dto.ModifyRequest;
import edu.campus.alloc.dto.RecommendationResponse;
import edu.campus.alloc.entity.Booking;
import edu.campus.alloc.entity.DecisionLog;
import edu.campus.alloc.service.WorkflowService;
import edu.campus.alloc.solver.dto.SolverAllocateResponse;
import edu.campus.alloc.solver.dto.SolverCompareResponse;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class WorkflowController {

    private final WorkflowService workflowService;

    public WorkflowController(WorkflowService workflowService) {
        this.workflowService = workflowService;
    }

    @PostMapping("/allocate")
    public ResponseEntity<SolverAllocateResponse> allocate(@Valid @RequestBody AllocateRequest request) {
        SolverAllocateResponse response = workflowService.allocate(request.getAlgorithm());
        return ResponseEntity.ok(response);
    }

    @GetMapping("/compare")
    public ResponseEntity<SolverCompareResponse> compare() {
        SolverCompareResponse response = workflowService.compare();
        return ResponseEntity.ok(response);
    }

    @GetMapping("/demo")
    public ResponseEntity<SolverCompareResponse> demo() {
        SolverCompareResponse response = workflowService.demo();
        return ResponseEntity.ok(response);
    }

    @GetMapping("/recommendations")
    public ResponseEntity<List<RecommendationResponse>> getRecommendations() {
        List<RecommendationResponse> list = workflowService.getRecommendations();
        return ResponseEntity.ok(list);
    }

    @PostMapping("/recommendations/{id}/approve")
    public ResponseEntity<ApiResponse<Booking>> approveRecommendation(@PathVariable Long id, HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        Booking booking = workflowService.approve(id, session);
        return ResponseEntity.ok(ApiResponse.success(booking));
    }

    @PostMapping("/recommendations/{id}/modify")
    public ResponseEntity<Map<String, Object>> modifyRecommendation(@PathVariable Long id,
                                                                   @Valid @RequestBody ModifyRequest modifyDto,
                                                                   HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        Map<String, Object> response = workflowService.modify(id, modifyDto.getRoomId(), session);
        return ResponseEntity.ok(response);
    }

    @GetMapping("/bookings")
    public ResponseEntity<List<Booking>> getBookings() {
        return ResponseEntity.ok(workflowService.getBookings());
    }

    @GetMapping("/decisions")
    public ResponseEntity<List<DecisionLog>> getDecisions() {
        return ResponseEntity.ok(workflowService.getDecisions());
    }
}
