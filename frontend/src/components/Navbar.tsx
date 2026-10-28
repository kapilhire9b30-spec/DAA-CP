import React, { useState, useRef, useEffect } from 'react';
import { NavLink, useNavigate, useLocation } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { useTheme } from '../context/ThemeContext';
import { LogoIcon, SunIcon, MoonIcon, MenuIcon, XIcon, ChevronDownIcon } from './Icons';
import './Navbar.css';

export const Navbar: React.FC = () => {
  const { user, logout } = useAuth();
  const { theme, toggleTheme } = useTheme();
  const navigate = useNavigate();
  const location = useLocation();

  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const [userDropdownOpen, setUserDropdownOpen] = useState(false);
  const [isScrolled, setIsScrolled] = useState(false);
  const dropdownRef = useRef<HTMLDivElement>(null);

  // Close menus on outside click
  useEffect(() => {
    const handleOutsideClick = (e: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setUserDropdownOpen(false);
      }
    };
    document.addEventListener('mousedown', handleOutsideClick);
    return () => document.removeEventListener('mousedown', handleOutsideClick);
  }, []);

  // Close menus on Escape key
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setUserDropdownOpen(false);
        setMobileMenuOpen(false);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  // Close mobile drawer on route change
  useEffect(() => {
    setMobileMenuOpen(false);
    setUserDropdownOpen(false);
  }, [location.pathname]);

  // Track window scroll for glass shrink micro-interaction
  useEffect(() => {
    const handleScroll = () => {
      setIsScrolled(window.scrollY > 20);
    };
    window.addEventListener('scroll', handleScroll, { passive: true });
    return () => window.removeEventListener('scroll', handleScroll);
  }, []);

  const handleLogout = async () => {
    await logout();
    navigate('/login');
  };

  if (!user) return null;

  // Determine avatar initials safely
  const displayName = user.clubName || user.username || 'User';
  const initials = (displayName.length >= 2 ? displayName.substring(0, 2) : displayName.substring(0, 1)).toUpperCase();

  return (
    <header className={`navbar ${isScrolled ? 'scrolled' : ''}`}>
      <div className="navbar-container">
        <div className="navbar-left">
          <NavLink
            to={user.role === 'ADMIN' ? '/admin/dashboard' : '/club/dashboard'}
            className="navbar-brand"
            aria-label="CampusAlloc Home"
          >
            <LogoIcon size={30} />
            <div className="brand-text">
              <span className="brand-title">CampusAlloc</span>
              <span className="brand-badge-role">{user.role}</span>
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
          {/* Sliding Pill Theme Switcher */}
          <button
            type="button"
            className="theme-pill-toggle"
            onClick={toggleTheme}
            aria-label={`Switch to ${theme === 'light' ? 'dark' : 'light'} theme`}
            title={`Switch to ${theme === 'light' ? 'dark' : 'light'} theme`}
          >
            <div className="theme-pill-track">
              <SunIcon size={12} />
              <MoonIcon size={12} />
            </div>
            <div className="theme-pill-thumb">
              {theme === 'light' ? <SunIcon size={13} /> : <MoonIcon size={13} />}
            </div>
          </button>

          {/* User Profile Chip & Dropdown */}
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
                <span className="user-name">{displayName}</span>
                <span className="user-role-badge-chip">
                  {user.role === 'ADMIN' ? 'Administrator' : 'Club Lead'}
                </span>
              </div>
              <ChevronDownIcon size={14} className="user-chevron" />
            </button>

            {userDropdownOpen && (
              <div className="dropdown-card" role="menu">
                <div className="dropdown-header">
                  <div className="dropdown-header-user">{user.username}</div>
                  <div className="dropdown-header-sub">
                    {user.clubName ? `${user.clubName} • ` : ''}{user.role}
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

          {/* Mobile hamburger button - strictly hidden on desktop via CSS */}
          <button
            type="button"
            className="mobile-menu-toggle"
            onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
            aria-label={mobileMenuOpen ? 'Close navigation menu' : 'Open navigation menu'}
            aria-expanded={mobileMenuOpen}
          >
            {mobileMenuOpen ? <XIcon size={20} /> : <MenuIcon size={20} />}
          </button>
        </div>

        {/* Scroll Progress Accent */}
        <div className="navbar-scroll-progress" />
      </div>

      {/* Mobile Drawer Backdrop */}
      {mobileMenuOpen && (
        <div
          className="mobile-drawer-overlay"
          onClick={() => setMobileMenuOpen(false)}
          aria-hidden="true"
        />
      )}

      {/* Mobile Drawer Sheet */}
      {mobileMenuOpen && (
        <div className="mobile-drawer" role="dialog" aria-modal="true" aria-label="Mobile Navigation">
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
