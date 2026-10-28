import React, { useEffect, useState } from 'react';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { Booking, Room, EventRequestItem } from '../../types/api';
import { SkeletonTable } from '../../components/Skeleton';
import { EmptyBoxIllustration } from '../../components/Icons';

export const BookingsPage: React.FC = () => {
  const { addToast } = useToast();

  const [bookings, setBookings] = useState<Booking[]>([]);
  const [rooms, setRooms] = useState<Room[]>([]);
  const [requests, setRequests] = useState<EventRequestItem[]>([]);
  const [loading, setLoading] = useState(true);

  const [selectedDate, setSelectedDate] = useState<string>('');
  const [searchQuery, setSearchQuery] = useState('');

  const loadData = async () => {
    setLoading(true);
    try {
      const [bookData, roomData, reqData] = await Promise.all([
        apiFetch<Booking[]>('/api/bookings'),
        apiFetch<Room[]>('/api/rooms'),
        apiFetch<EventRequestItem[]>('/api/requests'),
      ]);
      setBookings(bookData);
      setRooms(roomData);
      setRequests(reqData);

      if (bookData.length > 0 && !selectedDate) {
        setSelectedDate(bookData[0].date);
      }
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load bookings schedule', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const getRoomName = (roomId: number) => {
    const r = rooms.find((room) => room.id === roomId);
    return r ? `${r.name} (${r.building})` : `Room #${roomId}`;
  };

  const getRequestDetails = (requestId: number | null) => {
    if (requestId === null) return { title: 'Fixed Reservation', club: 'Institutional' };
    const req = requests.find((r) => r.id === requestId);
    return req ? { title: req.title, club: req.clubName } : { title: `Request #${requestId}`, club: '' };
  };

  const parseTimeToMinutes = (timeStr: string): number => {
    const parts = timeStr.split(':');
    const h = parseInt(parts[0], 10) || 0;
    const m = parseInt(parts[1], 10) || 0;
    return h * 60 + m;
  };

  // Operating window: 08:00 to 20:00 (12 hours)
  const START_HOUR = 8;
  const END_HOUR = 20;
  const TOTAL_HOURS = END_HOUR - START_HOUR;
  const hoursArray = Array.from({ length: TOTAL_HOURS + 1 }, (_, i) => START_HOUR + i);

  /* Schedule Room Palette:
     Auditorium terracotta #B4532A, Seminar Hall forest #1F6F54, Room A saffron #C9831A, Room B olive #6F7A2C.
     Assigned by index, cycling through fixed list for other rooms. */
  const ROOM_COLOR_PALETTE = [
    { bg: 'var(--room-color-1)', text: 'var(--room-text-1)' },
    { bg: 'var(--room-color-2)', text: 'var(--room-text-2)' },
    { bg: 'var(--room-color-3)', text: 'var(--room-text-3)' },
    { bg: 'var(--room-color-4)', text: 'var(--room-text-4)' },
    { bg: 'var(--room-color-5)', text: 'var(--room-text-5)' },
    { bg: 'var(--room-color-6)', text: 'var(--room-text-6)' },
    { bg: 'var(--room-color-7)', text: 'var(--room-text-7)' },
    { bg: 'var(--room-color-8)', text: 'var(--room-text-8)' },
  ];

  // Distinct dates from bookings for date switcher
  const distinctDates = Array.from(new Set(bookings.map((b) => b.date))).sort();

  const gridBookings = bookings.filter((b) => b.date === selectedDate);

  const filteredTableBookings = bookings.filter((b) => {
    if (!searchQuery) return true;
    const q = searchQuery.toLowerCase();
    const req = b.requestId ? requests.find((r) => r.id === b.requestId) : null;
    const room = rooms.find((r) => r.id === b.roomId);
    return (
      b.date.includes(q) ||
      (room && room.name.toLowerCase().includes(q)) ||
      (req && (req.title.toLowerCase().includes(q) || req.clubName.toLowerCase().includes(q))) ||
      b.approvedBy.toLowerCase().includes(q)
    );
  });

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">Confirmed Schedule & Bookings</h1>
          <p className="page-description">
            Verified campus space reservations. (Unconfirmed suggestions/recommendations are never drawn in this schedule).
          </p>
        </div>

        <button type="button" onClick={loadData} className="btn btn-secondary btn-sm" disabled={loading}>
          Refresh
        </button>
      </div>

      {loading ? (
        <SkeletonTable rows={6} />
      ) : (
        <>
          {/* 1. Per-Room Schedule Timeline Grid */}
          <div className="timeline-card">
            <div className="flex-between align-center mb-3">
              <div>
                <h2 className="card-title">Schedule Grid Timeline</h2>
                <p className="card-subtitle">Rooms as rows, time slots horizontally (08:00 to 20:00)</p>
              </div>

              <div className="flex-inline align-center gap-2">
                <label htmlFor="timeline-date-picker" className="text-sm font-medium">
                  Schedule Date:
                </label>
                {distinctDates.length > 0 ? (
                  <select
                    id="timeline-date-picker"
                    value={selectedDate}
                    onChange={(e) => setSelectedDate(e.target.value)}
                  >
                    {distinctDates.map((d) => (
                      <option key={d} value={d}>
                        {d}
                      </option>
                    ))}
                  </select>
                ) : (
                  <input
                    id="timeline-date-picker"
                    type="date"
                    value={selectedDate}
                    onChange={(e) => setSelectedDate(e.target.value)}
                  />
                )}
              </div>
            </div>

            {rooms.length === 0 ? (
              <p className="text-muted">No rooms registered.</p>
            ) : (
              <div className="timeline-wrapper">
                {/* Header Hour Ticks */}
                <div className="timeline-header">
                  <div className="timeline-room-col">Room / Time</div>
                  <div className="timeline-hours-track">
                    {hoursArray.map((hour) => (
                      <div key={hour} className="timeline-hour-tick">
                        {hour.toString().padStart(2, '0')}:00
                      </div>
                    ))}
                  </div>
                </div>

                {/* Body Rows */}
                <div className="timeline-body">
                  {rooms.map((room, roomIdx) => {
                    const roomEvents = gridBookings.filter((b) => b.roomId === room.id);
                    const roomColor = ROOM_COLOR_PALETTE[roomIdx % ROOM_COLOR_PALETTE.length];

                    return (
                      <div key={room.id} className="timeline-row">
                        <div className="timeline-room-info">
                          <span className="timeline-room-title">{room.name}</span>
                          <span className="timeline-room-meta">{room.building} • Cap: {room.capacity}</span>
                        </div>

                        <div className="timeline-events-track">
                          {/* Hour Dividers */}
                          {hoursArray.map((hour) => (
                            <div key={hour} className="timeline-hour-slot" />
                          ))}

                          {/* Event Blocks */}
                          {roomEvents.map((b) => {
                            const startMin = parseTimeToMinutes(b.startTime);
                            const endMin = parseTimeToMinutes(b.endTime);

                            const gridStartMin = START_HOUR * 60;
                            const gridTotalMin = TOTAL_HOURS * 60;

                            const leftPercent = Math.max(0, ((startMin - gridStartMin) / gridTotalMin) * 100);
                            const widthPercent = Math.max(3, ((endMin - startMin) / gridTotalMin) * 100);

                            const details = getRequestDetails(b.requestId);
                            const isInactive = b.roomActive === false || !room.active;

                            return (
                              <div
                                key={b.id}
                                className="timeline-block"
                                style={{
                                  left: `${leftPercent}%`,
                                  width: `${widthPercent}%`,
                                  backgroundColor: isInactive ? 'var(--surface-2)' : roomColor.bg,
                                  color: isInactive ? 'var(--text-muted)' : roomColor.text,
                                  border: isInactive ? '1.5px dashed var(--text-muted)' : undefined,
                                  opacity: isInactive ? 0.8 : 1,
                                }}
                                title={
                                  isInactive
                                    ? `[Room Inactive] ${details.title} [${details.club}]\n${b.startTime} - ${b.endTime}\nRoom: ${room.name} (INACTIVE)\nApproved by: ${b.approvedBy}\nWarning: This room is currently inactive!`
                                    : `${details.title} [${details.club}]\n${b.startTime} - ${b.endTime}\nRoom: ${room.name}\nApproved by: ${b.approvedBy}`
                                }
                              >
                                <span className="timeline-block-title">{details.title}</span>
                                <span className="timeline-block-time">
                                  {b.startTime.slice(0, 5)} - {b.endTime.slice(0, 5)}
                                </span>
                              </div>
                            );
                          })}
                        </div>
                      </div>
                    );
                  })}
                </div>
              </div>
            )}
          </div>

          {/* 2. Confirmed Bookings Table */}
          <div className="card">
            <div className="card-header flex-between align-center">
              <div>
                <h2 className="card-title">All Confirmed Bookings Record</h2>
                <p className="card-subtitle">Complete log of active bookings in database</p>
              </div>

              <input
                type="text"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Search bookings..."
                style={{ maxWidth: '240px' }}
              />
            </div>

            {filteredTableBookings.length === 0 ? (
              <div className="empty-state">
                <EmptyBoxIllustration size={80} />
                <h3 className="empty-state-title">No bookings matched</h3>
                <p className="empty-state-desc">Try clearing the search query or verifying the date.</p>
              </div>
            ) : (
              <div className="table-responsive">
                <table className="data-table">
                  <thead>
                    <tr>
                      <th>ID</th>
                      <th>Room</th>
                      <th>Date</th>
                      <th>Time Window</th>
                      <th>Event / Details</th>
                      <th>Approved By</th>
                      <th>Created</th>
                    </tr>
                  </thead>
                  <tbody>
                    {filteredTableBookings.map((b) => {
                      const details = getRequestDetails(b.requestId);
                      return (
                        <tr key={b.id}>
                          <td><strong>#{b.id}</strong></td>
                          <td>
                            {getRoomName(b.roomId)}
                            {(b.roomActive === false || rooms.find((r) => r.id === b.roomId)?.active === false) && (
                              <span className="badge badge-rejected" style={{ marginLeft: '8px', fontSize: '0.7rem' }}>
                                Room inactive
                              </span>
                            )}
                          </td>
                          <td>{b.date}</td>
                          <td>{b.startTime} - {b.endTime}</td>
                          <td>
                            <strong>{details.title}</strong>
                            {details.club && <div className="text-xs text-muted">Club: {details.club}</div>}
                          </td>
                          <td><span className="font-mono text-muted">{b.approvedBy}</span></td>
                          <td><span className="text-xs text-muted">{b.createdAt}</span></td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            )}
          </div>
        </>
      )}
    </div>
  );
};
