package edu.campus.alloc.repository;

import edu.campus.alloc.entity.Room;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface RoomRepository extends JpaRepository<Room, Long> {
    List<Room> findByActiveTrue();

    @Query("SELECT COALESCE(MAX(r.id), 100) FROM Room r")
    Long findMaxId();
}
