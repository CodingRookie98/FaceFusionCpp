import type { CreateTaskRequest, TaskDetail, TaskSummary, WsMessage } from './types';

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await fetch(path, {
    headers: init?.body ? { 'Content-Type': 'application/json' } : undefined,
    ...init,
  });
  if (!res.ok) {
    const body = await res.json().catch(() => ({}));
    throw new Error((body as { error?: string }).error ?? `HTTP ${res.status}`);
  }
  return res.json() as Promise<T>;
}

export const api = {
  health: () => request<{ status: string; version: string }>('/api/health'),
  listTasks: () => request<TaskSummary[]>('/api/tasks'),
  getTask: (id: string) => request<TaskDetail>(`/api/tasks/${id}`),
  submitTask: (body: CreateTaskRequest) =>
    request<{ id: string; status: string }>('/api/tasks', {
      method: 'POST',
      body: JSON.stringify(body),
    }),
  cancelTask: (id: string) =>
    request<{ ok: boolean }>(`/api/tasks/${id}/cancel`, { method: 'POST' }),
  getResult: (id: string) =>
    request<{ files: { name: string; url: string }[] }>(`/api/tasks/${id}/result`),
  setPriority: (id: string, priority: number) =>
    request<{ ok: boolean; priority: number }>(`/api/tasks/${id}/priority`, {
      method: 'POST',
      body: JSON.stringify({ priority }),
    }),
  uploadFile: async (file: File): Promise<{ path: string; name: string; size: number }> => {
    const res = await fetch('/api/upload', {
      method: 'POST',
      headers: { 'X-File-Name': file.name },
      body: file,
    });
    if (!res.ok) {
      const body = await res.json().catch(() => ({}));
      throw new Error((body as { error?: string }).error ?? `HTTP ${res.status}`);
    }
    return res.json() as Promise<{ path: string; name: string; size: number }>;
  },
};

/** Subscribe to task progress via WebSocket; returns an unsubscribe fn. */
export function subscribeProgress(
  taskId: string,
  onMessage: (msg: WsMessage) => void,
): () => void {
  const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
  const host = window.location.hostname;
  const port = window.location.port || (protocol === 'wss' ? '443' : '80');
  // Dev mode proxies /ws to the C++ server (see vite.config.ts)
  const ws = new WebSocket(`${protocol}://${host}:${port}/ws/tasks/${taskId}/progress`);
  ws.onmessage = (ev) => {
    try {
      onMessage(JSON.parse(ev.data as string) as WsMessage);
    } catch {
      /* ignore malformed frames */
    }
  };
  return () => ws.close();
}
