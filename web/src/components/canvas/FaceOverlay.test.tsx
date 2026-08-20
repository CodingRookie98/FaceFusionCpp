import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { FaceOverlay } from './FaceOverlay';
import type { DetectedFace } from '../../api/types';

describe('FaceOverlay Component', () => {
  const mockFaces: DetectedFace[] = [
    {
      index: 0,
      box: { x: 100, y: 100, width: 200, height: 200 },
      score: 0.95,
      gender: 'female',
      age_range: [20, 30],
    },
    {
      index: 1,
      box: { x: 400, y: 150, width: 200, height: 250 },
      score: 0.88,
      gender: 'male',
      age_range: [30, 40],
    },
  ];

  it('renders bounding boxes with correct relative percentages', () => {
    const onSelect = vi.fn();
    const { container } = render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndex={null}
        onSelectFace={onSelect}
      />
    );

    const faceBoxes = container.querySelectorAll('.border-2');
    expect(faceBoxes).toHaveLength(2);

    expect(screen.getByText('#1')).toBeDefined();
    expect(screen.getByText(/95%/i)).toBeDefined();
    expect(screen.getByText(/♀/i)).toBeDefined();

    expect(screen.getByText('#2')).toBeDefined();
    expect(screen.getByText(/88%/i)).toBeDefined();
    expect(screen.getByText(/♂/i)).toBeDefined();
  });

  it('triggers onSelectFace callback when clicked', () => {
    const onSelect = vi.fn();
    render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndex={null}
        onSelectFace={onSelect}
      />
    );

    const faceTag = screen.getByText('#1');
    fireEvent.click(faceTag);
    expect(onSelect).toHaveBeenCalledWith(mockFaces[0]);
  });

  it('highlights selected face with rose color scheme', () => {
    const onSelect = vi.fn();
    const { container } = render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndex={0}
        onSelectFace={onSelect}
      />
    );

    const activeBox = container.querySelector('.border-rose-500');
    expect(activeBox).not.toBeNull();
  });
});
