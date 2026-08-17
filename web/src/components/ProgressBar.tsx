interface Props {
  percent: number;
  postfix?: string;
}

export default function ProgressBar({ percent, postfix }: Props) {
  const clamped = Math.max(0, Math.min(100, percent));
  return (
    <div className="progress-wrap">
      <div className="progress-track">
        <div className="progress-fill" style={{ width: `${clamped}%` }} />
      </div>
      <span className="progress-text">
        {clamped.toFixed(1)}%{postfix ? ` — ${postfix}` : ''}
      </span>
    </div>
  );
}
