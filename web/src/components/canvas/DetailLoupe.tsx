import React, { useState, useRef } from 'react';

interface DetailLoupeProps {
  imageUrl: string;
  zoomLevel?: number;
  loupeSize?: number;
}

export const DetailLoupe: React.FC<DetailLoupeProps> = ({
  imageUrl,
  zoomLevel = 2.5,
  loupeSize = 160,
}) => {
  const [showLoupe, setShowLoupe] = useState(false);
  const [pos, setPos] = useState({ x: 0, y: 0, relX: 0, relY: 0 });
  const containerRef = useRef<HTMLDivElement>(null);
  const imgRef = useRef<HTMLImageElement>(null);

  const handleMouseMove = (e: React.MouseEvent<HTMLDivElement>) => {
    if (!containerRef.current || !imgRef.current) return;
    const rect = imgRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    if (x < 0 || y < 0 || x > rect.width || y > rect.height) {
      setShowLoupe(false);
      return;
    }

    setShowLoupe(true);
    setPos({
      x: e.clientX - containerRef.current.getBoundingClientRect().left,
      y: e.clientY - containerRef.current.getBoundingClientRect().top,
      relX: (x / rect.width) * 100,
      relY: (y / rect.height) * 100,
    });
  };

  return (
    <div
      ref={containerRef}
      onMouseMove={handleMouseMove}
      onMouseLeave={() => setShowLoupe(false)}
      className="relative w-full h-full flex items-center justify-center overflow-hidden cursor-crosshair"
    >
      <img
        ref={imgRef}
        src={imageUrl}
        alt="Loupe Target"
        className="max-h-full max-w-full object-contain pointer-events-auto"
      />

      {/* Floating Loupe Lens */}
      {showLoupe && (
        <div
          className="pointer-events-none absolute rounded-full border-2 border-cyan-400 shadow-[0_0_20px_rgba(0,0,0,0.8),0_0_10px_rgba(6,182,212,0.6)] z-30 overflow-hidden bg-slate-900"
          style={{
            width: `${loupeSize}px`,
            height: `${loupeSize}px`,
            left: `${pos.x - loupeSize / 2}px`,
            top: `${pos.y - loupeSize / 2}px`,
          }}
        >
          <div
            className="w-full h-full"
            style={{
              backgroundImage: `url(${imageUrl})`,
              backgroundPosition: `${pos.relX}% ${pos.relY}%`,
              backgroundSize: `${zoomLevel * 100}%`,
              backgroundRepeat: 'no-repeat',
            }}
          />
          <div className="absolute bottom-1 right-2 text-[9px] font-mono bg-black/60 text-cyan-300 px-1 rounded">
            {zoomLevel}x
          </div>
        </div>
      )}
    </div>
  );
};
