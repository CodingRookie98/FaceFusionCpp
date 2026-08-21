import React, { useState } from 'react';
import {
  Upload,
  Trash2,
  Image as ImageIcon,
  Film,
  Sparkles,
  CheckCircle2,
} from 'lucide-react';
import { api } from '../../api/client';
import type { StudioStore, MediaItem } from '../../store/studioState';
import { SAMPLE_MEDIA, getMediaPreviewUrl } from '../../store/studioState';

interface AssetPoolProps {
  store: StudioStore;
}

export const AssetPool: React.FC<AssetPoolProps> = ({ store }) => {
  const {
    sources,
    targets,
    selectedSourceId,
    selectedTargetId,
    setSelectedSourceId,
    setSelectedTargetId,
    addSource,
    removeSource,
    addTarget,
    removeTarget,
  } = store;

  const [activeTab, setActiveTab] = useState<'sources' | 'targets'>('sources');
  const [isUploading, setIsUploading] = useState(false);

  const handleFileUpload = async (e: React.ChangeEvent<HTMLInputElement>, isSource: boolean) => {
    const files = e.target.files;
    if (!files || files.length === 0) return;
    setIsUploading(true);
    try {
      for (let i = 0; i < files.length; i++) {
        const file = files[i];
        const res = await api.uploadFile(file);
        const item: MediaItem = {
          id: `media-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
          path: res.path,
          name: file.name,
          type: file.type.startsWith('video') ? 'video' : 'image',
          thumbnailUrl: URL.createObjectURL(file),
          file,
        };
        if (isSource) {
          addSource(item);
        } else {
          addTarget(item);
        }
      }
    } catch (err) {
      alert(err instanceof Error ? err.message : '上传失败');
    } finally {
      setIsUploading(false);
      e.target.value = '';
    }
  };

  const loadSample = (sample: MediaItem, isSource: boolean) => {
    const newItem: MediaItem = {
      ...sample,
      id: `sample-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
    };
    if (isSource) {
      addSource(newItem);
    } else {
      addTarget(newItem);
    }
  };

  const activeItems = activeTab === 'sources' ? sources : targets;
  const selectedId = activeTab === 'sources' ? selectedSourceId : selectedTargetId;

  return (
    <div className="w-72 flex-shrink-0 h-full bg-[#0f1523] border-r border-white/5 flex flex-col select-none overflow-hidden">
      {/* Header Tabs */}
      <div className="p-3 border-b border-white/10 flex items-center justify-between">
        <div className="flex bg-[#151d30] p-0.5 rounded-lg border border-white/10 w-full">
          <button
            onClick={() => setActiveTab('sources')}
            className={`flex-1 py-1.5 text-xs font-medium rounded-md transition-all flex items-center justify-center gap-1.5 ${
              activeTab === 'sources'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <ImageIcon className="w-3.5 h-3.5" />
            <span>源人脸 ({sources.length})</span>
          </button>

          <button
            onClick={() => setActiveTab('targets')}
            className={`flex-1 py-1.5 text-xs font-medium rounded-md transition-all flex items-center justify-center gap-1.5 ${
              activeTab === 'targets'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Film className="w-3.5 h-3.5" />
            <span>目标素材 ({targets.length})</span>
          </button>
        </div>
      </div>

      {/* Upload Dropzone / Button */}
      <div className="p-3 border-b border-white/5">
        <label className="flex flex-col items-center justify-center border border-dashed border-white/20 hover:border-blue-500/80 bg-[#151d30]/60 hover:bg-[#151d30] rounded-lg p-3 cursor-pointer transition-all group">
          <Upload className="w-5 h-5 text-slate-400 group-hover:text-blue-400 mb-1 transition-colors" />
          <span className="text-xs text-slate-300 font-medium group-hover:text-white">
            {isUploading ? '正在上传中...' : activeTab === 'sources' ? '上传源人脸图片' : '上传目标图片/视频'}
          </span>
          <span className="text-[10px] text-slate-500 mt-0.5">支持 JPG, PNG, BMP, MP4, MOV</span>
          <input
            type="file"
            multiple
            accept={activeTab === 'sources' ? 'image/*' : 'image/*,video/*'}
            onChange={(e) => handleFileUpload(e, activeTab === 'sources')}
            disabled={isUploading}
            className="hidden"
          />
        </label>
      </div>

      {/* Media Thumbnails List */}
      <div className="flex-1 overflow-y-auto p-3 space-y-2">
        {activeItems.map((item) => {
          const isSelected = item.id === selectedId;
          return (
            <div
              key={item.id}
              onClick={() => {
                if (activeTab === 'sources') setSelectedSourceId(item.id);
                else setSelectedTargetId(item.id);
              }}
              className={`group relative flex items-center gap-2.5 p-2 rounded-lg border transition-all cursor-pointer ${
                isSelected
                  ? 'bg-[#1e2a45] border-blue-500/80 shadow-[0_0_12px_rgba(59,130,246,0.25)]'
                  : 'bg-[#151d30] border-white/5 hover:border-white/20 hover:bg-[#182238]'
              }`}
            >
              {/* Thumbnail Container */}
              <div className="w-12 h-12 rounded bg-slate-950 flex-shrink-0 overflow-hidden relative border border-white/10 flex items-center justify-center">
                {item.type === 'image' ? (
                  <img
                    src={getMediaPreviewUrl(item)}
                    alt={item.name}
                    className="w-full h-full object-cover"
                    onError={(e) => {
                      (e.target as HTMLElement).style.display = 'none';
                    }}
                  />
                ) : (
                  <Film className="w-6 h-6 text-slate-400" />
                )}
                {item.type === 'video' && (
                  <span className="absolute bottom-0 right-0 bg-black/80 text-[9px] text-cyan-300 px-1 font-mono">
                    MOV
                  </span>
                )}
              </div>

              {/* Title & Path */}
              <div className="flex-1 min-w-0">
                <p className="text-xs font-medium text-slate-200 truncate">{item.name}</p>
                <p className="text-[10px] text-slate-500 font-mono truncate">{item.path}</p>
              </div>

              {/* Selection Check or Delete */}
              <div className="flex items-center gap-1">
                {isSelected && <CheckCircle2 className="w-4 h-4 text-blue-400 flex-shrink-0" />}
                <button
                  onClick={(e) => {
                    e.stopPropagation();
                    if (activeTab === 'sources') removeSource(item.id);
                    else removeTarget(item.id);
                  }}
                  className="p-1 text-slate-500 hover:text-rose-400 rounded opacity-0 group-hover:opacity-100 transition-opacity"
                  title="移除素材"
                >
                  <Trash2 className="w-3.5 h-3.5" />
                </button>
              </div>
            </div>
          );
        })}

        {activeItems.length === 0 && (
          <div className="p-6 text-center text-slate-500 text-xs flex flex-col items-center gap-2">
            <ImageIcon className="w-8 h-8 opacity-30" />
            <span>素材库为空，请点击上方上传</span>
          </div>
        )}
      </div>

      {/* Preset Sample Quick Loaders */}
      <div className="p-3 border-t border-white/5 bg-[#0a0f1a]">
        <div className="text-[11px] font-semibold text-slate-400 mb-2 flex items-center gap-1">
          <Sparkles className="w-3.5 h-3.5 text-amber-400" />
          <span>快速载入内置样张</span>
        </div>
        <div className="grid grid-cols-2 gap-1.5">
          {(activeTab === 'sources' ? SAMPLE_MEDIA.sources : SAMPLE_MEDIA.targets).map((sample) => (
            <button
              key={sample.id}
              onClick={() => loadSample(sample, activeTab === 'sources')}
              className="px-2 py-1.5 bg-[#151d30] hover:bg-[#1f2c47] border border-white/10 hover:border-blue-500/40 rounded text-[11px] text-slate-300 hover:text-white truncate text-left transition-colors"
              title={`载入 ${sample.name}`}
            >
              + {sample.name.split(' ')[0]}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
};
