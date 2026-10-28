import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { EventRequestItem, Booking, Room } from '../../types/api';
import { StatusBadge, getFacilityNames } from '../../components/StatusBadge';
import { SkeletonTable } from '../../components/Skeleton';
import { EmptyBoxIllustration } from '../../components/Icons';

export const MyRequestsPage: React.FC = () => {
  const { addToast } = useToast();

  const [requests, setRequests] = useState<EventRequestItem[]>([]);
  const [bookings, setBookings] = useState<Booking[]>([]);
  const [rooms, setRooms] = useState<Room[]>([]);
  const [loading, setLoading] = useState(true);
  const [filterStatus, setFilterStatus] = useState<string>('ALL');

  const loadData = async () => {
    setLoading(true);
    try {
      const [reqData, bookData, roomData] = await Promise.all([
        apiFetch<EventRequestItem[]>('/api/requests'),
        apiFetch<Booking[]>('/api/bookings'),
        apiFetch<Room[]>('/api/rooms'),
      ]);
      setRequests(reqData);
      setBookings(bookData);
      setRooms(roomData);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load your club requests', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const getConfirmedBooking = (reqId: number): Booking | undefined => {
    return bookings.find((b) => b.requestId === reqId);
  };

  const getRoomName = (roomId: number): string => {
    const r = rooms.find((room) => room.id === roomId);
    return r ? `${r.name} (${r.building})` : `Room #${roomId}`;
  };

  const filteredRequests = requests.filter((r) => {
    if (filterStatus === 'ALL') return true;
    return r.status === filterStatus;
  });

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">My Event Requests</h1>
          <p className="page-description">
            Track status, approved rooms, and feedback on all submissions made by your organization
          </p>
        </div>

        <div className="btn-group">
          <button type="button" onClick={loadData} className="btn btn-secondary btn-sm" disabled={loading}>
            Refresh
          </button>
          <Link to="/club/submit" className="btn btn-primary btn-sm">
            + New Request
          </Link>
        </div>
      </div>

      {/* Filter Chips */}
      <div className="filter-chip-group">
        <button
          type="button"
          className={`filter-chip ${filterStatus === 'ALL' ? 'active' : ''}`}
          onClick={() => setFilterStatus('ALL')}
        >
          All ({requests.length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterStatus === 'PENDING' ? 'active' : ''}`}
          onClick={() => setFilterStatus('PENDING')}
        >
          Pending ({requests.filter((r) => r.status === 'PENDING').length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterStatus === 'RECOMMENDED' ? 'active' : ''}`}
          onClick={() => setFilterStatus('RECOMMENDED')}
        >
          Recommended ({requests.filter((r) => r.status === 'RECOMMENDED').length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterStatus === 'CONFIRMED' ? 'active' : ''}`}
          onClick={() => setFilterStatus('CONFIRMED')}
        >
          Confirmed ({requests.filter((r) => r.status === 'CONFIRMED').length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterStatus === 'REJECTED' ? 'active' : ''}`}
          onClick={() => setFilterStatus('REJECTED')}
        >
          Rejected ({requests.filter((r) => r.status === 'REJECTED').length})
        </button>
      </div>

      {loading ? (
        <SkeletonTable rows={5} />
      ) : filteredRequests.length === 0 ? (
        <div className="card empty-state">
          <EmptyBoxIllustration size={80} />
          <h3 className="empty-state-title">No requests found</h3>
          <p className="empty-state-desc">You do not have any requests matching this category.</p>
          <Link to="/club/submit" className="btn btn-primary btn-sm">
            Submit New Request
          </Link>
        </div>
      ) : (
        <div className="table-container">
          <div className="table-responsive">
            <table className="data-table">
              <thead>
                <tr>
                  <th>ID</th>
                  <th>Event Title & Summary</th>
                  <th>Date & Time</th>
                  <th>Attendance</th>
                  <th>Facilities</th>
                  <th>Tier</th>
                  <th>Status</th>
                  <th>Allocation / Feedback</th>
                </tr>
              </thead>
              <tbody>
                {filteredRequests.map((req) => {
                  const confirmedBooking = getConfirmedBooking(req.id);
                  return (
                    <tr key={req.id}>
                      <td><strong>#{req.id}</strong></td>
                      <td>
                        <strong>{req.title}</strong>
                        <div className="text-xs text-muted">{req.description}</div>
                      </td>
                      <td>
                        <div>{req.date}</div>
                        <div className="text-xs text-muted">{req.startTime} - {req.endTime}</div>
                      </td>
                      <td>{req.attendance}</td>
                      <td>
                        <span className="text-xs">{getFacilityNames(req.facilityMask)}</span>
                      </td>
                      <td>Tier {req.tier}</td>
                      <td>
                        <StatusBadge status={req.status} />
                      </td>
                      <td>
                        {req.status === 'CONFIRMED' && (
                          <div className="text-success text-sm font-medium">
                            <div>✓ Confirmed in:</div>
                            {confirmedBooking ? (
                              <div>
                                <strong>{getRoomName(confirmedBooking.roomId)}</strong>
                                <div className="text-xs text-muted">
                                  Time: {confirmedBooking.startTime} - {confirmedBooking.endTime}
                                </div>
                              </div>
                            ) : (
                              <div className="text-xs text-muted">Confirmed on active schedule</div>
                            )}
                          </div>
                        )}

                        {req.status === 'REJECTED' && (
                          <div className="text-danger text-sm">
                            <strong>Reason: </strong>
                            <span>{req.rejectionReason || 'No reason provided'}</span>
                          </div>
                        )}

                        {req.status === 'PENDING' && (
                          <span className="text-muted text-sm">Awaiting optimization run</span>
                        )}

                        {req.status === 'RECOMMENDED' && (
                          <span className="text-info text-sm">Proposed by solver; awaiting final admin confirmation</span>
                        )}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </div>
  );
};
