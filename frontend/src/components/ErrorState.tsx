import { ApiError } from "../lib/types";
import { BmoFace } from "./bmo/BmoFace";
import { BmoButton } from "./bmo/BmoButton";

interface ErrorStateProps {
  error: Error | ApiError;
  onRetry?: () => void;
}

export function ErrorState({ error, onRetry }: ErrorStateProps) {
  const status = error instanceof ApiError ? error.status : null;
  const isNetwork = status === 0;

  return (
    <div
      role="alert"
      className="flex items-start gap-4 rounded-lg border border-bmo-red/40 bg-red-50 p-4 text-sm text-bmo-red"
    >
      <BmoFace expression="sad" size={72} className="shrink-0" />
      <div className="min-w-0 flex-1">
        <div className="flex items-center gap-2 font-semibold text-bmo-dark">
          {isNetwork ? "Backend tidak dapat dihubungi" : "Terjadi kesalahan"}
          {status != null && status > 0 ? (
            <span className="rounded-full bg-bmo-red/15 px-2 py-0.5 text-xs font-medium text-bmo-red">
              HTTP {status}
            </span>
          ) : null}
        </div>
        <p className="mt-1 whitespace-pre-wrap text-bmo-red/90">
          {error.message}
        </p>
        {isNetwork ? (
          <p className="mt-2 text-xs text-bmo-red/80">
            Pastikan FastAPI berjalan dan <code className="font-mono">VITE_API_BASE_URL</code>{" "}
            sesuai dengan port backend.
          </p>
        ) : null}
        {onRetry ? (
          <BmoButton
            variant="destructive"
            size="sm"
            onClick={onRetry}
            className="mt-3"
          >
            Coba lagi
          </BmoButton>
        ) : null}
      </div>
    </div>
  );
}
