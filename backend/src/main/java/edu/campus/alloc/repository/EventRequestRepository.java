package edu.campus.alloc.repository;

import edu.campus.alloc.entity.EventRequest;
import edu.campus.alloc.entity.RequestStatus;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface EventRequestRepository extends JpaRepository<EventRequest, Long> {
    List<EventRequest> findByUserId(Long userId);
    List<EventRequest> findByClubName(String clubName);
    List<EventRequest> findByStatus(RequestStatus status);
}
