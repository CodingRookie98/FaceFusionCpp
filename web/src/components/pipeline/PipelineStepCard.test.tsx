import React from 'react';
import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { PipelineStepCard } from './PipelineStepCard';
import type { PipelineStepConfig, ProcessorMeta } from '../../api/types';

describe('PipelineStepCard Component', () => {
  const mockStep: PipelineStepConfig = {
    id: 'step-1',
    step: 'face_enhancer',
    name: 'Face Enhancer #1',
    enabled: true,
    params: {
      model: 'codeformer',
      blend_factor: 0.85,
      face_selector_mode: 'many',
    },
  };

  const mockMeta: ProcessorMeta = {
    name: 'face_enhancer',
    params: [
      {
        name: 'model',
        type: 'string',
        allowed_values: ['codeformer', 'gfpgan_1.4'],
        description: 'Model name',
      },
    ],
  };

  it('renders processor step title, model selector and blend factor slider', () => {
    const onUpdate = vi.fn();
    const onRemove = vi.fn();
    const onMove = vi.fn();
    const onActivateBinding = vi.fn();

    render(
      <PipelineStepCard
        step={mockStep}
        index={0}
        totalSteps={2}
        meta={mockMeta}
        isBindingActive={false}
        onUpdate={onUpdate}
        onRemove={onRemove}
        onMove={onMove}
        onActivateBinding={onActivateBinding}
      />
    );

    expect(screen.getByDisplayValue('Face Enhancer #1')).toBeDefined();
    expect(screen.getByText(/混合强度/i)).toBeDefined();
    expect(screen.getByText('0.85')).toBeDefined();
  });

  it('handles parameter updates when changing slider and selector mode', () => {
    const onUpdate = vi.fn();
    const onRemove = vi.fn();
    const onMove = vi.fn();
    const onActivateBinding = vi.fn();

    render(
      <PipelineStepCard
        step={mockStep}
        index={0}
        totalSteps={2}
        meta={mockMeta}
        isBindingActive={false}
        onUpdate={onUpdate}
        onRemove={onRemove}
        onMove={onMove}
        onActivateBinding={onActivateBinding}
      />
    );

    // Switch to reference mode
    const refModeBtn = screen.getByText('参考人脸');
    fireEvent.click(refModeBtn);
    expect(onUpdate).toHaveBeenCalledWith('step-1', {
      params: expect.objectContaining({ face_selector_mode: 'reference' }),
    });

    // Change blend factor slider
    const slider = screen.getByRole('slider');
    fireEvent.change(slider, { target: { value: '0.5' } });
    expect(onUpdate).toHaveBeenCalledWith('step-1', {
      params: expect.objectContaining({ blend_factor: 0.5 }),
    });
  });

  it('triggers delete and move callbacks', () => {
    const onUpdate = vi.fn();
    const onRemove = vi.fn();
    const onMove = vi.fn();
    const onActivateBinding = vi.fn();

    render(
      <PipelineStepCard
        step={mockStep}
        index={0}
        totalSteps={2}
        meta={mockMeta}
        isBindingActive={false}
        onUpdate={onUpdate}
        onRemove={onRemove}
        onMove={onMove}
        onActivateBinding={onActivateBinding}
      />
    );

    const deleteBtn = screen.getByTitle('删除处理器');
    fireEvent.click(deleteBtn);
    expect(onRemove).toHaveBeenCalledWith('step-1');

    const downBtn = screen.getByTitle('下移步骤');
    fireEvent.click(downBtn);
    expect(onMove).toHaveBeenCalledWith(0, 'down');
  });
});
