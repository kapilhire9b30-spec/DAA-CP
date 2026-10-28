import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { ApiSuccessResponse, EventRequestItem, SubmitEventRequest } from '../../types/api';
import { CheckIcon } from '../../components/Icons';

export const SubmitRequestPage: React.FC = () => {
  const navigate = useNavigate();
  const { addToast } = useToast();

  const [title, setTitle] = useState('');
  const [description, setDescription] = useState('');
  const [date, setDate] = useState('');
  const [startTime, setStartTime] = useState('');
  const [endTime, setEndTime] = useState('');
  const [attendance, setAttendance] = useState<number | ''>('');
  const [tier, setTier] = useState<number>(3);

  // Facility checkboxes: 1 = Projector, 2 = Audio, 4 = Workstations
  const [facilityProjector, setFacilityProjector] = useState(false);
  const [facilityAudio, setFacilityAudio] = useState(false);
  const [facilityComputers, setFacilityComputers] = useState(false);

  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);

    // Validation
    if (startTime && endTime && startTime >= endTime) {
      setError('End time must be after start time.');
      return;
    }

    let mask = 0;
    if (facilityProjector) mask |= 1;
    if (facilityAudio) mask |= 2;
    if (facilityComputers) mask |= 4;

    const payload: SubmitEventRequest = {
      title: title.trim(),
      description: description.trim(),
      date,
      startTime,
      endTime,
      attendance: Number(attendance) || 0,
      facilityMask: mask,
      tier: Number(tier),
    };

    setLoading(true);
    try {
      const res = await apiFetch<ApiSuccessResponse<EventRequestItem>>('/api/requests', {
        method: 'POST',
        body: JSON.stringify(payload),
      });

      addToast(`Request #${res.data.id} "${res.data.title}" submitted successfully!`, 'success');
      navigate('/club/my-requests');
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        setError(err.message);
      } else if (err instanceof Error) {
        setError(err.message);
      } else {
        setError('Failed to submit event request.');
      }
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="page-container" style={{ maxWidth: '880px', margin: '0 auto' }}>
      <div className="page-header">
        <div>
          <h1 className="page-title">Submit New Event Request</h1>
          <p className="page-description">
            Enter event parameters for automated algorithmic room allocation and administrative scheduling
          </p>
        </div>
      </div>

      {error && (
        <div className="alert alert-error" role="alert">
          <strong>Submission Error: </strong>
          <span>{error}</span>
        </div>
      )}

      <div className="card">
        <form onSubmit={handleSubmit} className="form-grid">
          <div className="form-group full-width">
            <label htmlFor="title">Event Title *</label>
            <input
              id="title"
              type="text"
              required
              value={title}
              onChange={(e) => setTitle(e.target.value)}
              placeholder="e.g. Autonomous Robotics Hackathon"
            />
          </div>

          <div className="form-group full-width">
            <label htmlFor="description">Event Description *</label>
            <textarea
              id="description"
              required
              rows={3}
              value={description}
              onChange={(e) => setDescription(e.target.value)}
              placeholder="Provide a concise summary of the planned activities and equipment needed..."
            />
          </div>

          <div className="form-group">
            <label htmlFor="date">Scheduled Date *</label>
            <input
              id="date"
              type="date"
              required
              value={date}
              onChange={(e) => setDate(e.target.value)}
            />
          </div>

          <div className="form-group">
            <label htmlFor="attendance">Expected Attendance (Headcount) *</label>
            <input
              id="attendance"
              type="number"
              required
              min={1}
              value={attendance}
              onChange={(e) => setAttendance(e.target.value === '' ? '' : parseInt(e.target.value, 10))}
              placeholder="e.g. 50"
            />
          </div>

          <div className="form-group">
            <label htmlFor="startTime">Start Time (HH:mm) *</label>
            <input
              id="startTime"
              type="time"
              required
              value={startTime}
              onChange={(e) => setStartTime(e.target.value)}
            />
          </div>

          <div className="form-group">
            <label htmlFor="endTime">End Time (HH:mm) *</label>
            <input
              id="endTime"
              type="time"
              required
              value={endTime}
              onChange={(e) => setEndTime(e.target.value)}
            />
          </div>

          <div className="form-group full-width">
            <label htmlFor="tier">Event Priority Tier *</label>
            <select
              id="tier"
              value={tier}
              onChange={(e) => setTier(Number(e.target.value))}
            >
              <option value={1}>Tier 1 — Flagship / Institute-Level Symposium (Highest weight: 100)</option>
              <option value={2}>Tier 2 — Major Inter-College Workshop / Contest (Weight: 50)</option>
              <option value={3}>Tier 3 — Regular Club Workshop / Weekly Meeting (Weight: 20)</option>
              <option value={4}>Tier 4 — Informal / Internal Practice Session (Weight: 10)</option>
            </select>
          </div>

          <div className="form-group full-width">
            <label className="text-sm font-medium">Required Room Facilities</label>
            <div className="checkbox-group">
              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={facilityProjector}
                  onChange={(e) => setFacilityProjector(e.target.checked)}
                />
                High-Definition Projector (Bit 0)
              </label>

              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={facilityAudio}
                  onChange={(e) => setFacilityAudio(e.target.checked)}
                />
                Amplified Audio System / Mic (Bit 1)
              </label>

              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={facilityComputers}
                  onChange={(e) => setFacilityComputers(e.target.checked)}
                />
                Workstations / Lab PC Access (Bit 2)
              </label>
            </div>
          </div>

          <div className="modal-footer full-width" style={{ marginTop: '1rem' }}>
            <button
              type="button"
              onClick={() => navigate('/club/dashboard')}
              className="btn btn-secondary"
            >
              Cancel
            </button>
            <button type="submit" disabled={loading} className="btn btn-primary btn-lg">
              <CheckIcon size={16} />
              {loading ? 'Submitting...' : 'Submit Request for Optimization'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};
