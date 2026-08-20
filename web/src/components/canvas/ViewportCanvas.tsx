import React, { useState, useRef } from 'react';
import {
  Maximize2,
  Minimize2,
  Columns,
  Search,
  Crosshair,
  Sparkles,
  Layers,
  RotateCcw,
} from 'lucide-react';
import { FaceOverlay } from './FaceOverlay';
import { SplitSlider } from './SplitSlider';
import { DetailLoupe } from './DetailLoupe';
import type { StudioStore } from '../../store/studioState';
import { getMediaPreviewUrl } from '../../store/studioState';
import type { DetectedFace } from '../../api/types';

interface ViewportCanvasProps {
  store: StudioStore;
}

export const ViewportCanvas: React.FC<ViewportCanvasProps> = ({ store }) => {
  const {
    activeTarget,
    detectedFaces,
    isDetectingFaces,
    runFaceDetection,
    bindReferenceFaceToActiveStep,
    viewportMode,
    setViewportMode,
    activeTaskDetail,
  } = store;

  const [isFullscreen, setIsFullscreen] = useState(false);
  const [selectedFaceIndex, setSelectedFaceIndex] = useState<number | null>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const imgRef = useRef<HTMLImageElement>(null);
  const [naturalSize, setNaturalSize] = useState<{ width: number; height: number }>({
    width: 0,
    height: 0,
  });

  const toggleFullscreen = () => {
    if (!containerRef.current) return;
    if (!document.fullscreenElement) {
      containerRef.current.requestFullscreen().catch(() => {});
      setIsFullscreen(true);
    } else {
      document.exitFullscreen().catch(() => {});
      setIsFullscreen(false);
    }
  };

  const handleImageLoad = (e: React.SyntheticEvent<HTMLImageElement>) => {
    const target = e.currentTarget;
    setNaturalSize({
      width: target.naturalWidth,
      height: target.naturalHeight,
    });
  };

  const handleFaceClick = (face: DetectedFace) => {
    setSelectedFaceIndex(face.index);
    bindReferenceFaceToActiveStep(face, activeTarget?.path);
  };

  // Result file for comparison
  const latestResult = activeTaskDetail?.results?.[0]?.url;
  const originalUrl = getMediaPreviewUrl(activeTarget);
  const isVideo = activeTarget?.type === 'video';

  return (
    <div
      ref={containerRef}
      className="relative flex-1 h-full bg-[#080b11] flex flex-col overflow-hidden border-x border-white/5"
    >
      {/* Top Canvas Toolbar */}
      <div className="h-11 border-b border-white/10 bg-[#0f1523]/80 backdrop-blur-md px-4 flex items-center justify-between z-20">
        {/* Left: Viewport Mode Switcher */}
        <div className="flex items-center gap-1 bg-[#151d30] p-0.5 rounded-lg border border-white/10">
          <button
            onClick={() => setViewportMode('canvas')}
            className={`px-3 py-1 text-xs rounded-md font-medium flex items-center gap-1.5 transition-colors ${
              viewportMode === 'canvas'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Crosshair className="w-3.5 h-3.5" />
            <span>标注画布</span>
          </button>

          <button
            onClick={() => setViewportMode('compare')}
            disabled={!latestResult}
            className={`px-3 py-1 text-xs rounded-md font-medium flex items-center gap-1.5 transition-colors ${
              viewportMode === 'compare'
                ? 'bg-blue-600 text-white shadow-sm'
                : latestResult
                ? 'text-slate-400 hover:text-slate-200'
                : 'text-slate-600 cursor-not-allowed opacity-50'
            }`}
            title={latestResult ? '对比处理前后' : '需要先执行任务获得结果'}
          >
            <Columns className="w-3.5 h-3.5" />
            <span>卷帘对比</span>
          </button>

          <button
            onClick={() => setViewportMode('loupe')}
            disabled={!latestResult && !activeTarget}
            className={`px-3 py-1 text-xs rounded-md font-medium flex items-center gap-1.5 transition-colors ${
              viewportMode === 'loupe'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Search className="w-3.5 h-3.5" />
            <span>局部放大</span>
          </button>
        </div>

        {/* Center: Info Badge */}
        <div className="text-xs text-slate-400 flex items-center gap-3">
          {activeTarget && (
            <span className="font-mono bg-slate-900 px-2 py-0.5 rounded border border-white/5">
              目标: {activeTarget.name}
            </span>
          )}
          {isDetectingFaces && (
            <span className="text-cyan-400 flex items-center gap-1 animate-pulse">
              <Sparkles className="w-3 h-3 animate-spin" /> 检测人脸中...
            </span>
          )}
          {detectedFaces.length > 0 && !isDetectingFaces && (
            <span className="text-emerald-400 flex items-center gap-1">
              ✓ 检测到 {detectedFaces.length} 张人脸 (可点击框选)
            </span>
          )}
        </div>

        {/* Right: Actions */}
        <div className="flex items-center gap-2">
          {activeTarget?.type === 'image' && (
            <button
              onClick={() => runFaceDetection(activeTarget.path)}
              disabled={isDetectingFaces}
              className="p-1.5 text-slate-400 hover:text-slate-200 bg-slate-800/60 hover:bg-slate-800 rounded border border-white/10 text-xs flex items-center gap-1 transition-colors"
              title="重新检测人脸"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              <span>重新检测</span>
            </button>
          )}

          <button
            onClick={toggleFullscreen}
            className="p-1.5 text-slate-400 hover:text-slate-200 bg-slate-800/60 hover:bg-slate-800 rounded border border-white/10 transition-colors"
            title="全屏画布"
          >
            {isFullscreen ? <Minimize2 className="w-3.5 h-3.5" /> : <Maximize2 className="w-3.5 h-3.5" />}
          </button>
        </div>
      </div>

      {/* Main Viewport Stage */}
      <div className="relative flex-1 w-full h-full flex items-center justify-center p-4 overflow-hidden">
        {viewportMode === 'compare' && latestResult ? (
          <SplitSlider
            originalUrl={originalUrl}
            resultUrl={latestResult}
            isVideo={isVideo}
          />
        ) : viewportMode === 'loupe' ? (
          <DetailLoupe
            imageUrl={latestResult || originalUrl}
            zoomLevel={2.8}
          />
        ) : (
          /* Normal Canvas Viewport with FaceOverlay */
          <div className="relative max-h-full max-w-full flex items-center justify-center rounded-lg shadow-2xl overflow-hidden border border-white/10 bg-black/40">
            {activeTarget ? (
              isVideo ? (
                <video
                  src={originalUrl}
                  controls
                  className="max-h-[calc(100vh-220px)] max-w-full object-contain rounded"
                />
              ) : (
                <div className="relative inline-block leading-none">
                  <img
                    ref={imgRef}
                    src={originalUrl}
                    alt="Target Canvas"
                    onLoad={handleImageLoad}
                    className="max-h-[calc(100vh-220px)] max-w-full object-contain block rounded"
                  />
                  {naturalSize.width > 0 && (
                    <FaceOverlay
                      faces={detectedFaces}
                      imageWidth={naturalSize.width}
                      imageHeight={naturalSize.height}
                      selectedFaceIndex={selectedFaceIndex}
                      onSelectFace={handleFaceClick}
                    />
                  )}
                </div>
              )
            ) : (
              <div className="p-12 text-center text-slate-500 flex flex-col items-center gap-3">
                <Layers className="w-12 h-12 opacity-30" />
                <p className="text-sm">暂无目标素材，请在左侧素材池中上传或选择测试样张</p>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};
