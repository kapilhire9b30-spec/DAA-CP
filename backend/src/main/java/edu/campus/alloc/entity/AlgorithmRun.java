package edu.campus.alloc.entity;

import jakarta.persistence.*;
import java.time.Instant;

@Entity
@Table(name = "algorithm_runs")
public class AlgorithmRun {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false)
    private String algorithm;

    @Column(nullable = false)
    private Instant runAt;

    @Column(nullable = false)
    private Integer allocatedCount;

    @Column(nullable = false)
    private Integer totalWeight;

    @Column(nullable = false)
    private Double runtimeMs;

    public AlgorithmRun() {
    }

    public AlgorithmRun(String algorithm, Instant runAt, Integer allocatedCount, Integer totalWeight, Double runtimeMs) {
        this.algorithm = algorithm;
        this.runAt = runAt;
        this.allocatedCount = allocatedCount;
        this.totalWeight = totalWeight;
        this.runtimeMs = runtimeMs;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getAlgorithm() {
        return algorithm;
    }

    public void setAlgorithm(String algorithm) {
        this.algorithm = algorithm;
    }

    public Instant getRunAt() {
        return runAt;
    }

    public void setRunAt(Instant runAt) {
        this.runAt = runAt;
    }

    public Integer getAllocatedCount() {
        return allocatedCount;
    }

    public void setAllocatedCount(Integer allocatedCount) {
        this.allocatedCount = allocatedCount;
    }

    public Integer getTotalWeight() {
        return totalWeight;
    }

    public void setTotalWeight(Integer totalWeight) {
        this.totalWeight = totalWeight;
    }

    public Double getRuntimeMs() {
        return runtimeMs;
    }

    public void setRuntimeMs(Double runtimeMs) {
        this.runtimeMs = runtimeMs;
    }
}
