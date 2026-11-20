import { useEffect, useState } from 'react';
import { api, subscribeProgress } from '../api/client';
import type { TaskDetail, WsMessage } from '../api/types';
import ProgressBar from '../components/ProgressBar';

interface Props {
  taskId: string;
  onBack: () => void;
}

export default function TaskDetailPage({ taskId, onBack }: Props) {
  const [detail, setDetail] = useState<TaskDetail | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let unsub: (() => void) | undefined;
    api
      .getTask(taskId)
      .then((d) => {
        setDetail(d);
        if (d.status === 'queued' || d.status === 'running') {
          unsub = subscribeProgress(taskId, (msg: WsMessage) => {
            setDetail((prev) => {
              if (!prev) return prev;
              if (msg.type === 'progress') {
                return {
                  ...prev,
                  progress: {
                    current_frame: msg.frame,
                    total_frames: msg.total,
                    fps: msg.fps,
                  },
                };
              }
              return { ...prev, status: msg.status };
            });
          });
        }
      })
      .catch((e) => setError(e instanceof Error ? e.message : String(e)));
    return () => unsub?.();
  }, [taskId]);

  const cancel = async () => {
    try {
      await api.cancelTask(taskId);
      const d = await api.getTask(taskId);
      setDetail(d);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    }
  };

  if (error) return <div className="card"><p className="error">{error}</p></div>;
  if (!detail) return <div className="card">加载中...</div>;

  const percent =
    detail.progress.total_frames > 0
      ? (detail.progress.current_frame / detail.progress.total_frames) * 100
      : 0;

  const running = detail.status === 'queued' || detail.status === 'running';
  const firstTarget = detail.media.target[0];
  const firstResult = detail.results[0];

  return (
    <div>
      <div className="card">
        <h2>
          任务 {detail.id.slice(0, 16)}…{' '}
          <button className="btn" style={{ float: 'right' }} onClick={onBack}>
            返回
          </button>
        </h2>
        <p>
          状态: <strong>{detail.status}</strong>
          {detail.error_message && (
            <span className="error"> — {detail.error_message}</span>
          )}
        </p>
        {running && (
          <>
            <ProgressBar
              percent={percent}
              postfix={
                detail.progress.total_frames > 0
                  ? `帧 ${detail.progress.current_frame}/${detail.progress.total_frames} · ${detail.progress.fps.toFixed(1)} FPS`
                  : undefined
              }
            />
            <button className="btn btn-danger" onClick={cancel}>
              取消任务
            </button>
          </>
        )}
      </div>

      {detail.status === 'done' && firstResult && firstTarget && (
        <div className="card">
          <h3>处理前后对比</h3>
          <div className="compare-grid">
            <figure>
              {/\.(mp4|mov|avi|mkv|webm)$/i.test(firstTarget) ? (
                <video controls src={firstTarget} style={{ maxWidth: '100%' }} />
              ) : (
                <img src={firstTarget} alt="处理前" />
              )}
              <figcaption>处理前（目标素材）</figcaption>
            </figure>
            <figure>
              {/\.(mp4|mov|avi|mkv|webm)$/i.test(firstResult.name) ? (
                <video controls src={firstResult.url} style={{ maxWidth: '100%' }} />
              ) : (
                <img src={firstResult.url} alt="处理后" />
              )}
              <figcaption>处理后（结果）</figcaption>
            </figure>
          </div>
        </div>
      )}

      {detail.status === 'done' && detail.results.length > 0 && (
        <div className="card">
          <h3>结果文件</h3>
          <ul>
            {detail.results.map((r) => (
              <li key={r.name}>
                <a href={r.url} target="_blank" rel="noreferrer">
                  {r.name}
                </a>
              </li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
}
