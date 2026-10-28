package edu.campus.alloc.service;

import edu.campus.alloc.auth.UserSession;
import edu.campus.alloc.dto.CreateEventRequest;
import edu.campus.alloc.entity.EventRequest;
import edu.campus.alloc.entity.RequestStatus;
import edu.campus.alloc.entity.UserRole;
import edu.campus.alloc.repository.EventRequestRepository;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

import java.util.List;

@Service
public class RequestService {

    private final EventRequestRepository requestRepository;

    public RequestService(EventRequestRepository requestRepository) {
        this.requestRepository = requestRepository;
    }

    @Transactional
    public EventRequest submitRequest(CreateEventRequest dto, UserSession session) {
        if (!dto.getEndTime().isAfter(dto.getStartTime())) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "End time must be after start time");
        }

        String clubName = session.getClubName() != null ? session.getClubName() : "Admin";

        EventRequest request = new EventRequest(
                session.getUserId(),
                clubName,
                dto.getTitle(),
                dto.getDescription(),
                dto.getDate(),
                dto.getStartTime(),
                dto.getEndTime(),
                dto.getAttendance(),
                dto.getFacilityMask(),
                dto.getTier(),
                System.currentTimeMillis(),
                RequestStatus.PENDING
        );

        return requestRepository.save(request);
    }

    public List<EventRequest> getRequests(UserSession session) {
        if (session.getRole() == UserRole.CLUB) {
            if (session.getClubName() != null) {
                return requestRepository.findByClubName(session.getClubName());
            } else {
                return requestRepository.findByUserId(session.getUserId());
            }
        }
        return requestRepository.findAll();
    }
}
