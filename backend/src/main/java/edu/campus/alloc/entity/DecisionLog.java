package edu.campus.alloc.entity;

import jakarta.persistence.*;
import java.time.Instant;

@Entity
@Table(name = "decision_logs")
public class DecisionLog {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false)
    private String who;

    @Column(name = "logged_at", nullable = false)
    private Instant at;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false)
    private DecisionAction action;

    @Column
    private Long requestId;

    @Column
    private Long roomId;

    @Column
    private String reason;

    @Column(columnDefinition = "TEXT")
    private String details;

    public DecisionLog() {
    }

    public DecisionLog(String who, Instant at, DecisionAction action, Long requestId, Long roomId, String reason, String details) {
        this.who = who;
        this.at = at;
        this.action = action;
        this.requestId = (requestId == null) ? 0L : requestId;
        this.roomId = roomId;
        this.reason = reason;
        this.details = details;
    }

    @PrePersist
    @PreUpdate
    public void prePersist() {
        if (this.requestId == null) {
            this.requestId = 0L;
        }
    }

    @PostLoad
    public void postLoad() {
        if (this.requestId != null && this.requestId == 0L) {
            this.requestId = null;
        }
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getWho() {
        return who;
    }

    public void setWho(String who) {
        this.who = who;
    }

    public Instant getAt() {
        return at;
    }

    public void setAt(Instant at) {
        this.at = at;
    }

    public DecisionAction getAction() {
        return action;
    }

    public void setAction(DecisionAction action) {
        this.action = action;
    }

    public Long getRequestId() {
        return (requestId == null || requestId == 0L) ? null : requestId;
    }

    public void setRequestId(Long requestId) {
        this.requestId = (requestId == null) ? 0L : requestId;
    }

    public Long getRoomId() {
        return roomId;
    }

    public void setRoomId(Long roomId) {
        this.roomId = roomId;
    }

    public String getReason() {
        return reason;
    }

    public void setReason(String reason) {
        this.reason = reason;
    }

    public String getDetails() {
        return details;
    }

    public void setDetails(String details) {
        this.details = details;
    }
}
