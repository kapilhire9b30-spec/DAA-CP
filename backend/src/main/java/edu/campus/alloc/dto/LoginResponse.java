package edu.campus.alloc.dto;

import edu.campus.alloc.entity.UserRole;

public class LoginResponse {
    private String token;
    private UserRole role;
    private String username;
    private String clubName;

    public LoginResponse() {
    }

    public LoginResponse(String token, UserRole role, String username, String clubName) {
        this.token = token;
        this.role = role;
        this.username = username;
        this.clubName = clubName;
    }

    public String getToken() {
        return token;
    }

    public void setToken(String token) {
        this.token = token;
    }

    public UserRole getRole() {
        return role;
    }

    public void setRole(UserRole role) {
        this.role = role;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getClubName() {
        return clubName;
    }

    public void setClubName(String clubName) {
        this.clubName = clubName;
    }
}
