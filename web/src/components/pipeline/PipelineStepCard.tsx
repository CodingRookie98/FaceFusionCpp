import React from 'react';
import {
  ChevronUp,
  ChevronDown,
  Trash2,
  Sliders,
  Crosshair,
  Sparkles,
  Layers,
  Wand2,
} from 'lucide-react';
import type { PipelineStepConfig, ProcessorMeta } from '../../api/types';

interface PipelineStepCardProps {
  step: PipelineStepConfig;
  index: number;
  totalSteps: number;
  meta?: ProcessorMeta;
  isBindingActive: boolean;
  onUpdate: (id: string, partial: Partial<PipelineStepConfig>) => void;
  onRemove: (id: string) => void;
  onMove: (index: number, direction: 'up' | 'down') => void;
  onActivateBinding: (id: string) => void;
}

export const PipelineStepCard: React.FC<PipelineStepCardProps> = ({
  step,
  index,
  totalSteps,
  meta,
  isBindingActive,
  onUpdate,
  onRemove,
  onMove,
  onActivateBinding,
}) => {
  const getStepIcon = () => {
    switch (step.step) {
      case 'face_swapper':
        return <Wand2 className="w-4 h-4 text-blue-400" />;
      case 'face_enhancer':
        return <Sparkles className="w-4 h-4 text-cyan-400" />;
      case 'expression_restorer':
        return <Sliders className="w-4 h-4 text-emerald-400" />;
      case 'frame_enhancer':
        return <Layers className="w-4 h-4 text-amber-400" />;
      default:
        return <Sliders className="w-4 h-4 text-slate-400" />;
    }
  };

  const handleParamChange = (key: string, value: any) => {
    onUpdate(step.id, {
      params: {
        ...step.params,
        [key]: value,
      },
    });
  };

  // Find model options from meta or defaults
  const modelParam = meta?.params?.find((p) => p.name === 'model');
  const allowedModels = modelParam?.allowed_values || [];

  return (
    <div
      className={`rounded-lg border transition-all ${
        step.enabled
          ? 'bg-[#151d30] border-white/10 shadow-md'
          : 'bg-[#0f1523]/60 border-white/5 opacity-60'
      } ${isBindingActive ? 'ring-2 ring-rose-500 shadow-[0_0_15px_rgba(244,63,94,0.3)]' : ''}`}
    >
      {/* Header Row */}
      <div className="p-2.5 flex items-center justify-between border-b border-white/5">
        <div className="flex items-center gap-2 flex-1 min-w-0">
          <input
            type="checkbox"
            checked={step.enabled}
            onChange={(e) => onUpdate(step.id, { enabled: e.target.checked })}
            className="w-4 h-4 rounded bg-slate-900 border-white/20 text-blue-600 focus:ring-0 cursor-pointer"
          />

          <div className="flex items-center gap-1.5 flex-1 min-w-0">
            {getStepIcon()}
            <input
              type="text"
              value={step.name || step.step}
              onChange={(e) => onUpdate(step.id, { name: e.target.value })}
              className="bg-transparent border-b border-transparent hover:border-white/20 focus:border-blue-500 text-xs font-semibold text-slate-200 focus:text-white px-1 py-0.5 outline-none truncate"
            />
          </div>
        </div>

        {/* Step Actions */}
        <div className="flex items-center gap-0.5">
          <button
            onClick={() => onMove(index, 'up')}
            disabled={index === 0}
            className="p-1 text-slate-400 hover:text-white disabled:opacity-20 disabled:hover:text-slate-400 rounded transition-colors"
            title="上移步骤"
          >
            <ChevronUp className="w-3.5 h-3.5" />
          </button>

          <button
            onClick={() => onMove(index, 'down')}
            disabled={index === totalSteps - 1}
            className="p-1 text-slate-400 hover:text-white disabled:opacity-20 disabled:hover:text-slate-400 rounded transition-colors"
            title="下移步骤"
          >
            <ChevronDown className="w-3.5 h-3.5" />
          </button>

          <button
            onClick={() => onRemove(step.id)}
            className="p-1 text-slate-500 hover:text-rose-400 rounded transition-colors"
            title="删除处理器"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Body / Parameters */}
      {step.enabled && (
        <div className="p-3 space-y-2.5 text-xs">
          {/* Model Selector */}
          {allowedModels.length > 0 && (
            <div className="flex items-center justify-between gap-2">
              <label className="text-[11px] text-slate-400 font-medium">模型 (Model)</label>
              <select
                value={(step.params.model as string) || allowedModels[0]}
                onChange={(e) => handleParamChange('model', e.target.value)}
                className="bg-[#0b101c] border border-white/10 rounded px-2 py-1 text-slate-200 text-xs outline-none focus:border-blue-500 font-mono"
              >
                {allowedModels.map((m) => (
                  <option key={m} value={m}>
                    {m}
                  </option>
                ))}
              </select>
            </div>
          )}

          {/* Face Selection Mode (For Swapper / Enhancer / Restorer) */}
          {(step.step === 'face_swapper' ||
            step.step === 'face_enhancer' ||
            step.step === 'expression_restorer') && (
            <div className="space-y-1.5 pt-1 border-t border-white/5">
              <div className="flex items-center justify-between">
                <label className="text-[11px] text-slate-400 font-medium">人脸选择策略</label>
                <span className="text-[10px] text-slate-500 font-mono">
                  {step.params.face_selector_mode || 'many'}
                </span>
              </div>

              <div className="grid grid-cols-3 gap-1 bg-[#0b101c] p-0.5 rounded border border-white/10">
                {(['many', 'one', 'reference'] as const).map((mode) => (
                  <button
                    key={mode}
                    onClick={() => handleParamChange('face_selector_mode', mode)}
                    className={`py-1 text-[10px] font-medium rounded transition-all ${
                      (step.params.face_selector_mode || 'many') === mode
                        ? 'bg-blue-600 text-white shadow-sm'
                        : 'text-slate-400 hover:text-slate-200'
                    }`}
                  >
                    {mode === 'many' ? '全部人脸' : mode === 'one' ? '单最高分' : '参考人脸'}
                  </button>
                ))}
              </div>

              {/* Reference Face WYSIWYG Binding Button */}
              {step.params.face_selector_mode === 'reference' && (
                <div className="mt-1.5 p-2 bg-[#0b101c] border border-white/10 rounded-lg flex items-center justify-between gap-2">
                  <div className="flex-1 min-w-0">
                    <span className="text-[10px] text-slate-400 block truncate">
                      {step.params.reference_face_path
                        ? `已绑定: ${(step.params.reference_face_path as string).split('/').pop()}`
                        : '未绑定参考人脸'}
                    </span>
                  </div>

                  <button
                    onClick={() => onActivateBinding(step.id)}
                    className={`px-2 py-1 rounded text-[10px] font-semibold flex items-center gap-1 transition-all ${
                      isBindingActive
                        ? 'bg-rose-600 text-white animate-pulse'
                        : 'bg-slate-800 hover:bg-slate-700 text-cyan-300 border border-cyan-500/30'
                    }`}
                  >
                    <Crosshair className="w-3 h-3" />
                    <span>{isBindingActive ? '请在画布点选' : '在画布选人脸'}</span>
                  </button>
                </div>
              )}
            </div>
          )}

          {/* Blend Factor Slider (For Enhancer) */}
          {step.step === 'face_enhancer' && (
            <div className="space-y-1 pt-1 border-t border-white/5">
              <div className="flex items-center justify-between text-[11px]">
                <label className="text-slate-400 font-medium">混合强度 (Blend Factor)</label>
                <span className="font-mono text-cyan-400 font-semibold">
                  {Number(step.params.blend_factor ?? 0.8).toFixed(2)}
                </span>
              </div>
              <input
                type="range"
                min="0"
                max="1"
                step="0.05"
                value={Number(step.params.blend_factor ?? 0.8)}
                onChange={(e) => handleParamChange('blend_factor', parseFloat(e.target.value))}
                className="w-full"
              />
            </div>
          )}

          {/* Restore Factor Slider (For Expression Restorer) */}
          {step.step === 'expression_restorer' && (
            <div className="space-y-1 pt-1 border-t border-white/5">
              <div className="flex items-center justify-between text-[11px]">
                <label className="text-slate-400 font-medium">还原系数 (Restore Factor)</label>
                <span className="font-mono text-emerald-400 font-semibold">
                  {Number(step.params.restore_factor ?? 0.8).toFixed(2)}
                </span>
              </div>
              <input
                type="range"
                min="0"
                max="1"
                step="0.05"
                value={Number(step.params.restore_factor ?? 0.8)}
                onChange={(e) => handleParamChange('restore_factor', parseFloat(e.target.value))}
                className="w-full"
              />
            </div>
          )}

          {/* Enhance Factor Slider (For Frame Enhancer) */}
          {step.step === 'frame_enhancer' && (
            <div className="space-y-1 pt-1 border-t border-white/5">
              <div className="flex items-center justify-between text-[11px]">
                <label className="text-slate-400 font-medium">超分强度 (Enhance Factor)</label>
                <span className="font-mono text-amber-400 font-semibold">
                  {Number(step.params.enhance_factor ?? 0.8).toFixed(2)}
                </span>
              </div>
              <input
                type="range"
                min="0"
                max="1"
                step="0.05"
                value={Number(step.params.enhance_factor ?? 0.8)}
                onChange={(e) => handleParamChange('enhance_factor', parseFloat(e.target.value))}
                className="w-full"
              />
            </div>
          )}
        </div>
      )}
    </div>
  );
};
