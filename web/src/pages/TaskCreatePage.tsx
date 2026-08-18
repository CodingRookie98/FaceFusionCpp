import { useEffect, useState } from 'react';
import { api } from '../api/client';
import type { CreateTaskRequest, DetectedFace, FaceSelectorMode, ProcessorMeta } from '../api/types';
import FileUploader from '../components/FileUploader';
import FaceSelector from '../components/FaceSelector';
import FrameExtractor from '../components/FrameExtractor';

interface Props {
  onSubmitted: (taskId: string) => void;
}

const DEFAULT_PROCESSORS = ['face_swapper', 'face_enhancer', 'expression_restorer', 'frame_enhancer'];

export default function TaskCreatePage({ onSubmitted }: Props) {
  const [sourceText, setSourceText] = useState('');
  const [targetText, setTargetText] = useState('');
  const [output, setOutput] = useState('');
  const [availableProcessors, setAvailableProcessors] = useState<ProcessorMeta[]>([]);
  const [processors, setProcessors] = useState<string[]>(['face_swapper']);
  const [processorParams, setProcessorParams] = useState<Record<string, Record<string, string | number>>>({});
  const [faceSelectorMode, setFaceSelectorMode] = useState<FaceSelectorMode>('many');
  const [referenceFacePath, setReferenceFacePath] = useState('');
  const [selectedFaceIndex, setSelectedFaceIndex] = useState<number | null>(null);

  const [showFrameExtractor, setShowFrameExtractor] = useState(false);
  const [showFaceDetector, setShowFaceDetector] = useState(false);
  const [detectImagePath, setDetectImagePath] = useState('');

  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    api
      .listProcessors()
      .then((data) => {
        if (data && data.length > 0) {
          setAvailableProcessors(data);
        }
      })
      .catch(() => {
        // Fallback gracefully to default processors if endpoint unreachable
      });
  }, []);

  const toggleProcessor = (p: string) => {
    setProcessors((prev) =>
      prev.includes(p) ? prev.filter((x) => x !== p) : [...prev, p],
    );
  };

  const setParamValue = (procName: string, paramName: string, value: string | number) => {
    setProcessorParams((prev) => ({
      ...prev,
      [procName]: {
        ...(prev[procName] || {}),
        [paramName]: value,
      },
    }));
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

    const mergedParams: Record<string, Record<string, string | number>> = {
      ...processorParams,
    };

    if (processors.includes('face_swapper')) {
      mergedParams['face_swapper'] = {
        ...(mergedParams['face_swapper'] || {}),
        face_selector_mode: faceSelectorMode,
        ...(faceSelectorMode === 'reference' && referenceFacePath.trim()
          ? { reference_face_path: referenceFacePath.trim() }
          : {}),
      };
    }

    if (processors.includes('face_enhancer')) {
      mergedParams['face_enhancer'] = {
        ...(mergedParams['face_enhancer'] || {}),
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
      ...(Object.keys(mergedParams).length > 0 ? { processor_params: mergedParams } : {}),
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
  const displayProcessors =
    availableProcessors.length > 0
      ? availableProcessors.map((p) => p.name)
      : DEFAULT_PROCESSORS;

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
        <label>源图片路径（每行一个）</label>
        <FileUploader
          accept="image/*"
          onUploaded={(paths) =>
            setSourceText((prev) => [prev, ...paths].filter(Boolean).join('\n'))
          }
        />
        <textarea
          rows={3}
          value={sourceText}
          onChange={(e) => setSourceText(e.target.value)}
          placeholder="assets/standard_face_test_images/lenna.bmp"
        />
      </div>

      {/* 人脸检测标注与点选工具面板 */}
      <div className="collapsible-box">
        <div
          className="collapsible-header"
          onClick={() => {
            if (!showFaceDetector && !detectImagePath) {
              setDetectImagePath(firstSource);
            }
            setShowFaceDetector((prev) => !prev);
          }}
        >
          <span>👤 人脸检测与点选工具 (FaceSelector)</span>
          <span>{showFaceDetector ? '收起 ▲' : '展开 ▼'}</span>
        </div>
        {showFaceDetector && (
          <div className="collapsible-body">
            <p style={{ fontSize: '0.8rem', color: '#666', marginTop: 0 }}>
              输入或选择图片进行人脸检测，在交互画布上点选作为替换源或参考人脸。
            </p>
            <div style={{ display: 'flex', gap: '0.5rem', marginBottom: '0.8rem', alignItems: 'center' }}>
              <input
                type="text"
                placeholder="检测图片路径，例如 assets/standard_face_test_images/lenna.bmp"
                value={detectImagePath}
                onChange={(e) => setDetectImagePath(e.target.value)}
                style={{ flex: 1 }}
              />
              <FileUploader
                accept="image/*"
                onUploaded={(paths) => {
                  if (paths[0]) setDetectImagePath(paths[0]);
                }}
              />
              {firstSource && detectImagePath !== firstSource && (
                <button
                  className="btn"
                  style={{ fontSize: '0.8rem', padding: '0.4rem 0.8rem' }}
                  onClick={() => setDetectImagePath(firstSource)}
                >
                  使用源图 1
                </button>
              )}
            </div>
            <FaceSelector
              imagePath={detectImagePath}
              selectedFaceIndex={selectedFaceIndex}
              onSelectFace={handleFaceSelected}
            />
          </div>
        )}
      </div>

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

      {/* 处理器选择与动态参数配置 */}
      <div className="form-row">
        <label>处理器与参数 (Processors & Params)</label>
        <div style={{ display: 'flex', gap: '0.8rem', flexWrap: 'wrap', marginBottom: '0.5rem' }}>
          {displayProcessors.map((p) => (
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

        {/* 动态渲染已选处理器的可用参数表单 */}
        {availableProcessors
          .filter((p) => processors.includes(p.name))
          .map((proc) => {
            const visibleParams = proc.params.filter(
              (pm) => pm.name !== 'face_selector_mode' && pm.name !== 'reference_face_path',
            );
            if (visibleParams.length === 0) return null;
            return (
              <div
                key={proc.name}
                style={{
                  background: '#f8fafc',
                  border: '1px solid #e2e8f0',
                  borderRadius: '6px',
                  padding: '0.6rem 0.8rem',
                  marginBottom: '0.5rem',
                }}
              >
                <strong style={{ fontSize: '0.85rem', color: '#334155' }}>
                  {proc.name} 参数
                </strong>
                <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.8rem', marginTop: '0.4rem' }}>
                  {visibleParams.map((param) => (
                    <div key={param.name} style={{ display: 'flex', flexDirection: 'column', gap: '0.2rem' }}>
                      <span style={{ fontSize: '0.75rem', color: '#64748b' }}>
                        {param.name} {param.description ? `(${param.description})` : ''}
                      </span>
                      {param.allowed_values && param.allowed_values.length > 0 ? (
                        <select
                          value={
                            (processorParams[proc.name]?.[param.name] as string) ||
                            param.allowed_values[0]
                          }
                          onChange={(e) => setParamValue(proc.name, param.name, e.target.value)}
                          style={{ padding: '0.2rem 0.4rem', fontSize: '0.8rem' }}
                        >
                          {param.allowed_values.map((v) => (
                            <option key={v} value={v}>
                              {v}
                            </option>
                          ))}
                        </select>
                      ) : param.type === 'float' || param.type === 'int' ? (
                        <input
                          type="number"
                          step={param.type === 'float' ? '0.05' : '1'}
                          min={param.range ? param.range[0] : undefined}
                          max={param.range ? param.range[1] : undefined}
                          value={processorParams[proc.name]?.[param.name] ?? ''}
                          placeholder={
                            param.range ? `[${param.range[0]}, ${param.range[1]}]` : ''
                          }
                          onChange={(e) =>
                            setParamValue(
                              proc.name,
                              param.name,
                              param.type === 'float'
                                ? parseFloat(e.target.value) || 0
                                : parseInt(e.target.value, 10) || 0,
                            )
                          }
                          style={{ width: '120px', padding: '0.2rem 0.4rem', fontSize: '0.8rem' }}
                        />
                      ) : (
                        <input
                          type="text"
                          value={(processorParams[proc.name]?.[param.name] as string) || ''}
                          onChange={(e) => setParamValue(proc.name, param.name, e.target.value)}
                          style={{ padding: '0.2rem 0.4rem', fontSize: '0.8rem' }}
                        />
                      )}
                    </div>
                  ))}
                </div>
              </div>
            );
          })}
      </div>

      {error && <p className="error">{error}</p>}
      <button className="btn btn-primary" onClick={submit} disabled={busy}>
        {busy ? '提交中...' : '提交任务'}
      </button>
    </div>
  );
}
