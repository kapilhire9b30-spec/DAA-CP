import React, { useMemo } from 'react';
import { Room } from '../types/api';

interface RequestLiveSummaryProps {
  title: string;
  date: string;
  startTime: string;
  endTime: string;
  attendance: number | '';
  tier: number;
  facilityMask: number;
  rooms: Room[];
  preferredRoomId: number;
  roomMode: 0 | 1;
}

export const RequestLiveSummary: React.FC<RequestLiveSummaryProps> = ({
  title,
  date,
  startTime,
  endTime,
  attendance,
  tier,
  facilityMask,
  rooms,
  preferredRoomId,
  roomMode,
}) => {
  // Compute duration in hours and minutes
  const durationText = useMemo(() => {
    if (!startTime || !endTime) return '—';
    const [sh, sm] = startTime.split(':').map(Number);
    const [eh, em] = endTime.split(':').map(Number);
    if (isNaN(sh) || isNaN(sm) || isNaN(eh) || isNaN(em)) return '—';

    const startTotal = sh * 60 + sm;
    const endTotal = eh * 60 + em;
    const diff = endTotal - startTotal;

    if (diff <= 0) return 'Invalid interval';

    const h = Math.floor(diff / 60);
    const m = diff % 60;
    if (h === 0) return `${m} mins`;
    if (m === 0) return `${h}h`;
    return `${h}h ${m}m`;
  }, [startTime, endTime]);

  // Timeline bar position within day (08:00 to 22:00 = 840 mins span)
  const timelineStyle = useMemo(() => {
    if (!startTime || !endTime) return null;
    const [sh, sm] = startTime.split(':').map(Number);
    const [eh, em] = endTime.split(':').map(Number);
    if (isNaN(sh) || isNaN(sm) || isNaN(eh) || isNaN(em)) return null;

    const dayStartMin = 8 * 60; // 08:00
    const daySpan = 14 * 60; // 14 hours span
    const startMin = sh * 60 + sm;
    const endMin = eh * 60 + em;

    if (endMin <= startMin) return null;

    const leftPercent = Math.max(0, Math.min(100, ((startMin - dayStartMin) / daySpan) * 100));
    const widthPercent = Math.max(2, Math.min(100 - leftPercent, ((endMin - startMin) / daySpan) * 100));

    return { left: `${leftPercent}%`, width: `${widthPercent}%` };
  }, [startTime, endTime]);

  const tierDetails = useMemo(() => {
    switch (tier) {
      case 1:
        return { name: 'Tier 1 Flagship', weight: 100, color: 'var(--accent)' };
      case 2:
        return { name: 'Tier 2 Major', weight: 50, color: 'var(--primary)' };
      case 3:
        return { name: 'Tier 3 Regular', weight: 20, color: '#C2633A' };
      case 4:
        return { name: 'Tier 4 Informal', weight: 10, color: 'var(--text-muted)' };
      default:
        return { name: 'Tier 3 Regular', weight: 20, color: '#C2633A' };
    }
  }, [tier]);

  const preferredRoom = useMemo(() => {
    if (!preferredRoomId) return null;
    return rooms.find((r) => r.id === preferredRoomId) || null;
  }, [rooms, preferredRoomId]);

  return (
    <aside className="live-summary-card" aria-label="Request Live Summary">
      <div className="summary-header">
        <span className="summary-eyebrow">Real-Time Simulation</span>
        <h3 className="summary-title">Live Summary</h3>
      </div>

      <div className="summary-body">
        {/* Event Title */}
        <div className="summary-row">
          <span className="summary-label">Event:</span>
          <span className="summary-value summary-highlight">
            {title ? title : <em className="summary-placeholder">Untitled Event</em>}
          </span>
        </div>

        {/* Date & Time */}
        <div className="summary-row">
          <span className="summary-label">When:</span>
          <span className="summary-value">
            {date ? date : '—'}
            {startTime && endTime ? ` • ${startTime}–${endTime}` : ''}
          </span>
        </div>

        {/* Duration */}
        <div className="summary-row">
          <span className="summary-label">Duration:</span>
          <span className="summary-value">{durationText}</span>
        </div>

        {/* Timeline Bar Mini Indicator */}
        <div className="summary-timeline-widget">
          <div className="timeline-labels">
            <span>08:00</span>
            <span>12:00</span>
            <span>17:00</span>
            <span>22:00</span>
          </div>
          <div className="timeline-track">
            {timelineStyle ? (
              <div className="timeline-span-fill" style={timelineStyle} />
            ) : (
              <div className="timeline-placeholder-bar" />
            )}
          </div>
        </div>

        {/* Attendance */}
        <div className="summary-row">
          <span className="summary-label">Headcount:</span>
          <span className="summary-value">
            {attendance ? `${attendance} attendees` : '—'}
          </span>
        </div>

        {/* Priority Tier with Weight badge */}
        <div className="summary-row">
          <span className="summary-label">Priority:</span>
          <span className="summary-value">
            <span
              className="summary-tier-badge"
              style={{ borderColor: tierDetails.color, color: tierDetails.color }}
            >
              {tierDetails.name} (wt: {tierDetails.weight})
            </span>
          </span>
        </div>

        {/* Required Facilities */}
        <div className="summary-row">
          <span className="summary-label">Facilities:</span>
          <div className="summary-chips-wrap">
            {facilityMask === 0 && <span className="summary-subtle">Standard Hall (None)</span>}
            {Boolean(facilityMask & 1) && <span className="mini-chip">Projector</span>}
            {Boolean(facilityMask & 2) && <span className="mini-chip">Audio System</span>}
            {Boolean(facilityMask & 4) && <span className="mini-chip">Workstations</span>}
          </div>
        </div>

        {/* Room Preference */}
        <div className="summary-row summary-pref-row">
          <span className="summary-label">Room Pref:</span>
          <div className="summary-pref-details">
            {preferredRoom ? (
              <>
                <span className="summary-room-name">{preferredRoom.name}</span>
                <span className={`summary-mode-tag ${roomMode === 1 ? 'mode-strict' : 'mode-flexible'}`}>
                  {roomMode === 1 ? 'Strict Mode' : 'Flexible Mode'}
                </span>
              </>
            ) : (
              <span className="summary-subtle">No preference (optimizer chooses)</span>
            )}
          </div>
        </div>
      </div>

      <div className="summary-footer">
        <span className="optimizer-badge">
          <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" aria-hidden="true">
            <path d="M12 2v20M17 5H9.5a3.5 3.5 0 0 0 0 7h5a3.5 3.5 0 0 1 0 7H6" />
          </svg>
          Automated Conflict-Free Solver
        </span>
      </div>
    </aside>
  );
};
