import React from 'react';

interface SkeletonProps {
  rows?: number;
  height?: string;
}

export const SkeletonTable: React.FC<SkeletonProps> = ({ rows = 4, height = '44px' }) => {
  return (
    <div className="skeleton-container" aria-label="Loading content..." role="status">
      {Array.from({ length: rows }).map((_, i) => (
        <div key={i} className="skeleton-row" style={{ height }} />
      ))}
    </div>
  );
};

export const SkeletonCard: React.FC<{ count?: number }> = ({ count = 3 }) => {
  return (
    <div className="skeleton-grid" aria-label="Loading cards..." role="status">
      {Array.from({ length: count }).map((_, i) => (
        <div key={i} className="skeleton-card" />
      ))}
    </div>
  );
};
