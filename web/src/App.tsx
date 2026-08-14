import { useEffect, useState } from 'react';

interface HealthResponse {
  status: string;
  version: string;
}

export default function App() {
  const [health, setHealth] = useState<HealthResponse | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    fetch('/api/health')
      .then((res) => res.json())
      .then((data: HealthResponse) => setHealth(data))
      .catch((err: Error) => setError(err.message));
  }, []);

  return (
    <div className="app">
      <header className="app-header">
        <h1>ffc Web UI</h1>
        <p className="subtitle">FaceFusionCpp Web Interface (M1 skeleton)</p>
      </header>
      <main className="app-main">
        <section className="status-card">
          <h2>后端状态</h2>
          {error && <p className="error">连接失败: {error}</p>}
          {health ? (
            <p>
              状态: <strong>{health.status}</strong> ｜ 版本:{' '}
              <strong>{health.version}</strong>
            </p>
          ) : (
            !error && <p>正在连接后端（请先运行 ffc --web）...</p>
          )}
        </section>
      </main>
    </div>
  );
}
