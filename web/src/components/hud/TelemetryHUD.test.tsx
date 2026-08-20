import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { TelemetryHUD } from './TelemetryHUD';
import type { StudioStore } from '../../store/studioState';

describe('TelemetryHUD Component', () => {
  const createMockStore = (overrides?: Partial<StudioStore>): StudioStore => ({
    sources: [],
    targets: [],
    selectedSourceId: '',
    selectedTargetId: '',
    setSelectedSourceId: vi.fn(),
    setSelectedTargetId: vi.fn(),
    addSource: vi.fn(),
    removeSource: vi.fn(),
    addTarget: vi.fn(),
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
    detectedFaces: [],
    isDetectingFaces: false,
    runFaceDetection: vi.fn(),
    bindReferenceFaceToActiveStep: vi.fn(),
    viewportMode: 'canvas',
    setViewportMode: vi.fn(),
    compareSplitPos: 50,
    setCompareSplitPos: vi.fn(),
    tasks: [
      {
        id: 'task-1234567890abcdef',
        status: 'running',
        priority: 0,
        queue_position: 0,
        media_count: 1,
        error_message: '',
        progress: { current_frame: 45, total_frames: 100, fps: 28.5 },
      },
    ],
    activeTaskId: 'task-1234567890abcdef',
    activeTaskDetail: null,
    setActiveTaskId: vi.fn(),
    isSubmitting: false,
    errorMsg: null,
    backendStatus: 'ok v0.34.1',
    submitJob: vi.fn(),
    cancelTask: vi.fn(),
    bumpPriority: vi.fn(),
    refreshTasks: vi.fn(),
    activeSource: { id: 's1', name: 'Lenna', path: 'lenna.bmp', type: 'image' },
    activeTarget: { id: 't1', name: 'Girl', path: 'girl.bmp', type: 'image' },
    ...overrides,
  });

  it('renders live FPS, frame progress and status badge', () => {
    const store = createMockStore();
    render(<TelemetryHUD store={store} />);

    expect(screen.getByText(/28.5 FPS/i)).toBeDefined();
    expect(screen.getByText(/45\/100/i)).toBeDefined();
    expect(screen.getByText(/推理中/i)).toBeDefined();
    expect(screen.getByText(/ok v0.34.1/i)).toBeDefined();
  });

  it('handles cancel task action', () => {
    const store = createMockStore();
    render(<TelemetryHUD store={store} />);

    const cancelBtn = screen.getByTitle('取消当前任务');
    fireEvent.click(cancelBtn);
    expect(store.cancelTask).toHaveBeenCalledWith('task-1234567890abcdef');
  });

  it('opens history modal when clicking History button', () => {
    const store = createMockStore();
    render(<TelemetryHUD store={store} />);

    const historyBtn = screen.getByText(/历史成果 \(1\)/i);
    fireEvent.click(historyBtn);

    expect(screen.getByText(/历史任务与成果记录/i)).toBeDefined();
  });
});
