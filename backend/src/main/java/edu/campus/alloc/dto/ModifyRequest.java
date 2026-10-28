package edu.campus.alloc.dto;

import jakarta.validation.constraints.NotNull;

public class ModifyRequest {

    @NotNull(message = "Target roomId is required")
    private Long roomId;

    public ModifyRequest() {
    }

    public ModifyRequest(Long roomId) {
        this.roomId = roomId;
    }

    public Long getRoomId() {
        return roomId;
    }

    public void setRoomId(Long roomId) {
        this.roomId = roomId;
    }
}
