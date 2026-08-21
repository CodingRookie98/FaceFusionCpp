import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { PipelineEditor } from './PipelineEditor';
import type { StudioStore } from '../../store/studioState';

describe('PipelineEditor Component', () => {
  const createMockStore = (overrides?: Partial<StudioStore>): StudioStore => ({
    sources: [],
    targets: [],
    selectedSourceId: '',
    selectedTargetId: '',
    setSelectedSourceId: vi.fn(),
    setSelectedTargetId: vi.fn(),
    addSource: vi.fn(),
    addSources: vi.fn(),
    removeSource: vi.fn(),
    addTarget: vi.fn(),
    addTargets: vi.fn(),
    removeTarget: vi.fn(),
    steps: [
      {
        id: 's1',
        step: 'face_swapper',
        name: 'Face Swapper #1',
        enabled: true,
        params: { model: 'inswapper_128', face_selector_mode: 'many' },
      },
    ],
    availableProcessors: [
      { name: 'face_swapper', params: [] },
      { name: 'face_enhancer', params: [] },
    ],
    applyPreset: vi.fn(),
    addStep: vi.fn(),
    updateStep: vi.fn(),
    removeStep: vi.fn(),
    moveStep: vi.fn(),
    activeBindingStepId: null,
    setActiveBindingStepId: vi.fn(),
    detectedFaces: [],
    selectedFaceIndices: [],
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
    activePreviewTarget: 'target',
    setActivePreviewTarget: vi.fn(),
    activeTab: 'sources',
    setActiveTab: vi.fn(),
    selectSourceForPreview: vi.fn(),
    selectTargetForPreview: vi.fn(),
    selectTaskForPreview: vi.fn(),
    ...overrides,
  });

  it('renders pipeline steps and header counts', () => {
    const store = createMockStore();
    render(<PipelineEditor store={store} />);

    expect(screen.getByText(/1\/1 激活/i)).toBeDefined();
    expect(screen.getByDisplayValue('Face Swapper #1')).toBeDefined();
    expect(screen.getByText(/➕ 添加到任务队列/i)).toBeDefined();
  });

  it('opens add step menu and triggers addStep', () => {
    const store = createMockStore();
    render(<PipelineEditor store={store} />);

    const addBtn = screen.getByText(/添加步骤/i);
    fireEvent.click(addBtn);

    const swapperOption = screen.getByText(/Face Enhancer \(人脸高清修复\)/i);
    fireEvent.click(swapperOption);
    expect(store.addStep).toHaveBeenCalledWith('face_enhancer');
  });

  it('triggers submitJob when clicking Add to Queue CTA', () => {
    const store = createMockStore();
    render(<PipelineEditor store={store} />);

    const submitBtn = screen.getByText(/➕ 添加到任务队列/i);
    fireEvent.click(submitBtn);
    expect(store.submitJob).toHaveBeenCalled();
  });
});
