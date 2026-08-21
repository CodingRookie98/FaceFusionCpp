import React, { useState, useRef, useEffect, useCallback } from 'react';
import { Columns } from 'lucide-react';

interface SplitSliderProps {
  originalUrl: string;
  resultUrl: string;
  isVideo?: boolean;
  initialSplit?: number;
}

export const SplitSlider: React.FC<SplitSliderProps> = ({
  originalUrl,
  resultUrl,
  isVideo = false,
  initialSplit = 50,
}) => {
  const [split, setSplit] = useState(initialSplit);
  const [isDragging, setIsDragging] = useState(false);
  const containerRef = useRef<HTMLDivElement>(null);
  const video1Ref = useRef<HTMLVideoElement>(null);
  const video2Ref = useRef<HTMLVideoElement>(null);

  const handlePointerDown = (e: React.PointerEvent) => {
    e.preventDefault();
    setIsDragging(true);
  };

  const handlePointerMove = useCallback(
    (e: PointerEvent) => {
      if (!isDragging || !containerRef.current) return;
      const rect = containerRef.current.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const percent = Math.min(Math.max((x / rect.width) * 100, 0), 100);
      setSplit(percent);
    },
    [isDragging]
  );

  const handlePointerUp = useCallback(() => {
    setIsDragging(false);
  }, []);

  useEffect(() => {
    if (isDragging) {
      window.addEventListener('pointermove', handlePointerMove);
      window.addEventListener('pointerup', handlePointerUp);
    }
    return () => {
      window.removeEventListener('pointermove', handlePointerMove);
      window.removeEventListener('pointerup', handlePointerUp);
    };
  }, [isDragging, handlePointerMove, handlePointerUp]);

  // Synced video playback if media is video
  const syncVideos = (e: React.SyntheticEvent<HTMLVideoElement>) => {
    const target = e.currentTarget;
    const other = target === video1Ref.current ? video2Ref.current : video1Ref.current;
    if (other && Math.abs(other.currentTime - target.currentTime) > 0.05) {
      other.currentTime = target.currentTime;
    }
  };

  return (
    <div
      ref={containerRef}
      className="relative w-full h-full max-h-[calc(100vh-180px)] select-none overflow-hidden rounded-lg bg-slate-950/90 flex items-center justify-center border border-white/10 shadow-2xl"
    >
      {/* Background (Result - Processed) Layer */}
      <div className="absolute inset-0 w-full h-full flex items-center justify-center">
        {isVideo ? (
          <video
            ref={video2Ref}
            src={resultUrl}
            controls
            onTimeUpdate={syncVideos}
            className="w-full h-full object-contain pointer-events-auto"
          />
        ) : (
          <img
            src={resultUrl}
            alt="Processed"
            className="w-full h-full object-contain"
          />
        )}
        <div className="absolute top-3 right-3 bg-emerald-950/90 text-emerald-300 border border-emerald-500/50 text-xs px-2.5 py-1 rounded font-mono font-medium shadow-md backdrop-blur-sm z-10">
          处理后 (AFTER)
        </div>
      </div>

      {/* Foreground (Original) Clipped Layer */}
      <div
        className="absolute inset-0 w-full h-full flex items-center justify-center overflow-hidden pointer-events-none"
        style={{
          clipPath: `polygon(0 0, ${split}% 0, ${split}% 100%, 0 100%)`,
        }}
      >
        {isVideo ? (
          <video
            ref={video1Ref}
            src={originalUrl}
            controls
            onTimeUpdate={syncVideos}
            className="w-full h-full object-contain pointer-events-auto"
          />
        ) : (
          <img
            src={originalUrl}
            alt="Original"
            className="w-full h-full object-contain"
          />
        )}
        <div className="absolute top-3 left-3 bg-blue-950/90 text-blue-300 border border-blue-500/50 text-xs px-2.5 py-1 rounded font-mono font-medium shadow-md backdrop-blur-sm z-10">
          处理前 (BEFORE)
        </div>
      </div>

      {/* Draggable Divider Handle */}
      <div
        className="absolute top-0 bottom-0 w-1 bg-cyan-400 cursor-ew-resize z-20 shadow-[0_0_12px_rgba(6,182,212,0.8)]"
        style={{ left: `${split}%` }}
        onPointerDown={handlePointerDown}
      >
        <div className="absolute top-1/2 -translate-y-1/2 -translate-x-1/2 w-8 h-8 rounded-full bg-cyan-500 text-slate-950 flex items-center justify-center shadow-lg border-2 border-white cursor-ew-resize hover:scale-110 active:scale-95 transition-transform">
          <Columns className="w-4 h-4" />
        </div>
      </div>
    </div>
  );
};
