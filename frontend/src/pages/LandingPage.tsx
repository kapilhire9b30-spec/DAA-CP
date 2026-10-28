import React, { useEffect, useState, useRef } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { LogoIcon } from '../components/Icons';
import { TimetableConstellation } from '../components/TimetableConstellation';
import './LandingPage.css';

export const LandingPage: React.FC = () => {
  const { user } = useAuth();
  const navigate = useNavigate();
  const heroRef = useRef<HTMLDivElement>(null);

  // Animated counters on scroll-into-view
  const [counts, setCounts] = useState({ algorithms: 0, conflicts: 0, tiers: 0 });
  const [hasAnimatedCounters, setHasAnimatedCounters] = useState(false);
  const countersSectionRef = useRef<HTMLElement>(null);

  // Navigate to current home/dashboard
  const handleEnterApp = () => {
    if (user) {
      if (user.role === 'ADMIN') {
        navigate('/admin/dashboard');
      } else {
        navigate('/club/dashboard');
      }
    } else {
      navigate('/login');
    }
  };

  // Track mouse coordinates for subtle aurora mesh (disabled on touch devices)
  const handleMouseMove = (e: React.MouseEvent<HTMLDivElement>) => {
    if (window.matchMedia('(hover: hover)').matches) {
      const xPercent = (e.clientX / window.innerWidth) * 100;
      const yPercent = (e.clientY / window.innerHeight) * 100;
      document.documentElement.style.setProperty('--mouse-x', `${xPercent}%`);
      document.documentElement.style.setProperty('--mouse-y', `${yPercent}%`);
    }
  };

  // Scroll to how-it-works section
  const handleScrollToHowItWorks = (e: React.MouseEvent) => {
    e.preventDefault();
    const target = document.getElementById('how-it-works');
    if (target) {
      target.scrollIntoView({ behavior: 'smooth' });
    }
  };

  // Counter animation when scrolled into view
  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        const [entry] = entries;
        if (entry.isIntersecting && !hasAnimatedCounters) {
          setHasAnimatedCounters(true);

          const duration = 1200; // ms
          const startTime = performance.now();

          const animate = (now: number) => {
            const progress = Math.min((now - startTime) / duration, 1);
            // Ease out quad
            const eased = 1 - Math.pow(1 - progress, 3);

            setCounts({
              algorithms: Math.round(eased * 3),
              conflicts: 0, // Always 0 by design!
              tiers: Math.round(eased * 4),
            });

            if (progress < 1) {
              requestAnimationFrame(animate);
            }
          };

          requestAnimationFrame(animate);
        }
      },
      { threshold: 0.3 }
    );

    if (countersSectionRef.current) {
      observer.observe(countersSectionRef.current);
    }

    return () => observer.disconnect();
  }, [hasAnimatedCounters]);

  return (
    <div className="landing-viewport" onMouseMove={handleMouseMove}>
      <div className="landing-aurora" aria-hidden="true" />

      {/* Navigation Header */}
      <header className="landing-nav" role="banner">
        <div className="landing-brand">
          <LogoIcon size={32} />
          <span className="landing-brand-title">CampusAlloc</span>
          <span className="landing-brand-badge">DAA Optimizer</span>
        </div>

        <div className="landing-nav-action">
          <button
            onClick={handleEnterApp}
            className="btn btn-secondary"
            aria-label="Enter CampusAlloc portal"
          >
            {user ? 'Go to Dashboard' : 'Sign In'}
          </button>
        </div>
      </header>

      {/* Hero Section */}
      <main>
        <section className="landing-hero" ref={heroRef} aria-labelledby="hero-title">
          <div className="landing-pill-tag">
            <span className="landing-pill-dot" />
            <span>High-Performance Combinatorial Engine</span>
          </div>

          <h1 id="hero-title" className="landing-headline">
            <span className="landing-word" style={{ animationDelay: '0.05s' }}>Every</span>{' '}
            <span className="landing-word" style={{ animationDelay: '0.12s' }}>room.</span>{' '}
            <span className="landing-word" style={{ animationDelay: '0.19s' }}>Every</span>{' '}
            <span className="landing-word" style={{ animationDelay: '0.26s' }}>event.</span>{' '}
            <span className="landing-word word-shimmer" style={{ animationDelay: '0.33s' }}>
              Perfectly
            </span>{' '}
            <span className="landing-word" style={{ animationDelay: '0.40s' }}>allocated.</span>
          </h1>

          <p className="landing-subtitle">
            CampusAlloc uses mathematical optimization algorithms to eliminate double-bookings, respect priority
            tiers, and balance campus room allocation with zero manual conflicts.
          </p>

          <div className="landing-cta-group">
            <button
              onClick={handleEnterApp}
              className="btn-enter-primary"
              aria-label="Enter CampusAlloc system"
            >
              <span>Enter CampusAlloc</span>
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                <line x1="5" y1="12" x2="19" y2="12" />
                <polyline points="12 5 19 12 12 19" />
              </svg>
            </button>

            <a
              href="#how-it-works"
              onClick={handleScrollToHowItWorks}
              className="link-secondary-scroll"
            >
              <span>See how it works</span>
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                <polyline points="6 9 12 15 18 9" />
              </svg>
            </a>
          </div>

          {/* Timetable Constellation Visual */}
          <div className="constellation-wrapper">
            <TimetableConstellation />
          </div>
        </section>

        {/* Highlights Honest Metric Strip */}
        <section className="landing-highlights" ref={countersSectionRef} aria-label="System Highlights">
          <div className="highlights-container">
            <div className="highlight-stat">
              <span className="stat-number">{counts.algorithms}</span>
              <span className="stat-label">Optimization Algorithms</span>
              <span className="stat-subtext">Priority Greedy, Earliest Finish, & Branch-and-Bound</span>
            </div>

            <div className="highlight-stat">
              <span className="stat-number">{counts.conflicts}</span>
              <span className="stat-label">Double-Bookings By Design</span>
              <span className="stat-subtext">Mathematically verified half-open interval overlap testing</span>
            </div>

            <div className="highlight-stat">
              <span className="stat-number">{counts.tiers}</span>
              <span className="stat-label">Priority Tiers</span>
              <span className="stat-subtext">Exponential weights ensuring flagship events win contested slots</span>
            </div>
          </div>
        </section>

        {/* How It Works Section */}
        <section id="how-it-works" className="landing-how-it-works" aria-labelledby="how-it-works-title">
          <div className="section-head">
            <div className="section-eyebrow">Algorithmic Workflow</div>
            <h2 id="how-it-works-title" className="section-title">
              From Club Proposal to Conflict-Free Schedule
            </h2>
          </div>

          <div className="cards-grid">
            <div className="step-card">
              <div className="step-number">1</div>
              <h3 className="step-title">Submit Event Request</h3>
              <p className="step-desc">
                Student clubs declare headcount, facility needs (projectors, audio, lab workstations), and preferred
                rooms with flexible or strict modes.
              </p>
              <span className="step-badge">Client Submission</span>
            </div>

            <div className="step-card">
              <div className="step-number">2</div>
              <h3 className="step-title">Algorithms Compete</h3>
              <p className="step-desc">
                The high-performance C++ solver executes Priority Greedy, Earliest Finish Time Greedy, and Exact Exhaustive Search to maximize allocated tier weight.
              </p>
              <span className="step-badge">DAA Solver Binary</span>
            </div>

            <div className="step-card">
              <div className="step-number">3</div>
              <h3 className="step-title">Independent Validation</h3>
              <p className="step-desc">
                Every candidate assignment is independently verified against active room statuses, facility bitmasks, and confirmed bookings before admin approval.
              </p>
              <span className="step-badge">Audited Confirmation</span>
            </div>
          </div>
        </section>

        {/* Final Call to Action */}
        <section className="landing-final-cta" aria-labelledby="cta-title">
          <div className="cta-container">
            <h2 id="cta-title" className="cta-title">Ready to optimize campus space?</h2>
            <p className="cta-desc">
              Experience automated room allocation backed by rigorous algorithmic guarantees and an audited approval workflow.
            </p>
            <button
              onClick={handleEnterApp}
              className="btn-enter-primary"
              aria-label="Enter CampusAlloc portal now"
            >
              <span>Enter CampusAlloc</span>
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                <line x1="5" y1="12" x2="19" y2="12" />
                <polyline points="12 5 19 12 12 19" />
              </svg>
            </button>
          </div>
        </section>
      </main>

      {/* Footer */}
      <footer className="landing-footer" role="contentinfo">
        <div className="footer-content">
          <p>Campus Resource Allocation Optimizer — DAA Project Team</p>
          <div className="footer-meta">
            <span>Polyglot Architecture: C++17 • Spring Boot 3 • React/TypeScript</span>
            <span>•</span>
            <span>Offline Variable Fonts</span>
          </div>
        </div>
      </footer>
    </div>
  );
};
