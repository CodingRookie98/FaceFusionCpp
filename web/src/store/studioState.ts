import { useState, useEffect, useCallback } from 'react';
import { api, subscribeProgress } from '../api/client';
import type {
  DetectedFace,
  PipelineStepConfig,
  ProcessorMeta,
  TaskDetail,
  TaskSummary,
  WsMessage,
} from '../api/types';

export interface MediaItem {
  id: string;
  path: string;
  name: string;
  type: 'image' | 'video';
  thumbnailUrl?: string;
  file?: File;
}

export interface PresetConfig {
  id: string;
  name: string;
  icon: string;
  description: string;
  steps: Array<{
    step: string;
    name?: string;
    params: Record<string, string | number | boolean>;
  }>;
}

export const OFFICIAL_PRESETS: PresetConfig[] = [
  {
    id: 'fast_swap',
    name: '极速单人换脸',
    icon: 'Zap',
    description: '单 Inswapper FP16 高性能低显存换脸',
    steps: [
      {
        step: 'face_swapper',
        name: '主角人脸替换',
        params: {
          model: 'inswapper_128_fp16',
          face_selector_mode: 'one',
        },
      },
    ],
  },
  {
    id: 'hd_portrait',
    name: '高清写真重塑',
    icon: 'Sparkles',
    description: '换脸 + CodeFormer 高清面部修复 + 微表情还原',
    steps: [
      {
        step: 'face_swapper',
        name: '基础换脸',
        params: {
          model: 'inswapper_128',
          face_selector_mode: 'many',
        },
      },
      {
        step: 'face_enhancer',
        name: 'CodeFormer 面部高清增强',
        params: {
          model: 'codeformer',
          blend_factor: 0.85,
          face_selector_mode: 'many',
        },
      },
      {
        step: 'expression_restorer',
        name: '自然表情微调',
        params: {
          model: 'live_portrait',
          restore_factor: 0.6,
          face_selector_mode: 'many',
        },
      },
    ],
  },
  {
    id: 'multi_face_swap',
    name: '多人精准多脸替换',
    icon: 'Users',
    description: '双 Inswapper 独立参考人脸定向绑定',
    steps: [
      {
        step: 'face_swapper',
        name: '主角换脸 (参考脸 1)',
        params: {
          model: 'inswapper_128',
          face_selector_mode: 'reference',
          reference_face_path: '',
        },
      },
      {
        step: 'face_swapper',
        name: '配角换脸 (参考脸 2)',
        params: {
          model: 'inswapper_128',
          face_selector_mode: 'reference',
          reference_face_path: '',
        },
      },
      {
        step: 'face_enhancer',
        name: '全局人脸增强',
        params: {
          model: 'gfpgan_1.4',
          blend_factor: 0.8,
          face_selector_mode: 'many',
        },
      },
    ],
  },
  {
    id: 'cinematic',
    name: '影视级全流程超分',
    icon: 'Film',
    description: '换脸 + GFPGAN + 表情还原 + RealESRGAN 全画幅超分',
    steps: [
      {
        step: 'face_swapper',
        name: '电影级换脸',
        params: {
          model: 'inswapper_128',
          face_selector_mode: 'many',
        },
      },
      {
        step: 'face_enhancer',
        name: 'GFPGAN 面部增强',
        params: {
          model: 'gfpgan_1.4',
          blend_factor: 0.9,
          face_selector_mode: 'many',
        },
      },
      {
        step: 'expression_restorer',
        name: 'LivePortrait 表情精修',
        params: {
          model: 'live_portrait',
          restore_factor: 0.7,
          face_selector_mode: 'many',
        },
      },
      {
        step: 'frame_enhancer',
        name: 'Real-ESRGAN 全画幅 4x 超分',
        params: {
          model: 'real_esrgan_x4plus',
          enhance_factor: 1.0,
        },
      },
    ],
  },
];

