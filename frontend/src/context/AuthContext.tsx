import React, { createContext, useContext, useState, useEffect } from 'react';
import { UserSession } from '../types/api';
import { getStoredToken, getStoredUser, clearStoredSession, setStoredSession, apiFetch } from '../services/api';

interface AuthContextType {
  user: UserSession | null;
  loading: boolean;
  login: (token: string, user: { username: string; role: 'CLUB' | 'ADMIN'; clubName: string | null }) => void;
  logout: () => Promise<void>;
  refreshMe: () => Promise<void>;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const AuthProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [user, setUser] = useState<UserSession | null>(null);
  const [loading, setLoading] = useState(true);

  const refreshMe = async () => {
    const token = getStoredToken();
    if (!token) {
      setUser(null);
      setLoading(false);
      return;
    }

    try {
      const data = await apiFetch<UserSession>('/api/me');
      setUser({ ...data, token });
    } catch {
      const stored = getStoredUser();
      if (stored) {
        setUser({ ...stored, token });
      } else {
        setUser(null);
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    refreshMe();
  }, []);

  const login = (token: string, userData: { username: string; role: 'CLUB' | 'ADMIN'; clubName: string | null }) => {
    setStoredSession(token, userData);
    setUser({ ...userData, token });
  };

  const logout = async () => {
    try {
      await apiFetch<{ ok: boolean; data: string }>('/api/auth/logout', {
        method: 'POST',
      });
    } catch {
      // ignore network or session errors on logout
    } finally {
      clearStoredSession();
      setUser(null);
    }
  };

  return (
    <AuthContext.Provider value={{ user, loading, login, logout, refreshMe }}>
      {children}
    </AuthContext.Provider>
  );
};

export const useAuth = (): AuthContextType => {
  const context = useContext(AuthContext);
  if (!context) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
};
