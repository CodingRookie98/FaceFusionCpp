import { useState } from 'react';
import { api } from '../api/client';
import type { CreateTaskRequest, DetectedFace, FaceSelectorMode } from '../api/types';
import FileUploader from '../components/FileUploader';
import FaceSelector from '../components/FaceSelector';
import FrameExtractor from '../components/FrameExtractor';

interface Props {
  onSubmitted: (taskId: string) => void;
}

const PROCESSORS = ['face_swapper', 'face_enhancer', 'expression_restorer', 'frame_enhancer'];

export default function TaskCreatePage({ onSubmitted }: Props) {
  const [sourceText, setSourceText] = useState('');
  const [targetText, setTargetText] = useState('');
  const [output, setOutput] = useState('');
  const [processors, setProcessors] = useState<string[]>(['face_swapper']);
  const [faceSelectorMode, setFaceSelectorMode] = useState<FaceSelectorMode>('many');
  const [referenceFacePath, setReferenceFacePath] = useState('');
  const [selectedFaceIndex, setSelectedFaceIndex] = useState<number | null>(null);

  const [showFrameExtractor, setShowFrameExtractor] = useState(false);
  const [showFaceDetector, setShowFaceDetector] = useState(false);
  const [detectImagePath, setDetectImagePath] = useState('');

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

  const handleFaceSelected = (face: DetectedFace | null) => {
    if (face) {
      setSelectedFaceIndex(face.index);
      if (detectImagePath) {
        setReferenceFacePath(detectImagePath);
      }
    } else {
      setSelectedFaceIndex(null);
    }
  };

  const handleFrameExtracted = (path: string, targetType: 'source' | 'target' | 'reference') => {
    if (targetType === 'source') {
      setSourceText((prev) => [prev, path].filter(Boolean).join('\n'));
    } else if (targetType === 'target') {
      setTargetText((prev) => [prev, path].filter(Boolean).join('\n'));
    } else if (targetType === 'reference') {
      setReferenceFacePath(path);
      setFaceSelectorMode('reference');
    }
  };

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
    if (faceSelectorMode === 'reference' && !referenceFacePath.trim()) {
      setError('使用参考人脸模式时，必须指定参考人脸路径');
      return;
    }

    const processor_params: Record<string, Record<string, string | number>> = {};
    if (processors.includes('face_swapper')) {
      processor_params['face_swapper'] = {
        face_selector_mode: faceSelectorMode,
        ...(faceSelectorMode === 'reference' && referenceFacePath.trim()
          ? { reference_face_path: referenceFacePath.trim() }
          : {}),
      };
    }

    const body: CreateTaskRequest = {
      source_paths,
      target_paths,
      processors,
      ...(output.trim() ? { output_path: output.trim() } : {}),
      ...(Object.keys(processor_params).length > 0 ? { processor_params } : {}),
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

  const firstSource = splitLines(sourceText)[0] || '';

  return (
    <div className="card">
      <h2>提交任务</h2>

      {/* 视频截帧工具面板 */}
      <div className="collapsible-box">
        <div
          className="collapsible-header"
          onClick={() => setShowFrameExtractor((prev) => !prev)}
        >
          <span>📹 视频帧提取工具 (FrameExtractor)</span>
          <span>{showFrameExtractor ? '收起 ▲' : '展开 ▼'}</span>
        </div>
        {showFrameExtractor && (
          <div className="collapsible-body">
            <p style={{ fontSize: '0.8rem', color: '#666', marginTop: 0 }}>
              选择本地视频，定位播放时间并截取当前画面作为单帧素材。
            </p>
            <FrameExtractor
              onFrameExtracted={(path) => {
                handleFrameExtracted(path, 'source');
              }}
            />
          </div>
        )}
      </div>

      {/* 源素材输入 */}
      <div className="form-row">
        <label>源人脸图片路径（每行一个，或逗号分隔）</label>
        <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
          <FileUploader
            accept="image/*"
            onUploaded={(paths) =>
              setSourceText((prev) => [prev, ...paths].filter(Boolean).join('\n'))
            }
          />
          {firstSource && (
            <button
              type="button"
              className="btn-small"
              onClick={() => {
                setDetectImagePath(firstSource);
                setShowFaceDetector(true);
              }}
            >
              🔍 检测源图人脸
            </button>
          )}
        </div>
        <textarea
          rows={3}
          value={sourceText}
          onChange={(e) => setSourceText(e.target.value)}
          placeholder="assets/standard_face_test_images/lenna.bmp"
        />
      </div>

      {/* 人脸检测标注与点选面板 */}
      {showFaceDetector && (
        <div className="collapsible-box">
          <div
            className="collapsible-header"
            onClick={() => setShowFaceDetector((prev) => !prev)}
          >
            <span>👤 人脸检测与点选标注 (FaceSelector)</span>
            <span>收起 ▲</span>
          </div>
          <div className="collapsible-body">
            <div style={{ display: 'flex', gap: '0.5rem', marginBottom: '0.5rem' }}>
              <input
                type="text"
                placeholder="输入待检测图片路径"
                value={detectImagePath}
                onChange={(e) => setDetectImagePath(e.target.value)}
                style={{ flex: 1 }}
              />
            </div>
            <FaceSelector
              imagePath={detectImagePath}
              selectedFaceIndex={selectedFaceIndex}
              onSelectFace={handleFaceSelected}
            />
          </div>
        </div>
      )}

      {/* 目标素材输入 */}
      <div className="form-row">
        <label>目标图片/视频路径（每行一个）</label>
        <FileUploader
          accept="image/*,video/*"
          onUploaded={(paths) =>
            setTargetText((prev) => [prev, ...paths].filter(Boolean).join('\n'))
          }
        />
        <textarea
          rows={3}
          value={targetText}
          onChange={(e) => setTargetText(e.target.value)}
          placeholder="assets/standard_face_test_images/girl.bmp"
        />
      </div>

      {/* 人脸选择策略配置 */}
      <div className="form-row">
        <label>人脸选择策略 (Face Selector Mode)</label>
        <div style={{ display: 'flex', gap: '1rem', flexWrap: 'wrap', marginTop: '0.2rem' }}>
          <label style={{ display: 'flex', alignItems: 'center', gap: '0.3rem', fontSize: '0.9rem' }}>
            <input
              type="radio"
              name="faceSelectorMode"
              value="many"
              checked={faceSelectorMode === 'many'}
              onChange={() => setFaceSelectorMode('many')}
            />
            全部人脸 (Many)
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: '0.3rem', fontSize: '0.9rem' }}>
            <input
              type="radio"
              name="faceSelectorMode"
              value="one"
              checked={faceSelectorMode === 'one'}
              onChange={() => setFaceSelectorMode('one')}
            />
            单个最高分人脸 (One)
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: '0.3rem', fontSize: '0.9rem' }}>
            <input
              type="radio"
              name="faceSelectorMode"
              value="reference"
              checked={faceSelectorMode === 'reference'}
              onChange={() => setFaceSelectorMode('reference')}
            />
            参考人脸比对 (Reference)
          </label>
        </div>
      </div>

      {/* 参考人脸路径配置（仅在 reference 模式显示） */}
      {faceSelectorMode === 'reference' && (
        <div className="form-row" style={{ background: '#f8fafc', padding: '0.8rem', borderRadius: '8px', border: '1px solid #e2e8f0' }}>
          <label>参考人脸图片路径 (Reference Face Path)</label>
          <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
            <FileUploader
              accept="image/*"
              onUploaded={(paths) => {
                if (paths[0]) setReferenceFacePath(paths[0]);
              }}
            />
            <input
              type="text"
              placeholder="assets/standard_face_test_images/lenna.bmp"
              value={referenceFacePath}
              onChange={(e) => setReferenceFacePath(e.target.value)}
              style={{ flex: 1 }}
            />
          </div>
          <span style={{ fontSize: '0.75rem', color: '#64748b' }}>
            目标素材中将只替换与该参考人脸相似度最高的面部。
          </span>
        </div>
      )}

      {/* 输出目录 */}
      <div className="form-row">
        <label>输出目录（可选）</label>
        <input
          value={output}
          onChange={(e) => setOutput(e.target.value)}
          placeholder="output"
        />
      </div>

      {/* 处理器选择 */}
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
