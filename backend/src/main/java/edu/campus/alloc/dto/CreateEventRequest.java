package edu.campus.alloc.dto;

import com.fasterxml.jackson.annotation.JsonFormat;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

import java.time.LocalDate;
import java.time.LocalTime;

public class CreateEventRequest {

    @NotBlank(message = "Title is required")
    private String title;

    private String description;

    @NotNull(message = "Date is required (yyyy-MM-dd)")
    @JsonFormat(pattern = "yyyy-MM-dd")
    private LocalDate date;

    @NotNull(message = "Start time is required (HH:mm)")
    @JsonFormat(pattern = "HH:mm")
    private LocalTime startTime;

    @NotNull(message = "End time is required (HH:mm)")
    @JsonFormat(pattern = "HH:mm")
    private LocalTime endTime;

    @NotNull(message = "Attendance is required")
    @Min(value = 1, message = "Attendance must be at least 1")
    private Integer attendance;

    @NotNull(message = "Facility mask is required")
    @Min(value = 0, message = "Facility mask must be non-negative")
    private Integer facilityMask;

    @NotNull(message = "Tier is required (1..4)")
    @Min(value = 1, message = "Tier must be between 1 and 4")
    @Max(value = 4, message = "Tier must be between 1 and 4")
    private Integer tier;

    public CreateEventRequest() {
    }

    public CreateEventRequest(String title, String description, LocalDate date, LocalTime startTime, LocalTime endTime,
                              Integer attendance, Integer facilityMask, Integer tier) {
        this.title = title;
        this.description = description;
        this.date = date;
        this.startTime = startTime;
        this.endTime = endTime;
        this.attendance = attendance;
        this.facilityMask = facilityMask;
        this.tier = tier;
    }

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public LocalDate getDate() {
        return date;
    }

    public void setDate(LocalDate date) {
        this.date = date;
    }

    public LocalTime getStartTime() {
        return startTime;
    }

    public void setStartTime(LocalTime startTime) {
        this.startTime = startTime;
    }

    public LocalTime getEndTime() {
        return endTime;
    }

    public void setEndTime(LocalTime endTime) {
        this.endTime = endTime;
    }

    public Integer getAttendance() {
        return attendance;
    }

    public void setAttendance(Integer attendance) {
        this.attendance = attendance;
    }

    public Integer getFacilityMask() {
        return facilityMask;
    }

    public void setFacilityMask(Integer facilityMask) {
        this.facilityMask = facilityMask;
    }

    public Integer getTier() {
        return tier;
    }

    public void setTier(Integer tier) {
        this.tier = tier;
    }
}