export function getMediaPreviewUrl(item?: MediaItem | null): string {
  if (!item) return '';
  if (item.thumbnailUrl && !item.thumbnailUrl.startsWith('/media/')) {
    return item.thumbnailUrl;
  }
  if (item.file) {
    try {
      return URL.createObjectURL(item.file);
    } catch {
      // fallback
    }
  }
  if (item.path) {
    return `/api/preview?path=${encodeURIComponent(item.path)}`;
  }
  return '';
}

export const SAMPLE_MEDIA: { sources: MediaItem[]; targets: MediaItem[] } = {
  sources: [
    {
      id: 'sample-s1',
      path: 'assets/standard_face_test_images/lenna.bmp',
      name: 'Lenna (经典测试头像)',
      type: 'image',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/lenna.bmp')}`,
    },
    {
      id: 'sample-s2',
      path: 'assets/standard_face_test_images/man.bmp',
      name: 'Man (男士肖像)',
      type: 'image',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/man.bmp')}`,
    },
    {
      id: 'sample-s3',
      path: 'assets/standard_face_test_images/barbara.bmp',
      name: 'Barbara (女士肖像)',
      type: 'image',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/barbara.bmp')}`,
    },
  ],
  targets: [
    {
      id: 'sample-t1',
      path: 'assets/standard_face_test_images/girl.bmp',
      name: 'Girl (单人目标图)',
      type: 'image',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/girl.bmp')}`,
    },
    {
      id: 'sample-t2',
      path: 'assets/standard_face_test_images/woman.jpg',
      name: 'Woman (女士目标图)',
      type: 'image',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/woman.jpg')}`,
    },
    {
      id: 'sample-t3',
      path: 'assets/standard_face_test_images/tiffany.bmp',
      name: 'Tiffany (肖像目标图)',
      type: 'image',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/tiffany.bmp')}`,
    },
    {
      id: 'sample-t4',
      path: 'assets/standard_face_test_videos/slideshow_scaled.mp4',
      name: 'Slideshow (测试视频)',
      type: 'video',
      thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_videos/slideshow_scaled.mp4')}`,
    },
  ],
};

const sanitizeSources = (items: MediaItem[]): MediaItem[] => {
  return items.map((item) => {
    if (item.path === 'assets/standard_face_test_images/avatar_man.png') {
      return {
        ...item,
        path: 'assets/standard_face_test_images/man.bmp',
        name: 'Man (男士肖像)',
        thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/man.bmp')}`,
      };
    }
    return item;
  });
};

const sanitizeTargets = (items: MediaItem[]): MediaItem[] => {
  return items.map((item) => {
    if (item.path === 'assets/standard_face_test_images/family.jpg') {
      return {
        ...item,
        path: 'assets/standard_face_test_images/woman.jpg',
        name: 'Woman (女士目标图)',
        thumbnailUrl: `/api/preview?path=${encodeURIComponent('assets/standard_face_test_images/woman.jpg')}`,
      };
    }
    return item;
  });
};

const STORAGE_KEY_SOURCES = 'ffc_studio_sources_v2';
const STORAGE_KEY_TARGETS = 'ffc_studio_targets_v2';
const STORAGE_KEY_PIPELINE = 'ffc_studio_pipeline_v2';

