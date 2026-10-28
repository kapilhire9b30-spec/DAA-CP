package edu.campus.alloc.dto;

import edu.campus.alloc.entity.EventRequest;
import edu.campus.alloc.entity.Room;

import java.time.Instant;
import java.util.List;

public class RecommendationResponse {
    private Long id;
    private EventRequest request;
    private Room proposedRoom;
    private String algorithm;
    private boolean capacityWarning; // true if proposedRoom.capacity < request.attendance
    private List<Room> otherFeasibleRooms; // obtained from solver validate, never Java
    private Instant createdAt;
    private Boolean active;

    public RecommendationResponse() {
    }

    public RecommendationResponse(Long id, EventRequest request, Room proposedRoom, String algorithm,
                                  boolean capacityWarning, List<Room> otherFeasibleRooms,
                                  Instant createdAt, Boolean active) {
        this.id = id;
        this.request = request;
        this.proposedRoom = proposedRoom;
        this.algorithm = algorithm;
        this.capacityWarning = capacityWarning;
        this.otherFeasibleRooms = otherFeasibleRooms;
        this.createdAt = createdAt;
        this.active = active;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public EventRequest getRequest() {
        return request;
    }

    public void setRequest(EventRequest request) {
        this.request = request;
    }

    public Room getProposedRoom() {
        return proposedRoom;
    }

    public void setProposedRoom(Room proposedRoom) {
        this.proposedRoom = proposedRoom;
    }

    public String getAlgorithm() {
        return algorithm;
    }

    public void setAlgorithm(String algorithm) {
        this.algorithm = algorithm;
    }

    public boolean isCapacityWarning() {
        return capacityWarning;
    }

    public void setCapacityWarning(boolean capacityWarning) {
        this.capacityWarning = capacityWarning;
    }

    public List<Room> getOtherFeasibleRooms() {
        return otherFeasibleRooms;
    }

    public void setOtherFeasibleRooms(List<Room> otherFeasibleRooms) {
        this.otherFeasibleRooms = otherFeasibleRooms;
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
