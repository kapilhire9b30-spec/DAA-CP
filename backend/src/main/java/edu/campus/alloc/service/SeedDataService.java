package edu.campus.alloc.service;

import edu.campus.alloc.entity.*;
import edu.campus.alloc.repository.BookingRepository;
import edu.campus.alloc.repository.EventRequestRepository;
import edu.campus.alloc.repository.RoomRepository;
import edu.campus.alloc.repository.UserRepository;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.CommandLineRunner;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalTime;
import java.util.List;

@Service
public class SeedDataService implements CommandLineRunner {

    private final UserRepository userRepository;
    private final RoomRepository roomRepository;
    private final EventRequestRepository requestRepository;
    private final BookingRepository bookingRepository;
    private final PasswordEncoder passwordEncoder;
    private final JdbcTemplate jdbcTemplate;

    @Value("${app.admin.password:admin123}")
    private String adminPassword;

    public SeedDataService(UserRepository userRepository,
                           RoomRepository roomRepository,
                           EventRequestRepository requestRepository,
                           BookingRepository bookingRepository,
                           PasswordEncoder passwordEncoder,
                           JdbcTemplate jdbcTemplate) {
        this.userRepository = userRepository;
        this.roomRepository = roomRepository;
        this.requestRepository = requestRepository;
        this.bookingRepository = bookingRepository;
        this.passwordEncoder = passwordEncoder;
        this.jdbcTemplate = jdbcTemplate;
    }

    @Override
    @Transactional
    public void run(String... args) {
        // Migrate legacy table constraints on DECISION_LOGS if present
        migrateLegacyConstraints();

        // Seed only if users or rooms are missing
        if (userRepository.count() > 0 && roomRepository.count() > 0) {
            return;
        }

        // 1. Seed Admin
        User admin = new User("admin", passwordEncoder.encode(adminPassword), UserRole.ADMIN, null);
        userRepository.save(admin);

        // 2. Seed 3 Club Users
        User club1 = new User("robotics", passwordEncoder.encode("robotics123"), UserRole.CLUB, "Robotics");
        User club2 = new User("coding", passwordEncoder.encode("coding123"), UserRole.CLUB, "CodingClub");
        User club3 = new User("literary", passwordEncoder.encode("literary123"), UserRole.CLUB, "Literary");
        userRepository.saveAll(List.of(club1, club2, club3));

        // 3. Seed 4 Rooms (1 inactive)
        Room r101 = new Room(101L, "Main Auditorium", "Campus Center", 300, 3, true); // Projector (1) + Audio (2)
        Room r102 = new Room(102L, "Seminar Hall A", "North Block", 80, 1, true);    // Projector (1)
        Room r103 = new Room(103L, "Computer Lab 1", "Tech Wing", 40, 4, true);     // Workstations (4)
        Room r104 = new Room(104L, "Conference Room B", "Admin Block", 20, 3, false); // Inactive
        roomRepository.saveAll(List.of(r101, r102, r103, r104));

        // 4. Seed 1 Confirmed Booking that is valid
        LocalDate seedDate = LocalDate.of(2026, 10, 26);
        Booking fixedBooking = new Booking(
                null,
                101L,
                seedDate,
                LocalTime.of(9, 0),
                LocalTime.of(10, 30),
                "admin",
                Instant.now()
        );
        bookingRepository.save(fixedBooking);

        // 5. Seed a few PENDING Requests
        EventRequest req1 = new EventRequest(
                club1.getId(),
                "Robotics",
                "Robotics Workshop",
                "Hands-on robotics hardware workshop",
                seedDate,
                LocalTime.of(10, 30),
                LocalTime.of(12, 30),
                60,
                3,
                3,
                100L,
                RequestStatus.PENDING
        );

        EventRequest req2 = new EventRequest(
                club2.getId(),
                "CodingClub",
                "Coding Hackathon",
                "24-hour coding challenge",
                seedDate,
                LocalTime.of(10, 0),
                LocalTime.of(14, 0),
                35,
                4,
                2,
                110L,
                RequestStatus.PENDING
        );

        EventRequest req3 = new EventRequest(
                club3.getId(),
                "Literary",
                "Poetry Slam",
                "Annual inter-college poetry slam",
                seedDate,
                LocalTime.of(11, 0),
                LocalTime.of(13, 0),
                45,
                1,
                1,
                120L,
                RequestStatus.PENDING
        );

        requestRepository.saveAll(List.of(req1, req2, req3));
    }

    private void migrateLegacyConstraints() {
        try {
            // Drop any check constraints on DECISION_LOGS dynamically
            try {
                List<String> checkConstraints = jdbcTemplate.query(
                        "SELECT CONSTRAINT_NAME FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS WHERE TABLE_NAME = 'DECISION_LOGS' AND CONSTRAINT_TYPE = 'CHECK'",
                        (rs, rowNum) -> rs.getString("CONSTRAINT_NAME")
                );
                for (String cName : checkConstraints) {
                    try {
                        jdbcTemplate.execute("ALTER TABLE DECISION_LOGS DROP CONSTRAINT IF EXISTS " + cName);
                    } catch (Exception ignored) {}
                }
            } catch (Exception ignored) {
                try {
                    List<String> checkConstraints = jdbcTemplate.query(
                            "SELECT CONSTRAINT_NAME FROM INFORMATION_SCHEMA.CONSTRAINTS WHERE TABLE_NAME = 'DECISION_LOGS' AND CONSTRAINT_TYPE = 'CHECK'",
                            (rs, rowNum) -> rs.getString("CONSTRAINT_NAME")
                    );
                    for (String cName : checkConstraints) {
                        try {
                            jdbcTemplate.execute("ALTER TABLE DECISION_LOGS DROP CONSTRAINT IF EXISTS " + cName);
                        } catch (Exception ignored2) {}
                    }
                } catch (Exception ignored3) {}
            }

            // Also explicitly drop CONSTRAINT_1 through CONSTRAINT_30 if any match legacy H2 names
            for (int i = 1; i <= 30; i++) {
                try {
                    jdbcTemplate.execute("ALTER TABLE DECISION_LOGS DROP CONSTRAINT IF EXISTS CONSTRAINT_" + i);
                } catch (Exception ignored) {}
            }

            // Drop NOT NULL on REQUEST_ID if present in older H2 schemas
            try {
                jdbcTemplate.execute("ALTER TABLE DECISION_LOGS ALTER COLUMN REQUEST_ID DROP NOT NULL");
            } catch (Exception ignored) {}
        } catch (Exception ignored) {
        }
    }
}
