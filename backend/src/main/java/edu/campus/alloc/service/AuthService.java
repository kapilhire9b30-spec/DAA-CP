package edu.campus.alloc.service;

import edu.campus.alloc.auth.SessionStore;
import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.LoginRequest;
import edu.campus.alloc.dto.LoginResponse;
import edu.campus.alloc.entity.User;
import edu.campus.alloc.repository.UserRepository;
import org.springframework.http.HttpStatus;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.web.server.ResponseStatusException;

@Service
public class AuthService {

    private final UserRepository userRepository;
    private final PasswordEncoder passwordEncoder;
    private final SessionStore sessionStore;

    public AuthService(UserRepository userRepository, PasswordEncoder passwordEncoder, SessionStore sessionStore) {
        this.userRepository = userRepository;
        this.passwordEncoder = passwordEncoder;
        this.sessionStore = sessionStore;
    }

    public LoginResponse login(LoginRequest request) {
        User user = userRepository.findByUsername(request.getUsername())
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "Invalid username or password"));

        if (!passwordEncoder.matches(request.getPassword(), user.getPasswordHash())) {
            throw new ResponseStatusException(HttpStatus.UNAUTHORIZED, "Invalid username or password");
        }

        UserSession session = sessionStore.createSession(
                user.getId(),
                user.getUsername(),
                user.getRole(),
                user.getClubName()
        );

        return new LoginResponse(
                session.getToken(),
                session.getRole(),
                session.getUsername(),
                session.getClubName()
        );
    }

    public void logout(String token) {
        sessionStore.removeSession(token);
    }
}
