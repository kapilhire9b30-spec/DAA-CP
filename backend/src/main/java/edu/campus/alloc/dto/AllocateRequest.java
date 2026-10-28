package edu.campus.alloc.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

public class AllocateRequest {

    @NotBlank(message = "Algorithm is required")
    @Pattern(regexp = "^[ABC]$", message = "Algorithm must be A, B, or C")
    private String algorithm;

    public AllocateRequest() {
    }

    public AllocateRequest(String algorithm) {
        this.algorithm = algorithm;
    }

    public String getAlgorithm() {
        return algorithm;
    }

    public void setAlgorithm(String algorithm) {
        this.algorithm = algorithm;
    }
}
