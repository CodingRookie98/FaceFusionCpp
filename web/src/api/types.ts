// API types aligned with the C++ web server contract
export interface TaskProgress {
  current_frame: number;
  total_frames: number;
  fps: number;
}

export type TaskStatus = 'queued' | 'running' | 'done' | 'failed' | 'cancelled';

export interface TaskSummary {
  id: string;
  status: TaskStatus;
  progress: TaskProgress;
  error_message: string;
  media_count: number;
}

export interface TaskResultFile {
  name: string;
  url: string;
}

export interface TaskDetail extends TaskSummary {
  output_path: string;
  media: {
    source: string[];
    target: string[];
  };
  results: TaskResultFile[];
}

export interface CreateTaskRequest {
  source_paths: string[];
  target_paths: string[];
  output_path?: string;
  processors?: string[];
  processor_params?: Record<string, Record<string, string | number>>;
}

export interface WsProgressMessage {
  type: 'progress';
  frame: number;
  total: number;
  fps: number;
}

export interface WsStatusMessage {
  type: 'status';
  status: TaskStatus;
  message?: string;
}

export type WsMessage = WsProgressMessage | WsStatusMessage;
