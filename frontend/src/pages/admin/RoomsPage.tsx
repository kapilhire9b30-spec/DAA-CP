import React, { useEffect, useState } from 'react';
import { apiFetch, ApiError, activateRoom, deactivateRoom as apiDeactivateRoom, getAffectedBookings } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { Room, CreateRoomRequest, ApiSuccessResponse, AffectedBooking } from '../../types/api';
import { getFacilityNames } from '../../components/StatusBadge';
import { Modal } from '../../components/Modal';
import { SkeletonTable } from '../../components/Skeleton';
import { EmptyBoxIllustration, CheckIcon } from '../../components/Icons';

export const RoomsPage: React.FC = () => {
  const { addToast } = useToast();

  const [rooms, setRooms] = useState<Room[]>([]);
  const [loading, setLoading] = useState(true);

  // Add Room Modal State
  const [showAddModal, setShowAddModal] = useState(false);
  const [name, setName] = useState('');
  const [building, setBuilding] = useState('');
  const [capacity, setCapacity] = useState<number | ''>('');
  const [facProjector, setFacProjector] = useState(false);
  const [facAudio, setFacAudio] = useState(false);
  const [facComputers, setFacComputers] = useState(false);
  const [savingRoom, setSavingRoom] = useState(false);

  // Deactivate Modal State
  const [targetRoom, setTargetRoom] = useState<Room | null>(null);
  const [affectedBookings, setAffectedBookings] = useState<AffectedBooking[]>([]);
  const [loadingAffected, setLoadingAffected] = useState(false);
  const [deactivating, setDeactivating] = useState(false);

  // Activating state tracker
  const [activatingId, setActivatingId] = useState<number | null>(null);

  // Filter
  const [filterActive, setFilterActive] = useState<'ALL' | 'ACTIVE' | 'INACTIVE'>('ALL');

  const loadRooms = async () => {
    setLoading(true);
    try {
      const data = await apiFetch<Room[]>('/api/rooms');
      setRooms(data);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load rooms', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadRooms();
  }, []);

  const handleCreateRoom = async (e: React.FormEvent) => {
    e.preventDefault();
    setSavingRoom(true);

    let mask = 0;
    if (facProjector) mask |= 1;
    if (facAudio) mask |= 2;
    if (facComputers) mask |= 4;

    const payload: CreateRoomRequest = {
      name: name.trim(),
      building: building.trim(),
      capacity: Number(capacity) || 0,
      facilityMask: mask,
    };

    try {
      const res = await apiFetch<ApiSuccessResponse<Room>>('/api/rooms', {
        method: 'POST',
        body: JSON.stringify(payload),
      });

      addToast(`Room "${res.data.name}" added successfully!`, 'success');
      setShowAddModal(false);
      setName('');
      setBuilding('');
      setCapacity('');
      setFacProjector(false);
      setFacAudio(false);
      setFacComputers(false);
      await loadRooms();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to create room', 'error');
      }
    } finally {
      setSavingRoom(false);
    }
  };

  const handleActivate = async (room: Room) => {
    setActivatingId(room.id);
    try {
      const res = await activateRoom(room.id);
      addToast(`Room "${res.data.name}" activated successfully!`, 'success');
      await loadRooms();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to activate room', 'error');
      }
    } finally {
      setActivatingId(null);
    }
  };

  const handleOpenDeactivateModal = async (room: Room) => {
    setTargetRoom(room);
    setLoadingAffected(true);
    setAffectedBookings([]);
    try {
      const list = await getAffectedBookings(room.id);
      setAffectedBookings(list);
    } catch {
      setAffectedBookings([]);
    } finally {
      setLoadingAffected(false);
    }
  };

  const handleConfirmDeactivate = async () => {
    if (!targetRoom) return;
    setDeactivating(true);
    try {
      const res = await apiDeactivateRoom(targetRoom.id);
      const affectedCount = res.data.affectedBookings ? res.data.affectedBookings.length : 0;
      addToast(
        `Room "${res.data.name || targetRoom.name}" deactivated.${affectedCount > 0 ? ` (${affectedCount} affected booking${affectedCount > 1 ? 's' : ''})` : ''}`,
        'info'
      );
      setTargetRoom(null);
      await loadRooms();
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to deactivate room', 'error');
      }
    } finally {
      setDeactivating(false);
    }
  };

  const filteredRooms = rooms.filter((r) => {
    if (filterActive === 'ACTIVE') return r.active;
    if (filterActive === 'INACTIVE') return !r.active;
    return true;
  });

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">Campus Room Inventory</h1>
          <p className="page-description">
            Manage lecture halls, auditoriums, capacities, and facility bitmasks for automated allocation
          </p>
        </div>

        <button
          type="button"
          onClick={() => setShowAddModal(true)}
          className="btn btn-primary"
        >
          + Add New Room
        </button>
      </div>

      {/* Filter Chips */}
      <div className="filter-chip-group">
        <button
          type="button"
          className={`filter-chip ${filterActive === 'ALL' ? 'active' : ''}`}
          onClick={() => setFilterActive('ALL')}
        >
          All Rooms ({rooms.length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterActive === 'ACTIVE' ? 'active' : ''}`}
          onClick={() => setFilterActive('ACTIVE')}
        >
          Active ({rooms.filter((r) => r.active).length})
        </button>
        <button
          type="button"
          className={`filter-chip ${filterActive === 'INACTIVE' ? 'active' : ''}`}
          onClick={() => setFilterActive('INACTIVE')}
        >
          Inactive ({rooms.filter((r) => !r.active).length})
        </button>
      </div>

      {loading ? (
        <SkeletonTable rows={4} />
      ) : filteredRooms.length === 0 ? (
        <div className="card empty-state">
          <EmptyBoxIllustration size={80} />
          <h3 className="empty-state-title">No rooms found</h3>
          <p className="empty-state-desc">No rooms match the selected filter.</p>
        </div>
      ) : (
        <div className="table-container">
          <div className="table-responsive">
            <table className="data-table">
              <thead>
                <tr>
                  <th>ID</th>
                  <th>Room Name</th>
                  <th>Building</th>
                  <th>Capacity</th>
                  <th>Facilities</th>
                  <th>Status</th>
                  <th>Action</th>
                </tr>
              </thead>
              <tbody>
                {filteredRooms.map((room) => (
                  <tr key={room.id} style={!room.active ? { opacity: 0.65 } : undefined}>
                    <td><strong>#{room.id}</strong></td>
                    <td><strong>{room.name}</strong></td>
                    <td>{room.building}</td>
                    <td>{room.capacity} seats</td>
                    <td>
                      <span className="badge badge-facility">{getFacilityNames(room.facilityMask)}</span>
                    </td>
                    <td>
                      {room.active ? (
                        <span className="badge badge-confirmed">ACTIVE</span>
                      ) : (
                        <span className="badge badge-rejected">INACTIVE</span>
                      )}
                    </td>
                    <td>
                      {room.active ? (
                        <button
                          type="button"
                          onClick={() => handleOpenDeactivateModal(room)}
                          className="btn btn-secondary btn-sm"
                        >
                          Deactivate
                        </button>
                      ) : (
                        <button
                          type="button"
                          onClick={() => handleActivate(room)}
                          disabled={activatingId === room.id}
                          className="btn btn-primary btn-sm"
                        >
                          {activatingId === room.id ? 'Activating...' : 'Activate'}
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

      {/* Modal: Add Room */}
      <Modal
        isOpen={showAddModal}
        onClose={() => setShowAddModal(false)}
        title="Create New Campus Room"
        description="Register a new room into the campus inventory for automated scheduling."
      >
        <form onSubmit={handleCreateRoom} className="form-group">
          <div className="form-group">
            <label htmlFor="create-room-name">Room Name *</label>
            <input
              id="create-room-name"
              type="text"
              required
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g. Science Auditorium 2"
            />
          </div>

          <div className="form-group">
            <label htmlFor="create-room-building">Building / Complex *</label>
            <input
              id="create-room-building"
              type="text"
              required
              value={building}
              onChange={(e) => setBuilding(e.target.value)}
              placeholder="e.g. North Science Block"
            />
          </div>

          <div className="form-group">
            <label htmlFor="create-room-capacity">Seating Capacity *</label>
            <input
              id="create-room-capacity"
              type="number"
              required
              min={1}
              value={capacity}
              onChange={(e) => setCapacity(e.target.value === '' ? '' : parseInt(e.target.value, 10))}
              placeholder="e.g. 150"
            />
          </div>

          <div className="form-group">
            <label>Available Facilities</label>
            <div className="checkbox-group">
              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={facProjector}
                  onChange={(e) => setFacProjector(e.target.checked)}
                />
                Projector (Bit 0)
              </label>

              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={facAudio}
                  onChange={(e) => setFacAudio(e.target.checked)}
                />
                Audio System (Bit 1)
              </label>

              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={facComputers}
                  onChange={(e) => setFacComputers(e.target.checked)}
                />
                Workstations (Bit 2)
              </label>
            </div>
          </div>

          <div className="modal-footer">
            <button
              type="button"
              onClick={() => setShowAddModal(false)}
              disabled={savingRoom}
              className="btn btn-secondary"
            >
              Cancel
            </button>
            <button
              type="submit"
              disabled={savingRoom}
              className="btn btn-primary"
            >
              <CheckIcon size={14} />
              {savingRoom ? 'Saving...' : 'Save Room'}
            </button>
          </div>
        </form>
      </Modal>

      {/* Modal: Confirm Deactivation */}
      <Modal
        isOpen={!!targetRoom}
        onClose={() => setTargetRoom(null)}
        title="Deactivate Campus Room"
        description="Review affected bookings before confirming deactivation."
      >
        {targetRoom && (
          <div>
            <p className="mb-3">
              Deactivating <strong>{targetRoom.name}</strong> ({targetRoom.building}, Capacity: {targetRoom.capacity} seats).
            </p>

            {loadingAffected ? (
              <p className="text-sm text-muted mb-3">Checking upcoming confirmed bookings...</p>
            ) : affectedBookings.length > 0 ? (
              <div className="mb-3">
                <div className="alert alert-warning mb-2" style={{ fontSize: '0.875rem' }}>
                  <strong>Warning:</strong> Deactivating this room directly impacts{' '}
                  <strong>{affectedBookings.length}</strong> upcoming confirmed booking
                  {affectedBookings.length > 1 ? 's' : ''}:
                </div>
                <div className="table-responsive" style={{ maxHeight: '180px', overflowY: 'auto' }}>
                  <table className="data-table" style={{ fontSize: '0.8rem' }}>
                    <thead>
                      <tr>
                        <th>Booking ID</th>
                        <th>Event Title</th>
                        <th>Club</th>
                        <th>Date</th>
                        <th>Time</th>
                      </tr>
                    </thead>
                    <tbody>
                      {affectedBookings.map((b) => (
                        <tr key={b.id}>
                          <td><strong>#{b.id}</strong></td>
                          <td>{b.title || `Booking #${b.id}`}</td>
                          <td>{b.club || '-'}</td>
                          <td>{b.date}</td>
                          <td>{b.start} - {b.end}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </div>
            ) : (
              <p className="text-sm text-muted mb-3" style={{ fontStyle: 'italic' }}>
                No upcoming confirmed bookings are currently scheduled in this room.
              </p>
            )}

            <div className="modal-footer">
              <button
                type="button"
                onClick={() => setTargetRoom(null)}
                disabled={deactivating}
                className="btn btn-secondary"
              >
                Cancel
              </button>
              <button
                type="button"
                onClick={handleConfirmDeactivate}
                disabled={deactivating || loadingAffected}
                className="btn btn-danger"
              >
                {deactivating ? 'Deactivating...' : 'Confirm Deactivation'}
              </button>
            </div>
          </div>
        )}
      </Modal>
    </div>
  );
};
