package edu.campus.alloc.entity;

import jakarta.persistence.*;

@Entity
@Table(name = "rooms")
public class Room {

    @Id
    private Long id;

    @Column(nullable = false)
    private String name;

    @Column(nullable = false)
    private String building;

    @Column(nullable = false)
    private Integer capacity; // informational ONLY

    @Column(nullable = false)
    private Integer facilityMask;

    @Column(nullable = false)
    private Boolean active;

    public Room() {
    }

    public Room(Long id, String name, String building, Integer capacity, Integer facilityMask, Boolean active) {
        this.id = id;
        this.name = name;
        this.building = building;
        this.capacity = capacity;
        this.facilityMask = facilityMask;
        this.active = active;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
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

    public Boolean getActive() {
        return active;
    }

    public void setActive(Boolean active) {
        this.active = active;
    }
}
