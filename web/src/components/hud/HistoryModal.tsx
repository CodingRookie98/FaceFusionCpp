import React from 'react';
import { X, CheckCircle2, AlertCircle, Clock, ArrowRight } from 'lucide-react';
import type { TaskSummary } from '../../api/types';

interface HistoryModalProps {
  tasks: TaskSummary[];
  activeTaskId: string | null;
  isOpen: boolean;
  onClose: () => void;
  onSelectTask: (taskId: string) => void;
}

export const HistoryModal: React.FC<HistoryModalProps> = ({
  tasks,
  activeTaskId,
  isOpen,
  onClose,
  onSelectTask,
}) => {
  if (!isOpen) return null;

  const getStatusBadge = (status: string) => {
    switch (status) {
      case 'done':
        return (
          <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-emerald-950 text-emerald-300 border border-emerald-500/30 flex items-center gap-1">
            <CheckCircle2 className="w-3 h-3" /> 已完成
          </span>
        );
      case 'running':
        return (
          <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-cyan-950 text-cyan-300 border border-cyan-500/30 flex items-center gap-1 animate-pulse">
            <Clock className="w-3 h-3 animate-spin" /> 执行中
          </span>
        );
      case 'failed':
        return (
          <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-rose-950 text-rose-300 border border-rose-500/30 flex items-center gap-1">
            <AlertCircle className="w-3 h-3" /> 失败
          </span>
        );
      default:
        return (
          <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-slate-800 text-slate-300 border border-slate-700">
            {status}
          </span>
        );
    }
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-sm flex items-center justify-center p-4 animate-in fade-in duration-150">
      <div className="w-full max-w-2xl bg-[#0f1523] border border-white/15 rounded-xl shadow-2xl overflow-hidden flex flex-col max-h-[80vh]">
        {/* Header */}
        <div className="p-4 border-b border-white/10 flex items-center justify-between">
          <h3 className="text-sm font-semibold text-white flex items-center gap-2">
            <span>📜 历史任务与成果记录</span>
            <span className="text-xs text-slate-400 font-mono">({tasks.length} 项)</span>
          </h3>
          <button
            onClick={onClose}
            className="p-1 text-slate-400 hover:text-white rounded-lg hover:bg-white/5 transition-colors"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Task List */}
        <div className="flex-1 overflow-y-auto p-4 space-y-2.5">
          {tasks.map((task) => {
            const isCurrent = task.id === activeTaskId;
            return (
              <div
                key={task.id}
                onClick={() => {
                  onSelectTask(task.id);
                  onClose();
                }}
                className={`p-3 rounded-lg border transition-all cursor-pointer flex items-center justify-between gap-4 ${
                  isCurrent
                    ? 'bg-[#1e2a45] border-blue-500/80 shadow-[0_0_15px_rgba(59,130,246,0.2)]'
                    : 'bg-[#151d30] border-white/5 hover:border-white/20 hover:bg-[#192338]'
                }`}
              >
                <div className="space-y-1 min-w-0">
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-mono font-semibold text-slate-200 truncate">
                      {task.id.slice(0, 16)}…
                    </span>
                    {getStatusBadge(task.status)}
                    {task.priority !== 0 && (
                      <span className="text-[10px] font-mono text-amber-400 bg-amber-950/60 px-1.5 py-0.2 rounded border border-amber-500/30">
                        P{task.priority}
                      </span>
                    )}
                  </div>
                  <p className="text-[11px] text-slate-400">
                    素材数: {task.media_count} · 帧数: {task.progress.current_frame}/
                    {task.progress.total_frames}
                    {task.error_message && (
                      <span className="text-rose-400 ml-2">— {task.error_message}</span>
                    )}
                  </p>
                </div>

                <div className="flex items-center gap-2 flex-shrink-0">
                  <button
                    className="px-3 py-1.5 bg-blue-600/80 hover:bg-blue-600 text-white rounded text-xs font-medium flex items-center gap-1 transition-colors"
                    onClick={(e) => {
                      e.stopPropagation();
                      onSelectTask(task.id);
                      onClose();
                    }}
                  >
                    <span>在工作台载入</span>
                    <ArrowRight className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>
            );
          })}

          {tasks.length === 0 && (
            <div className="p-12 text-center text-slate-500 text-xs">暂无历史任务记录</div>
          )}
        </div>
      </div>
    </div>
  );
};
