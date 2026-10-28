import React, { useEffect, useState } from 'react';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { DecisionLog, Room } from '../../types/api';
import { SkeletonTable } from '../../components/Skeleton';
import { EmptyBoxIllustration } from '../../components/Icons';

export const DecisionLogPage: React.FC = () => {
  const { addToast } = useToast();

  const [logs, setLogs] = useState<DecisionLog[]>([]);
  const [rooms, setRooms] = useState<Room[]>([]);
  const [loading, setLoading] = useState(true);
  const [filterAction, setFilterAction] = useState<string>('ALL');

  const loadData = async () => {
    setLoading(true);
    try {
      const [logsData, roomsData] = await Promise.all([
        apiFetch<DecisionLog[]>('/api/decisions'),
        apiFetch<Room[]>('/api/rooms'),
      ]);
      setLogs(logsData);
      setRooms(roomsData);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load decision logs', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const getRoomName = (roomId: number | null): string => {
    if (roomId === null) return '-';
    const r = rooms.find((room) => room.id === roomId);
    return r ? `${r.name} (#${roomId})` : `Room #${roomId}`;
  };

  const filteredLogs = logs.filter((l) => {
    if (filterAction === 'ALL') return true;
    if (filterAction === 'ROOM_OPS') return l.action.startsWith('ROOM_');
    return l.action === filterAction;
  });

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">Administrative Decision Audit Log</h1>
          <p className="page-description">
            Cryptographically sound, append-only audit trail of administrative approvals, rejections, and room overrides
          </p>
        </div>

        <button type="button" onClick={loadData} className="btn btn-secondary btn-sm" disabled={loading}>
          Refresh Log
        </button>
      </div>

      {/* Filter Chips */}
      <div className="filter-chip-group">
        <button
          type="button"
          className={`filter-chip ${filterAction === 'ALL' ? 'active' : ''}`}
          onClick={() => setFilterAction('ALL')}
        >
          All Actions ({logs.length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterAction === 'APPROVE' ? 'active' : ''}`}
          onClick={() => setFilterAction('APPROVE')}
        >
          Approvals ({logs.filter((l) => l.action === 'APPROVE').length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterAction === 'REJECT' ? 'active' : ''}`}
          onClick={() => setFilterAction('REJECT')}
        >
          Rejections ({logs.filter((l) => l.action === 'REJECT').length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterAction === 'MODIFY' ? 'active' : ''}`}
          onClick={() => setFilterAction('MODIFY')}
        >
          Overrides ({logs.filter((l) => l.action === 'MODIFY' || l.action === 'MODIFY_APPROVE').length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterAction === 'ROOM_OPS' ? 'active' : ''}`}
          onClick={() => setFilterAction('ROOM_OPS')}
        >
          Room Ops ({logs.filter((l) => l.action.startsWith('ROOM_')).length})
        </button>
      </div>

      {loading ? (
        <SkeletonTable rows={5} />
      ) : filteredLogs.length === 0 ? (
        <div className="card empty-state">
          <EmptyBoxIllustration size={80} />
          <h3 className="empty-state-title">No audit records found</h3>
          <p className="empty-state-desc">No actions matching this category have been performed yet.</p>
        </div>
      ) : (
        <div className="table-container">
          <div className="table-responsive">
            <table className="data-table">
              <thead>
                <tr>
                  <th>Log ID</th>
                  <th>Timestamp</th>
                  <th>Admin User</th>
                  <th>Action</th>
                  <th>Request ID</th>
                  <th>Room Assigned</th>
                  <th>Decision Details</th>
                  <th>Reason Given</th>
                </tr>
              </thead>
              <tbody>
                {filteredLogs.map((log) => {
                  let badge = <span className="badge badge-default">{log.action}</span>;
                  if (log.action === 'APPROVE') badge = <span className="badge badge-confirmed">APPROVE</span>;
                  if (log.action === 'REJECT') badge = <span className="badge badge-rejected">REJECT</span>;
                  if (log.action === 'MODIFY' || log.action === 'MODIFY_APPROVE') badge = <span className="badge badge-recommended">MODIFY</span>;
                  if (log.action === 'ROOM_ACTIVATED') badge = <span className="badge badge-confirmed">ACTIVATED</span>;
                  if (log.action === 'ROOM_DEACTIVATED') badge = <span className="badge badge-rejected">DEACTIVATED</span>;

                  return (
                    <tr key={log.id}>
                      <td><strong>#{log.id}</strong></td>
                      <td><span className="font-mono text-xs text-muted">{log.at}</span></td>
                      <td><strong>{log.who}</strong></td>
                      <td>{badge}</td>
                      <td>{log.requestId !== null && log.requestId !== 0 ? `#${log.requestId}` : '-'}</td>
                      <td>{getRoomName(log.roomId)}</td>
                      <td>{log.details}</td>
                      <td>
                        {log.reason ? (
                          <span className="text-danger font-medium text-xs">{log.reason}</span>
                        ) : (
                          <span className="text-muted">-</span>
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
