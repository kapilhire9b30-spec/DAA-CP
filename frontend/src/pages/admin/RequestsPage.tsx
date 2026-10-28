import React, { useEffect, useState } from 'react';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { EventRequestItem, ApiSuccessResponse } from '../../types/api';
import { StatusBadge, getFacilityNames } from '../../components/StatusBadge';
import { Modal } from '../../components/Modal';
import { SkeletonTable } from '../../components/Skeleton';
import { EmptyBoxIllustration } from '../../components/Icons';

export const RequestsPage: React.FC = () => {
  const { addToast } = useToast();

  const [requests, setRequests] = useState<EventRequestItem[]>([]);
  const [filterStatus, setFilterStatus] = useState<string>('ALL');
  const [loading, setLoading] = useState(true);

  // Reject Modal
  const [rejectReq, setRejectReq] = useState<EventRequestItem | null>(null);
  const [rejectReason, setRejectReason] = useState('');
  const [rejecting, setRejecting] = useState(false);

  const loadRequests = async () => {
    setLoading(true);
    try {
      const data = await apiFetch<EventRequestItem[]>('/api/requests');
      setRequests(data);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load requests', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadRequests();
  }, []);

  const handleConfirmReject = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!rejectReq) return;
    if (!rejectReason.trim()) {
      addToast('A rejection reason is required', 'warning');
      return;
    }

    setRejecting(true);
    try {
      await apiFetch<ApiSuccessResponse<EventRequestItem>>(`/api/requests/${rejectReq.id}/reject`, {
        method: 'POST',
        body: JSON.stringify({ reason: rejectReason.trim() }),
      });
      addToast(`Request #${rejectReq.id} rejected.`, 'info');
      setRejectReq(null);
      await loadRequests();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to reject request', 'error');
      }
    } finally {
      setRejecting(false);
    }
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
          <h1 className="page-title">Club Event Requests</h1>
          <p className="page-description">
            Complete registry of student organization requests submitted across the campus
          </p>
        </div>

        <button type="button" onClick={loadRequests} className="btn btn-secondary btn-sm" disabled={loading}>
          Refresh
        </button>
      </div>

      {/* Filter Chips */}
      <div className="filter-chip-group">
        <button
          type="button"
          className={`filter-chip ${filterStatus === 'ALL' ? 'active' : ''}`}
          onClick={() => setFilterStatus('ALL')}
        >
          All Requests ({requests.length})
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
          <p className="empty-state-desc">There are no requests matching the "{filterStatus}" filter.</p>
        </div>
      ) : (
        <div className="table-container">
          <div className="table-responsive">
            <table className="data-table">
              <thead>
                <tr>
                  <th>ID</th>
                  <th>Club</th>
                  <th>Title & Description</th>
                  <th>Date & Time</th>
                  <th>Att.</th>
                  <th>Facilities</th>
                  <th>Tier</th>
                  <th>Status</th>
                  <th>Action</th>
                </tr>
              </thead>
              <tbody>
                {filteredRequests.map((req) => (
                  <tr key={req.id}>
                    <td><strong>#{req.id}</strong></td>
                    <td><span className="badge badge-facility">{req.clubName}</span></td>
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
                      {req.rejectionReason && (
                        <div className="text-xs text-danger mt-1">Reason: {req.rejectionReason}</div>
                      )}
                    </td>
                    <td>
                      {req.status !== 'REJECTED' && req.status !== 'CONFIRMED' && (
                        <button
                          type="button"
                          onClick={() => {
                            setRejectReq(req);
                            setRejectReason('');
                          }}
                          className="btn btn-danger btn-sm"
                        >
                          Reject
                        </button>
                      )}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Reject Modal */}
      <Modal
        isOpen={!!rejectReq}
        onClose={() => setRejectReq(null)}
        title="Reject Event Request"
        description="Provide a justification for rejecting this club event submission."
      >
        {rejectReq && (
          <form onSubmit={handleConfirmReject} className="form-group">
            <div>
              <strong>Event:</strong> #{rejectReq.id} - "{rejectReq.title}" by club <strong>{rejectReq.clubName}</strong>
            </div>

            <div className="form-group">
              <label htmlFor="req-reject-reason">Rejection Reason *</label>
              <textarea
                id="req-reject-reason"
                required
                rows={3}
                value={rejectReason}
                onChange={(e) => setRejectReason(e.target.value)}
                placeholder="State the reason (e.g. Schedule clash or missing requisites)"
              />
            </div>

            <div className="modal-footer">
              <button
                type="button"
                onClick={() => setRejectReq(null)}
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
                {rejecting ? 'Rejecting...' : 'Confirm Rejection'}
              </button>
            </div>
          </form>
        )}
      </Modal>
    </div>
  );
};
