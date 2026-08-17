import { useEffect, useRef, useState } from 'react';
import { api } from '../api/client';
import type { DetectedFace } from '../api/types';

interface Props {
  imagePath: string;
  imageSrc?: string;
  selectedFaceIndex: number | null;
  onSelectFace: (face: DetectedFace | null) => void;
}

export default function FaceSelector({
  imagePath,
  imageSrc,
  selectedFaceIndex,
  onSelectFace,
}: Props) {
  const [faces, setFaces] = useState<DetectedFace[]>([]);
  const [loading, setLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);
  const [naturalSize, setNaturalSize] = useState<{ width: number; height: number } | null>(null);
  const [displaySize, setDisplaySize] = useState<{ width: number; height: number } | null>(null);
  const imgRef = useRef<HTMLImageElement | null>(null);

  const detect = async (path: string) => {
    if (!path.trim()) return;
    setLoading(true);
    setError(null);
    try {
      const res = await api.detectFaces(path.trim());
      setFaces(res.faces);
      if (res.faces.length === 0) {
        setError('未检测到人脸');
      }
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
      setFaces([]);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (imagePath) {
      detect(imagePath);
    } else {
      setFaces([]);
    }
  }, [imagePath]);

  const handleImageLoad = (e: React.SyntheticEvent<HTMLImageElement>) => {
    const img = e.currentTarget;
    setNaturalSize({ width: img.naturalWidth, height: img.naturalHeight });
    setDisplaySize({ width: img.clientWidth, height: img.clientHeight });
  };

  // Update display size on window resize
  useEffect(() => {
    const updateSize = () => {
      if (imgRef.current) {
        setDisplaySize({
          width: imgRef.current.clientWidth,
          height: imgRef.current.clientHeight,
        });
      }
    };
    window.addEventListener('resize', updateSize);
    return () => window.removeEventListener('resize', updateSize);
  }, []);

  const scaleX =
    naturalSize && displaySize && naturalSize.width > 0
      ? displaySize.width / naturalSize.width
      : 1;
  const scaleY =
    naturalSize && displaySize && naturalSize.height > 0
      ? displaySize.height / naturalSize.height
      : 1;

  const displayUrl = imageSrc || (imagePath.startsWith('/') ? imagePath : undefined);

  return (
    <div className="face-selector-container">
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '0.4rem' }}>
        <span style={{ fontSize: '0.85rem', fontWeight: 600 }}>
          人脸检测与选择 {faces.length > 0 && `(共发现 ${faces.length} 张人脸)`}
        </span>
        <button
          type="button"
          className="btn-small"
          onClick={() => detect(imagePath)}
          disabled={loading || !imagePath}
        >
          {loading ? '检测中…' : '🔄 重新检测'}
        </button>
      </div>

      {loading && <div style={{ fontSize: '0.8rem', color: '#666', padding: '0.5rem 0' }}>正在检测人脸…</div>}
      {error && <div className="error" style={{ padding: '0.3rem 0' }}>{error}</div>}

      {displayUrl && (
        <div style={{ position: 'relative', display: 'inline-block', maxWidth: '100%', marginTop: '0.4rem' }}>
          <img
            ref={imgRef}
            src={displayUrl}
            alt="Face detection target"
            onLoad={handleImageLoad}
            style={{ display: 'block', maxWidth: '100%', maxHeight: '320px', borderRadius: '6px', border: '1px solid #ddd' }}
          />

          {naturalSize && displaySize && faces.map((face) => {
            const isSelected = selectedFaceIndex === face.index;
            const left = face.box.x * scaleX;
            const top = face.box.y * scaleY;
            const width = face.box.width * scaleX;
            const height = face.box.height * scaleY;

            return (
              <div
                key={face.index}
                onClick={() => onSelectFace(isSelected ? null : face)}
                className={`face-bounding-box ${isSelected ? 'selected' : ''}`}
                style={{
                  position: 'absolute',
                  left: `${left}px`,
                  top: `${top}px`,
                  width: `${width}px`,
                  height: `${height}px`,
                  cursor: 'pointer',
                }}
                title={`人脸 #${face.index + 1} (置信度: ${(face.score * 100).toFixed(0)}%) - 点击${isSelected ? '取消选择' : '选中'}`}
              >
                <div className="face-tag">
                  #{face.index + 1}
                  {face.gender && ` · ${face.gender}`}
                </div>
              </div>
            );
          })}
        </div>
      )}

      {faces.length > 0 && (
        <div style={{ display: 'flex', gap: '0.5rem', marginTop: '0.5rem', flexWrap: 'wrap' }}>
          {faces.map((f) => (
            <button
              key={f.index}
              type="button"
              className={`btn-tag ${selectedFaceIndex === f.index ? 'active' : ''}`}
              onClick={() => onSelectFace(selectedFaceIndex === f.index ? null : f)}
            >
              人脸 #{f.index + 1} ({(f.score * 100).toFixed(0)}%)
              {f.age_range && ` [${f.age_range[0]}-${f.age_range[1]}岁]`}
            </button>
          ))}
          {selectedFaceIndex !== null && (
            <button
              type="button"
              className="btn-tag"
              style={{ background: '#f3f4f6', color: '#666' }}
              onClick={() => onSelectFace(null)}
            >
              清除选择
            </button>
          )}
        </div>
      )}
    </div>
  );
}
