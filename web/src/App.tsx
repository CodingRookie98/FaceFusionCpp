import { useEffect, useState } from 'react';
import { api } from './api/client';
import TaskCreatePage from './pages/TaskCreatePage';
import TaskListPage from './pages/TaskListPage';
import TaskDetailPage from './pages/TaskDetailPage';

type View =
  | { name: 'create' }
  | { name: 'list' }
  | { name: 'detail'; taskId: string };

export default function App() {
  const [view, setView] = useState<View>({ name: 'list' });
  const [backend, setBackend] = useState<string | null>(null);

  useEffect(() => {
    api
      .health()
      .then((h) => setBackend(`${h.status} v${h.version}`))
      .catch(() => setBackend('offline'));
  }, []);

  return (
    <div className="app">
      <header className="app-header">
        <h1>ffc Web UI</h1>
        <p className="subtitle">
          FaceFusionCpp · 后端状态: <strong>{backend ?? '检测中...'}</strong>
        </p>
        <nav className="nav">
          <button
            className={view.name === 'list' ? 'active' : ''}
            onClick={() => setView({ name: 'list' })}
          >
            任务列表
          </button>
          <button
            className={view.name === 'create' ? 'active' : ''}
            onClick={() => setView({ name: 'create' })}
          >
            提交任务
          </button>
        </nav>
      </header>

      <main>
        {view.name === 'create' && (
          <TaskCreatePage
            onSubmitted={(taskId) => setView({ name: 'detail', taskId })}
          />
        )}
        {view.name === 'list' && (
          <TaskListPage onSelect={(taskId) => setView({ name: 'detail', taskId })} />
        )}
        {view.name === 'detail' && (
          <TaskDetailPage
            taskId={view.taskId}
            onBack={() => setView({ name: 'list' })}
          />
        )}
      </main>
    </div>
  );
}
