import React from 'react';
import { EventRequestStatus } from '../types/api';
import { CheckIcon, ClockIcon, SparklesIcon, XIcon } from './Icons';

interface StatusBadgeProps {
  status: EventRequestStatus | string;
}

export const StatusBadge: React.FC<StatusBadgeProps> = ({ status }) => {
  const norm = status.toUpperCase();
  let badgeClass = 'badge badge-default';
  let icon = null;

  if (norm === 'PENDING') {
    badgeClass = 'badge badge-pending';
    icon = <ClockIcon size={12} />;
  } else if (norm === 'RECOMMENDED') {
    badgeClass = 'badge badge-recommended';
    icon = <SparklesIcon size={12} />;
  } else if (norm === 'CONFIRMED') {
    badgeClass = 'badge badge-confirmed';
    icon = <CheckIcon size={12} />;
  } else if (norm === 'REJECTED') {
    badgeClass = 'badge badge-rejected';
    icon = <XIcon size={12} />;
  }

  return (
    <span className={badgeClass}>
      {icon}
      <span>{norm}</span>
    </span>
  );
};

export function getFacilityNames(mask: number): string {
  const facilities: string[] = [];
  if (mask & 1) facilities.push('Projector');
  if (mask & 2) facilities.push('Audio');
  if (mask & 4) facilities.push('Workstations');
  if (facilities.length === 0) return 'Standard';
  return facilities.join(', ');
}
