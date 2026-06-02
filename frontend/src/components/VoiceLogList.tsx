import { VoiceCommandLog } from "../lib/types";
import { formatDateTime, formatStatus } from "../lib/format";
import { BmoBadge } from "./bmo/BmoBadge";
import { BmoFace, BmoExpression } from "./bmo/BmoFace";

interface VoiceLogListProps {
  logs: VoiceCommandLog[];
}

type Tone = "success" | "error" | "idle";

const statusTone = (status: string): Tone => {
  if (status === "success") return "success";
  if (status === "error") return "error";
  return "idle";
};

const FACE_BY_TONE: Record<Tone, BmoExpression> = {
  success: "happy",
  error: "sad",
  idle: "idle",
};

export function VoiceLogList({ logs }: VoiceLogListProps) {
  if (logs.length === 0) {
    return (
      <p className="rounded-lg border border-dashed border-bmo-border bg-surface-elev p-4 text-sm text-slate-600">
        Belum ada riwayat perintah.
      </p>
    );
  }

  return (
    <ul className="space-y-2">
      {logs.map((log) => {
        const tone = statusTone(log.status);
        return (
          <li
            key={log.id}
            className="rounded-lg border border-bmo-border bg-surface-elev p-3 transition-all duration-200 hover:-translate-y-0.5 hover:shadow-bmo"
          >
            <div className="flex items-start gap-3">
              <BmoFace
                expression={FACE_BY_TONE[tone]}
                size={40}
                className="mt-0.5 shrink-0"
              />
              <div className="min-w-0 flex-1">
                <div className="flex items-center justify-between gap-2">
                  <div className="text-xs text-slate-600">
                    {formatDateTime(log.created_at)}
                  </div>
                  <BmoBadge tone={tone}>{formatStatus(log.status)}</BmoBadge>
                </div>
                <p className="mt-1 break-words text-sm font-medium text-bmo-dark">
                  {log.input_text}
                </p>
                {log.response_text ? (
                  <p className="mt-1 line-clamp-2 break-words text-sm text-slate-600">
                    {log.response_text}
                  </p>
                ) : null}
                {log.parsed_actions && log.parsed_actions.length > 0 ? (
                  <details className="mt-2">
                    <summary className="text-xs text-slate-600 hover:text-bmo-dark">
                      {log.parsed_actions.length} aksi
                    </summary>
                    <pre className="mt-1 overflow-x-auto rounded-md bg-bmo-screen/40 p-2 font-mono text-xs text-bmo-dark">
                      {JSON.stringify(log.parsed_actions, null, 2)}
                    </pre>
                  </details>
                ) : null}
              </div>
            </div>
          </li>
        );
      })}
    </ul>
  );
}
