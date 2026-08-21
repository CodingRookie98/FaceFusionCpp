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
  Activity,
  Clock,
  AlertCircle,
} from 'lucide-react';
import { FaceOverlay } from './FaceOverlay';
import { SplitSlider } from './SplitSlider';
import { DetailLoupe } from './DetailLoupe';
import type { StudioStore } from '../../store/studioState';
import { getMediaPreviewUrl } from '../../store/studioState';

interface ViewportCanvasProps {
  store: StudioStore;
}

export const ViewportCanvas: React.FC<ViewportCanvasProps> = ({ store }) => {
  const {
    activeSource,
    activeTarget,
    activePreviewTarget = 'target',
    detectedFaces,
    selectedFaceIndices,
    toggleFaceSelection,
    selectAllFaces,
    clearFaceSelection,
    isDetectingFaces,
    runFaceDetection,
    viewportMode,
    setViewportMode,
    tasks,
    activeTaskId,
    activeTaskDetail,
  } = store;

  const [isFullscreen, setIsFullscreen] = useState(false);
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

  const formatTaskId = (id?: string | null) => {
    if (!id) return '';
    return id.length > 12 ? `#${id.slice(-8)}` : `#${id}`;
  };

  // Determine current active display media based on omni-preview target
  const isSourceMode = activePreviewTarget === 'source';
  const isTaskMode = activePreviewTarget === 'task';
  const displayMedia = isSourceMode ? activeSource : activeTarget;
  const originalUrl = getMediaPreviewUrl(displayMedia);
  const isVideo = displayMedia?.type === 'video';

  // Selected task in queue mode
  const currentTask = tasks.find((t) => t.id === activeTaskId) || (activeTaskDetail?.id === activeTaskId ? activeTaskDetail : null);
  const isTaskPending = currentTask?.status === 'queued';
  const isTaskRunning = currentTask?.status === 'running' || activeTaskDetail?.status === 'running';
  const isTaskDone = currentTask?.status === 'done' || activeTaskDetail?.status === 'done';
  const isTaskFailed = currentTask?.status === 'failed';
  const isTaskCancelled = currentTask?.status === 'cancelled';

  // Result file for comparison: match against current activeTarget if multiple results exist
  const pathStem =
    activeTarget?.path?.split(/[\\/]/).pop()?.replace(/\.[^/.]+$/, '').toLowerCase() || '';
  const nameClean = activeTarget?.name?.replace(/\.[^/.]+$/, '') || '';
  const nameFirstWord = nameClean.split(/[\s(（]/)[0]?.toLowerCase() || '';

  const matchingResult =
    activeTaskDetail?.results?.find((r) => {
      const rName = r.name.toLowerCase();
      if (pathStem && rName.includes(pathStem)) return true;
      if (nameFirstWord && rName.includes(nameFirstWord)) return true;
      if (nameClean && rName.includes(nameClean.toLowerCase())) return true;
      return false;
    }) ||
    activeTaskDetail?.results?.[activeTaskDetail.results.length - 1] ||
    activeTaskDetail?.results?.[0];

  const latestResult = matchingResult?.url;
  const latestResultName = matchingResult?.name;

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
            <span>{isSourceMode ? '源素材画布' : isTaskMode ? '任务视口' : '标注画布'}</span>
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
            disabled={!latestResult && !displayMedia}
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
          {isSourceMode && activeSource && (
            <span className="font-mono bg-blue-950/80 text-blue-300 px-2 py-0.5 rounded border border-blue-500/30 truncate max-w-[220px]">
              源素材: {activeSource.name}
            </span>
          )}

          {isTaskMode && currentTask && (
            <span className="font-mono bg-indigo-950/80 text-indigo-300 px-2 py-0.5 rounded border border-indigo-500/30 truncate max-w-[220px]">
              任务: {formatTaskId(currentTask.id)} (
              {isTaskRunning
                ? '处理中'
                : isTaskPending
                ? `排队 P${currentTask.priority}`
                : isTaskDone
                ? '完成'
                : isTaskFailed
                ? '失败'
                : '已取消'}
              )
            </span>
          )}

          {!isSourceMode && !isTaskMode && activeTarget && (
            <span className="font-mono bg-slate-900 px-2 py-0.5 rounded border border-white/5 truncate max-w-[200px]">
              目标: {activeTarget.name}
            </span>
          )}

          {!isSourceMode && !isTaskMode && isDetectingFaces && (
            <span className="text-cyan-400 flex items-center gap-1 animate-pulse flex-shrink-0">
              <Sparkles className="w-3 h-3 animate-spin" /> 检测人脸中...
            </span>
          )}

          {!isSourceMode && !isTaskMode && detectedFaces.length > 0 && !isDetectingFaces && (
            <div className="flex items-center gap-1.5 bg-slate-900/90 px-2 py-0.5 rounded border border-white/10 text-xs flex-shrink-0">
              <span className="text-emerald-400 font-medium">✓ {detectedFaces.length} 张人脸</span>
              <span className="text-white/20">|</span>
              <span
                className={
                  selectedFaceIndices.length > 0 ? 'text-rose-400 font-semibold' : 'text-slate-400'
                }
              >
                已选 {selectedFaceIndices.length}/{detectedFaces.length}
              </span>
              <button
                onClick={selectAllFaces}
                className="px-1.5 py-0.5 text-[10px] bg-slate-800 hover:bg-slate-700 text-slate-200 rounded transition-colors"
                title="全选所有人脸"
              >
                全选
              </button>
              <button
                onClick={clearFaceSelection}
                className="px-1.5 py-0.5 text-[10px] bg-slate-800 hover:bg-slate-700 text-slate-200 rounded transition-colors"
                title="清空人脸选择"
              >
                清空
              </button>
            </div>
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

          {!isSourceMode && !isTaskMode && activeTarget?.type === 'image' && (
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
                key={latestResult}
                src={latestResult}
                controls
                autoPlay
                className="w-full h-full object-contain rounded"
              />
            ) : (
              <div className="relative w-full h-full flex items-center justify-center">
                <img
                  key={latestResult}
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
          /* Normal Canvas Viewport (Omni Perception) */
          <div className="relative w-full h-full max-h-[calc(100vh-180px)] flex items-center justify-center rounded-lg shadow-2xl overflow-hidden border border-white/10 bg-black/40">
            {displayMedia ? (
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
                  {/* Face overlay only shown in target media mode */}
                  {!isSourceMode && !isTaskMode && naturalSize.width > 0 && (
                    <FaceOverlay
                      faces={detectedFaces}
                      imageWidth={naturalSize.width}
                      imageHeight={naturalSize.height}
                      selectedFaceIndices={selectedFaceIndices}
                      onToggleFace={toggleFaceSelection}
                    />
                  )}

                  {/* Task Running Progress HUD Overlay */}
                  {isTaskMode && isTaskRunning && (
                    <div className="absolute inset-0 bg-black/60 backdrop-blur-sm flex flex-col items-center justify-center gap-3 p-6 text-center z-10 rounded">
                      <Activity className="w-8 h-8 text-cyan-400 animate-pulse" />
                      <div className="text-sm font-semibold text-slate-100">
                        正在处理任务 {formatTaskId(activeTaskId)}
                      </div>
                      {activeTaskDetail?.progress && (
                        <>
                          <div className="w-64 bg-slate-800 h-2 rounded-full overflow-hidden border border-white/10">
                            <div
                              className="bg-gradient-to-r from-blue-500 to-cyan-400 h-full rounded-full transition-all duration-300"
                              style={{
                                width: `${Math.round(
                                  (activeTaskDetail.progress.current_frame /
                                    Math.max(activeTaskDetail.progress.total_frames, 1)) *
                                    100
                                )}%`,
                              }}
                            />
                          </div>
                          <div className="text-xs text-slate-400 font-mono flex items-center gap-2">
                            <span>
                              {activeTaskDetail.progress.current_frame} /{' '}
                              {activeTaskDetail.progress.total_frames} 帧 (
                              {Math.round(
                                (activeTaskDetail.progress.current_frame /
                                  Math.max(activeTaskDetail.progress.total_frames, 1)) *
                                  100
                              )}
                              %)
                            </span>
                            {activeTaskDetail.progress.fps > 0 && (
                              <span>| {activeTaskDetail.progress.fps.toFixed(1)} FPS</span>
                            )}
                          </div>
                        </>
                      )}
                    </div>
                  )}

                  {/* Task Pending in Queue HUD Overlay */}
                  {isTaskMode && isTaskPending && (
                    <div className="absolute inset-0 bg-black/60 backdrop-blur-sm flex flex-col items-center justify-center gap-3 p-6 text-center z-10 rounded">
                      <Clock className="w-8 h-8 text-amber-400 animate-bounce" />
                      <div className="text-sm font-semibold text-slate-100">
                        任务排队中... {formatTaskId(activeTaskId)}
                      </div>
                      <div className="text-xs text-amber-300/90 font-mono bg-amber-950/80 px-3 py-1 rounded-full border border-amber-500/30">
                        调度优先级: P{currentTask?.priority ?? 0}
                      </div>
                      <span className="text-[11px] text-slate-400">
                        后台推理引擎就绪后将按优先级自动分配执行
                      </span>
                    </div>
                  )}

                  {/* Task Failed / Cancelled HUD Overlay */}
                  {isTaskMode && (isTaskFailed || isTaskCancelled) && (
                    <div className="absolute inset-0 bg-black/60 backdrop-blur-sm flex flex-col items-center justify-center gap-3 p-6 text-center z-10 rounded">
                      <AlertCircle className={`w-8 h-8 ${isTaskFailed ? 'text-rose-400' : 'text-slate-400'}`} />
                      <div className="text-sm font-semibold text-slate-100">
                        {isTaskFailed ? '任务执行失败' : '任务已取消'}
                      </div>
                      {currentTask?.error && (
                        <div className="text-xs text-rose-300 max-w-sm bg-rose-950/80 p-2 rounded border border-rose-500/30 font-mono">
                          {currentTask.error}
                        </div>
                      )}
                    </div>
                  )}
                </div>
              )
            ) : (
              <div className="p-12 text-center text-slate-500 flex flex-col items-center gap-3">
                <Layers className="w-12 h-12 opacity-30" />
                <p className="text-sm">
                  {isSourceMode
                    ? '暂无源素材，请在左侧素材池中上传'
                    : isTaskMode
                    ? '暂无选中的任务'
                    : '暂无目标素材，请在左侧素材池中上传或选择测试样张'}
                </p>
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
