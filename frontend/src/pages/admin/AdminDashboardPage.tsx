import React, { useEffect, useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { EventRequestItem, RecommendationItem, Booking, Room, AllocateResponse } from '../../types/api';
import { SkeletonCard, SkeletonTable } from '../../components/Skeleton';
import { SparklesIcon, AlertCircleIcon, ClockIcon, CheckIcon, XIcon } from '../../components/Icons';

export const AdminDashboardPage: React.FC = () => {
  const [requests, setRequests] = useState<EventRequestItem[]>([]);
  const [recommendations, setRecommendations] = useState<RecommendationItem[]>([]);
  const [bookings, setBookings] = useState<Booking[]>([]);
  const [rooms, setRooms] = useState<Room[]>([]);
  const [loading, setLoading] = useState(true);
  const [generating, setGenerating] = useState(false);

  const { addToast } = useToast();
  const navigate = useNavigate();

  const loadData = async () => {
    setLoading(true);
    try {
      const [reqData, recData, bookData, roomData] = await Promise.all([
        apiFetch<EventRequestItem[]>('/api/requests'),
        apiFetch<RecommendationItem[]>('/api/recommendations'),
        apiFetch<Booking[]>('/api/bookings'),
        apiFetch<Room[]>('/api/rooms'),
      ]);
      setRequests(reqData);
      setRecommendations(recData);
      setBookings(bookData);
      setRooms(roomData);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load dashboard data', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const handleGenerateRecommendations = async () => {
    setGenerating(true);
    try {
      const res = await apiFetch<AllocateResponse>('/api/allocate', {
        method: 'POST',
        body: JSON.stringify({ algorithm: 'A' }),
      });

      if (res.ok) {
        addToast(`Generated ${res.allocatedCount} new recommendations!`, 'success');
        await loadData();
      } else {
        addToast(res.error || 'Allocation returned an error.', 'error');
      }
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to generate recommendations.', 'error');
      }
    } finally {
      setGenerating(false);
    }
  };

  const getRoomName = (roomId: number) => {
    const r = rooms.find((room) => room.id === roomId);
    return r ? `${r.name}` : `Room #${roomId}`;
  };

  const pendingCount = requests.filter((r) => r.status === 'PENDING').length;
  const recommendedCount = recommendations.length;
  const confirmedCount = requests.filter((r) => r.status === 'CONFIRMED').length;
  const rejectedCount = requests.filter((r) => r.status === 'REJECTED').length;

  const needsAttentionCount = pendingCount + recommendedCount;

  return (
    <div className="page-container">
      {/* Page Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">Admin Dashboard</h1>
          <p className="page-description">Overview of campus spaces, requests, and automated scheduling</p>
        </div>

        <div className="btn-group">
          <button
            type="button"
            onClick={handleGenerateRecommendations}
            disabled={generating}
            className="btn btn-primary"
          >
            <SparklesIcon size={16} />
            {generating ? 'Optimizing...' : 'Generate recommendations'}
          </button>
        </div>
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
                <span className="stat-title">Pending Requests</span>
                <ClockIcon size={16} className="text-warning" />
              </div>
              <div className="stat-value">{pendingCount}</div>
              <div className="stat-desc">Waiting for allocation</div>
            </div>

            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">Recommended</span>
                <SparklesIcon size={16} className="text-info" />
              </div>
              <div className="stat-value">{recommendedCount}</div>
              <div className="stat-desc">Awaiting admin review</div>
            </div>

            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">Confirmed Bookings</span>
                <CheckIcon size={16} className="text-success" />
              </div>
              <div className="stat-value">{confirmedCount}</div>
              <div className="stat-desc">Approved schedule entries</div>
            </div>

            <div className="stat-card">
              <div className="stat-header">
                <span className="stat-title">Rejected Requests</span>
                <XIcon size={16} className="text-danger" />
              </div>
              <div className="stat-value">{rejectedCount}</div>
              <div className="stat-desc">Declined submissions</div>
            </div>
          </div>

          {/* Needs Attention Panel */}
          {needsAttentionCount > 0 ? (
            <div className="attention-card">
              <div className="attention-left">
                <div className="attention-icon-box">
                  <AlertCircleIcon size={24} />
                </div>
                <div>
                  <h3 className="attention-title">Action Required: {needsAttentionCount} items need attention</h3>
                  <p className="attention-desc">
                    {recommendedCount > 0
                      ? `${recommendedCount} recommendation(s) are waiting for your approval in Review.`
                      : `${pendingCount} pending request(s) require automated allocation or administrative review.`}
                  </p>
                </div>
              </div>

              <div className="btn-group">
                {recommendedCount > 0 ? (
                  <button
                    type="button"
                    onClick={() => navigate('/admin/review')}
                    className="btn btn-warning"
                  >
                    Go to Review ({recommendedCount})
                  </button>
                ) : (
                  <button
                    type="button"
                    onClick={handleGenerateRecommendations}
                    disabled={generating}
                    className="btn btn-primary"
                  >
                    {generating ? 'Running...' : 'Generate recommendations'}
                  </button>
                )}
              </div>
            </div>
          ) : (
            <div className="card flex-between align-center" style={{ padding: '1rem 1.5rem' }}>
              <div className="flex-inline align-center gap-2">
                <span className="text-success font-medium">✓ All caught up!</span>
                <span className="text-muted text-sm">No pending requests or unreviewed recommendations.</span>
              </div>
              <button
                type="button"
                onClick={() => navigate('/admin/bookings')}
                className="btn btn-secondary btn-sm"
              >
                View Full Schedule
              </button>
            </div>
          )}

          {/* Upcoming Confirmed Bookings Section */}
          <div className="card">
            <div className="card-header">
              <div>
                <h2 className="card-title">Recent Confirmed Bookings</h2>
                <p className="card-subtitle">Active verified reservations across campus facilities</p>
              </div>
              <Link to="/admin/bookings" className="btn btn-secondary btn-sm">
                View Timeline Schedule &rarr;
              </Link>
            </div>

            {bookings.length === 0 ? (
              <p className="text-muted">No confirmed bookings currently recorded.</p>
            ) : (
              <div className="table-responsive">
                <table className="data-table">
                  <thead>
                    <tr>
                      <th>Booking ID</th>
                      <th>Room</th>
                      <th>Date</th>
                      <th>Time Window</th>
                      <th>Approved By</th>
                    </tr>
                  </thead>
                  <tbody>
                    {bookings.slice(0, 5).map((b) => (
                      <tr key={b.id}>
                        <td><strong>#{b.id}</strong></td>
                        <td>{getRoomName(b.roomId)}</td>
                        <td>{b.date}</td>
                        <td>{b.startTime} - {b.endTime}</td>
                        <td><span className="text-muted font-mono">{b.approvedBy}</span></td>
                      </tr>
                    ))}
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
