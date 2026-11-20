import React, { useState } from 'react';
import {
  Activity,
  ArrowUp,
  ArrowDown,
  XCircle,
  History,
  CheckCircle2,
  Clock,
  Radio,
} from 'lucide-react';
import { HistoryModal } from './HistoryModal';
import type { StudioStore } from '../../store/studioState';

interface TelemetryHUDProps {
  store: StudioStore;
}

export const TelemetryHUD: React.FC<TelemetryHUDProps> = ({ store }) => {
  const {
    tasks,
    activeTaskId,
    activeTaskDetail,
    setActiveTaskId,
    cancelTask,
    bumpPriority,
    backendStatus,
  } = store;

  const [showHistory, setShowHistory] = useState(false);

  // Compute stats for the current active task
  const activeTask = tasks.find((t) => t.id === activeTaskId) || tasks[0];
  const progress = activeTaskDetail?.progress || activeTask?.progress || {
    current_frame: 0,
    total_frames: 0,
    fps: 0,
  };

  const status = activeTaskDetail?.status || activeTask?.status || 'idle';
  const percent =
    progress.total_frames > 0
      ? Math.min((progress.current_frame / progress.total_frames) * 100, 100)
      : 0;

  return (
    <>
      <div className="h-12 bg-[#0a0f1a] border-t border-white/10 px-4 flex items-center justify-between text-xs select-none z-30">
        {/* Left: Backend status & Active running status */}
        <div className="flex items-center gap-4">
          {/* Backend Status indicator */}
          <div className="flex items-center gap-2">
            <span
              className={`w-2 h-2 rounded-full ${
                backendStatus.includes('online') || backendStatus.includes('ok')
                  ? 'bg-emerald-400 shadow-[0_0_8px_rgba(16,185,129,0.8)]'
                  : 'bg-rose-500 shadow-[0_0_8px_rgba(244,63,94,0.8)]'
              }`}
            />
            <span className="text-[11px] font-mono text-slate-400">
              C++ Core: <strong className="text-slate-200">{backendStatus}</strong>
            </span>
          </div>

          <div className="h-4 w-px bg-white/10" />

          {/* Current Running / Active Task pill */}
          {activeTask ? (
            <div className="flex items-center gap-2">
              <span className="text-slate-400 font-mono text-[11px]">
                任务: <strong>{activeTask.id.slice(0, 10)}…</strong>
              </span>

              {status === 'running' && (
                <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-cyan-950 text-cyan-300 border border-cyan-500/30 flex items-center gap-1 animate-pulse">
                  <Radio className="w-3 h-3 text-cyan-400 animate-spin" /> 推理中
                </span>
              )}
              {status === 'queued' && (
                <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-amber-950 text-amber-300 border border-amber-500/30 flex items-center gap-1">
                  <Clock className="w-3 h-3" /> 排队中 (#{activeTask.queue_position})
                </span>
              )}
              {status === 'done' && (
                <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-emerald-950 text-emerald-300 border border-emerald-500/30 flex items-center gap-1">
                  <CheckCircle2 className="w-3 h-3" /> 已完成
                </span>
              )}
            </div>
          ) : (
            <span className="text-slate-500 text-[11px]">就绪 (无活跃任务)</span>
          )}
        </div>

        {/* Center: Live Progress Bar & Telemetry */}
        <div className="flex items-center gap-4 flex-1 max-w-lg mx-6">
          <div className="flex-1 space-y-1">
            <div className="flex items-center justify-between text-[10px] text-slate-400 font-mono">
              <span>
                帧进度: {progress.current_frame}/{progress.total_frames} ({percent.toFixed(0)}%)
              </span>
              <span className="text-cyan-400 font-semibold flex items-center gap-1">
                <Activity className="w-3 h-3" />
                {progress.fps > 0 ? `${progress.fps.toFixed(1)} FPS` : '— FPS'}
              </span>
            </div>

            {/* Progress Track */}
            <div className="w-full h-1.5 bg-[#151d30] rounded-full overflow-hidden border border-white/5">
              <div
                className="h-full bg-gradient-to-r from-blue-500 via-cyan-400 to-emerald-400 transition-all duration-200 rounded-full"
                style={{ width: `${percent}%` }}
              />
            </div>
          </div>
        </div>

        {/* Right: Queue Actions & History Drawer */}
        <div className="flex items-center gap-2">
          {/* Priority Controls for Queued Task */}
          {activeTask && activeTask.status === 'queued' && (
            <div className="flex items-center gap-1 mr-2 bg-[#151d30] px-1.5 py-0.5 rounded border border-white/10">
              <span className="text-[10px] font-mono text-amber-400 mr-1">
                P{activeTask.priority}
              </span>
              <button
                onClick={() => bumpPriority(activeTask.id, 1)}
                className="p-1 hover:text-white text-slate-400 rounded"
                title="提高优先级 (优先调度)"
              >
                <ArrowUp className="w-3 h-3" />
              </button>
              <button
                onClick={() => bumpPriority(activeTask.id, -1)}
                className="p-1 hover:text-white text-slate-400 rounded"
                title="降低优先级"
              >
                <ArrowDown className="w-3 h-3" />
              </button>
            </div>
          )}

          {/* Cancel active task */}
          {activeTask &&
            (activeTask.status === 'running' || activeTask.status === 'queued') && (
              <button
                onClick={() => cancelTask(activeTask.id)}
                className="px-2.5 py-1 bg-rose-950 hover:bg-rose-900 text-rose-300 border border-rose-500/30 rounded text-[11px] font-medium flex items-center gap-1 transition-colors mr-2"
                title="取消当前任务"
              >
                <XCircle className="w-3.5 h-3.5" />
                <span>取消</span>
              </button>
            )}

          {/* History Drawer Trigger */}
          <button
            onClick={() => setShowHistory(true)}
            className="px-3 py-1 bg-[#151d30] hover:bg-[#1f2c47] text-slate-200 hover:text-white border border-white/10 hover:border-blue-500/40 rounded-lg text-xs font-medium flex items-center gap-1.5 shadow-sm transition-all"
          >
            <History className="w-3.5 h-3.5 text-blue-400" />
            <span>历史成果 ({tasks.length})</span>
          </button>
        </div>
      </div>

      {/* History Modal */}
      <HistoryModal
        tasks={tasks}
        activeTaskId={activeTaskId}
        isOpen={showHistory}
        onClose={() => setShowHistory(false)}
        onSelectTask={(id) => setActiveTaskId(id)}
      />
    </>
  );
};
