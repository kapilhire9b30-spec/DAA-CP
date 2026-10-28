import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { useAuth } from '../../context/AuthContext';
import { useToast } from '../../context/ToastContext';
import { apiFetch, ApiError } from '../../services/api';
import { EventRequestItem, Booking, Room } from '../../types/api';
import { StatusBadge } from '../../components/StatusBadge';
import { SkeletonCard, SkeletonTable } from '../../components/Skeleton';
import { ClockIcon, CheckIcon, SparklesIcon, XIcon, EmptyBoxIllustration } from '../../components/Icons';

export const ClubDashboardPage: React.FC = () => {
  const { user } = useAuth();
  const { addToast } = useToast();

  const [requests, setRequests] = useState<EventRequestItem[]>([]);
  const [bookings, setBookings] = useState<Booking[]>([]);
  const [rooms, setRooms] = useState<Room[]>([]);
  const [loading, setLoading] = useState(true);

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
        addToast('Failed to load club requests', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const pendingCount = requests.filter((r) => r.status === 'PENDING').length;
  const recommendedCount = requests.filter((r) => r.status === 'RECOMMENDED').length;
  const confirmedCount = requests.filter((r) => r.status === 'CONFIRMED').length;
  const rejectedCount = requests.filter((r) => r.status === 'REJECTED').length;

  const getConfirmedBooking = (reqId: number): Booking | undefined => {
    return bookings.find((b) => b.requestId === reqId);
  };

  const getRoomName = (roomId: number) => {
    const r = rooms.find((room) => room.id === roomId);
    return r ? `${r.name} (${r.building})` : `Room #${roomId}`;
  };

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">{user?.clubName || 'Club'} Space Management</h1>
          <p className="page-description">
            Submit resource requests and track optimization progress for upcoming club events
          </p>
        </div>

        <Link to="/club/submit" className="btn btn-primary btn-lg">
          + New Event Request
        </Link>
      </div>

      {loading ? (
        <>
          <SkeletonCard count={4} />
          <div style={{ marginTop: '1.5rem' }}>
            <SkeletonTable rows={4} />
          </div>
        </>
      ) : (
        <>
          {/* Stat Cards */}
          <div className="stat-grid">
            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">Submitted</span>
                <ClockIcon size={16} className="text-muted" />
              </div>
              <div className="stat-value">{requests.length}</div>
              <div className="stat-desc">Total requests submitted</div>
            </div>

            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">In Review</span>
                <SparklesIcon size={16} className="text-info" />
              </div>
              <div className="stat-value">{pendingCount + recommendedCount}</div>
              <div className="stat-desc">{recommendedCount} proposed by solver</div>
            </div>

            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">Confirmed</span>
                <CheckIcon size={16} className="text-success" />
              </div>
              <div className="stat-value">{confirmedCount}</div>
              <div className="stat-desc">Room allocated & verified</div>
            </div>

            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">Rejected</span>
                <XIcon size={16} className="text-danger" />
              </div>
              <div className="stat-value">{rejectedCount}</div>
              <div className="stat-desc">Declined by admin</div>
            </div>
          </div>

          {/* Recent Requests Section */}
          <div className="card">
            <div className="card-header">
              <div>
                <h2 className="card-title">Recent Submissions</h2>
                <p className="card-subtitle">Latest status updates for your club's reservations</p>
              </div>

              <Link to="/club/my-requests" className="btn btn-secondary btn-sm">
                View All Requests &rarr;
              </Link>
            </div>

            {requests.length === 0 ? (
              <div className="empty-state">
                <EmptyBoxIllustration size={80} />
                <h3 className="empty-state-title">No event requests yet</h3>
                <p className="empty-state-desc">
                  You haven't submitted any room allocation requests yet. Click "New Event Request" to get started!
                </p>
                <Link to="/club/submit" className="btn btn-primary">
                  Submit Your First Request
                </Link>
              </div>
            ) : (
              <div className="table-responsive">
                <table className="data-table">
                  <thead>
                    <tr>
                      <th>ID</th>
                      <th>Title</th>
                      <th>Date & Time</th>
                      <th>Attendance</th>
                      <th>Status</th>
                      <th>Confirmed Details / Feedback</th>
                    </tr>
                  </thead>
                  <tbody>
                    {requests.slice(0, 5).map((req) => {
                      const confirmed = getConfirmedBooking(req.id);
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
                          <td>{req.attendance} attendees</td>
                          <td>
                            <StatusBadge status={req.status} />
                          </td>
                          <td>
                            {req.status === 'CONFIRMED' && confirmed && (
                              <div className="text-success text-sm font-medium">
                                ✓ Room: {getRoomName(confirmed.roomId)}
                              </div>
                            )}
                            {req.status === 'REJECTED' && (
                              <div className="text-danger text-sm">
                                <strong>Reason:</strong> {req.rejectionReason || 'No reason provided'}
                              </div>
                            )}
                            {req.status === 'RECOMMENDED' && (
                              <span className="text-info text-sm">Proposed by solver; awaiting final admin approval</span>
                            )}
                            {req.status === 'PENDING' && (
                              <span className="text-muted text-sm">Waiting for allocation run</span>
                            )}
                          </td>
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
