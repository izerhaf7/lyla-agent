import { ReactNode } from "react";

type Tone = "neutral" | "warn" | "good";

interface StatCardProps {
  label: string;
  value: string | number;
  hint?: string;
  tone?: Tone;
  icon?: ReactNode;
}

interface ToneStyle {
  border: string;
  value: string;
  dot: string;
  chip: string;
}

const TONE_CLASSES: Record<Tone, ToneStyle> = {
  neutral: {
    border: "border-bmo-border",
    value: "text-bmo-dark",
    dot: "bg-bmo-blue",
    chip: "bg-bmo-blue-light/50 text-bmo-blue",
  },
  warn: {
    border: "border-bmo-yellow",
    value: "text-bmo-dark",
    dot: "bg-bmo-yellow",
    chip: "bg-bmo-yellow/30 text-bmo-dark",
  },
  good: {
    border: "border-bmo-mouth/40",
    value: "text-bmo-mouth",
    dot: "bg-bmo-mouth",
    chip: "bg-bmo-screen text-bmo-dark",
  },
};

export function StatCard({
  label,
  value,
  hint,
  tone = "neutral",
  icon,
}: StatCardProps) {
  const t = TONE_CLASSES[tone];
  return (
    <div
      className={`group relative overflow-hidden rounded-lg border bg-surface-elev p-4 transition-all duration-200 hover:-translate-y-0.5 hover:shadow-bmo ${t.border}`}
    >
      <div className="flex items-start justify-between gap-2">
        <div className="flex items-center gap-1.5">
          <span className={`h-1.5 w-1.5 rounded-full ${t.dot}`} aria-hidden="true" />
          <span className="text-xs font-medium uppercase tracking-wide text-slate-600">
            {label}
          </span>
        </div>
        {icon ? (
          <span
            className={`flex h-7 w-7 items-center justify-center rounded-md ${t.chip}`}
            aria-hidden="true"
          >
            {icon}
          </span>
        ) : null}
      </div>
      <div className={`mt-2 text-2xl font-semibold tabular-nums ${t.value}`}>
        {value}
      </div>
      {hint ? (
        <div className="mt-1 text-xs text-slate-600/80">{hint}</div>
      ) : null}
    </div>
  );
}
