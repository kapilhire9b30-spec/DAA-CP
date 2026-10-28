package edu.campus.alloc.repository;

import edu.campus.alloc.entity.AlgorithmRun;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface AlgorithmRunRepository extends JpaRepository<AlgorithmRun, Long> {
}
