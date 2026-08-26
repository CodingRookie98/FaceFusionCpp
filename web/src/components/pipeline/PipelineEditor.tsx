import React, { useState } from 'react';
import {
  Plus,
  Play,
  Zap,
  Sparkles,
  Sliders,
  Wand2,
  Layers,
  AlertCircle,
  Cpu,
} from 'lucide-react';
import { PresetSelector } from './PresetSelector';
import { PipelineStepCard } from './PipelineStepCard';
import type { StudioStore } from '../../store/studioState';

interface PipelineEditorProps {
  store: StudioStore;
}

const AVAILABLE_PROCESSOR_TYPES = [
  { type: 'face_swapper', label: 'Face Swapper (换脸)', icon: Wand2 },
  { type: 'face_enhancer', label: 'Face Enhancer (人脸高清修复)', icon: Sparkles },
  { type: 'expression_restorer', label: 'Expression Restorer (表情还原)', icon: Sliders },
  { type: 'frame_enhancer', label: 'Frame Enhancer (全画幅超分)', icon: Layers },
];

export const PipelineEditor: React.FC<PipelineEditorProps> = ({ store }) => {
  const {
    steps,
    availableProcessors,
    applyPreset,
    addStep,
    updateStep,
    removeStep,
    moveStep,
    activeBindingStepId,
    setActiveBindingStepId,
    submitJob,
    isSubmitting,
    isRenderingPreview,
    renderPreview,
    isVideoPlaying,
    activeTarget,
    errorMsg,
  } = store;

  const [showAddMenu, setShowAddMenu] = useState(false);

  return (
    <div className="w-80 flex-shrink-0 h-full bg-[#0f1523] border-l border-white/5 flex flex-col select-none overflow-hidden">
      {/* Header */}
      <div className="p-3 border-b border-white/10 flex items-center justify-between">
        <div className="flex items-center gap-1.5 text-xs font-semibold text-slate-200">
          <Cpu className="w-4 h-4 text-blue-400" />
          <span>管线编排 (Pipeline)</span>
        </div>
        <span className="text-[10px] font-mono bg-[#151d30] border border-white/10 text-slate-400 px-2 py-0.5 rounded">
          {steps.filter((s) => s.enabled).length}/{steps.length} 激活
        </span>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 overflow-y-auto p-3 space-y-3.5">
        {/* Presets */}
        <PresetSelector onSelectPreset={applyPreset} />

        {/* Dynamic Steps Header + Add Button */}
        <div className="pt-2 border-t border-white/5">
          <div className="flex items-center justify-between mb-2">
            <span className="text-[11px] font-semibold uppercase tracking-wider text-slate-400">
              处理步骤流水线 ({steps.length})
            </span>

            <div className="relative">
              <button
                onClick={() => setShowAddMenu((prev) => !prev)}
                className="px-2 py-1 bg-blue-600 hover:bg-blue-500 text-white rounded text-[11px] font-semibold flex items-center gap-1 shadow-sm transition-colors"
              >
                <Plus className="w-3 h-3" />
                <span>添加步骤</span>
              </button>

              {/* Add Dropdown Menu */}
              {showAddMenu && (
                <div
                  className="absolute right-0 top-7 w-56 bg-[#151d30] border border-white/15 rounded-lg shadow-2xl py-1 z-30 animate-in fade-in zoom-in-95 duration-100"
                  onMouseLeave={() => setShowAddMenu(false)}
                >
                  <div className="px-2.5 py-1 text-[10px] text-slate-400 font-semibold uppercase">
                    选择处理器类型
                  </div>
                  {AVAILABLE_PROCESSOR_TYPES.map((proc) => {
                    const Icon = proc.icon;
                    return (
                      <button
                        key={proc.type}
                        onClick={() => {
                          addStep(proc.type);
                          setShowAddMenu(false);
                        }}
                        className="w-full px-2.5 py-1.5 text-left text-xs text-slate-200 hover:text-white hover:bg-blue-600/80 flex items-center gap-2 transition-colors"
                      >
                        <Icon className="w-3.5 h-3.5 text-blue-400" />
                        <span>{proc.label}</span>
                      </button>
                    );
                  })}
                </div>
              )}
            </div>
          </div>

          {/* Steps List */}
          <div className="space-y-2">
            {steps.map((step, idx) => {
              const meta = availableProcessors.find((p) => p.name === step.step);
              return (
                <PipelineStepCard
                  key={step.id}
                  step={step}
                  index={idx}
                  totalSteps={steps.length}
                  meta={meta}
                  isBindingActive={activeBindingStepId === step.id}
                  onUpdate={updateStep}
                  onRemove={removeStep}
                  onMove={moveStep}
                  onActivateBinding={(id) =>
                    setActiveBindingStepId(activeBindingStepId === id ? null : id)
                  }
                />
              );
            })}

            {steps.length === 0 && (
              <div className="p-6 text-center text-slate-500 text-xs border border-dashed border-white/10 rounded-lg">
                管线为空，请点击上方“添加步骤”或选用预设方案
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Bottom CTA Area */}
      <div className="p-3 border-t border-white/10 bg-[#0a0f1a] space-y-2">
        {errorMsg && (
          <div className="p-2 bg-rose-950/80 border border-rose-500/50 rounded text-xs text-rose-300 flex items-center gap-1.5">
            <AlertCircle className="w-3.5 h-3.5 flex-shrink-0" />
            <span className="truncate">{errorMsg}</span>
          </div>
        )}

        {/* Video pause frame hint */}
        {activeTarget?.type === 'video' && isVideoPlaying && (
          <div className="px-2 py-1 bg-amber-950/50 border border-amber-500/30 rounded text-[11px] text-amber-300/90 flex items-center gap-1">
            <AlertCircle className="w-3 h-3 flex-shrink-0 text-amber-400" />
            <span>暂停视频播放后可抓取当前画面帧进行渲染预览</span>
          </div>
        )}

        <div className="grid grid-cols-2 gap-2">
          <button
            onClick={() => renderPreview()}
            disabled={
              isRenderingPreview ||
              isSubmitting ||
              !activeTarget ||
              (activeTarget.type === 'video' && isVideoPlaying)
            }
            className="py-2.5 px-2 bg-gradient-to-r from-amber-600 to-orange-600 hover:from-amber-500 hover:to-orange-500 disabled:opacity-40 disabled:cursor-not-allowed text-white rounded-lg font-semibold text-xs flex items-center justify-center gap-1.5 shadow-md shadow-amber-500/20 transition-all hover:shadow-amber-500/30 active:scale-[0.98] cursor-pointer"
            title={
              activeTarget?.type === 'video' && isVideoPlaying
                ? '请先暂停/停止视频播放以抓取当前帧进行渲染预览'
                : '单帧即时临时预览效果（不加入任务队列）'
            }
          >
            <Zap className={`w-3.5 h-3.5 fill-white ${isRenderingPreview ? 'animate-bounce' : ''}`} />
            <span>{isRenderingPreview ? '预览渲染中...' : '⚡ 渲染预览'}</span>
          </button>

          <button
            onClick={submitJob}
            disabled={isSubmitting || isRenderingPreview}
            className="py-2.5 px-2 bg-gradient-to-r from-blue-600 to-cyan-600 hover:from-blue-500 hover:to-cyan-500 disabled:opacity-50 text-white rounded-lg font-semibold text-xs flex items-center justify-center gap-1.5 shadow-md shadow-blue-500/20 transition-all hover:shadow-blue-500/30 active:scale-[0.98] cursor-pointer"
            title="将任务加入持久化后台执行队列"
          >
            <Play className="w-3.5 h-3.5 fill-white" />
            <span>{isSubmitting ? '正在加入...' : '➕ 加入队列'}</span>
          </button>
        </div>
      </div>
    </div>
  );
};
