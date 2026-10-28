import React from 'react';
import { Routes, Route, Navigate, useLocation } from 'react-router-dom';
import { useAuth } from './context/AuthContext';
import { Navbar } from './components/Navbar';
import { ProtectedRoute } from './components/ProtectedRoute';

import { LandingPage } from './pages/LandingPage';
import { LoginPage } from './pages/LoginPage';

// Club Pages
import { ClubDashboardPage } from './pages/club/ClubDashboardPage';
import { SubmitRequestPage } from './pages/club/SubmitRequestPage';
import { MyRequestsPage } from './pages/club/MyRequestsPage';

// Admin Pages
import { AdminDashboardPage } from './pages/admin/AdminDashboardPage';
import { RoomsPage } from './pages/admin/RoomsPage';
import { RequestsPage } from './pages/admin/RequestsPage';
import { ReviewPage } from './pages/admin/ReviewPage';
import { BookingsPage } from './pages/admin/BookingsPage';
import { DecisionLogPage } from './pages/admin/DecisionLogPage';
import { AlgorithmInsightsPage } from './pages/admin/AlgorithmInsightsPage';

export const App: React.FC = () => {
  const { user } = useAuth();
  const location = useLocation();
  const isLanding = location.pathname === '/' || location.pathname === '/welcome';

  return (
    <div className={`app-container ${isLanding ? 'is-landing' : ''}`}>
      {!isLanding && user && <Navbar />}

      <main className={isLanding ? 'landing-main' : 'main-content'}>
        <Routes>
          {/* Cinematic Landing Page */}
          <Route path="/" element={<LandingPage />} />
          <Route path="/welcome" element={<LandingPage />} />

          <Route path="/login" element={<LoginPage />} />

          {/* Club Routes */}
          <Route
            path="/club/dashboard"
            element={
              <ProtectedRoute allowedRole="CLUB">
                <ClubDashboardPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/club/submit"
            element={
              <ProtectedRoute allowedRole="CLUB">
                <SubmitRequestPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/club/my-requests"
            element={
              <ProtectedRoute allowedRole="CLUB">
                <MyRequestsPage />
              </ProtectedRoute>
            }
          />

          {/* Admin Routes */}
          <Route
            path="/admin/dashboard"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <AdminDashboardPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/admin/rooms"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <RoomsPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/admin/requests"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <RequestsPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/admin/review"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <ReviewPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/admin/bookings"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <BookingsPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/admin/decisions"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <DecisionLogPage />
              </ProtectedRoute>
            }
          />
          <Route
            path="/admin/insights"
            element={
              <ProtectedRoute allowedRole="ADMIN">
                <AlgorithmInsightsPage />
              </ProtectedRoute>
            }
          />

          {/* Fallback route */}
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </main>
    </div>
  );
};
export default App;
