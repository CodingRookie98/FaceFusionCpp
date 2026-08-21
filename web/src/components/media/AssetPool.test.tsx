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
    tasks: [],
    activeTaskId: null,
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
  });

  it('renders sources and targets with tab switching', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    // Initial tab is sources
    expect(screen.getByText(/源人脸 \(2\)/i)).toBeDefined();
    expect(screen.getByText('Lenna')).toBeDefined();
    expect(screen.getByText('Avatar')).toBeDefined();

    // Switch to targets tab
    const targetTabBtn = screen.getByText(/目标素材 \(1\)/i);
    fireEvent.click(targetTabBtn);

    expect(screen.getByText('Girl')).toBeDefined();
  });

  it('handles item selection on click', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    const avatarItem = screen.getByText('Avatar');
    fireEvent.click(avatarItem);
    expect(store.setSelectedSourceId).toHaveBeenCalledWith('s2');
  });

  it('triggers addSource when clicking sample loader', () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    const sampleBtns = screen.getAllByText(/\+ /i);
    expect(sampleBtns.length).toBeGreaterThan(0);
    fireEvent.click(sampleBtns[0]);
    expect(store.addSource).toHaveBeenCalled();
  });

  it('handles multi-file drag and drop onto dropzone', async () => {
    const store = createMockStore();
    render(<AssetPool store={store} />);

    const dropzone = screen.getByText(/点击或拖拽上传多张源人脸/i).closest('label');
    expect(dropzone).toBeDefined();

    if (dropzone) {
      fireEvent.dragOver(dropzone);
      expect(screen.getByText(/松开以批量导入素材/i)).toBeDefined();

      fireEvent.dragLeave(dropzone);
      expect(screen.getByText(/点击或拖拽上传多张源人脸/i)).toBeDefined();
    }
  });
});
