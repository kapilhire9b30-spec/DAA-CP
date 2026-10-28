package edu.campus.alloc.repository;

import edu.campus.alloc.entity.DecisionLog;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface DecisionLogRepository extends JpaRepository<DecisionLog, Long> {
    List<DecisionLog> findAllByOrderByAtDesc();
}
