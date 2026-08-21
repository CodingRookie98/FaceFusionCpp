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
  Eye,
  Download,
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

  // Result file for comparison: match against current activeTarget if multiple results exist
  const targetStem = activeTarget?.name?.replace(/\.[^/.]+$/, '') || '';
  const matchingResult =
    activeTaskDetail?.results?.find(
      (r) => targetStem && r.name.toLowerCase().includes(targetStem.toLowerCase())
    ) || activeTaskDetail?.results?.[0];

  const latestResult = matchingResult?.url;
  const latestResultName = matchingResult?.name;
  const originalUrl = getMediaPreviewUrl(activeTarget);
  const isVideo = activeTarget?.type === 'video';

  return (
    <div
      ref={containerRef}
      className="relative flex-1 min-w-0 min-h-0 h-full bg-[#080b11] flex flex-col overflow-hidden border-x border-white/5"
    >
      {/* Top Canvas Toolbar */}
      <div className="h-11 flex-shrink-0 border-b border-white/10 bg-[#0f1523]/80 backdrop-blur-md px-4 flex items-center justify-between z-20 overflow-hidden">
        {/* Left: Viewport Mode Switcher */}
        <div className="flex items-center gap-1 bg-[#151d30] p-0.5 rounded-lg border border-white/10 flex-shrink-0">
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
            onClick={() => setViewportMode('result')}
            disabled={!latestResult}
            className={`px-3 py-1 text-xs rounded-md font-medium flex items-center gap-1.5 transition-colors ${
              viewportMode === 'result'
                ? 'bg-blue-600 text-white shadow-sm'
                : latestResult
                ? 'text-emerald-400 hover:text-emerald-300 font-semibold'
                : 'text-slate-600 cursor-not-allowed opacity-50'
            }`}
            title={latestResult ? '查看处理结果' : '需要先执行任务获得结果'}
          >
            <Eye className="w-3.5 h-3.5" />
            <span>处理结果</span>
          </button>

          <button
            onClick={() => setViewportMode('compare')}
            disabled={!latestResult}
            className={`px-3 py-1 text-xs rounded-md font-medium flex items-center gap-1.5 transition-colors ${
              viewportMode === 'compare'
                ? 'bg-blue-600 text-white shadow-sm'
                : latestResult
                ? 'text-cyan-400 hover:text-cyan-300 font-semibold'
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
        <div className="text-xs text-slate-400 flex items-center gap-2 overflow-hidden truncate mx-2 min-w-0 flex-1 justify-center">
          {activeTarget && (
            <span className="font-mono bg-slate-900 px-2 py-0.5 rounded border border-white/5 truncate max-w-[200px]">
              目标: {activeTarget.name}
            </span>
          )}
          {isDetectingFaces && (
            <span className="text-cyan-400 flex items-center gap-1 animate-pulse flex-shrink-0">
              <Sparkles className="w-3 h-3 animate-spin" /> 检测人脸中...
            </span>
          )}
          {detectedFaces.length > 0 && !isDetectingFaces && (
            <span className="text-emerald-400 flex items-center gap-1 flex-shrink-0">
              ✓ 检测到 {detectedFaces.length} 张人脸
            </span>
          )}
          {latestResult && activeTaskDetail?.status === 'done' && (
            <span className="text-emerald-400 font-mono bg-emerald-950/60 px-2 py-0.5 rounded border border-emerald-500/30 flex items-center gap-1 flex-shrink-0 truncate max-w-[220px]">
              ✓ 已生成: {latestResultName || 'result'}
            </span>
          )}
        </div>

        {/* Right: Actions */}
        <div className="flex items-center gap-2 flex-shrink-0">
          {latestResult && (
            <a
              href={latestResult}
              download={latestResultName || 'result'}
              className="p-1.5 text-emerald-400 hover:text-emerald-300 bg-emerald-950/60 hover:bg-emerald-900/60 rounded border border-emerald-500/30 text-xs flex items-center gap-1 transition-colors"
              title="下载处理结果"
            >
              <Download className="w-3.5 h-3.5" />
              <span>下载结果</span>
            </a>
          )}

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
        ) : viewportMode === 'result' && latestResult ? (
          /* Single Result Viewer */
          <div className="relative w-full h-full max-h-[calc(100vh-180px)] flex items-center justify-center rounded-lg shadow-2xl overflow-hidden border border-emerald-500/20 bg-black/40">
            {isVideo ? (
              <video
                src={latestResult}
                controls
                autoPlay
                className="w-full h-full object-contain rounded"
              />
            ) : (
              <div className="relative w-full h-full flex items-center justify-center">
                <img
                  src={latestResult}
                  alt="Processed Result"
                  className="w-full h-full object-contain block rounded"
                />
                <div className="absolute top-3 right-3 bg-emerald-950/90 text-emerald-300 border border-emerald-500/50 text-xs px-2.5 py-1 rounded font-mono font-medium shadow-md backdrop-blur-sm z-10">
                  处理后结果 (AFTER)
                </div>
              </div>
            )}
          </div>
        ) : viewportMode === 'loupe' ? (
          <DetailLoupe
            imageUrl={latestResult || originalUrl}
            zoomLevel={2.8}
          />
        ) : (
          /* Normal Canvas Viewport with FaceOverlay */
          <div className="relative w-full h-full max-h-[calc(100vh-180px)] flex items-center justify-center rounded-lg shadow-2xl overflow-hidden border border-white/10 bg-black/40">
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

        {/* Floating Quick Compare Bar when Result is ready in Canvas Mode */}
        {latestResult && viewportMode === 'canvas' && (
          <div className="absolute bottom-6 z-30 bg-slate-900/90 border border-emerald-500/40 px-4 py-2 rounded-full shadow-2xl backdrop-blur-md flex items-center gap-3 animate-fade-in">
            <div className="flex items-center gap-1.5 text-xs text-emerald-400 font-medium">
              <Sparkles className="w-4 h-4 text-emerald-400" />
              <span>任务渲染完成</span>
            </div>
            <div className="h-3 w-px bg-white/20" />
            <button
              onClick={() => setViewportMode('compare')}
              className="px-3 py-1 bg-emerald-600 hover:bg-emerald-500 text-white rounded-full text-xs font-semibold flex items-center gap-1 transition-all shadow-md active:scale-95"
            >
              <Columns className="w-3.5 h-3.5" />
              <span>开启卷帘对比</span>
            </button>
            <button
              onClick={() => setViewportMode('result')}
              className="px-3 py-1 bg-blue-600/80 hover:bg-blue-600 text-white rounded-full text-xs font-medium flex items-center gap-1 transition-all"
            >
              <Eye className="w-3.5 h-3.5" />
              <span>结果预览</span>
            </button>
          </div>
        )}
      </div>
    </div>
  );
};
