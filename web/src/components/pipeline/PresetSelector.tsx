import React from 'react';
import { Zap, Sparkles, Users, Film, Sliders } from 'lucide-react';
import { OFFICIAL_PRESETS, type PresetConfig } from '../../store/studioState';

interface PresetSelectorProps {
  onSelectPreset: (preset: PresetConfig) => void;
}

export const PresetSelector: React.FC<PresetSelectorProps> = ({ onSelectPreset }) => {
  const getIcon = (iconName: string) => {
    switch (iconName) {
      case 'Zap':
        return <Zap className="w-3.5 h-3.5 text-amber-400" />;
      case 'Sparkles':
        return <Sparkles className="w-3.5 h-3.5 text-cyan-400" />;
      case 'Users':
        return <Users className="w-3.5 h-3.5 text-indigo-400" />;
      case 'Film':
        return <Film className="w-3.5 h-3.5 text-rose-400" />;
      default:
        return <Sliders className="w-3.5 h-3.5 text-slate-400" />;
    }
  };

  return (
    <div className="space-y-1.5">
      <div className="text-[11px] font-semibold uppercase tracking-wider text-slate-400 flex items-center justify-between">
        <span>管线预设方案</span>
        <span className="text-[10px] text-slate-500 font-normal">点击快速应用</span>
      </div>

      <div className="grid grid-cols-2 gap-1.5">
        {OFFICIAL_PRESETS.map((preset) => (
          <button
            key={preset.id}
            onClick={() => onSelectPreset(preset)}
            className="p-2 rounded-lg bg-[#151d30] hover:bg-[#1e2a45] border border-white/5 hover:border-blue-500/40 text-left transition-all group flex flex-col justify-between"
          >
            <div className="flex items-center gap-1.5 mb-1">
              {getIcon(preset.icon)}
              <span className="text-xs font-medium text-slate-200 group-hover:text-white truncate">
                {preset.name}
              </span>
            </div>
            <p className="text-[10px] text-slate-400 line-clamp-1 leading-tight">
              {preset.description}
            </p>
          </button>
        ))}
      </div>
    </div>
  );
};
