import React from 'react';
import type { DetectedFace } from '../../api/types';

interface FaceOverlayProps {
  faces: DetectedFace[];
  imageWidth: number;
  imageHeight: number;
  selectedFaceIndex: number | null;
  onSelectFace: (face: DetectedFace) => void;
}

export const FaceOverlay: React.FC<FaceOverlayProps> = ({
  faces,
  imageWidth,
  imageHeight,
  selectedFaceIndex,
  onSelectFace,
}) => {
  if (!faces || faces.length === 0 || !imageWidth || !imageHeight) return null;

  return (
    <div className="absolute inset-0 pointer-events-none">
      {faces.map((face) => {
        const { box, score, gender, age_range, index } = face;
        const leftPercent = (box.x / imageWidth) * 100;
        const topPercent = (box.y / imageHeight) * 100;
        const widthPercent = (box.width / imageWidth) * 100;
        const heightPercent = (box.height / imageHeight) * 100;
        const isSelected = selectedFaceIndex === index;

        return (
          <div
            key={`face-${index}`}
            onClick={(e) => {
              e.stopPropagation();
              onSelectFace(face);
            }}
            className={`absolute pointer-events-auto cursor-pointer rounded transition-all duration-200 group ${
              isSelected
                ? 'border-2 border-rose-500 bg-rose-500/20 shadow-[0_0_15px_rgba(244,63,94,0.6)]'
                : 'border-2 border-cyan-400/80 bg-cyan-400/10 hover:border-cyan-300 hover:bg-cyan-400/25 hover:shadow-[0_0_12px_rgba(6,182,212,0.4)]'
            }`}
            style={{
              left: `${leftPercent}%`,
              top: `${topPercent}%`,
              width: `${widthPercent}%`,
              height: `${heightPercent}%`,
            }}
          >
            {/* Tag / Badge above face box */}
            <div
              className={`absolute -top-6 left-0 px-1.5 py-0.5 text-[10px] font-mono whitespace-nowrap rounded font-semibold flex items-center gap-1 shadow-md transition-colors ${
                isSelected
                  ? 'bg-rose-600 text-white'
                  : 'bg-slate-900/90 text-cyan-300 border border-cyan-500/40 group-hover:bg-cyan-900 group-hover:text-cyan-100'
              }`}
            >
              <span>#{index + 1}</span>
              <span>{(score * 100).toFixed(0)}%</span>
              {gender && <span>· {gender === 'female' ? '♀' : '♂'}</span>}
              {age_range && <span>{age_range[0]}-{age_range[1]}</span>}
              {isSelected && <span className="ml-1 text-[9px] bg-rose-800 px-1 rounded">REFERENCE</span>}
            </div>

            {/* Corner Indicators */}
            <div className="absolute top-0 left-0 w-2 h-2 border-t-2 border-l-2 border-white/80" />
            <div className="absolute top-0 right-0 w-2 h-2 border-t-2 border-r-2 border-white/80" />
            <div className="absolute bottom-0 left-0 w-2 h-2 border-b-2 border-l-2 border-white/80" />
            <div className="absolute bottom-0 right-0 w-2 h-2 border-b-2 border-r-2 border-white/80" />
          </div>
        );
      })}
    </div>
  );
};
