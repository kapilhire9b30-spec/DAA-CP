import React, { useState, useRef, useEffect } from 'react';
import { NavLink, useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { useTheme } from '../context/ThemeContext';
import { LogoIcon, SunIcon, MoonIcon, MenuIcon, XIcon } from './Icons';

export const Navbar: React.FC = () => {
  const { user, logout } = useAuth();
  const { theme, toggleTheme } = useTheme();
  const navigate = useNavigate();

  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const [userDropdownOpen, setUserDropdownOpen] = useState(false);
  const dropdownRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const handleOutsideClick = (e: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setUserDropdownOpen(false);
      }
    };
    document.addEventListener('mousedown', handleOutsideClick);
    return () => document.removeEventListener('mousedown', handleOutsideClick);
  }, []);

  const handleLogout = async () => {
    await logout();
    navigate('/login');
  };

  if (!user) return null;

  const initials = (user.username || 'U').substring(0, 2).toUpperCase();

  return (
    <header className="navbar">
      <div className="navbar-container">
        <div className="navbar-left">
          <NavLink to={user.role === 'ADMIN' ? '/admin/dashboard' : '/club/dashboard'} className="navbar-brand">
            <LogoIcon size={30} />
            <div className="brand-text">
              <span className="brand-title">CampusAlloc</span>
              <span className="brand-role-tag">{user.role}</span>
            </div>
          </NavLink>

          {/* Desktop Navigation */}
          <nav className="desktop-nav" aria-label="Main Navigation">
            {user.role === 'ADMIN' ? (
              <>
                <NavLink to="/admin/dashboard" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Dashboard
                </NavLink>
                <NavLink to="/admin/rooms" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Rooms
                </NavLink>
                <NavLink to="/admin/requests" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Requests
                </NavLink>
                <NavLink to="/admin/review" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Review
                </NavLink>
                <NavLink to="/admin/bookings" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Bookings
                </NavLink>
                <NavLink to="/admin/decisions" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Audit Log
                </NavLink>
                <NavLink to="/admin/insights" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Algorithm Insights
                </NavLink>
              </>
            ) : (
              <>
                <NavLink to="/club/dashboard" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  Dashboard
                </NavLink>
                <NavLink to="/club/submit" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  New Request
                </NavLink>
                <NavLink to="/club/my-requests" className={({ isActive }) => (isActive ? 'nav-link active' : 'nav-link')}>
                  My Requests
                </NavLink>
              </>
            )}
          </nav>
        </div>

        <div className="navbar-right">
          {/* Theme Quick-toggle */}
          <button
            type="button"
            className="btn-icon"
            onClick={toggleTheme}
            aria-label={`Switch to ${theme === 'light' ? 'dark' : 'light'} theme`}
            title={`Switch to ${theme === 'light' ? 'dark' : 'light'} theme`}
          >
            {theme === 'light' ? <MoonIcon size={18} /> : <SunIcon size={18} />}
          </button>

          {/* User Menu Dropdown */}
          <div className="user-dropdown-container" ref={dropdownRef}>
            <button
              type="button"
              className="user-profile-button"
              onClick={() => setUserDropdownOpen(!userDropdownOpen)}
              aria-expanded={userDropdownOpen}
              aria-haspopup="true"
              aria-label="User profile menu"
            >
              <div className="user-avatar">{initials}</div>
              <div className="user-meta-label">
                <span className="user-name">{user.username}</span>
                {user.clubName && <span className="user-subtext">{user.clubName}</span>}
              </div>
            </button>

            {userDropdownOpen && (
              <div className="dropdown-card" role="menu">
                <div className="dropdown-header">
                  <div className="user-full-name">{user.username}</div>
                  <div className="user-role-badge">
                    {user.role} {user.clubName ? `• ${user.clubName}` : ''}
                  </div>
                </div>

                <div className="dropdown-divider" />

                <button
                  type="button"
                  className="dropdown-item"
                  onClick={() => {
                    toggleTheme();
                    setUserDropdownOpen(false);
                  }}
                  role="menuitem"
                >
                  <span className="item-icon">{theme === 'light' ? <MoonIcon size={16} /> : <SunIcon size={16} />}</span>
                  <span>{theme === 'light' ? 'Dark Mode' : 'Light Mode'}</span>
                </button>

                <div className="dropdown-divider" />

                <button
                  type="button"
                  className="dropdown-item text-danger"
                  onClick={() => {
                    setUserDropdownOpen(false);
                    handleLogout();
                  }}
                  role="menuitem"
                >
                  <span className="item-icon"><XIcon size={16} /></span>
                  <span>Sign out</span>
                </button>
              </div>
            )}
          </div>

          {/* Mobile hamburger button */}
          <button
            type="button"
            className="mobile-menu-toggle btn-icon"
            onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
            aria-label={mobileMenuOpen ? 'Close menu' : 'Open menu'}
            aria-expanded={mobileMenuOpen}
          >
            {mobileMenuOpen ? <XIcon size={22} /> : <MenuIcon size={22} />}
          </button>
        </div>
      </div>

      {/* Mobile Drawer */}
      {mobileMenuOpen && (
        <div className="mobile-drawer">
          <nav className="mobile-nav">
            {user.role === 'ADMIN' ? (
              <>
                <NavLink
                  to="/admin/dashboard"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Dashboard
                </NavLink>
                <NavLink
                  to="/admin/rooms"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Rooms
                </NavLink>
                <NavLink
                  to="/admin/requests"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Requests
                </NavLink>
                <NavLink
                  to="/admin/review"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Review
                </NavLink>
                <NavLink
                  to="/admin/bookings"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Bookings
                </NavLink>
                <NavLink
                  to="/admin/decisions"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Audit Log
                </NavLink>
                <NavLink
                  to="/admin/insights"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Algorithm Insights
                </NavLink>
              </>
            ) : (
              <>
                <NavLink
                  to="/club/dashboard"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  Dashboard
                </NavLink>
                <NavLink
                  to="/club/submit"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  New Request
                </NavLink>
                <NavLink
                  to="/club/my-requests"
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) => (isActive ? 'mobile-nav-link active' : 'mobile-nav-link')}
                >
                  My Requests
                </NavLink>
              </>
            )}

            <div className="mobile-nav-divider" />
            <button
              type="button"
              className="mobile-nav-link text-danger"
              onClick={() => {
                setMobileMenuOpen(false);
                handleLogout();
              }}
            >
              Sign out
            </button>
          </nav>
        </div>
      )}
    </header>
  );
};
