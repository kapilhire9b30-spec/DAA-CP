import React, { useEffect, useState } from 'react';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import {
  RecommendationItem,
  Room,
  ApiSuccessResponse,
  Booking,
  ModifyRecommendationResponse,
  EventRequestItem,
} from '../../types/api';
import { getFacilityNames } from '../../components/StatusBadge';
import { Modal } from '../../components/Modal';
import { SkeletonCard } from '../../components/Skeleton';
import { CheckIcon, EmptyBoxIllustration, AlertCircleIcon } from '../../components/Icons';

export const ReviewPage: React.FC = () => {
  const { addToast } = useToast();

  const [recommendations, setRecommendations] = useState<RecommendationItem[]>([]);
  const [allRooms, setAllRooms] = useState<Room[]>([]);
  const [loading, setLoading] = useState(true);

  // Modals state
  const [approveModalRec, setApproveModalRec] = useState<RecommendationItem | null>(null);
  const [approving, setApproving] = useState(false);

  const [rejectModalRec, setRejectModalRec] = useState<RecommendationItem | null>(null);
  const [rejectReason, setRejectReason] = useState('');
  const [rejecting, setRejecting] = useState(false);

  const [changeRoomModalRec, setChangeRoomModalRec] = useState<RecommendationItem | null>(null);
  const [selectedChangeRoomId, setSelectedChangeRoomId] = useState<number | null>(null);
  const [changeRoomError, setChangeRoomError] = useState<string | null>(null);
  const [modifying, setModifying] = useState(false);

  const loadData = async () => {
    setLoading(true);
    try {
      const [recs, rooms] = await Promise.all([
        apiFetch<RecommendationItem[]>('/api/recommendations'),
        apiFetch<Room[]>('/api/rooms'),
      ]);
      setRecommendations(recs);
      setAllRooms(rooms);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load recommendations', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  // 1. Confirm Approve
  const handleConfirmApprove = async () => {
    if (!approveModalRec) return;
    setApproving(true);
    try {
      await apiFetch<ApiSuccessResponse<Booking>>(`/api/recommendations/${approveModalRec.id}/approve`, {
        method: 'POST',
      });
      addToast(`Booking confirmed for Request #${approveModalRec.request.id}!`, 'success');
      setApproveModalRec(null);
      await loadData();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(`Approval blocked: ${err.message}`, 'error');
      } else {
        addToast('Approval failed.', 'error');
      }
    } finally {
      setApproving(false);
    }
  };

  // 2. Confirm Reject
  const handleConfirmReject = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!rejectModalRec) return;
    if (!rejectReason.trim()) {
      addToast('A rejection reason is required', 'warning');
      return;
    }

    setRejecting(true);
    try {
      await apiFetch<ApiSuccessResponse<EventRequestItem>>(`/api/requests/${rejectModalRec.request.id}/reject`, {
        method: 'POST',
        body: JSON.stringify({ reason: rejectReason.trim() }),
      });
      addToast(`Request #${rejectModalRec.request.id} rejected.`, 'info');
      setRejectModalRec(null);
      await loadData();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to reject request.', 'error');
      }
    } finally {
      setRejecting(false);
    }
  };

  // 3. Confirm Change Room
  const handleOpenChangeRoom = (rec: RecommendationItem) => {
    setChangeRoomModalRec(rec);
    setSelectedChangeRoomId(rec.proposedRoom.id);
    setChangeRoomError(null);
  };

  const handleConfirmChangeRoom = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!changeRoomModalRec || !selectedChangeRoomId) return;

    setModifying(true);
    setChangeRoomError(null);
    try {
      const res = await apiFetch<ModifyRecommendationResponse>(`/api/recommendations/${changeRoomModalRec.id}/modify`, {
        method: 'POST',
        body: JSON.stringify({ roomId: selectedChangeRoomId }),
      });

      let msg = `Request #${changeRoomModalRec.request.id} confirmed with Room #${selectedChangeRoomId}!`;
      if (res.warning) {
        msg += ` (${res.warning})`;
      }
      addToast(msg, 'success');
      setChangeRoomModalRec(null);
      await loadData();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        setChangeRoomError(err.message);
      } else if (err instanceof Error) {
        setChangeRoomError(err.message);
      } else {
        setChangeRoomError('Failed to change room assignment.');
      }
    } finally {
      setModifying(false);
    }
  };

  const activeRooms = allRooms.filter((r) => r.active);

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">Review Recommendations</h1>
          <p className="page-description">
            Inspect automated recommendations, verify facility compatibility, and confirm or modify bookings
          </p>
        </div>

        <button type="button" onClick={loadData} className="btn btn-secondary btn-sm" disabled={loading}>
          Refresh
        </button>
      </div>

      {loading ? (
        <SkeletonCard count={3} />
      ) : recommendations.length === 0 ? (
        <div className="card empty-state">
          <EmptyBoxIllustration size={88} />
          <h3 className="empty-state-title">No pending recommendations</h3>
          <p className="empty-state-desc">
            All submitted requests have been reviewed or confirmed. Use the Admin Dashboard to generate new recommendations.
          </p>
        </div>
      ) : (
        <div className="review-grid">
          {recommendations.map((rec) => {
            const isExceeding =
              rec.capacityWarning || rec.request.attendance > rec.proposedRoom.capacity;

            return (
              <div key={rec.id} className="review-card">
                <div className="review-card-header">
                  <div>
                    <h3 className="review-req-title">
                      Req #{rec.request.id}: {rec.request.title}
                    </h3>
                    <span className="review-club-pill">{rec.request.clubName}</span>
                  </div>
                  <span className="badge-suggestion">Suggestion - not confirmed</span>
                </div>

                <div className="review-card-body">
                  <div className="review-meta-row">
                    <div><strong>Date:</strong> {rec.request.date}</div>
                    <div><strong>Time:</strong> {rec.request.startTime} - {rec.request.endTime}</div>
                    <div><strong>Priority:</strong> Tier {rec.request.tier}</div>
                    <div><strong>Facilities:</strong> {getFacilityNames(rec.request.facilityMask)}</div>
                  </div>

                  {/* Proposed Room Section */}
                  <div className="review-room-banner">
                    <span className="review-room-label">Proposed Room</span>
                    <span className="review-room-name">
                      {rec.proposedRoom.name} ({rec.proposedRoom.building})
                    </span>
                    <div className="review-room-capacity-line">
                      Room Capacity: <strong>{rec.proposedRoom.capacity}</strong> • Expected Attendance: <strong>{rec.request.attendance}</strong>
                    </div>

                    {isExceeding && (
                      <div className="review-warning-flag">
                        <AlertCircleIcon size={14} />
                        <span>attendance exceeds listed capacity (informational)</span>
                      </div>
                    )}
                  </div>

                  {/* Other Feasible Rooms */}
                  <div>
                    <label className="text-xs text-muted" style={{ textTransform: 'uppercase', fontWeight: 700 }}>
                      Other Feasible Rooms from Solver:
                    </label>
                    {rec.otherFeasibleRooms && rec.otherFeasibleRooms.length > 0 ? (
                      <ul style={{ paddingLeft: '1.25rem', marginTop: '0.25rem', fontSize: '0.82rem', color: 'var(--text-muted)' }}>
                        {rec.otherFeasibleRooms.map((room) => (
                          <li key={room.id}>
                            {room.name} ({room.building}) — Cap: {room.capacity}
                          </li>
                        ))}
                      </ul>
                    ) : (
                      <div className="text-muted text-xs mt-1">None available</div>
                    )}
                  </div>
                </div>

                {/* Actions Footer */}
                <div className="review-card-footer">
                  <button
                    type="button"
                    onClick={() => handleOpenChangeRoom(rec)}
                    className="btn btn-secondary btn-sm"
                  >
                    Change room...
                  </button>

                  <button
                    type="button"
                    onClick={() => {
                      setRejectModalRec(rec);
                      setRejectReason('');
                    }}
                    className="btn btn-danger btn-sm"
                  >
                    Reject
                  </button>

                  <button
                    type="button"
                    onClick={() => setApproveModalRec(rec)}
                    className="btn btn-primary btn-sm"
                  >
                    <CheckIcon size={14} />
                    Approve
                  </button>
                </div>
              </div>
            );
          })}
        </div>
      )}

      {/* 1. Modal: Confirm Approve */}
      <Modal
        isOpen={!!approveModalRec}
        onClose={() => setApproveModalRec(null)}
        title="Confirm Room Allocation"
        description="This will lock the booking and finalize the room reservation on the campus schedule."
      >
        {approveModalRec && (
          <div>
            <div className="review-room-banner mb-3">
              <div className="review-req-title">
                Request #{approveModalRec.request.id}: "{approveModalRec.request.title}"
              </div>
              <div className="text-sm text-muted">
                Club: {approveModalRec.request.clubName} • Date: {approveModalRec.request.date} ({approveModalRec.request.startTime} - {approveModalRec.request.endTime})
              </div>
              <div className="text-sm font-medium mt-1">
                Allocating Room: <strong>{approveModalRec.proposedRoom.name}</strong> ({approveModalRec.proposedRoom.building})
              </div>
            </div>

            <div className="modal-footer">
              <button
                type="button"
                onClick={() => setApproveModalRec(null)}
                disabled={approving}
                className="btn btn-secondary"
              >
                Cancel
              </button>
              <button
                type="button"
                onClick={handleConfirmApprove}
                disabled={approving}
                className="btn btn-primary"
              >
                {approving ? 'Confirming...' : 'Confirm & Approve'}
              </button>
            </div>
          </div>
        )}
      </Modal>

      {/* 2. Modal: Reject with Required Reason */}
      <Modal
        isOpen={!!rejectModalRec}
        onClose={() => setRejectModalRec(null)}
        title="Reject Event Request"
        description="Please provide a clear reason for rejecting this event submission. The club will be notified."
      >
        {rejectModalRec && (
          <form onSubmit={handleConfirmReject} className="form-group">
            <div>
              <strong>Request:</strong> #{rejectModalRec.request.id} - {rejectModalRec.request.title} ({rejectModalRec.request.clubName})
            </div>

            <div className="form-group">
              <label htmlFor="modal-reject-reason">Rejection Reason *</label>
              <textarea
                id="modal-reject-reason"
                required
                rows={3}
                value={rejectReason}
                onChange={(e) => setRejectReason(e.target.value)}
                placeholder="e.g. Venue maintenance scheduled or institutional priority clash"
              />
            </div>

            <div className="modal-footer">
              <button
                type="button"
                onClick={() => setRejectModalRec(null)}
                disabled={rejecting}
                className="btn btn-secondary"
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={rejecting}
                className="btn btn-danger"
              >
                {rejecting ? 'Rejecting...' : 'Reject Request'}
              </button>
            </div>
          </form>
        )}
      </Modal>

      {/* 3. Modal: Change Room Override */}
      <Modal
        isOpen={!!changeRoomModalRec}
        onClose={() => setChangeRoomModalRec(null)}
        title="Override & Assign Different Room"
        description="Select an alternate room for this event. The system will verify availability and capacity."
      >
        {changeRoomModalRec && (
          <form onSubmit={handleConfirmChangeRoom} className="form-group">
            {changeRoomError && (
              <div className="alert alert-error" role="alert">
                <strong>Schedule Conflict / Error: </strong>
                <span>{changeRoomError}</span>
              </div>
            )}

            <div>
              <strong>Request:</strong> #{changeRoomModalRec.request.id} - "{changeRoomModalRec.request.title}" ({changeRoomModalRec.request.date}, {changeRoomModalRec.request.startTime} - {changeRoomModalRec.request.endTime})
            </div>

            <div className="form-group">
              <label htmlFor="modal-room-select">Choose Active Room *</label>
              <select
                id="modal-room-select"
                value={selectedChangeRoomId || ''}
                onChange={(e) => setSelectedChangeRoomId(Number(e.target.value))}
                required
              >
                {activeRooms.map((room) => (
                  <option key={room.id} value={room.id}>
                    {room.name} ({room.building}) — Capacity: {room.capacity}
                  </option>
                ))}
              </select>
            </div>

            <div className="modal-footer">
              <button
                type="button"
                onClick={() => setChangeRoomModalRec(null)}
                disabled={modifying}
                className="btn btn-secondary"
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={modifying}
                className="btn btn-primary"
              >
                {modifying ? 'Validating & Approving...' : 'Approve with Selected Room'}
              </button>
            </div>
          </form>
        )}
      </Modal>
    </div>
  );
};
