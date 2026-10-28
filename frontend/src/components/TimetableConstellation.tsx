import React, { useEffect, useRef } from 'react';

interface ConstellationProps {
  className?: string;
}

interface EventBlock {
  id: number;
  roomIdx: number;
  trackY: number;
  targetX: number;
  targetW: number;
  curX: number;
  curY: number;
  curW: number;
  color: string;
  label: string;
  driftAngle: number;
  driftSpeed: number;
}

export const TimetableConstellation: React.FC<ConstellationProps> = ({ className }) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Check prefers-reduced-motion
    const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

    let animationFrameId: number;
    let width = (canvas.width = canvas.parentElement?.clientWidth || 800);
    let height = (canvas.height = canvas.parentElement?.clientHeight || 450);

    const handleResize = () => {
      if (!canvas || !canvas.parentElement) return;
      width = canvas.width = canvas.parentElement.clientWidth;
      height = canvas.height = canvas.parentElement.clientHeight;
      initBlocks();
    };

    window.addEventListener('resize', handleResize);

    // Warm colors matching CampusAlloc room palette
    const colors = [
      '#C2633A', // Terracotta
      '#4FB38A', // Forest green
      '#E0A24A', // Saffron amber
      '#839139', // Olive
      '#B87C24', // Tawny
    ];

    const roomNames = ['Auditorium', 'Seminar Hall', 'Lab 1', 'Hall B'];
    let blocks: EventBlock[] = [];

    const initBlocks = () => {
      const numRooms = roomNames.length;
      const rowHeight = (height - 60) / numRooms;
      blocks = [];

      const rawData = [
        { roomIdx: 0, targetX: 0.15, targetW: 0.28, label: 'Tech Symposium' },
        { roomIdx: 0, targetX: 0.48, targetW: 0.35, label: 'Keynote Lecture' },
        { roomIdx: 1, targetX: 0.10, targetW: 0.22, label: 'AI Workshop' },
        { roomIdx: 1, targetX: 0.36, targetW: 0.25, label: 'Robotics Demo' },
        { roomIdx: 1, targetX: 0.65, targetW: 0.26, label: 'Literary Slam' },
        { roomIdx: 2, targetX: 0.20, targetW: 0.38, label: 'CodeSprint 2026' },
        { roomIdx: 2, targetX: 0.62, targetW: 0.28, label: 'Hardware Lab' },
        { roomIdx: 3, targetX: 0.12, targetW: 0.24, label: 'Debate Finals' },
        { roomIdx: 3, targetX: 0.40, targetW: 0.45, label: 'Hackathon Prep' },
      ];

      blocks = rawData.map((d, i) => {
        const trackY = 40 + d.roomIdx * rowHeight + rowHeight * 0.2;
        const targetX = d.targetX * (width - 140) + 120;
        const targetW = d.targetW * (width - 140);

        return {
          id: i,
          roomIdx: d.roomIdx,
          trackY,
          targetX,
          targetW,
          // Initial chaotic float
          curX: targetX + (Math.sin(i * 1.7) * 90),
          curY: trackY + (Math.cos(i * 2.3) * 35),
          curW: targetW,
          color: colors[i % colors.length],
          label: d.label,
          driftAngle: i * 0.8,
          driftSpeed: 0.02 + (i % 3) * 0.01,
        };
      });
    };

    initBlocks();

    let phase = 0; // 0 to 1 cycle: 0..0.4 = drift/chaos, 0.4..0.55 = snap to order, 0.55..0.85 = orderly locked, 0.85..1.0 = release
    let lastTime = performance.now();
    let isVisible = !document.hidden;

    const handleVisibilityChange = () => {
      isVisible = !document.hidden;
      if (isVisible) {
        lastTime = performance.now();
        loop(performance.now());
      } else {
        cancelAnimationFrame(animationFrameId);
      }
    };

    document.addEventListener('visibilitychange', handleVisibilityChange);

    const loop = (now: number) => {
      if (!isVisible) return;

      const dt = Math.min((now - lastTime) / 1000, 0.1);
      lastTime = now;

      if (!prefersReducedMotion) {
        phase = (phase + dt * 0.18) % 1.0;
      } else {
        phase = 0.7; // Static ordered state for reduced motion
      }

      ctx.clearRect(0, 0, width, height);

      // Determine orderliness interpolation factor: 0 = chaotic drift, 1 = perfectly snapped
      let orderliness = 0;
      if (phase < 0.35) {
        orderliness = 0; // Pure chaotic drift
      } else if (phase >= 0.35 && phase < 0.55) {
        const t = (phase - 0.35) / 0.2;
        // Smooth easeInOutCubic
        orderliness = t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
      } else if (phase >= 0.55 && phase < 0.85) {
        orderliness = 1; // Locked conflict-free
      } else {
        const t = (phase - 0.85) / 0.15;
        orderliness = 1 - (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2);
      }

      const numRooms = roomNames.length;
      const rowHeight = (height - 60) / numRooms;

      // Draw background room tracks
      ctx.lineWidth = 1;
      for (let r = 0; r < numRooms; r++) {
        const y = 40 + r * rowHeight;

        // Subtle track guide
        ctx.strokeStyle = 'rgba(235, 225, 210, 0.08)';
        ctx.setLineDash([4, 6]);
        ctx.beginPath();
        ctx.moveTo(110, y + rowHeight * 0.5);
        ctx.lineTo(width - 20, y + rowHeight * 0.5);
        ctx.stroke();
        ctx.setLineDash([]);

        // Room label badge
        ctx.fillStyle = 'rgba(235, 225, 210, 0.45)';
        ctx.font = '12px "Source Sans 3 Variable", system-ui, sans-serif';
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        ctx.fillText(roomNames[r], 95, y + rowHeight * 0.5);
      }

      // Draw connecting constellation lines between harmonious blocks
      if (orderliness > 0.4) {
        ctx.strokeStyle = `rgba(224, 162, 74, ${(orderliness - 0.4) * 0.35})`;
        ctx.lineWidth = 1;
        ctx.beginPath();
        for (let i = 0; i < blocks.length - 1; i++) {
          const b1 = blocks[i];
          const b2 = blocks[i + 1];
          if (b1.roomIdx !== b2.roomIdx) {
            ctx.moveTo(b1.curX + b1.curW * 0.5, b1.curY + 12);
            ctx.lineTo(b2.curX + b2.curW * 0.5, b2.curY + 12);
          }
        }
        ctx.stroke();
      }

      // Update and draw blocks
      blocks.forEach((b) => {
        b.driftAngle += b.driftSpeed;
        const driftX = Math.sin(b.driftAngle) * 45;
        const driftY = Math.cos(b.driftAngle * 1.3) * 18;

        const chaoticX = b.targetX + driftX;
        const chaoticY = b.trackY + driftY;

        // Interpolate between chaotic position and snapped conflict-free slot
        b.curX = chaoticX * (1 - orderliness) + b.targetX * orderliness;
        b.curY = chaoticY * (1 - orderliness) + b.trackY * orderliness;

        const blockH = 24;

        // Soft glow when ordered
        if (orderliness > 0.2) {
          ctx.save();
          ctx.shadowColor = b.color;
          ctx.shadowBlur = 10 * orderliness;
          ctx.fillStyle = b.color;
          ctx.globalAlpha = 0.25 * orderliness;
          ctx.beginPath();
          ctx.roundRect(b.curX - 2, b.curY - 2, b.curW + 4, blockH + 4, 6);
          ctx.fill();
          ctx.restore();
        }

        // Main pill body
        ctx.fillStyle = b.color;
        ctx.globalAlpha = 0.88;
        ctx.beginPath();
        ctx.roundRect(b.curX, b.curY, b.curW, blockH, 5);
        ctx.fill();

        // Border highlighting lock state
        ctx.strokeStyle = orderliness > 0.7 ? '#FFF6E5' : 'rgba(255, 255, 255, 0.4)';
        ctx.lineWidth = 1;
        ctx.stroke();

        // Label text
        ctx.globalAlpha = 1.0;
        ctx.fillStyle = '#151310';
        ctx.font = '600 11px "Source Sans 3 Variable", system-ui, sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        const labelText = b.label;
        if (b.curW > 50) {
          ctx.fillText(labelText, b.curX + 8, b.curY + blockH / 2, b.curW - 14);
        }
      });

      // Status indicator on top right
      ctx.fillStyle = orderliness > 0.7 ? '#4FB38A' : '#E0A24A';
      ctx.beginPath();
      ctx.arc(width - 150, 20, 4, 0, Math.PI * 2);
      ctx.fill();

      ctx.fillStyle = 'rgba(241, 236, 227, 0.75)';
      ctx.font = '500 12px "Source Sans 3 Variable", system-ui, sans-serif';
      ctx.textAlign = 'left';
      ctx.fillText(
        orderliness > 0.7 ? 'Optimal Schedule: Conflict-Free' : 'Simulating Allocations...',
        width - 140,
        21
      );

      if (!prefersReducedMotion) {
        animationFrameId = requestAnimationFrame(loop);
      }
    };

    animationFrameId = requestAnimationFrame(loop);

    return () => {
      cancelAnimationFrame(animationFrameId);
      window.removeEventListener('resize', handleResize);
      document.removeEventListener('visibilitychange', handleVisibilityChange);
    };
  }, []);

  return (
    <div className={`constellation-container ${className || ''}`} aria-hidden="true">
      <canvas ref={canvasRef} className="constellation-canvas" />
    </div>
  );
};
