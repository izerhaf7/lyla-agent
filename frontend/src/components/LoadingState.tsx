interface LoadingStateProps {
  label?: string;
}

export function LoadingState({ label = "Memuat data…" }: LoadingStateProps) {
  return (
    <div
      className="flex items-center gap-3 rounded-lg border border-bmo-border bg-surface-elev p-4 text-sm text-slate-600"
      role="status"
      aria-live="polite"
    >
      <span className="flex items-center gap-1" aria-hidden="true">
        <span className="h-2 w-2 animate-bounce rounded-full bg-bmo-mouth [animation-delay:-0.3s]" />
        <span className="h-2 w-2 animate-bounce rounded-full bg-bmo-blue [animation-delay:-0.15s]" />
        <span className="h-2 w-2 animate-bounce rounded-full bg-bmo-red" />
      </span>
      <span>{label}</span>
    </div>
  );
}
