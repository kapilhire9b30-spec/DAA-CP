package edu.campus.alloc.solver.dto;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

@JsonIgnoreProperties(ignoreUnknown = true)
public class UnallocatedDto {
    private Long requestId;
    private String reason;

    public UnallocatedDto() {
    }

    public UnallocatedDto(Long requestId, String reason) {
        this.requestId = requestId;
        this.reason = reason;
    }

    public Long getRequestId() {
        return requestId;
    }

    public void setRequestId(Long requestId) {
        this.requestId = requestId;
    }

    public String getReason() {
        return reason;
    }

    public void setReason(String reason) {
        this.reason = reason;
    }
}
