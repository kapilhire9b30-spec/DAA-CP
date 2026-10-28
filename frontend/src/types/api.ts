export type UserRole = 'CLUB' | 'ADMIN';

export interface UserSession {
  userId?: number;
  username: string;
  role: UserRole;
  clubName?: string | null;
  token?: string;
}

export interface LoginResponse {
  token: string;
  role: UserRole;
  username: string;
  clubName: string | null;
}

export interface ApiSuccessResponse<T> {
  ok: true;
  data: T;
}

export interface ApiErrorResponse {
  ok: false;
  error: string;
}

export interface Room {
  id: number;
  name: string;
  building: string;
  capacity: number;
  facilityMask: number;
  active: boolean;
}

export interface CreateRoomRequest {
  name: string;
  building: string;
  capacity: number;
  facilityMask: number;
}

export type EventRequestStatus = 'PENDING' | 'RECOMMENDED' | 'CONFIRMED' | 'REJECTED';

export interface EventRequestItem {
  id: number;
  userId: number;
  clubName: string;
  title: string;
  description: string;
  date: string;
  startTime: string;
  endTime: string;
  attendance: number;
  facilityMask: number;
  tier: number;
  submittedAt: number;
  status: EventRequestStatus;
  rejectionReason: string | null;
  lastUnallocatedReason: string | null;
}

export interface SubmitEventRequest {
  title: string;
  description: string;
  date: string;
  startTime: string;
  endTime: string;
  attendance: number;
  facilityMask: number;
  tier: number;
}

export interface Assignment {
  requestId: number;
  roomId: number;
}

export interface AllocateResponse {
  ok: boolean;
  error: string | null;
  algorithm: string;
  assignments: Assignment[];
  unallocated: number[];
  totalWeight: number;
  allocatedCount: number;
  runtimeMs: number;
  violations: string[];
  warnings: string[];
}

export interface CompareRow {
  algorithm: string;
  assignments: Assignment[];
  unallocated: number[];
  totalWeight: number;
  allocatedCount: number;
  runtimeMs: number;
  violations: string[];
  warnings: string[];
}

export interface CompareResponse {
  ok: boolean;
  error: string | null;
  rows: CompareRow[];
}

export interface RecommendationItem {
  id: number;
  request: EventRequestItem;
  proposedRoom: Room;
  algorithm: string;
  capacityWarning: boolean;
  otherFeasibleRooms: Room[];
  createdAt: string;
  active: boolean;
}

export interface Booking {
  id: number;
  requestId: number | null;
  roomId: number;
  date: string;
  startTime: string;
  endTime: string;
  approvedBy: string;
  createdAt: string;
  roomActive?: boolean;
}

export interface AffectedBooking {
  id: number;
  requestId: number | null;
  title: string;
  club: string;
  date: string;
  start: string;
  end: string;
  startTime?: string;
  endTime?: string;
  clubName?: string;
}

export interface RoomDeactivationResponse {
  id: number;
  name: string;
  building: string;
  capacity: number;
  facilityMask: number;
  active: boolean;
  room?: Room;
  affectedBookings?: AffectedBooking[];
}

export interface DecisionLog {
  id: number;
  who: string;
  at: string;
  action: string;
  requestId: number | null;
  roomId: number | null;
  reason: string | null;
  details: string;
}

export interface ModifyRecommendationResponse {
  ok: boolean;
  booking?: Booking;
  warning?: string;
  error?: string;
}
