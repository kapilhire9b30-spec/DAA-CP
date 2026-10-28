package edu.campus.alloc.repository;

import edu.campus.alloc.entity.Recommendation;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface RecommendationRepository extends JpaRepository<Recommendation, Long> {
    List<Recommendation> findByActiveTrue();
    List<Recommendation> findByRequestId(Long requestId);
}
