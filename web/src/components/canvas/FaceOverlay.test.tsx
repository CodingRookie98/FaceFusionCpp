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

  it('renders bounding boxes with correct relative percentages and info', () => {
    const onToggle = vi.fn();
    const { container } = render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndices={[0]}
        onToggleFace={onToggle}
      />
    );

    const faceBoxes = container.querySelectorAll('.border-2');
    expect(faceBoxes).toHaveLength(2);

    expect(screen.getByText(/#1/)).toBeDefined();
    expect(screen.getByText(/95%/i)).toBeDefined();
    expect(screen.getByText(/♀/i)).toBeDefined();

    expect(screen.getByText(/#2/)).toBeDefined();
    expect(screen.getByText(/88%/i)).toBeDefined();
    expect(screen.getByText(/♂/i)).toBeDefined();
  });

  it('displays selected and unselected status labels on face boxes', () => {
    const onToggle = vi.fn();
    render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndices={[0]}
        onToggleFace={onToggle}
      />
    );

    expect(screen.getByText(/已选 #1/i)).toBeDefined();
    expect(screen.getByText(/未选 #2/i)).toBeDefined();
  });

  it('triggers onToggleFace callback when clicking a face box to toggle selection', () => {
    const onToggle = vi.fn();
    render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndices={[0]}
        onToggleFace={onToggle}
      />
    );

    const face1 = screen.getByText(/已选 #1/i);
    fireEvent.click(face1);
    expect(onToggle).toHaveBeenCalledWith(0);

    const face2 = screen.getByText(/未选 #2/i);
    fireEvent.click(face2);
    expect(onToggle).toHaveBeenCalledWith(1);
  });

  it('supports selecting multiple faces simultaneously with rose highlight color scheme', () => {
    const onToggle = vi.fn();
    const { container } = render(
      <FaceOverlay
        faces={mockFaces}
        imageWidth={1000}
        imageHeight={1000}
        selectedFaceIndices={[0, 1]}
        onToggleFace={onToggle}
      />
    );

    const activeBoxes = container.querySelectorAll('.border-rose-500');
    expect(activeBoxes).toHaveLength(2);
    expect(screen.getByText(/已选 #1/i)).toBeDefined();
    expect(screen.getByText(/已选 #2/i)).toBeDefined();
  });
});
