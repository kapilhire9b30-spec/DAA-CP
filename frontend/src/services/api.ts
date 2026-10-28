import { ApiErrorResponse } from '../types/api';

const TOKEN_KEY = 'caro_auth_token';
const USER_KEY = 'caro_auth_user';

export class ApiError extends Error {
  status: number;
  constructor(message: string, status: number) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
  }
}

export function getStoredToken(): string | null {
  return sessionStorage.getItem(TOKEN_KEY);
}

export function setStoredSession(token: string, user: { username: string; role: 'CLUB' | 'ADMIN'; clubName: string | null }): void {
  sessionStorage.setItem(TOKEN_KEY, token);
  sessionStorage.setItem(USER_KEY, JSON.stringify(user));
}

export function getStoredUser(): { username: string; role: 'CLUB' | 'ADMIN'; clubName: string | null } | null {
  const item = sessionStorage.getItem(USER_KEY);
  if (!item) return null;
  try {
    return JSON.parse(item);
  } catch {
    return null;
  }
}

export function clearStoredSession(): void {
  sessionStorage.removeItem(TOKEN_KEY);
  sessionStorage.removeItem(USER_KEY);
}

export async function apiFetch<T>(
  endpoint: string,
  options: RequestInit = {}
): Promise<T> {
  const token = getStoredToken();
  const headers = new Headers(options.headers || {});

  if (!headers.has('Content-Type') && !(options.body instanceof FormData)) {
    headers.set('Content-Type', 'application/json; charset=UTF-8');
  }

  if (token) {
    headers.set('Authorization', `Bearer ${token}`);
  }

  const response = await fetch(endpoint, {
    ...options,
    headers,
  });

  if (response.status === 401) {
    clearStoredSession();
    // Only redirect if not already on the login page
    if (!window.location.pathname.startsWith('/login')) {
      window.location.href = '/login';
    }
    throw new ApiError('Unauthorized. Please log in again.', 401);
  }

  let json: unknown;
  try {
    json = await response.json();
  } catch {
    if (!response.ok) {
      throw new ApiError(`Request failed with status ${response.status}`, response.status);
    }
    return {} as T;
  }

  if (!response.ok) {
    const errorBody = json as Partial<ApiErrorResponse>;
    const msg = errorBody.error || `HTTP error ${response.status}`;
    throw new ApiError(msg, response.status);
  }

  return json as T;
}

export async function activateRoom(roomId: number) {
  return apiFetch<{ ok: boolean; data: import('../types/api').Room }>(`/api/rooms/${roomId}/activate`, {
    method: 'POST',
  });
}

export async function deactivateRoom(roomId: number) {
  return apiFetch<{ ok: boolean; data: import('../types/api').RoomDeactivationResponse }>(`/api/rooms/${roomId}/deactivate`, {
    method: 'POST',
  });
}

export async function getAffectedBookings(roomId: number) {
  return apiFetch<import('../types/api').AffectedBooking[]>(`/api/rooms/${roomId}/affected-bookings`);
}
