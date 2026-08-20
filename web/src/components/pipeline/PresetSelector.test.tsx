import React from 'react';
import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { PresetSelector } from './PresetSelector';

describe('PresetSelector Component', () => {
  it('renders official presets and handles selection', () => {
    const onSelect = vi.fn();
    render(<PresetSelector onSelectPreset={onSelect} />);

    expect(screen.getByText('极速单人换脸')).toBeDefined();
    expect(screen.getByText('高清写真重塑')).toBeDefined();
    expect(screen.getByText('多人精准多脸替换')).toBeDefined();
    expect(screen.getByText('影视级全流程超分')).toBeDefined();

    const fastSwapBtn = screen.getByText('极速单人换脸');
    fireEvent.click(fastSwapBtn);
    expect(onSelect).toHaveBeenCalledWith(
      expect.objectContaining({ id: 'fast_swap' })
    );
  });
});
