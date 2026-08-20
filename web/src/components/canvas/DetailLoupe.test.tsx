import React from 'react';
import { describe, expect, it } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { DetailLoupe } from './DetailLoupe';

describe('DetailLoupe Component', () => {
  it('renders target image and loupe upon mouse movement', () => {
    const { container } = render(
      <DetailLoupe imageUrl="/media/sample.jpg" zoomLevel={3.0} loupeSize={180} />
    );

    const img = screen.getByAltText('Loupe Target');
    expect(img).toBeDefined();
    expect(img.getAttribute('src')).toBe('/media/sample.jpg');

    // Simulate mouse move over container
    const wrapper = container.firstChild as HTMLElement;
    // Mock getBoundingClientRect
    Object.defineProperty(img, 'getBoundingClientRect', {
      value: () => ({ left: 0, top: 0, width: 500, height: 500 }),
    });
    Object.defineProperty(wrapper, 'getBoundingClientRect', {
      value: () => ({ left: 0, top: 0, width: 500, height: 500 }),
    });

    fireEvent.mouseMove(wrapper, { clientX: 250, clientY: 250 });

    expect(screen.getByText('3x')).toBeDefined();

    // Mouse leave hides loupe
    fireEvent.mouseLeave(wrapper);
    expect(screen.queryByText('3x')).toBeNull();
  });
});
