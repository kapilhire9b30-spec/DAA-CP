package edu.campus.alloc.solver.dto;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

@JsonIgnoreProperties(ignoreUnknown = true)
public class AssignmentDto {
    private Long requestId;
    private Long roomId;

    public AssignmentDto() {
    }

    public AssignmentDto(Long requestId, Long roomId) {
        this.requestId = requestId;
        this.roomId = roomId;
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
}
