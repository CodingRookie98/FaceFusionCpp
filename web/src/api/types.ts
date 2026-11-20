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
  error_message?: string;
  error?: string;
  created_at?: string;
  media_count?: number;
  priority: number;
  queue_position?: number;
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

export interface PipelineStepConfig {
  id: string;
  step: string;
  name?: string;
  enabled: boolean;
  params: Record<string, string | number | boolean | number[] | undefined>;
}

export interface CreateTaskRequest {
  source_paths: string[];
  target_paths: string[];
  output_path?: string;
  pipeline_steps?: Array<{
    step: string;
    name?: string;
    enabled?: boolean;
    params?: Record<string, string | number | boolean | number[] | undefined>;
  }>;
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

export interface FaceBox {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface FacePoint {
  x: number;
  y: number;
}

export interface DetectedFace {
  index: number;
  box: FaceBox;
  score: number;
  gender?: string;
  age_range?: [number, number];
  kps?: FacePoint[];
}

export interface DetectFacesResponse {
  image: string;
  faces: DetectedFace[];
}

export type FaceSelectorMode = 'many' | 'one' | 'reference';

export interface ParamMeta {
  name: string;
  type: 'string' | 'int' | 'float' | 'bool' | 'path';
  description?: string;
  allowed_values?: string[];
  range?: [number, number];
}

export interface ProcessorMeta {
  name: string;
  params: ParamMeta[];
}

export interface TaskProgressResponse {
  id: string;
  status: TaskStatus;
  progress: TaskProgress;
  error_message?: string;
}

export interface PreviewRenderRequest {
  source_paths: string[];
  target_paths?: string[];
  target_frame_base64?: string;
  target_face_indices?: number[];
  pipeline_steps?: Array<{
    step: string;
    name?: string;
    enabled?: boolean;
    params?: Record<string, string | number | boolean | number[] | undefined>;
  }>;
}

export interface PreviewRenderResponse {
  status: string;
  preview_url: string;
  output_path: string;
}
