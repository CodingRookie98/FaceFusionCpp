import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { AssetPool } from './AssetPool';
import type { StudioStore } from '../../store/studioState';

describe('AssetPool Component', () => {
  const createMockStore = (): StudioStore => ({
    sources: [
      { id: 's1', name: 'Lenna', path: 'lenna.bmp', type: 'image' },
      { id: 's2', name: 'Avatar', path: 'avatar.png', type: 'image' },
    ],
    targets: [
      { id: 't1', name: 'Girl', path: 'girl.bmp', type: 'image' },
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
    detectedFaces: [],
    isDetectingFaces: false,
    runFaceDetection: vi.fn(),
    bindReferenceFaceToActiveStep: vi.fn(),
    viewportMode: 'canvas',
    setViewportMode: vi.fn(),
    compareSplitPos: 50,
    setCompareSplitPos: vi.fn(),
    tasks: [
      { id: 'task-1', status: 'queued', priority: 10, created_at: '2026-08-21T10:00:00Z', progress: { current_frame: 0, total_frames: 100, fps: 0 } },
      { id: 'task-2', status: 'running', priority: 5, created_at: '2026-08-21T10:05:00Z', progress: { current_frame: 45, total_frames: 100, fps: 24.5 } },
    ],
    activeTaskId: 'task-1',
    activeTaskDetail: null,
    setActiveTaskId: vi.fn(),
    isSubmitting: false,
    errorMsg: null,
    backendStatus: 'online',
    submitJob: vi.fn(),
    cancelTask: vi.fn(),
    bumpPriority: vi.fn(),
    refreshTasks: vi.fn(),
    activeSource: { id: 's1', name: 'Lenna', path: 'lenna.bmp', type: 'image' },
    activeTarget: { id: 't1', name: 'Girl', path: 'girl.bmp', type: 'image' },
    activePreviewTarget: 'target',
    setActivePreviewTarget: vi.fn(),
    activeTab: 'sources',
    setActiveTab: vi.fn(),
    selectedFaceIndices: [0],
    setSelectedFaceIndices: vi.fn(),
    toggleFaceSelection: vi.fn(),
    selectAllFaces: vi.fn(),
    clearFaceSelection: vi.fn(),
    selectSourceForPreview: vi.fn(),
    selectTargetForPreview: vi.fn(),
    selectTaskForPreview: vi.fn(),
  });

  it('renders sources, targets, and queue with 3-tab switching', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    // Initial tab is sources (renamed to 源素材)
    expect(screen.getByText(/源素材 \(2\)/i)).toBeDefined();
    expect(screen.getByText(/目标素材 \(1\)/i)).toBeDefined();
    expect(screen.getByText(/任务队列 \(2\)/i)).toBeDefined();
    expect(screen.getByText('Lenna')).toBeDefined();
    expect(screen.getByText('Avatar')).toBeDefined();

    // Switch to targets tab
    const targetTabBtn = screen.getByText(/目标素材 \(1\)/i);
    fireEvent.click(targetTabBtn);
    expect(screen.getByText('Girl')).toBeDefined();

    // Switch to task queue tab
    const queueTabBtn = screen.getByText(/任务队列 \(2\)/i);
    fireEvent.click(queueTabBtn);
    expect(screen.getByText(/task-1/i)).toBeDefined();
    expect(screen.getByText(/task-2/i)).toBeDefined();
  });

  it('handles item selection on click and sets preview target', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    const avatarItem = screen.getByText('Avatar');
    fireEvent.click(avatarItem);
    expect(store.setSelectedSourceId).toHaveBeenCalledWith('s2');
    expect(store.setActivePreviewTarget).toHaveBeenCalledWith('source');
  });

  it('triggers addSource when clicking sample loader', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    const sampleBtns = screen.getAllByText(/\+ /i);
    expect(sampleBtns.length).toBeGreaterThan(0);
    fireEvent.click(sampleBtns[0]);
    expect(store.addSource).toHaveBeenCalled();
  });

  it('handles task queue priority bump, demotion, and cancellation', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    // Switch to queue tab
    const queueTabBtn = screen.getByText(/任务队列 \(2\)/i);
    fireEvent.click(queueTabBtn);

    // Promote priority button
    const promoteBtn = screen.getByTitle(/提升优先级/i);
    fireEvent.click(promoteBtn);
    expect(store.bumpPriority).toHaveBeenCalledWith('task-1', 1);

    // Demote priority button
    const demoteBtn = screen.getByTitle(/降低优先级/i);
    fireEvent.click(demoteBtn);
    expect(store.bumpPriority).toHaveBeenCalledWith('task-1', -1);

    // Cancel task button
    const cancelBtns = screen.getAllByTitle(/取消任务/i);
    fireEvent.click(cancelBtns[0]);
    expect(store.cancelTask).toHaveBeenCalledWith('task-1');
  });

  it('handles multi-file drag and drop onto dropzone', async () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    const dropzone = screen.getByText(/点击或拖拽上传多张源素材/i).closest('label');
    expect(dropzone).toBeDefined();

    if (dropzone) {
      fireEvent.dragOver(dropzone);
      fireEvent.drop(dropzone, {
        dataTransfer: {
          files: [new File(['foo'], 'foo.jpg', { type: 'image/jpeg' })],
        },
      });
    }
  });
});
