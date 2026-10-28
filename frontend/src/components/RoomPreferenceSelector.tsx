import React, { useState, useMemo } from 'react';
import { Room } from '../types/api';

export type RoomPreferenceMode = 0 | 1; // 0 = Flexible, 1 = Strict

interface RoomPreferenceSelectorProps {
  rooms: Room[];
  loadingRooms: boolean;
  preferredRoomId: number;
  roomMode: RoomPreferenceMode;
  attendance: number | '';
  requiredFacilityMask: number;
  onRoomChange: (roomId: number) => void;
  onModeChange: (mode: RoomPreferenceMode) => void;
}

export const RoomPreferenceSelector: React.FC<RoomPreferenceSelectorProps> = ({
  rooms,
  loadingRooms,
  preferredRoomId,
  roomMode,
  attendance,
  requiredFacilityMask,
  onRoomChange,
  onModeChange,
}) => {
  const [searchQuery, setSearchQuery] = useState('');

  // Filter to active rooms only and apply search query
  const activeRooms = useMemo(() => {
    return rooms.filter((r) => r.active);
  }, [rooms]);

  const filteredRooms = useMemo(() => {
    if (!searchQuery.trim()) return activeRooms;
    const q = searchQuery.toLowerCase();
    return activeRooms.filter(
      (r) =>
        r.name.toLowerCase().includes(q) ||
        r.building.toLowerCase().includes(q) ||
        r.id.toString().includes(q)
    );
  }, [activeRooms, searchQuery]);

  const selectedRoom = useMemo(() => {
    return activeRooms.find((r) => r.id === preferredRoomId) || null;
  }, [activeRooms, preferredRoomId]);

  // Live non-blocking warnings
  const capacityWarning = useMemo(() => {
    if (!selectedRoom || !attendance || Number(attendance) <= 0) return null;
    if (selectedRoom.capacity < Number(attendance)) {
      return `Room capacity (${selectedRoom.capacity}) is below the entered headcount (${attendance}). Capacity is informational, but the venue may be crowded.`;
    }
    return null;
  }, [selectedRoom, attendance]);

  const facilityWarning = useMemo(() => {
    if (!selectedRoom || requiredFacilityMask === 0) return null;
    const missing = requiredFacilityMask & ~selectedRoom.facilityMask;
    if (missing !== 0) {
      const missingNames: string[] = [];
      if (missing & 1) missingNames.push('Projector (Bit 0)');
      if (missing & 2) missingNames.push('Audio System (Bit 1)');
      if (missing & 4) missingNames.push('Workstations (Bit 2)');

      return `${selectedRoom.name} lacks required equipment: ${missingNames.join(', ')}.`;
    }
    return null;
  }, [selectedRoom, requiredFacilityMask]);

  return (
    <div className="room-preference-box">
      <div className="preference-header">
        <label htmlFor="room-search-input" className="form-label-styled">
          Preferred Campus Room
        </label>
        <span className="preference-help-text">
          Optional. Guide the solver to place your event in a specific space.
        </span>
      </div>

      {loadingRooms ? (
        <div className="room-selector-loading">Loading active campus rooms...</div>
      ) : (
        <div className="room-select-group">
          {/* Search filter input */}
          <div className="room-search-wrapper">
            <svg
              className="search-icon"
              width="15"
              height="15"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
              aria-hidden="true"
            >
              <circle cx="11" cy="11" r="8" />
              <line x1="21" y1="21" x2="16.65" y2="16.65" />
            </svg>
            <input
              id="room-search-input"
              type="text"
              placeholder="Search active rooms by name, building, or #id..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="room-search-input"
            />
            {searchQuery && (
              <button
                type="button"
                onClick={() => setSearchQuery('')}
                className="search-clear-btn"
                aria-label="Clear search query"
              >
                ✕
              </button>
            )}
          </div>

          {/* Room Selector Dropdown */}
          <select
            id="room-select-dropdown"
            className="room-select-dropdown"
            value={preferredRoomId}
            onChange={(e) => onRoomChange(Number(e.target.value))}
            aria-label="Select preferred room"
          >
            <option value={0}>✨ No preference (let the optimizer decide)</option>
            {filteredRooms.map((r) => {
              const facList: string[] = [];
              if (r.facilityMask & 1) facList.push('Projector');
              if (r.facilityMask & 2) facList.push('Audio');
              if (r.facilityMask & 4) facList.push('Workstations');
              const facText = facList.length > 0 ? ` • ${facList.join(', ')}` : '';

              return (
                <option key={r.id} value={r.id}>
                  {r.name} ({r.building}) — Cap: {r.capacity}
                  {facText}
                </option>
              );
            })}
          </select>

          {/* Segmented Control when a room is selected */}
          {preferredRoomId !== 0 && selectedRoom && (
            <div className="preference-mode-container" role="region" aria-label="Room Preference Mode">
              <span className="mode-label">Allocation Mode:</span>
              <div className="segmented-control" role="radiogroup" aria-label="Preference mode">
                <button
                  type="button"
                  role="radio"
                  aria-checked={roomMode === 0}
                  className={`segment-btn ${roomMode === 0 ? 'active' : ''}`}
                  onClick={() => onModeChange(0)}
                >
                  <span className="segment-title">Flexible (Default)</span>
                  <span className="segment-subtext">
                    Try this room first; fall back to another if busy
                  </span>
                </button>

                <button
                  type="button"
                  role="radio"
                  aria-checked={roomMode === 1}
                  className={`segment-btn ${roomMode === 1 ? 'active' : ''}`}
                  onClick={() => onModeChange(1)}
                >
                  <span className="segment-title">Strict</span>
                  <span className="segment-subtext">
                    Only this room. If unavailable, request stays unallocated
                  </span>
                </button>
              </div>

              {/* Live Non-Blocking Warnings */}
              {(capacityWarning || facilityWarning) && (
                <div className="preference-warnings" role="status">
                  {capacityWarning && (
                    <div className="amber-hint-pill">
                      <span className="hint-icon" aria-hidden="true">⚠️</span>
                      <span>{capacityWarning}</span>
                    </div>
                  )}

                  {facilityWarning && (
                    <div className={`amber-hint-pill ${roomMode === 1 ? 'strict-alert' : ''}`}>
                      <span className="hint-icon" aria-hidden="true">⚠️</span>
                      <span>
                        {facilityWarning}{' '}
                        {roomMode === 1 && (
                          <strong>
                            Strict mode will fail for this room unless facilities match.
                          </strong>
                        )}
                      </span>
                    </div>
                  )}
                </div>
              )}
            </div>
          )}
        </div>
      )}
    </div>
  );
};
