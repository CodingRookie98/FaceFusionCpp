import type {
  CreateTaskRequest,
  DetectFacesResponse,
  ProcessorMeta,
  TaskDetail,
  TaskProgressResponse,
  TaskSummary,
  WsMessage,
} from './types';

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
  listProcessors: () => request<ProcessorMeta[]>('/api/processors'),
  listTasks: () => request<TaskSummary[]>('/api/tasks'),
  getTask: (id: string) => request<TaskDetail>(`/api/tasks/${id}`),
  getProgress: (id: string) =>
    request<TaskProgressResponse>(`/api/tasks/${id}/progress`),
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
  detectFaces: (imagePath: string) =>
    request<DetectFacesResponse>('/api/faces', {
      method: 'POST',
      body: JSON.stringify({ image_path: imagePath }),
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

/**
 * Compute WebSocket reconnect delay with exponential backoff.
 * @param attempt zero-based reconnect attempt count (resets to 0 on success)
 * @param baseMs base delay for the first attempt
 * @param maxMs upper bound for the delay
 * @returns delay in milliseconds (min(baseMs * 2^attempt, maxMs))
 */
export function computeReconnectDelay(
  attempt: number,
  baseMs = 1000,
  maxMs = 30000,
): number {
  const delay = baseMs * 2 ** attempt;
  return Math.min(delay, maxMs);
}

/**
 * Subscribe to task progress via WebSocket; returns an unsubscribe fn.
 *
 * Auto-reconnects on close/error with exponential backoff (1s -> 30s cap).
 * After a successful connection the backoff resets, and the server's
 * connection handler replays current status + progress on (re)connect, so
 * no separate state refetch is needed. Calling the returned fn stops
 * reconnection permanently.
 */
export function subscribeProgress(
  taskId: string,
  onMessage: (msg: WsMessage) => void,
): () => void {
  const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
  const host = window.location.hostname;
  const port = window.location.port || (protocol === 'wss' ? '443' : '80');
  // Dev mode proxies /ws to the C++ server (see vite.config.ts)
  let closed = false;
  let attempt = 0;
  let timer: ReturnType<typeof setTimeout> | undefined;
  let ws: WebSocket | null = null;

  const connect = () => {
    ws = new WebSocket(`${protocol}://${host}:${port}/ws/tasks/${taskId}/progress`);
    ws.onopen = () => {
      // connection established: reset backoff
      attempt = 0;
    };
    ws.onmessage = (ev) => {
      try {
        onMessage(JSON.parse(ev.data as string) as WsMessage);
      } catch {
        /* ignore malformed frames */
      }
    };
    ws.onerror = () => {
      // close() fires onclose, which is the single reconnect trigger
      ws?.close();
    };
    ws.onclose = () => {
      if (closed) return;
      const delay = computeReconnectDelay(attempt++);
      timer = setTimeout(connect, delay);
    };
  };

  connect();
  return () => {
    closed = true;
    if (timer !== undefined) clearTimeout(timer);
    ws?.close();
  };
}
