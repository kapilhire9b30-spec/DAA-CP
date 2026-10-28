import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { useToast } from '../context/ToastContext';
import { apiFetch, ApiError } from '../services/api';
import { LoginResponse } from '../types/api';
import { LogoIcon, CheckIcon, ChevronDownIcon } from '../components/Icons';

export const LoginPage: React.FC = () => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [demoOpen, setDemoOpen] = useState(false);

  const { login } = useAuth();
  const { addToast } = useToast();
  const navigate = useNavigate();

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setLoading(true);

    try {
      const res = await apiFetch<LoginResponse>('/api/auth/login', {
        method: 'POST',
        body: JSON.stringify({ username: username.trim(), password }),
      });

      login(res.token, {
        username: res.username,
        role: res.role,
        clubName: res.clubName,
      });

      addToast(`Welcome back, ${res.username}!`, 'success');

      if (res.role === 'ADMIN') {
        navigate('/admin/dashboard');
      } else {
        navigate('/club/dashboard');
      }
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        setError(err.message);
      } else if (err instanceof Error) {
        setError(err.message);
      } else {
        setError('Login failed. Please verify your credentials.');
      }
    } finally {
      setLoading(false);
    }
  };

  const fillCredentials = (u: string, p: string) => {
    setUsername(u);
    setPassword(p);
  };

  return (
    <div className="login-split-layout">
      {/* Left Branding Panel */}
      <div className="login-brand-panel">
        <div className="brand-panel-header">
          <LogoIcon size={36} />
          <span className="brand-panel-title">CampusAlloc</span>
        </div>

        <div className="brand-panel-content">
          <h1 className="brand-headline">Automated Campus Resource Scheduling</h1>
          <p className="brand-pitch">
            High-performance algorithmic allocation for campus spaces, lecture halls, and club events
            backed by exact C++ constraint solvers.
          </p>

          <div className="brand-features">
            <div className="feature-bullet">
              <div className="feature-icon-box">
                <CheckIcon size={16} />
              </div>
              <div className="feature-text">
                <h4>Conflict-Free Interval Scheduling</h4>
                <p>Ensures zero double-bookings with rigorous time and room validation.</p>
              </div>
            </div>

            <div className="feature-bullet">
              <div className="feature-icon-box">
                <CheckIcon size={16} />
              </div>
              <div className="feature-text">
                <h4>Automated Recommendations</h4>
                <p>Generates optimal room assignments respecting capacities, facilities, and club tiers.</p>
              </div>
            </div>

            <div className="feature-bullet">
              <div className="feature-icon-box">
                <CheckIcon size={16} />
              </div>
              <div className="feature-text">
                <h4>Full Administrative Governance</h4>
                <p>Review proposals, approve, modify, or reject with a complete immutable audit log.</p>
              </div>
            </div>
          </div>
        </div>

        <div className="brand-panel-footer">
          &copy; 2026 Campus Resource Allocation Optimizer • Phase 3d
        </div>
      </div>

      {/* Right Form Panel */}
      <div className="login-form-panel">
        <div className="login-card-saas">
          <div>
            <h2 className="login-welcome-title">Sign in to your account</h2>
            <p className="login-welcome-subtitle">Enter your institutional credentials to continue</p>
          </div>

          {error && (
            <div className="alert alert-error" role="alert">
              {error}
            </div>
          )}

          <form onSubmit={handleSubmit} className="form-group" style={{ gap: '1.2rem' }}>
            <div className="form-group">
              <label htmlFor="username">Username</label>
              <input
                id="username"
                type="text"
                required
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                placeholder="e.g. admin or robotics"
                autoComplete="username"
              />
            </div>

            <div className="form-group">
              <label htmlFor="password">Password</label>
              <input
                id="password"
                type="password"
                required
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="••••••••"
                autoComplete="current-password"
              />
            </div>

            <button type="submit" disabled={loading} className="btn btn-primary btn-block btn-lg">
              {loading ? 'Authenticating...' : 'Sign In'}
            </button>
          </form>

          {/* Demo Accounts Collapsible */}
          <div className="demo-collapsible">
            <div
              className="demo-collapsible-summary flex-between align-center"
              onClick={() => setDemoOpen(!demoOpen)}
              role="button"
              tabIndex={0}
              aria-expanded={demoOpen}
              onKeyDown={(e) => {
                if (e.key === 'Enter' || e.key === ' ') setDemoOpen(!demoOpen);
              }}
            >
              <span>Quick Demo Accounts (Seed Data)</span>
              <ChevronDownIcon size={14} className={demoOpen ? 'rotate-180' : ''} />
            </div>

            {demoOpen && (
              <table className="demo-accounts-table">
                <tbody>
                  <tr>
                    <td><strong>Administrator</strong></td>
                    <td><code>admin</code> / <code>admin123</code></td>
                    <td>
                      <button
                        type="button"
                        onClick={() => fillCredentials('admin', 'admin123')}
                        className="btn btn-ghost btn-sm"
                      >
                        Fill
                      </button>
                    </td>
                  </tr>
                  <tr>
                    <td><strong>Robotics Club</strong></td>
                    <td><code>robotics</code> / <code>robotics123</code></td>
                    <td>
                      <button
                        type="button"
                        onClick={() => fillCredentials('robotics', 'robotics123')}
                        className="btn btn-ghost btn-sm"
                      >
                        Fill
                      </button>
                    </td>
                  </tr>
                  <tr>
                    <td><strong>Coding Club</strong></td>
                    <td><code>coding</code> / <code>coding123</code></td>
                    <td>
                      <button
                        type="button"
                        onClick={() => fillCredentials('coding', 'coding123')}
                        className="btn btn-ghost btn-sm"
                      >
                        Fill
                      </button>
                    </td>
                  </tr>
                  <tr>
                    <td><strong>Literary Society</strong></td>
                    <td><code>literary</code> / <code>literary123</code></td>
                    <td>
                      <button
                        type="button"
                        onClick={() => fillCredentials('literary', 'literary123')}
                        className="btn btn-ghost btn-sm"
                      >
                        Fill
                      </button>
                    </td>
                  </tr>
                </tbody>
              </table>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
