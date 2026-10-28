package edu.campus.alloc.repository;

import edu.campus.alloc.entity.Booking;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface BookingRepository extends JpaRepository<Booking, Long> {
    Optional<Booking> findByRequestId(Long requestId);
    List<Booking> findByRoomId(Long roomId);
}
