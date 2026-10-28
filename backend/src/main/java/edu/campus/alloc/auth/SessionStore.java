package edu.campus.alloc.auth;

import org.springframework.stereotype.Component;

import java.util.Map;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

@Component
public class SessionStore {

    private final Map<String, UserSession> sessions = new ConcurrentHashMap<>();

    public UserSession createSession(Long userId, String username, edu.campus.alloc.entity.UserRole role, String clubName) {
        String token = UUID.randomUUID().toString();
        UserSession session = new UserSession(token, userId, username, role, clubName);
        sessions.put(token, session);
        return session;
    }

    public Optional<UserSession> getSession(String token) {
        if (token == null) {
            return Optional.empty();
        }
        return Optional.ofNullable(sessions.get(token));
    }

    public void removeSession(String token) {
        if (token != null) {
            sessions.remove(token);
        }
    }

    public void clear() {
        sessions.clear();
    }
}
