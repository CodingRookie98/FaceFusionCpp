import { useRef, useState } from 'react';
import { api } from '../api/client';

interface Props {
  onFrameExtracted: (path: string) => void;
}

export default function FrameExtractor({ onFrameExtracted }: Props) {
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const [videoSrc, setVideoSrc] = useState<string | null>(null);
  const [currentTime, setCurrentTime] = useState<number>(0);
  const [duration, setDuration] = useState<number>(0);
  const [isUploading, setIsUploading] = useState<boolean>(false);
  const [lastExtractedPath, setLastExtractedPath] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setError(null);
    const file = e.target.files?.[0];
    if (!file) return;
    const url = URL.createObjectURL(file);
    setVideoSrc(url);
    setCurrentTime(0);
    setLastExtractedPath(null);
  };

  const handleTimeUpdate = () => {
    if (videoRef.current) {
      setCurrentTime(videoRef.current.currentTime);
    }
  };

  const handleLoadedMetadata = () => {
    if (videoRef.current) {
      setDuration(videoRef.current.duration || 0);
    }
  };

  const handleSeek = (e: React.ChangeEvent<HTMLInputElement>) => {
    const time = parseFloat(e.target.value);
    setCurrentTime(time);
    if (videoRef.current) {
      videoRef.current.currentTime = time;
    }
  };

  const stepTime = (delta: number) => {
    if (!videoRef.current) return;
    const newTime = Math.max(0, Math.min(duration, videoRef.current.currentTime + delta));
    videoRef.current.currentTime = newTime;
    setCurrentTime(newTime);
  };

  const captureFrame = async () => {
    const video = videoRef.current;
    if (!video || !video.videoWidth || !video.videoHeight) {
      setError('无法获取视频画面，请先加载视频');
      return;
    }
    setError(null);
    setIsUploading(true);
    try {
      const canvas = document.createElement('canvas');
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const ctx = canvas.getContext('2d');
      if (!ctx) throw new Error('Canvas 2D 上下文创建失败');

      ctx.drawImage(video, 0, 0, canvas.width, canvas.height);

      const blob = await new Promise<Blob | null>((resolve) =>
        canvas.toBlob(resolve, 'image/jpeg', 0.95),
      );
      if (!blob) throw new Error('截帧图像生成失败');

      const timestamp = Math.floor(currentTime * 1000);
      const filename = `frame_${timestamp}ms.jpg`;
      const file = new File([blob], filename, { type: 'image/jpeg' });

      const res = await api.uploadFile(file);
      setLastExtractedPath(res.path);
      onFrameExtracted(res.path);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setIsUploading(false);
    }
  };

  const formatTime = (secs: number) => {
    const m = Math.floor(secs / 60);
    const s = Math.floor(secs % 60);
    const ms = Math.floor((secs % 1) * 1000);
    return `${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}.${String(ms).padStart(3, '0')}`;
  };

  return (
    <div className="frame-extractor-box">
      <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem', marginBottom: '0.6rem' }}>
        <input
          type="file"
          accept="video/*"
          id="frame-video-upload"
          style={{ display: 'none' }}
          onChange={handleFileChange}
        />
        <label htmlFor="frame-video-upload" className="btn btn-secondary" style={{ padding: '0.35rem 0.8rem', fontSize: '0.85rem' }}>
          📹 选择本地视频截帧
        </label>
        {videoSrc && (
          <span style={{ fontSize: '0.8rem', color: '#666' }}>
            {formatTime(currentTime)} / {formatTime(duration)}
          </span>
        )}
      </div>

      {videoSrc && (
        <div style={{ marginTop: '0.5rem' }}>
          <div style={{ position: 'relative', background: '#000', borderRadius: '6px', overflow: 'hidden', textAlign: 'center' }}>
            <video
              ref={videoRef}
              src={videoSrc}
              controls={false}
              onTimeUpdate={handleTimeUpdate}
              onLoadedMetadata={handleLoadedMetadata}
              style={{ maxHeight: '240px', maxWidth: '100%', display: 'block', margin: '0 auto' }}
            />
          </div>

          <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', marginTop: '0.5rem' }}>
            <input
              type="range"
              min={0}
              max={duration || 100}
              step={0.01}
              value={currentTime}
              onChange={handleSeek}
              style={{ flex: 1 }}
            />
          </div>

          <div style={{ display: 'flex', gap: '0.4rem', marginTop: '0.5rem', alignItems: 'center', flexWrap: 'wrap' }}>
            <button type="button" className="btn-small" onClick={() => stepTime(-1)} title="后退 1 秒">-1s</button>
            <button type="button" className="btn-small" onClick={() => stepTime(-0.1)} title="后退 0.1 秒">-0.1s</button>
            <button type="button" className="btn-small" onClick={() => stepTime(0.1)} title="前进 0.1 秒">+0.1s</button>
            <button type="button" className="btn-small" onClick={() => stepTime(1)} title="前进 1 秒">+1s</button>
            <button
              type="button"
              className="btn btn-primary"
              style={{ padding: '0.35rem 0.9rem', fontSize: '0.85rem', marginLeft: 'auto' }}
              onClick={captureFrame}
              disabled={isUploading}
            >
              {isUploading ? '截帧上传中…' : '📷 截取当前帧'}
            </button>
          </div>
        </div>
      )}

      {lastExtractedPath && (
        <div style={{ marginTop: '0.5rem', fontSize: '0.8rem', color: '#166534', background: '#dcfce7', padding: '0.4rem 0.6rem', borderRadius: '4px' }}>
          ✅ 已截帧并上传: <code>{lastExtractedPath}</code>
        </div>
      )}

      {error && <p className="error" style={{ marginTop: '0.4rem' }}>{error}</p>}
    </div>
  );
}
