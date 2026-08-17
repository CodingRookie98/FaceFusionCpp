import { useEffect, useState } from 'react';
import { api } from '../api/client';
import type { TaskSummary } from '../api/types';
import ProgressBar from '../components/ProgressBar';

interface Props {
  onSelect: (taskId: string) => void;
}

const STATUS_CLASS: Record<string, string> = {
  done: 'status-done',
  running: 'status-running',
  queued: 'status-queued',
  failed: 'status-failed',
  cancelled: 'status-cancelled',
};

export default function TaskListPage({ onSelect }: Props) {
  const [tasks, setTasks] = useState<TaskSummary[]>([]);
  const [error, setError] = useState<string | null>(null);

  const refresh = () => {
    api
      .listTasks()
      .then(setTasks)
      .catch((e) => setError(e instanceof Error ? e.message : String(e)));
  };

  useEffect(() => {
    refresh();
    const timer = setInterval(refresh, 3000);
    return () => clearInterval(timer);
  }, []);

  const cancel = async (id: string) => {
    try {
      await api.cancelTask(id);
      refresh();
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    }
  };

  const percent = (t: TaskSummary) =>
    t.progress.total_frames > 0
      ? (t.progress.current_frame / t.progress.total_frames) * 100
      : 0;

  return (
    <div className="card">
      <h2>
        任务列表{' '}
        <button className="btn" style={{ float: 'right' }} onClick={refresh}>
          刷新
        </button>
      </h2>
      {error && <p className="error">{error}</p>}
      <table className="tasks">
        <thead>
          <tr>
            <th>ID</th>
            <th>状态</th>
            <th>进度</th>
            <th>素材数</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          {tasks.map((t) => (
            <tr key={t.id}>
              <td>
                <a href="#" onClick={(e) => { e.preventDefault(); onSelect(t.id); }}>
                  {t.id.slice(0, 16)}…
                </a>
              </td>
              <td>
                <span className={'status-badge ' + (STATUS_CLASS[t.status] ?? '')}>
                  {t.status}
                </span>
              </td>
              <td>
                {t.status === 'running' || t.status === 'queued' ? (
                  <ProgressBar percent={percent(t)} />
                ) : (
                  <span style={{ fontSize: '0.8rem', color: '#777' }}>
                    {t.status === 'done' ? '已完成' : t.error_message || '—'}
                  </span>
                )}
              </td>
              <td>{t.media_count}</td>
              <td>
                {(t.status === 'queued' || t.status === 'running') && (
                  <button className="btn btn-danger" onClick={() => cancel(t.id)}>
                    取消
                  </button>
                )}
              </td>
            </tr>
          ))}
          {tasks.length === 0 && (
            <tr>
              <td colSpan={5} style={{ color: '#888' }}>
                暂无任务
              </td>
            </tr>
          )}
        </tbody>
      </table>
    </div>
  );
}
