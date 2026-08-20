import React from 'react';
import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { HistoryModal } from './HistoryModal';
import type { TaskSummary } from '../../api/types';

describe('HistoryModal Component', () => {
  const mockTasks: TaskSummary[] = [
    {
      id: 'task-done-1234567890',
      status: 'done',
      priority: 2,
      queue_position: 0,
      media_count: 2,
      progress: { current_frame: 100, total_frames: 100, fps: 30 },
    },
    {
      id: 'task-failed-0987654321',
      status: 'failed',
      priority: 0,
      queue_position: 0,
      media_count: 1,
      progress: { current_frame: 10, total_frames: 100, fps: 0 },
      error_message: 'Model missing',
    },
  ];

  it('renders task list with status badges and priorities', () => {
    const onClose = vi.fn();
    const onSelect = vi.fn();

    render(
      <HistoryModal
        tasks={mockTasks}
        activeTaskId="task-done-1234567890"
        isOpen={true}
        onClose={onClose}
        onSelectTask={onSelect}
      />
    );

    expect(screen.getByText(/历史任务与成果记录/i)).toBeDefined();
    expect(screen.getByText(/已完成/i)).toBeDefined();
    expect(screen.getByText(/失败/i)).toBeDefined();
    expect(screen.getByText('P2')).toBeDefined();
    expect(screen.getByText(/Model missing/i)).toBeDefined();
  });

  it('handles selecting task and closing modal', () => {
    const onClose = vi.fn();
    const onSelect = vi.fn();

    render(
      <HistoryModal
        tasks={mockTasks}
        activeTaskId="task-done-1234567890"
        isOpen={true}
        onClose={onClose}
        onSelectTask={onSelect}
      />
    );

    const loadBtns = screen.getAllByText(/在工作台载入/i);
    fireEvent.click(loadBtns[0]);
    expect(onSelect).toHaveBeenCalledWith('task-done-1234567890');
    expect(onClose).toHaveBeenCalled();
  });
});
