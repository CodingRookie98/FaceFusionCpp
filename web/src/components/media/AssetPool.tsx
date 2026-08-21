import React, { useState } from 'react';
import {
  Upload,
  Trash2,
  Image as ImageIcon,
  Film,
  Sparkles,
  CheckCircle2,
  ListOrdered,
  ArrowUp,
  ArrowDown,
  X,
  Clock,
  Activity,
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
    addSources,
    removeSource,
    addTarget,
    addTargets,
    removeTarget,
    tasks,
    activeTaskId,
    setActiveTaskId,
    bumpPriority,
    cancelTask,
    refreshTasks,
    activeTab: storeActiveTab,
    setActiveTab: storeSetActiveTab,
    setActivePreviewTarget,
  } = store;

  const [activeTab, setActiveTab] = useState<'sources' | 'targets' | 'queue'>(
    storeActiveTab || 'sources'
  );

  React.useEffect(() => {
    if (storeActiveTab) {
      setActiveTab(storeActiveTab);
    }
  }, [storeActiveTab]);

  const handleTabChange = (tab: 'sources' | 'targets' | 'queue') => {
    setActiveTab(tab);
    if (storeSetActiveTab) {
      storeSetActiveTab(tab);
    }
  };

  const [uploadProgress, setUploadProgress] = useState<{ current: number; total: number } | null>(null);
  const [isDragging, setIsDragging] = useState(false);

  const uploadMultipleFiles = async (fileList: FileList | File[], isSource: boolean) => {
    const files = Array.from(fileList);
    if (files.length === 0) return;

    setUploadProgress({ current: 0, total: files.length });
    const successfulItems: MediaItem[] = [];
    const failedFiles: { name: string; error: string }[] = [];

    let completedCount = 0;
    await Promise.all(
      files.map(async (file) => {
        try {
          const res = await api.uploadFile(file);
          const item: MediaItem = {
            id: `media-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
            path: res.path,
            name: file.name,
            type: file.type.startsWith('video') ? 'video' : 'image',
            thumbnailUrl: URL.createObjectURL(file),
            file,
          };
          successfulItems.push(item);
        } catch (err) {
          failedFiles.push({
            name: file.name,
            error: err instanceof Error ? err.message : String(err),
          });
        } finally {
          completedCount++;
          setUploadProgress({ current: completedCount, total: files.length });
        }
      })
    );

    if (successfulItems.length > 0) {
      if (isSource) {
        addSources(successfulItems);
      } else {
        addTargets(successfulItems);
      }
    }

    setUploadProgress(null);

    if (failedFiles.length > 0 && typeof window !== 'undefined' && typeof window.alert === 'function') {
      window.alert(
        `上传完成：成功 ${successfulItems.length} 个，失败 ${failedFiles.length} 个：\n` +
          failedFiles.map((f) => `• ${f.name}: ${f.error}`).join('\n')
      );
    }
  };

  const handleFileInputChange = (e: React.ChangeEvent<HTMLInputElement>, isSource: boolean) => {
    if (e.target.files) {
      uploadMultipleFiles(e.target.files, isSource);
      e.target.value = '';
    }
  };

  const handleDragOver = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(true);
  };

  const handleDragLeave = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(false);
  };

  const handleDrop = (e: React.DragEvent, isSource: boolean) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(false);
    if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      uploadMultipleFiles(e.dataTransfer.files, isSource);
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
    <div className="w-80 flex-shrink-0 h-full bg-[#0f1523] border-r border-white/5 flex flex-col select-none overflow-hidden">
      {/* Header Tabs */}
      <div className="p-2.5 border-b border-white/10 flex items-center justify-between">
        <div className="flex bg-[#151d30] p-0.5 rounded-lg border border-white/10 w-full gap-0.5">
          <button
            onClick={() => {
              handleTabChange('sources');
              if (selectedSourceId && setActivePreviewTarget) setActivePreviewTarget('source');
            }}
            className={`flex-1 py-1.5 px-1 text-[11px] font-medium rounded-md transition-all flex items-center justify-center gap-1 truncate ${
              activeTab === 'sources'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <ImageIcon className="w-3.5 h-3.5 flex-shrink-0" />
            <span className="truncate">源素材 ({sources.length})</span>
          </button>

          <button
            onClick={() => {
              handleTabChange('targets');
              if (selectedTargetId && setActivePreviewTarget) setActivePreviewTarget('target');
            }}
            className={`flex-1 py-1.5 px-1 text-[11px] font-medium rounded-md transition-all flex items-center justify-center gap-1 truncate ${
              activeTab === 'targets'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Film className="w-3.5 h-3.5 flex-shrink-0" />
            <span className="truncate">目标素材 ({targets.length})</span>
          </button>

          <button
            onClick={() => {
              handleTabChange('queue');
              if (activeTaskId && setActivePreviewTarget) setActivePreviewTarget('task');
            }}
            className={`flex-1 py-1.5 px-1 text-[11px] font-medium rounded-md transition-all flex items-center justify-center gap-1 truncate ${
              activeTab === 'queue'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <ListOrdered className="w-3.5 h-3.5 flex-shrink-0" />
            <span className="truncate">任务队列 ({tasks.length})</span>
          </button>
        </div>
      </div>

      {/* Conditional Content: Sources & Targets vs Task Queue */}
      {activeTab !== 'queue' ? (
        <>
          {/* Upload Dropzone / Button */}
          <div className="p-3 border-b border-white/5">
            <label
              onDragOver={handleDragOver}
              onDragLeave={handleDragLeave}
              onDrop={(e) => handleDrop(e, activeTab === 'sources')}
              className={`flex flex-col items-center justify-center border border-dashed rounded-lg p-3 cursor-pointer transition-all group ${
                isDragging
                  ? 'border-blue-400 bg-blue-600/20 shadow-[0_0_15px_rgba(59,130,246,0.3)] scale-[1.02]'
                  : 'border-white/20 hover:border-blue-500/80 bg-[#151d30]/60 hover:bg-[#151d30]'
              }`}
            >
              <Upload className={`w-5 h-5 mb-1 transition-colors ${isDragging ? 'text-blue-400 animate-bounce' : 'text-slate-400 group-hover:text-blue-400'}`} />
              <span className="text-xs text-slate-300 font-medium group-hover:text-white text-center">
                {uploadProgress
                  ? `正在上传 (${uploadProgress.current}/${uploadProgress.total})...`
                  : isDragging
                  ? '松开以批量导入素材'
                  : activeTab === 'sources'
                  ? '点击或拖拽上传多张源素材'
                  : '点击或拖拽上传多份目标素材'}
              </span>
              <span className="text-[10px] text-slate-500 mt-0.5">支持同时选中多个 JPG, PNG, BMP, MP4, MOV</span>
              <input
                type="file"
                multiple
                accept={activeTab === 'sources' ? 'image/*' : 'image/*,video/*'}
                onChange={(e) => handleFileInputChange(e, activeTab === 'sources')}
                disabled={uploadProgress !== null}
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
                    if (activeTab === 'sources') {
                      setSelectedSourceId(item.id);
                      if (setActivePreviewTarget) setActivePreviewTarget('source');
                    } else {
                      setSelectedTargetId(item.id);
                      if (setActivePreviewTarget) setActivePreviewTarget('target');
                    }
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
        </>
      ) : (
        /* Task Queue View */
        <div className="flex-1 flex flex-col min-h-0 overflow-hidden">
          {/* Queue List Header */}
          <div className="px-3 py-2 border-b border-white/5 bg-[#121929] flex items-center justify-between text-xs text-slate-400">
            <span className="font-semibold flex items-center gap-1 text-slate-300">
              <Activity className="w-3.5 h-3.5 text-cyan-400" />
              <span>调度队列</span>
            </span>
            <button
              onClick={() => refreshTasks()}
              className="text-[10px] text-slate-400 hover:text-cyan-300 px-1.5 py-0.5 rounded bg-slate-800 hover:bg-slate-700 transition-colors"
              title="刷新任务列表"
            >
              刷新
            </button>
          </div>

          {/* Task Cards List */}
          <div className="flex-1 overflow-y-auto p-2.5 space-y-2">
            {tasks.map((task) => {
              const isActive = task.id === activeTaskId;
              const isPending = task.status === 'queued';
              const isRunning = task.status === 'running';
              const isDone = task.status === 'done';
              const isFailed = task.status === 'failed';

              const progressPercent =
                task.progress && task.progress.total_frames > 0
                  ? Math.round((task.progress.current_frame / task.progress.total_frames) * 100)
                  : 0;

              return (
                <div
                  key={task.id}
                  onClick={() => {
                    setActiveTaskId(task.id);
                    if (setActivePreviewTarget) setActivePreviewTarget('task');
                  }}
                  className={`group relative p-2.5 rounded-lg border transition-all cursor-pointer flex flex-col gap-2 ${
                    isActive
                      ? 'bg-[#1e2a45] border-blue-500/80 shadow-[0_0_12px_rgba(59,130,246,0.25)]'
                      : 'bg-[#151d30] border-white/5 hover:border-white/20 hover:bg-[#182238]'
                  }`}
                >
                  {/* Card Header: Status Badge & ID */}
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-1.5">
                      <span
                        className={`text-[10px] font-semibold px-1.5 py-0.5 rounded font-mono ${
                          isRunning
                            ? 'bg-blue-950 text-blue-300 border border-blue-500/50 animate-pulse'
                            : isPending
                            ? 'bg-amber-950/80 text-amber-300 border border-amber-500/40'
                            : isDone
                            ? 'bg-emerald-950/80 text-emerald-300 border border-emerald-500/40'
                            : isFailed
                            ? 'bg-rose-950/80 text-rose-300 border border-rose-500/40'
                            : 'bg-slate-800 text-slate-400 border border-white/10'
                        }`}
                      >
                        {isRunning
                          ? '⚡ 正在处理'
                          : isPending
                          ? `⏳ 排队中 (P${task.priority})`
                          : isDone
                          ? '✓ 已完成'
                          : isFailed
                          ? '✕ 失败'
                          : '已取消'}
                      </span>
                      <span className="text-[10px] font-mono text-slate-400 truncate max-w-[100px]">
                        #{task.id.slice(-8)}
                      </span>
                    </div>

                    {/* Actions: Up, Down, Cancel */}
                    <div className="flex items-center gap-0.5">
                      {isPending && (
                        <>
                          <button
                            onClick={(e) => {
                              e.stopPropagation();
                              bumpPriority(task.id, 1);
                            }}
                            className="p-1 text-slate-400 hover:text-cyan-300 hover:bg-white/10 rounded transition-colors"
                            title="提升优先级"
                          >
                            <ArrowUp className="w-3.5 h-3.5" />
                          </button>
                          <button
                            onClick={(e) => {
                              e.stopPropagation();
                              bumpPriority(task.id, -1);
                            }}
                            className="p-1 text-slate-400 hover:text-amber-300 hover:bg-white/10 rounded transition-colors"
                            title="降低优先级"
                          >
                            <ArrowDown className="w-3.5 h-3.5" />
                          </button>
                        </>
                      )}

                      {(isPending || isRunning) && (
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            cancelTask(task.id);
                          }}
                          className="p-1 text-slate-400 hover:text-rose-400 hover:bg-rose-950/50 rounded transition-colors"
                          title="取消任务"
                        >
                          <X className="w-3.5 h-3.5" />
                        </button>
                      )}
                    </div>
                  </div>

                  {/* Progress Bar for Running Tasks */}
                  {isRunning && (
                    <div className="space-y-1">
                      <div className="w-full bg-black/50 h-1.5 rounded-full overflow-hidden">
                        <div
                          className="bg-gradient-to-r from-blue-500 to-cyan-400 h-full rounded-full transition-all duration-300"
                          style={{ width: `${progressPercent}%` }}
                        />
                      </div>
                      <div className="flex justify-between text-[10px] text-slate-400 font-mono">
                        <span>{progressPercent}%</span>
                        {task.progress && task.progress.fps > 0 && (
                          <span>{task.progress.fps.toFixed(1)} FPS</span>
                        )}
                      </div>
                    </div>
                  )}

                  {/* Card Footer Info */}
                  <div className="flex items-center justify-between text-[10px] text-slate-500 pt-0.5">
                    <span className="truncate">
                      {task.created_at ? new Date(task.created_at).toLocaleTimeString() : '刚刚'}
                    </span>
                    <span className="text-blue-400/80 group-hover:text-blue-300">
                      点击画布预览 →
                    </span>
                  </div>
                </div>
              );
            })}

            {tasks.length === 0 && (
              <div className="p-8 text-center text-slate-500 text-xs flex flex-col items-center gap-2">
                <Clock className="w-8 h-8 opacity-30" />
                <span>任务队列为空</span>
                <span className="text-[10px] text-slate-600">在右侧管线点击“添加到任务队列”以创建任务</span>
              </div>
            )}
          </div>
        </div>
      )}
    </div>
  );
};
