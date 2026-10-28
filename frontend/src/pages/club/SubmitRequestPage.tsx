import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { ApiSuccessResponse, EventRequestItem, SubmitEventRequest, Room } from '../../types/api';
import { CheckIcon } from '../../components/Icons';
import { RoomPreferenceSelector, RoomPreferenceMode } from '../../components/RoomPreferenceSelector';
import { RequestLiveSummary } from '../../components/RequestLiveSummary';
import './SubmitRequestPage.css';

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

  // Room Preference State (0 = No preference, mode: 0 = Flexible, 1 = Strict)
  const [preferredRoomId, setPreferredRoomId] = useState<number>(0);
  const [roomMode, setRoomMode] = useState<RoomPreferenceMode>(0);

  // Active rooms list
  const [rooms, setRooms] = useState<Room[]>([]);
  const [loadingRooms, setLoadingRooms] = useState(true);

  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let isMounted = true;
    const fetchRooms = async () => {
      try {
        const data = await apiFetch<Room[]>('/api/rooms');
        if (isMounted) {
          setRooms(data);
        }
      } catch {
        // Fallback or quiet catch; rooms remain empty without blocking the form
      } finally {
        if (isMounted) {
          setLoadingRooms(false);
        }
      }
    };
    fetchRooms();
    return () => {
      isMounted = false;
    };
  }, []);

  const facilityMask =
    (facilityProjector ? 1 : 0) | (facilityAudio ? 2 : 0) | (facilityComputers ? 4 : 0);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);

    // Validation (matching existing business rules)
    if (startTime && endTime && startTime >= endTime) {
      setError('End time must be after start time.');
      return;
    }

    const payload: SubmitEventRequest = {
      title: title.trim(),
      description: description.trim(),
      date,
      startTime,
      endTime,
      attendance: Number(attendance) || 0,
      facilityMask,
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
    <div className="page-container">
      <div className="page-header">
        <div>
          <h1 className="page-title">Submit New Event Request</h1>
          <p className="page-description">
            Define scheduling parameters, equipment requirements, and venue preferences for automated allocation
          </p>
        </div>
      </div>

      {error && (
        <div className="alert alert-error" role="alert" style={{ marginBottom: '1.5rem' }}>
          <strong>Submission Error: </strong>
          <span>{error}</span>
        </div>
      )}

      <div className="submit-request-layout">
        {/* Main Form Body */}
        <div className="submit-form-card">
          <form onSubmit={handleSubmit} noValidate={false}>
            {/* Section 1: Event Details */}
            <section className="form-section" aria-labelledby="sec-1-title">
              <div className="section-title-wrap">
                <span className="section-num">1</span>
                <h2 id="sec-1-title" className="section-heading">Event Details</h2>
                <span className="section-subtext">Basic identification</span>
              </div>

              <div className="form-field">
                <label htmlFor="title" className="field-label">
                  <span>Event Title <span className="req-marker">*</span></span>
                </label>
                <input
                  id="title"
                  type="text"
                  required
                  value={title}
                  onChange={(e) => setTitle(e.target.value)}
                  placeholder="e.g. Autonomous Robotics Hackathon"
                  className="field-input"
                />
              </div>

              <div className="form-field">
                <label htmlFor="description" className="field-label">
                  <span>Event Description <span className="req-marker">*</span></span>
                  <span className="char-counter">{description.length} / 500</span>
                </label>
                <textarea
                  id="description"
                  required
                  rows={3}
                  maxLength={500}
                  value={description}
                  onChange={(e) => setDescription(e.target.value)}
                  placeholder="Provide a concise summary of the planned activities, target audience, and equipment needed..."
                  className="field-textarea"
                />
              </div>
            </section>

            {/* Section 2: Schedule & Headcount */}
            <section className="form-section" aria-labelledby="sec-2-title">
              <div className="section-title-wrap">
                <span className="section-num">2</span>
                <h2 id="sec-2-title" className="section-heading">Schedule & Capacity</h2>
                <span className="section-subtext">Time and attendance</span>
              </div>

              <div className="field-grid-2">
                <div className="form-field">
                  <label htmlFor="date" className="field-label">
                    <span>Scheduled Date <span className="req-marker">*</span></span>
                  </label>
                  <input
                    id="date"
                    type="date"
                    required
                    value={date}
                    onChange={(e) => setDate(e.target.value)}
                    className="field-input"
                  />
                </div>

                <div className="form-field">
                  <label htmlFor="attendance" className="field-label">
                    <span>Expected Attendance <span className="req-marker">*</span></span>
                  </label>
                  <input
                    id="attendance"
                    type="number"
                    required
                    min={1}
                    value={attendance}
                    onChange={(e) =>
                      setAttendance(e.target.value === '' ? '' : parseInt(e.target.value, 10))
                    }
                    placeholder="e.g. 50"
                    className="field-input"
                  />
                </div>
              </div>

              <div className="field-grid-2" style={{ marginTop: '1.25rem' }}>
                <div className="form-field">
                  <label htmlFor="startTime" className="field-label">
                    <span>Start Time (HH:mm) <span className="req-marker">*</span></span>
                  </label>
                  <input
                    id="startTime"
                    type="time"
                    required
                    value={startTime}
                    onChange={(e) => setStartTime(e.target.value)}
                    className="field-input"
                  />
                </div>

                <div className="form-field">
                  <label htmlFor="endTime" className="field-label">
                    <span>End Time (HH:mm) <span className="req-marker">*</span></span>
                  </label>
                  <input
                    id="endTime"
                    type="time"
                    required
                    value={endTime}
                    onChange={(e) => setEndTime(e.target.value)}
                    className="field-input"
                  />
                </div>
              </div>
            </section>

            {/* Section 3: Priority Tier */}
            <section className="form-section" aria-labelledby="sec-3-title">
              <div className="section-title-wrap">
                <span className="section-num">3</span>
                <h2 id="sec-3-title" className="section-heading">Priority Tier</h2>
                <span className="section-subtext">Weighting in optimization solver</span>
              </div>

              <div className="tier-cards-grid" role="radiogroup" aria-label="Event Priority Tier">
                <button
                  type="button"
                  role="radio"
                  aria-checked={tier === 1}
                  className={`tier-card-btn ${tier === 1 ? 'selected' : ''}`}
                  onClick={() => setTier(1)}
                >
                  <div className="tier-top-row">
                    <span className="tier-name">Tier 1 — Flagship</span>
                    <span className="tier-weight-pill">Wt: 100</span>
                  </div>
                  <span className="tier-desc">Institute-Level Symposium, Annual Summit, or Convocation</span>
                </button>

                <button
                  type="button"
                  role="radio"
                  aria-checked={tier === 2}
                  className={`tier-card-btn ${tier === 2 ? 'selected' : ''}`}
                  onClick={() => setTier(2)}
                >
                  <div className="tier-top-row">
                    <span className="tier-name">Tier 2 — Major</span>
                    <span className="tier-weight-pill">Wt: 50</span>
                  </div>
                  <span className="tier-desc">Inter-College Hackathon, Conference, or Department Fest</span>
                </button>

                <button
                  type="button"
                  role="radio"
                  aria-checked={tier === 3}
                  className={`tier-card-btn ${tier === 3 ? 'selected' : ''}`}
                  onClick={() => setTier(3)}
                >
                  <div className="tier-top-row">
                    <span className="tier-name">Tier 3 — Regular</span>
                    <span className="tier-weight-pill">Wt: 20</span>
                  </div>
                  <span className="tier-desc">Weekly Club Meeting, Technical Workshop, or Speaker Session</span>
                </button>

                <button
                  type="button"
                  role="radio"
                  aria-checked={tier === 4}
                  className={`tier-card-btn ${tier === 4 ? 'selected' : ''}`}
                  onClick={() => setTier(4)}
                >
                  <div className="tier-top-row">
                    <span className="tier-name">Tier 4 — Informal</span>
                    <span className="tier-weight-pill">Wt: 10</span>
                  </div>
                  <span className="tier-desc">Internal Practice, Rehearsal, or Ad-Hoc Team Sync</span>
                </button>
              </div>
            </section>

            {/* Section 4: Required Room Facilities */}
            <section className="form-section" aria-labelledby="sec-4-title">
              <div className="section-title-wrap">
                <span className="section-num">4</span>
                <h2 id="sec-4-title" className="section-heading">Required Facilities</h2>
                <span className="section-subtext">Equipment bitmask constraints</span>
              </div>

              <div className="facility-chips-grid">
                <button
                  type="button"
                  className={`facility-chip-btn ${facilityProjector ? 'checked' : ''}`}
                  onClick={() => setFacilityProjector(!facilityProjector)}
                  aria-pressed={facilityProjector}
                >
                  <div className="facility-icon-wrap" aria-hidden="true">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <rect x="2" y="3" width="20" height="14" rx="2" />
                      <line x1="8" y1="21" x2="16" y2="21" />
                      <line x1="12" y1="17" x2="12" y2="21" />
                    </svg>
                  </div>
                  <div className="facility-info">
                    <span className="facility-name">HD Projector</span>
                    <span className="facility-bit">Bit 0</span>
                  </div>
                </button>

                <button
                  type="button"
                  className={`facility-chip-btn ${facilityAudio ? 'checked' : ''}`}
                  onClick={() => setFacilityAudio(!facilityAudio)}
                  aria-pressed={facilityAudio}
                >
                  <div className="facility-icon-wrap" aria-hidden="true">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <path d="M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z" />
                      <path d="M19 10v2a7 7 0 0 1-14 0v-2" />
                      <line x1="12" y1="19" x2="12" y2="23" />
                      <line x1="8" y1="23" x2="16" y2="23" />
                    </svg>
                  </div>
                  <div className="facility-info">
                    <span className="facility-name">Audio & Mic</span>
                    <span className="facility-bit">Bit 1</span>
                  </div>
                </button>

                <button
                  type="button"
                  className={`facility-chip-btn ${facilityComputers ? 'checked' : ''}`}
                  onClick={() => setFacilityComputers(!facilityComputers)}
                  aria-pressed={facilityComputers}
                >
                  <div className="facility-icon-wrap" aria-hidden="true">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <rect x="2" y="3" width="20" height="14" rx="2" />
                      <line x1="6" y1="21" x2="18" y2="21" />
                      <line x1="12" y1="17" x2="12" y2="21" />
                    </svg>
                  </div>
                  <div className="facility-info">
                    <span className="facility-name">Workstations</span>
                    <span className="facility-bit">Bit 2</span>
                  </div>
                </button>
              </div>
            </section>

            {/* Section 5: Room Preference */}
            <section className="form-section" aria-labelledby="sec-5-title">
              <div className="section-title-wrap">
                <span className="section-num">5</span>
                <h2 id="sec-5-title" className="section-heading">Room Preference</h2>
                <span className="section-subtext">Optional venue preference</span>
              </div>

              <RoomPreferenceSelector
                rooms={rooms}
                loadingRooms={loadingRooms}
                preferredRoomId={preferredRoomId}
                roomMode={roomMode}
                attendance={attendance}
                requiredFacilityMask={facilityMask}
                onRoomChange={setPreferredRoomId}
                onModeChange={setRoomMode}
              />
            </section>

            {/* Actions Bar */}
            <div className="form-actions-bar">
              <button
                type="button"
                onClick={() => navigate('/club/dashboard')}
                className="btn btn-secondary"
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={loading}
                className="btn-submit-primary"
              >
                <CheckIcon size={16} />
                <span>{loading ? 'Submitting Request...' : 'Submit Request for Optimization'}</span>
              </button>
            </div>
          </form>
        </div>

        {/* Sticky Live Summary Panel (Wide Screens) */}
        <RequestLiveSummary
          title={title}
          date={date}
          startTime={startTime}
          endTime={endTime}
          attendance={attendance}
          tier={tier}
          facilityMask={facilityMask}
          rooms={rooms}
          preferredRoomId={preferredRoomId}
          roomMode={roomMode}
        />
      </div>
    </div>
  );
};
export default SubmitRequestPage;
