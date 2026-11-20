import { describe, expect, it, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import { ViewportCanvas } from './ViewportCanvas';
import type { StudioStore } from '../../store/studioState';

describe('ViewportCanvas Component', () => {
  const createMockStore = (overrides?: Partial<StudioStore>): StudioStore => ({
    sources: [
      { id: 's1', name: 'Lenna.bmp', path: 'assets/lenna.bmp', type: 'image' },
    ],
    targets: [
      { id: 't1', name: 'Girl.bmp', path: 'assets/girl.bmp', type: 'image' },
    ],
    selectedSourceId: 's1',
    selectedTargetId: 't1',
    setSelectedSourceId: vi.fn(),
    setSelectedTargetId: vi.fn(),
    addSource: vi.fn(),
    addSources: vi.fn(),
    removeSource: vi.fn(),
    addTarget: vi.fn(),
    addTargets: vi.fn(),
    removeTarget: vi.fn(),
    steps: [],
    availableProcessors: [],
    applyPreset: vi.fn(),
    addStep: vi.fn(),
    updateStep: vi.fn(),
    removeStep: vi.fn(),
    moveStep: vi.fn(),
    activeBindingStepId: null,
    setActiveBindingStepId: vi.fn(),
    detectedFaces: [
      { index: 0, box: { x: 10, y: 10, width: 40, height: 40 }, score: 0.95, kps: [] },
      { index: 1, box: { x: 60, y: 10, width: 40, height: 40 }, score: 0.92, kps: [] },
    ],
    selectedFaceIndices: [0],
    setSelectedFaceIndices: vi.fn(),
    toggleFaceSelection: vi.fn(),
    selectAllFaces: vi.fn(),
    clearFaceSelection: vi.fn(),
    isDetectingFaces: false,
    runFaceDetection: vi.fn(),
    bindReferenceFaceToActiveStep: vi.fn(),
    viewportMode: 'canvas',
    setViewportMode: vi.fn(),
    compareSplitPos: 50,
    setCompareSplitPos: vi.fn(),
    tasks: [
      {
        id: 'task-1001',
        status: 'queued',
        priority: 15,
        created_at: '2026-08-21T10:00:00Z',
        progress: { current_frame: 0, total_frames: 100, fps: 0 },
      },
      {
        id: 'task-1002',
        status: 'running',
        priority: 5,
        created_at: '2026-08-21T10:05:00Z',
        progress: { current_frame: 60, total_frames: 100, fps: 25.4 },
      },
    ],
    activeTaskId: 'task-1001',
    activeTaskDetail: null,
    setActiveTaskId: vi.fn(),
    isSubmitting: false,
    isRenderingPreview: false,
    previewResultUrl: null,
    setPreviewResultUrl: vi.fn(),
    renderPreview: vi.fn(),
    clearPreviewResult: vi.fn(),
    isVideoPlaying: false,
    setIsVideoPlaying: vi.fn(),
    errorMsg: null,
    backendStatus: 'online',
    submitJob: vi.fn(),
    cancelTask: vi.fn(),
    bumpPriority: vi.fn(),
    refreshTasks: vi.fn(),
    activeSource: { id: 's1', name: 'Lenna.bmp', path: 'assets/lenna.bmp', type: 'image' },
    activeTarget: { id: 't1', name: 'Girl.bmp', path: 'assets/girl.bmp', type: 'image' },
    activePreviewTarget: 'target',
    setActivePreviewTarget: vi.fn(),
    activeTab: 'targets',
    setActiveTab: vi.fn(),
    selectSourceForPreview: vi.fn(),
    selectTargetForPreview: vi.fn(),
    selectTaskForPreview: vi.fn(),
    ...overrides,
  });

  it('renders target mode with face selection counters and buttons', () => {
    const store = createMockStore({ activePreviewTarget: 'target' });
    render(<ViewportCanvas store={store} />);

    expect(screen.getByText(/目标: Girl.bmp/i)).toBeDefined();
    expect(screen.getByText(/✓ 2 张人脸/i)).toBeDefined();
    expect(screen.getByText(/已选 1\/2/i)).toBeDefined();
    expect(screen.getByTitle(/全选所有人脸/i)).toBeDefined();
    expect(screen.getByTitle(/清空人脸选择/i)).toBeDefined();
  });

  it('renders source mode preview when activePreviewTarget is source', () => {
    const store = createMockStore({ activePreviewTarget: 'source' });
    render(<ViewportCanvas store={store} />);

    expect(screen.getByText(/源素材: Lenna.bmp/i)).toBeDefined();
    expect(screen.getByText(/源素材画布/i)).toBeDefined();
  });

  it('renders queued task preview overlay with priority badge', () => {
    const store = createMockStore({
      activePreviewTarget: 'task',
      activeTaskId: 'task-1001',
    });
    render(<ViewportCanvas store={store} />);

    expect(screen.getByText(/任务排队中.../i)).toBeDefined();
    expect(screen.getByText(/调度优先级: P15/i)).toBeDefined();
  });

  it('renders running task preview overlay with progress and FPS HUD', () => {
    const store = createMockStore({
      activePreviewTarget: 'task',
      activeTaskId: 'task-1002',
      activeTaskDetail: {
        id: 'task-1002',
        status: 'running',
        priority: 5,
        created_at: '2026-08-21T10:05:00Z',
        progress: { current_frame: 60, total_frames: 100, fps: 25.4 },
        results: [],
      } as any,
    });
    render(<ViewportCanvas store={store} />);

    expect(screen.getByText(/正在处理任务 #task-1002/i)).toBeDefined();
    expect(screen.getByText(/60 \/ 100 帧 \(60%\)/i)).toBeDefined();
    expect(screen.getByText(/25.4 FPS/i)).toBeDefined();
  });
});
