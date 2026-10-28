package edu.campus.alloc.dto;

import edu.campus.alloc.entity.Room;
import java.util.List;

public class RoomDeactivationResponse {

    private Room room;
    private List<AffectedBookingDto> affectedBookings;

    public RoomDeactivationResponse() {
    }

    public RoomDeactivationResponse(Room room, List<AffectedBookingDto> affectedBookings) {
        this.room = room;
        this.affectedBookings = affectedBookings;
    }

    public Room getRoom() {
        return room;
    }

    public void setRoom(Room room) {
        this.room = room;
    }

    public List<AffectedBookingDto> getAffectedBookings() {
        return affectedBookings;
    }

    public void setAffectedBookings(List<AffectedBookingDto> affectedBookings) {
        this.affectedBookings = affectedBookings;
    }

    // Direct accessors matching Room properties for backwards compatibility
    public Long getId() {
        return room != null ? room.getId() : null;
    }

    public String getName() {
        return room != null ? room.getName() : null;
    }

    public String getBuilding() {
        return room != null ? room.getBuilding() : null;
    }

    public Integer getCapacity() {
        return room != null ? room.getCapacity() : null;
    }

    public Integer getFacilityMask() {
        return room != null ? room.getFacilityMask() : null;
    }

    public Boolean getActive() {
        return room != null ? room.getActive() : null;
    }
}
