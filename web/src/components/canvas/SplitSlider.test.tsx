import React from 'react';
import { describe, expect, it } from 'vitest';
import { render, screen } from '@testing-library/react';
import { SplitSlider } from './SplitSlider';

describe('SplitSlider Component', () => {
  it('renders before and after layers with clipPath', () => {
    const { container } = render(
      <SplitSlider
        originalUrl="/media/original.png"
        resultUrl="/media/result.png"
        initialSplit={60}
      />
    );

    expect(screen.getByText(/处理前 \(BEFORE\)/i)).toBeDefined();
    expect(screen.getByText(/处理后 \(AFTER\)/i)).toBeDefined();

    const originalImg = screen.getByAltText('Original');
    expect(originalImg.getAttribute('src')).toBe('/media/original.png');

    const resultImg = screen.getByAltText('Processed');
    expect(resultImg.getAttribute('src')).toBe('/media/result.png');

    const clippedLayer = container.querySelector('[style*="clip-path"]');
    expect(clippedLayer).not.toBeNull();
  });

  it('renders video elements when isVideo is true', () => {
    const { container } = render(
      <SplitSlider
        originalUrl="/media/original.mp4"
        resultUrl="/media/result.mp4"
        isVideo={true}
      />
    );

    const videos = container.querySelectorAll('video');
    expect(videos).toHaveLength(2);
  });
});
