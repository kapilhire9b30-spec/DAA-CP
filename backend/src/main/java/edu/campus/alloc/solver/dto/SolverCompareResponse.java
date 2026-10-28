package edu.campus.alloc.solver.dto;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

import java.util.ArrayList;
import java.util.List;

@JsonIgnoreProperties(ignoreUnknown = true)
public class SolverCompareResponse {
    private boolean ok;
    private String error;
    private List<SolverRowDto> rows = new ArrayList<>();

    public SolverCompareResponse() {
    }

    public boolean isOk() {
        return ok;
    }

    public void setOk(boolean ok) {
        this.ok = ok;
    }

    public String getError() {
        return error;
    }

    public void setError(String error) {
        this.error = error;
    }

    public List<SolverRowDto> getRows() {
        return rows;
    }

    public void setRows(List<SolverRowDto> rows) {
        this.rows = rows;
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    public static class SolverRowDto {
        private String algorithm;
        private List<AssignmentDto> assignments = new ArrayList<>();
        private List<UnallocatedDto> unallocated = new ArrayList<>();
        private Integer totalWeight;
        private Integer allocatedCount;
        private Double runtimeMs;
        private List<String> violations = new ArrayList<>();
        private List<String> warnings = new ArrayList<>();

        public SolverRowDto() {
        }

        public String getAlgorithm() {
            return algorithm;
        }

        public void setAlgorithm(String algorithm) {
            this.algorithm = algorithm;
        }

        public List<AssignmentDto> getAssignments() {
            return assignments;
        }

        public void setAssignments(List<AssignmentDto> assignments) {
            this.assignments = assignments;
        }

        public List<UnallocatedDto> getUnallocated() {
            return unallocated;
        }

        public void setUnallocated(List<UnallocatedDto> unallocated) {
            this.unallocated = unallocated;
        }

        public Integer getTotalWeight() {
            return totalWeight;
        }

        public void setTotalWeight(Integer totalWeight) {
            this.totalWeight = totalWeight;
        }

        public Integer getAllocatedCount() {
            return allocatedCount;
        }

        public void setAllocatedCount(Integer allocatedCount) {
            this.allocatedCount = allocatedCount;
        }

        public Double getRuntimeMs() {
            return runtimeMs;
        }

        public void setRuntimeMs(Double runtimeMs) {
            this.runtimeMs = runtimeMs;
        }

        public List<String> getViolations() {
            return violations;
        }

        public void setViolations(List<String> violations) {
            this.violations = violations;
        }

        public List<String> getWarnings() {
            return warnings;
        }

        public void setWarnings(List<String> warnings) {
            this.warnings = warnings;
        }
    }
}
