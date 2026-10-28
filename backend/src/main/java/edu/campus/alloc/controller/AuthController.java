package edu.campus.alloc.controller;

import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.ApiResponse;
import edu.campus.alloc.dto.LoginRequest;
import edu.campus.alloc.dto.LoginResponse;
import edu.campus.alloc.service.AuthService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/api")
public class AuthController {

    private final AuthService authService;

    public AuthController(AuthService authService) {
        this.authService = authService;
    }

    @PostMapping("/auth/login")
    public ResponseEntity<LoginResponse> login(@Valid @RequestBody LoginRequest request) {
        LoginResponse response = authService.login(request);
        return ResponseEntity.ok(response);
    }

    @PostMapping("/auth/logout")
    public ResponseEntity<ApiResponse<String>> logout(HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        if (session != null) {
            authService.logout(session.getToken());
        }
        return ResponseEntity.ok(ApiResponse.success("Logged out successfully"));
    }

    @GetMapping("/me")
    public ResponseEntity<Map<String, Object>> getCurrentUser(HttpServletRequest request) {
        UserSession session = (UserSession) request.getAttribute("userSession");
        return ResponseEntity.ok(Map.of(
                "userId", session.getUserId(),
                "username", session.getUsername(),
                "role", session.getRole(),
                "clubName", session.getClubName() != null ? session.getClubName() : ""
        ));
    }
}
