import { useRef, useState } from 'react';
import { api } from '../api/client';

interface Props {
  accept?: string;
  multiple?: boolean;
  onUploaded: (paths: string[]) => void;
}

export default function FileUploader({ accept, multiple = true, onUploaded }: Props) {
  const inputRef = useRef<HTMLInputElement>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleFiles = async (files: FileList | null) => {
    if (!files || files.length === 0) return;
    setBusy(true);
    setError(null);
    const paths: string[] = [];
    try {
      for (const file of Array.from(files)) {
        const result = await api.uploadFile(file);
        paths.push(result.path);
      }
      onUploaded(paths);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setBusy(false);
      if (inputRef.current) { inputRef.current.value = ''; }
    }
  };

  return (
    <div>
      <button
        type="button"
        className="btn"
        disabled={busy}
        onClick={() => inputRef.current?.click()}
      >
        {busy ? '上传中...' : '上传文件'}
      </button>
      <input
        ref={inputRef}
        type="file"
        accept={accept}
        multiple={multiple}
        style={{ display: 'none' }}
        onChange={(e) => handleFiles(e.target.files)}
      />
      {error && <p className="error">{error}</p>}
    </div>
  );
}
