import React from 'react';
import type { DetectedFace } from '../../api/types';

interface FaceOverlayProps {
  faces: DetectedFace[];
  imageWidth: number;
  imageHeight: number;
  selectedFaceIndices?: number[];
  selectedFaceIndex?: number | null;
  onToggleFace?: (faceOrIndex: number | DetectedFace) => void;
  onSelectFace?: (face: DetectedFace) => void;
}

export const FaceOverlay: React.FC<FaceOverlayProps> = ({
  faces,
  imageWidth,
  imageHeight,
  selectedFaceIndices,
  selectedFaceIndex,
  onToggleFace,
  onSelectFace,
}) => {
  if (!faces || faces.length === 0 || !imageWidth || !imageHeight) return null;

  const handleFaceClick = (face: DetectedFace) => {
    if (onToggleFace) {
      onToggleFace(face.index);
    }
    if (onSelectFace) {
      onSelectFace(face);
    }
  };

  return (
    <div className="absolute inset-0 pointer-events-none">
      {faces.map((face) => {
        const { box, score, gender, age_range, index } = face;
        const leftPercent = (box.x / imageWidth) * 100;
        const topPercent = (box.y / imageHeight) * 100;
        const widthPercent = (box.width / imageWidth) * 100;
        const heightPercent = (box.height / imageHeight) * 100;

        const isSelected = selectedFaceIndices !== undefined
          ? selectedFaceIndices.includes(index)
          : selectedFaceIndex === index;

        return (
          <div
            key={`face-${index}`}
            onClick={(e) => {
              e.stopPropagation();
              handleFaceClick(face);
            }}
            className={`absolute pointer-events-auto cursor-pointer rounded transition-all duration-200 group ${
              isSelected
                ? 'border-2 border-rose-500 bg-rose-500/25 shadow-[0_0_18px_rgba(244,63,94,0.6)]'
                : 'border-2 border-cyan-400/60 bg-cyan-400/5 hover:border-cyan-300 hover:bg-cyan-400/20'
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
                  ? 'bg-rose-600 text-white shadow-rose-900/50'
                  : 'bg-slate-900/90 text-slate-300 border border-slate-700/60 group-hover:border-cyan-500/60 group-hover:text-cyan-200'
              }`}
            >
              <span className="font-bold">
                {isSelected ? `✓ 已选 #${index + 1}` : `○ 未选 #${index + 1}`}
              </span>
              <span className="opacity-80">{(score * 100).toFixed(0)}%</span>
              {gender && <span className="opacity-90">{gender === 'female' ? '♀' : '♂'}</span>}
              {age_range && <span className="opacity-80">{age_range[0]}-{age_range[1]}</span>}
            </div>

            {/* Selection Checkbox Pill in top-right */}
            <div
              className={`absolute top-1 right-1 w-4 h-4 rounded-full flex items-center justify-center text-[10px] font-bold shadow transition-all ${
                isSelected
                  ? 'bg-rose-500 text-white ring-1 ring-white/60'
                  : 'bg-black/60 text-slate-400 border border-white/20 group-hover:border-cyan-400 group-hover:text-cyan-300'
              }`}
            >
              {isSelected ? '✓' : ''}
            </div>

            {/* Corner Indicators */}
            <div
              className={`absolute top-0 left-0 w-2 h-2 border-t-2 border-l-2 transition-colors ${
                isSelected ? 'border-rose-300' : 'border-cyan-200/80'
              }`}
            />
            <div
              className={`absolute top-0 right-0 w-2 h-2 border-t-2 border-r-2 transition-colors ${
                isSelected ? 'border-rose-300' : 'border-cyan-200/80'
              }`}
            />
            <div
              className={`absolute bottom-0 left-0 w-2 h-2 border-b-2 border-l-2 transition-colors ${
                isSelected ? 'border-rose-300' : 'border-cyan-200/80'
              }`}
            />
            <div
              className={`absolute bottom-0 right-0 w-2 h-2 border-b-2 border-r-2 transition-colors ${
                isSelected ? 'border-rose-300' : 'border-cyan-200/80'
              }`}
            />
          </div>
        );
      })}
    </div>
  );
};