export function useStudioStore() {
  // 1. Sources & Targets
  const [sources, setSources] = useState<MediaItem[]>(() => {
    try {
      const saved = localStorage.getItem(STORAGE_KEY_SOURCES);
      return saved ? sanitizeSources(JSON.parse(saved)) : SAMPLE_MEDIA.sources;
    } catch {
      return SAMPLE_MEDIA.sources;
    }
  });

  const [targets, setTargets] = useState<MediaItem[]>(() => {
    try {
      const saved = localStorage.getItem(STORAGE_KEY_TARGETS);
      return saved ? sanitizeTargets(JSON.parse(saved)) : SAMPLE_MEDIA.targets;
    } catch {
      return SAMPLE_MEDIA.targets;
    }
  });

  const [selectedSourceId, setSelectedSourceId] = useState<string>(
    () => sources[0]?.id || ''
  );
  const [selectedTargetId, setSelectedTargetId] = useState<string>(
    () => targets[0]?.id || ''
  );

  // 2. Processors & Pipeline Steps
  const [availableProcessors, setAvailableProcessors] = useState<ProcessorMeta[]>([]);
  const [steps, setSteps] = useState<PipelineStepConfig[]>(() => {
    try {
      const saved = localStorage.getItem(STORAGE_KEY_PIPELINE);
      if (saved) return JSON.parse(saved);
    } catch {}
    return OFFICIAL_PRESETS[0].steps.map((s, idx) => ({
      id: `step-${Date.now()}-${idx}`,
      step: s.step,
      name: s.name || s.step,
      enabled: true,
      params: { ...s.params },
    }));
  });

  const [activeBindingStepId, setActiveBindingStepId] = useState<string | null>(null);

  // 3. Canvas State & Detected Faces
  const [detectedFaces, setDetectedFaces] = useState<DetectedFace[]>([]);
  const [isDetectingFaces, setIsDetectingFaces] = useState(false);
  const [viewportMode, setViewportMode] = useState<'canvas' | 'compare' | 'loupe' | 'result'>('canvas');
  const [compareSplitPos, setCompareSplitPos] = useState<number>(50); // percentage 0-100

  // 4. Tasks & Telemetry
  const [tasks, setTasks] = useState<TaskSummary[]>([]);
  const [activeTaskId, setActiveTaskId] = useState<string | null>(null);
  const [activeTaskDetail, setActiveTaskDetail] = useState<TaskDetail | null>(null);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);
  const [backendStatus, setBackendStatus] = useState<string>('connecting');

  // Persistence
  useEffect(() => {
    try {
      localStorage.setItem(STORAGE_KEY_SOURCES, JSON.stringify(sources));
    } catch {}
  }, [sources]);

  useEffect(() => {
    try {
      localStorage.setItem(STORAGE_KEY_TARGETS, JSON.stringify(targets));
    } catch {}
  }, [targets]);

  useEffect(() => {
    try {
      localStorage.setItem(STORAGE_KEY_PIPELINE, JSON.stringify(steps));
    } catch {}
  }, [steps]);

  // Initial Data Fetch
  useEffect(() => {
    api
      .health()
      .then((h) => setBackendStatus(`${h.status} v${h.version}`))
      .catch(() => setBackendStatus('offline'));

    api
      .listProcessors()
      .then((list) => {
        if (list && list.length > 0) setAvailableProcessors(list);
      })
      .catch(() => {});
  }, []);

  // Poll Tasks
  const refreshTasks = useCallback(() => {
    api
      .listTasks()
      .then((list) => {
        setTasks(list);
        // auto-select currently running task if none selected
        if (!activeTaskId) {
          const running = list.find((t) => t.status === 'running' || t.status === 'queued');
          if (running) setActiveTaskId(running.id);
          else if (list[0]) setActiveTaskId(list[0].id);
        } else {
          // If active task changed to done, ensure we have full detail with results
          const cur = list.find((t) => t.id === activeTaskId);
          if (
            cur &&
            cur.status === 'done' &&
            (!activeTaskDetail ||
              activeTaskDetail.status !== 'done' ||
              !activeTaskDetail.results ||
              activeTaskDetail.results.length === 0)
          ) {
            api
              .getTask(activeTaskId)
              .then((d) => {
                setActiveTaskDetail(d);
                if (d.status === 'done' && d.results && d.results.length > 0) {
                  setViewportMode('compare');
                }
              })
              .catch(() => {});
          }
        }
      })
      .catch(() => {});
  }, [activeTaskId, activeTaskDetail]);

  useEffect(() => {
    refreshTasks();
    const timer = setInterval(refreshTasks, 2500);
    return () => clearInterval(timer);
  }, [refreshTasks]);

  // Active Task Detail & WS Subscription
  useEffect(() => {
    if (!activeTaskId) {
      setActiveTaskDetail(null);
      return;
    }
    let unsub: (() => void) | undefined;
    api
      .getTask(activeTaskId)
      .then((detail) => {
        setActiveTaskDetail(detail);
        if (detail.status === 'done' && detail.results && detail.results.length > 0) {
          setViewportMode('compare');
        }
        if (detail.status === 'queued' || detail.status === 'running') {
          unsub = subscribeProgress(activeTaskId, (msg: WsMessage) => {
            if (msg.type === 'progress') {
              setActiveTaskDetail((prev) =>
                prev && prev.id === activeTaskId
                  ? {
                      ...prev,
                      status: 'running',
                      progress: {
                        current_frame: msg.frame,
                        total_frames: msg.total,
                        fps: msg.fps,
                      },
                    }
                  : prev
              );
            } else if (msg.type === 'status') {
              api
                .getTask(activeTaskId)
                .then((latestDetail) => {
                  setActiveTaskDetail(latestDetail);
                  if (latestDetail.status === 'done' && latestDetail.results && latestDetail.results.length > 0) {
                    setViewportMode('compare');
                  }
                })
                .catch(() => {
                  setActiveTaskDetail((prev) =>
                    prev && prev.id === activeTaskId
                      ? { ...prev, status: msg.status, error_message: msg.message || '' }
                      : prev
                  );
                });
              refreshTasks();
            }
          });
        }
      })
      .catch(() => {});

    return () => unsub?.();
  }, [activeTaskId, refreshTasks]);

  // Auto Detect Faces on Target Media Change
  const activeTarget = targets.find((t) => t.id === selectedTargetId) || targets[0];
  const activeSource = sources.find((s) => s.id === selectedSourceId) || sources[0];

  const runFaceDetection = useCallback((imagePath: string) => {
    if (!imagePath) return;
    setIsDetectingFaces(true);
    api
      .detectFaces(imagePath)
      .then((res) => {
        if (res && res.faces) {
          setDetectedFaces(res.faces);
        } else {
          setDetectedFaces([]);
        }
      })
      .catch(() => {
        setDetectedFaces([]);
      })
      .finally(() => {
        setIsDetectingFaces(false);
      });
  }, []);

  useEffect(() => {
    if (activeTarget && activeTarget.type === 'image') {
      runFaceDetection(activeTarget.path);
    } else {
      setDetectedFaces([]);
    }
  }, [activeTarget, runFaceDetection]);

  // Actions
  const addSource = (item: MediaItem) => {
    setSources((prev) => [...prev, item]);
    setSelectedSourceId(item.id);
  };

  const removeSource = (id: string) => {
    setSources((prev) => prev.filter((s) => s.id !== id));
  };

  const addTarget = (item: MediaItem) => {
    setTargets((prev) => [...prev, item]);
    setSelectedTargetId(item.id);
  };

  const removeTarget = (id: string) => {
    setTargets((prev) => prev.filter((t) => t.id !== id));
  };

  const applyPreset = (preset: PresetConfig) => {
    setSteps(
      preset.steps.map((s, idx) => ({
        id: `step-${Date.now()}-${idx}`,
        step: s.step,
        name: s.name || s.step,
        enabled: true,
        params: { ...s.params },
      }))
    );
  };

  const addStep = (stepType: string) => {
    const initialParams: Record<string, any> = {};
    if (stepType === 'face_swapper') {
      initialParams.model = 'inswapper_128';
      initialParams.face_selector_mode = 'many';
    } else if (stepType === 'face_enhancer') {
      initialParams.model = 'codeformer';
      initialParams.blend_factor = 0.8;
      initialParams.face_selector_mode = 'many';
    } else if (stepType === 'expression_restorer') {
      initialParams.model = 'live_portrait';
      initialParams.restore_factor = 0.8;
      initialParams.face_selector_mode = 'many';
    } else if (stepType === 'frame_enhancer') {
      initialParams.model = 'real_esrgan_x4plus';
      initialParams.enhance_factor = 0.8;
    }
    const newStep: PipelineStepConfig = {
      id: `step-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
      step: stepType,
      name: `${stepType} #${steps.filter((s) => s.step === stepType).length + 1}`,
      enabled: true,
      params: initialParams,
    };
    setSteps((prev) => [...prev, newStep]);
  };

  const updateStep = (id: string, partial: Partial<PipelineStepConfig>) => {
    setSteps((prev) =>
      prev.map((s) => (s.id === id ? { ...s, ...partial } : s))
    );
  };

  const removeStep = (id: string) => {
    setSteps((prev) => prev.filter((s) => s.id !== id));
  };

  const moveStep = (index: number, direction: 'up' | 'down') => {
    setSteps((prev) => {
      const copy = [...prev];
      const targetIdx = direction === 'up' ? index - 1 : index + 1;
      if (targetIdx < 0 || targetIdx >= copy.length) return prev;
      const [item] = copy.splice(index, 1);
      copy.splice(targetIdx, 0, item);
      return copy;
    });
  };

  const bindReferenceFaceToActiveStep = (face: DetectedFace, sourcePath?: string) => {
    const targetStepId = activeBindingStepId || steps.find((s) => s.step === 'face_swapper')?.id;
    if (!targetStepId) return;
    const path = sourcePath || activeSource?.path || activeTarget?.path || '';
    setSteps((prev) =>
      prev.map((s) =>
        s.id === targetStepId
          ? {
              ...s,
              params: {
                ...s.params,
                face_selector_mode: 'reference',
                reference_face_path: path,
                selected_face_index: face.index,
              },
            }
          : s
      )
    );
  };

  const submitJob = async () => {
    setErrorMsg(null);
    if (sources.length === 0 || targets.length === 0) {
      setErrorMsg('请至少添加一个源图片素材和一个目标素材');
      return;
    }
    if (steps.filter((s) => s.enabled).length === 0) {
      setErrorMsg('请至少启用一个处理器步骤');
      return;
    }

    const payload = {
      source_paths: sources.map((s) => s.path),
      target_paths: targets.map((t) => t.path),
      output_path: './output',
      pipeline_steps: steps
        .filter((s) => s.enabled)
        .map((s) => {
          const params = { ...s.params };
          if (
            params.face_selector_mode === 'reference' &&
            (!params.reference_face_path || params.reference_face_path === '')
          ) {
            params.reference_face_path = sources[0]?.path || '';
          }
          return {
            step: s.step,
            name: s.name,
            enabled: true,
            params,
          };
        }),
    };

    setIsSubmitting(true);
    try {
      const res = await api.submitTask(payload);
      setActiveTaskId(res.id);
      refreshTasks();
    } catch (e) {
      setErrorMsg(e instanceof Error ? e.message : String(e));
    } finally {
      setIsSubmitting(false);
    }
  };

  const cancelTask = async (taskId: string) => {
    try {
      await api.cancelTask(taskId);
      refreshTasks();
    } catch (e) {
      setErrorMsg(e instanceof Error ? e.message : String(e));
    }
  };

  const bumpPriority = async (taskId: string, delta: number) => {
    const task = tasks.find((t) => t.id === taskId);
    if (!task) return;
    try {
      await api.setPriority(taskId, task.priority + delta);
      refreshTasks();
    } catch (e) {
      setErrorMsg(e instanceof Error ? e.message : String(e));
    }
  };

  return {
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
    steps,
    availableProcessors,
    applyPreset,
    addStep,
    updateStep,
    removeStep,
    moveStep,
    activeBindingStepId,
    setActiveBindingStepId,
    detectedFaces,
    isDetectingFaces,
    runFaceDetection,
    bindReferenceFaceToActiveStep,
    viewportMode,
    setViewportMode,
    compareSplitPos,
    setCompareSplitPos,
    tasks,
    activeTaskId,
    activeTaskDetail,
    setActiveTaskId,
    isSubmitting,
    errorMsg,
    backendStatus,
    submitJob,
    cancelTask,
    bumpPriority,
    refreshTasks,
    activeSource,
    activeTarget,
  };
}

export type StudioStore = ReturnType<typeof useStudioStore>;
