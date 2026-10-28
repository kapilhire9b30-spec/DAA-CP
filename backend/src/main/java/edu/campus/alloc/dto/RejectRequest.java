package edu.campus.alloc.dto;

import jakarta.validation.constraints.NotBlank;

public class RejectRequest {

    @NotBlank(message = "Rejection reason is required")
    private String reason;

    public RejectRequest() {
    }

    public RejectRequest(String reason) {
        this.reason = reason;
    }

    public String getReason() {
        return reason;
    }

    public void setReason(String reason) {
        this.reason = reason;
    }
}
