package edu.campus.alloc.auth;

import edu.campus.alloc.entity.UserRole;

public class UserSession {
    private final String token;
    private final Long userId;
    private final String username;
    private final UserRole role;
    private final String clubName;

    public UserSession(String token, Long userId, String username, UserRole role, String clubName) {
        this.token = token;
        this.userId = userId;
        this.username = username;
        this.role = role;
        this.clubName = clubName;
    }

    public String getToken() {
        return token;
    }

    public Long getUserId() {
        return userId;
    }

    public String getUsername() {
        return username;
    }

    public UserRole getRole() {
        return role;
    }

    public String getClubName() {
        return clubName;
    }
}
