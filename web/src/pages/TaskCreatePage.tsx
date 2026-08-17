import { useState } from 'react';
import { api } from '../api/client';
import type { CreateTaskRequest } from '../api/types';
import FileUploader from '../components/FileUploader';

interface Props {
  onSubmitted: (taskId: string) => void;
}

const PROCESSORS = ['face_swapper', 'face_enhancer', 'expression_restorer', 'frame_enhancer'];

export default function TaskCreatePage({ onSubmitted }: Props) {
  const [sourceText, setSourceText] = useState('');
  const [targetText, setTargetText] = useState('');
  const [output, setOutput] = useState('');
  const [processors, setProcessors] = useState<string[]>(['face_swapper']);
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  const toggleProcessor = (p: string) => {
    setProcessors((prev) =>
      prev.includes(p) ? prev.filter((x) => x !== p) : [...prev, p],
    );
  };

  const splitLines = (text: string) =>
    text
      .split(/[,\n]/)
      .map((s) => s.trim())
      .filter(Boolean);

  const submit = async () => {
    setError(null);
    const source_paths = splitLines(sourceText);
    const target_paths = splitLines(targetText);
    if (source_paths.length === 0 || target_paths.length === 0) {
      setError('源图片与目标素材至少各填写一个路径');
      return;
    }
    if (processors.length === 0) {
      setError('请至少选择一个处理器');
      return;
    }
    const body: CreateTaskRequest = {
      source_paths,
      target_paths,
      processors,
      ...(output.trim() ? { output_path: output.trim() } : {}),
    };
    setBusy(true);
    try {
      const created = await api.submitTask(body);
      onSubmitted(created.id);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="card">
      <h2>提交任务</h2>
      <div className="form-row">
        <label>源人脸图片路径（每行一个，或逗号分隔）</label>
        <FileUploader accept="image/*" onUploaded={(paths) => setSourceText((prev) => [prev, ...paths].filter(Boolean).join('\n'))} />
        <textarea
          rows={3}
          value={sourceText}
          onChange={(e) => setSourceText(e.target.value)}
          placeholder="assets/standard_face_test_images/lenna.bmp"
        />
      </div>
      <div className="form-row">
        <label>目标图片/视频路径（每行一个）</label>
        <FileUploader accept="image/*,video/*" onUploaded={(paths) => setTargetText((prev) => [prev, ...paths].filter(Boolean).join('\n'))} />
        <textarea
          rows={3}
          value={targetText}
          onChange={(e) => setTargetText(e.target.value)}
          placeholder="assets/standard_face_test_images/girl.bmp"
        />
      </div>
      <div className="form-row">
        <label>输出目录（可选）</label>
        <input
          value={output}
          onChange={(e) => setOutput(e.target.value)}
          placeholder="output"
        />
      </div>
      <div className="form-row">
        <label>处理器</label>
        <div style={{ display: 'flex', gap: '0.8rem', flexWrap: 'wrap' }}>
          {PROCESSORS.map((p) => (
            <label key={p} style={{ display: 'flex', gap: '0.3rem', alignItems: 'center' }}>
              <input
                type="checkbox"
                checked={processors.includes(p)}
                onChange={() => toggleProcessor(p)}
              />
              {p}
            </label>
          ))}
        </div>
      </div>
      {error && <p className="error">{error}</p>}
      <button className="btn btn-primary" onClick={submit} disabled={busy}>
        {busy ? '提交中...' : '提交任务'}
      </button>
    </div>
  );
}
