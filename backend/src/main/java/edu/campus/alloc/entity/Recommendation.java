package edu.campus.alloc.entity;

import jakarta.persistence.*;
import java.time.Instant;

@Entity
@Table(name = "recommendations")
public class Recommendation {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false)
    private Long requestId;

    @Column(nullable = false)
    private Long roomId;

    @Column(nullable = false)
    private String algorithm;

    @Column(nullable = false)
    private Long runId;

    @Column(nullable = false)
    private Instant createdAt;

    @Column(nullable = false)
    private Boolean active;

    public Recommendation() {
    }

    public Recommendation(Long requestId, Long roomId, String algorithm, Long runId, Instant createdAt, Boolean active) {
        this.requestId = requestId;
        this.roomId = roomId;
        this.algorithm = algorithm;
        this.runId = runId;
        this.createdAt = createdAt;
        this.active = active;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public Long getRequestId() {
        return requestId;
    }

    public void setRequestId(Long requestId) {
        this.requestId = requestId;
    }

    public Long getRoomId() {
        return roomId;
    }

    public void setRoomId(Long roomId) {
        this.roomId = roomId;
    }

    public String getAlgorithm() {
        return algorithm;
    }

    public void setAlgorithm(String algorithm) {
        this.algorithm = algorithm;
    }

    public Long getRunId() {
        return runId;
    }

    public void setRunId(Long runId) {
        this.runId = runId;
    }

    public Instant getCreatedAt() {
        return createdAt;
    }

    public void setCreatedAt(Instant createdAt) {
        this.createdAt = createdAt;
    }

    public Boolean getActive() {
        return active;
    }

    public void setActive(Boolean active) {
        this.active = active;
    }
}
