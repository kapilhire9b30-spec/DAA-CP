package edu.campus.alloc.auth;

import com.fasterxml.jackson.databind.ObjectMapper;
import edu.campus.alloc.entity.UserRole;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;

@Component
public class AuthInterceptor implements HandlerInterceptor {

    private final SessionStore sessionStore;
    private final ObjectMapper objectMapper;

    public AuthInterceptor(SessionStore sessionStore, ObjectMapper objectMapper) {
        this.sessionStore = sessionStore;
        this.objectMapper = objectMapper;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) throws Exception {
        // Preflight OPTIONS requests bypass auth
        if ("OPTIONS".equalsIgnoreCase(request.getMethod())) {
            return true;
        }

        String uri = request.getRequestURI();
        String method = request.getMethod();

        // Whitelisted public endpoints
        if (uri.equals("/api/auth/login") && "POST".equalsIgnoreCase(method)) {
            return true;
        }

        // Check Authorization header
        String authHeader = request.getHeader("Authorization");
        if (authHeader == null || !authHeader.startsWith("Bearer ")) {
            sendJsonError(response, HttpServletResponse.SC_UNAUTHORIZED, "Unauthorized: missing or invalid token");
            return false;
        }

        String token = authHeader.substring(7).trim();
        Optional<UserSession> sessionOpt = sessionStore.getSession(token);
        if (sessionOpt.isEmpty()) {
            sendJsonError(response, HttpServletResponse.SC_UNAUTHORIZED, "Unauthorized: missing or invalid token");
            return false;
        }

        UserSession session = sessionOpt.get();
        request.setAttribute("userSession", session);

        // Role-based permission checks:
        // CLUB users may only access:
        // - POST /api/auth/logout
        // - GET /api/me
        // - GET /api/rooms
        // - GET /api/bookings
        // - GET /api/requests
        // - POST /api/requests
        // All other /api/** endpoints require ADMIN role.
        if (session.getRole() == UserRole.CLUB) {
            boolean isAllowedClub = false;

            if (uri.equals("/api/auth/logout") && "POST".equalsIgnoreCase(method)) {
                isAllowedClub = true;
            } else if (uri.equals("/api/me") && "GET".equalsIgnoreCase(method)) {
                isAllowedClub = true;
            } else if (uri.equals("/api/rooms") && "GET".equalsIgnoreCase(method)) {
                isAllowedClub = true;
            } else if (uri.equals("/api/bookings") && "GET".equalsIgnoreCase(method)) {
                isAllowedClub = true;
            } else if (uri.equals("/api/requests") && ("GET".equalsIgnoreCase(method) || "POST".equalsIgnoreCase(method))) {
                isAllowedClub = true;
            }

            if (!isAllowedClub) {
                sendJsonError(response, HttpServletResponse.SC_FORBIDDEN, "Forbidden: insufficient permissions");
                return false;
            }
        }

        return true;
    }

    private void sendJsonError(HttpServletResponse response, int status, String message) throws IOException {
        response.setStatus(status);
        response.setContentType("application/json");
        response.setCharacterEncoding("UTF-8");
        Map<String, Object> errorBody = Map.of("ok", false, "error", message);
        response.getWriter().write(objectMapper.writeValueAsString(errorBody));
    }
}
