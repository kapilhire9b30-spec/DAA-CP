package edu.campus.alloc.dto;

import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

public class CreateRoomRequest {
    @NotBlank(message = "Room name is required")
    private String name;

    @NotBlank(message = "Building is required")
    private String building;

    @NotNull(message = "Capacity is required")
    @Min(value = 0, message = "Capacity must be non-negative")
    private Integer capacity;

    @NotNull(message = "Facility mask is required")
    @Min(value = 0, message = "Facility mask must be non-negative")
    private Integer facilityMask;

    public CreateRoomRequest() {
    }

    public CreateRoomRequest(String name, String building, Integer capacity, Integer facilityMask) {
        this.name = name;
        this.building = building;
        this.capacity = capacity;
        this.facilityMask = facilityMask;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getBuilding() {
        return building;
    }

    public void setBuilding(String building) {
        this.building = building;
    }

    public Integer getCapacity() {
        return capacity;
    }

    public void setCapacity(Integer capacity) {
        this.capacity = capacity;
    }

    public Integer getFacilityMask() {
        return facilityMask;
    }

    public void setFacilityMask(Integer facilityMask) {
        this.facilityMask = facilityMask;
    }
}
