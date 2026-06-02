import { useState } from "react";
import * as api from "../lib/api";
import { ApiError, AgentTextResponse } from "../lib/types";
import { DEMO_DEVICE_ID, DEMO_USER_ID } from "../lib/env";
import { BmoFace, BmoExpression } from "./bmo/BmoFace";
import { BmoButton } from "./bmo/BmoButton";

interface AgentCommandBoxProps {
  onSuccess?: () => void;
}

const SAMPLE = "catat makan siang 20000";

export function AgentCommandBox({ onSuccess }: AgentCommandBoxProps) {
  const [text, setText] = useState("");
  const [loading, setLoading] = useState(false);
  const [result, setResult] = useState<AgentTextResponse | null>(null);
  const [error, setError] = useState<ApiError | null>(null);

  const reset = () => {
    setText("");
    setResult(null);
    setError(null);
  };

  const submit = async () => {
    if (!DEMO_USER_ID) return;
    const trimmed = text.trim();
    if (!trimmed) {
      setError(new ApiError("Perintah tidak boleh kosong.", 422));
      return;
    }
    setLoading(true);
    setError(null);
    try {
      const response = await api.runAgentText({
        user_id: DEMO_USER_ID,
        device_id: DEMO_DEVICE_ID ?? undefined,
        text: trimmed,
        timezone: "Asia/Jakarta",
      });
      setResult(response);
      onSuccess?.();
    } catch (err) {
      setError(err instanceof ApiError ? err : new ApiError(String(err), 0));
    } finally {
      setLoading(false);
    }
  };

  const face: BmoExpression = loading
    ? "excited"
    : error
      ? "sad"
      : result
        ? "happy"
        : "idle";

  const caption = loading
    ? "BMO sedang memproses…"
    : error
      ? "Aduh, ada yang salah."
      : result
        ? "Beres! Cek hasilnya di bawah."
        : "BMO siap mendengarkan perintahmu.";

  const fb = result?.device_feedback as
    | { command?: { face?: string; sound?: string; text?: string } }
    | null
    | undefined;
  const command = fb?.command;

  return (
    <section className="overflow-hidden rounded-lg border border-bmo-border bg-surface-elev">
      <div className="flex flex-col gap-4 p-4 sm:flex-row sm:items-start">
        <div className="flex shrink-0 flex-col items-center gap-1 sm:w-28">
          <BmoFace expression={face} size={96} />
          <p className="text-center text-xs text-slate-600">{caption}</p>
        </div>

        <div className="min-w-0 flex-1">
          <header className="flex items-center justify-between gap-2">
            <h2 className="text-sm font-semibold text-bmo-dark">
              Agent Command
            </h2>
            <span className="text-xs text-slate-600">
              Contoh:{" "}
              <code className="font-mono text-bmo-dark">{SAMPLE}</code>
            </span>
          </header>

          {!DEMO_DEVICE_ID ? (
            <p className="mt-2 rounded-md border border-bmo-yellow bg-bmo-yellow/20 p-2 text-xs text-bmo-dark">
              <code className="font-mono">VITE_DEMO_DEVICE_ID</code> belum
              diset. Perintah akan tetap dijalankan, tetapi{" "}
              <em>device feedback</em> tidak akan dikirim.
            </p>
          ) : null}

          <div className="mt-3">
            <textarea
              rows={2}
              value={text}
              onChange={(e) => setText(e.target.value)}
              placeholder={SAMPLE}
              className="w-full rounded-md border-2 border-bmo-body bg-surface-elev px-3 py-2 text-sm text-bmo-dark placeholder:text-slate-400 focus:border-bmo-mouth focus:outline-none focus:ring-2 focus:ring-bmo-mouth/20 disabled:cursor-not-allowed disabled:opacity-60"
              disabled={loading}
            />
          </div>

          <div className="mt-2 flex gap-2">
            <BmoButton
              onClick={submit}
              disabled={loading || !DEMO_USER_ID}
              size="sm"
            >
              {loading ? "Menjalankan…" : "Jalankan"}
            </BmoButton>
            <BmoButton
              variant="secondary"
              onClick={reset}
              disabled={loading}
              size="sm"
            >
              Bersihkan
            </BmoButton>
          </div>
        </div>
      </div>

      {error ? (
        <div className="border-t border-bmo-red/30 bg-red-50 p-4 text-sm text-bmo-red">
          <div className="font-semibold text-bmo-dark">
            Gagal menjalankan perintah
          </div>
          <p className="mt-1 whitespace-pre-wrap text-bmo-red/90">
            {error.message}
          </p>
        </div>
      ) : null}

      {result ? (
        <div className="space-y-3 border-t border-bmo-border bg-surface px-4 py-4">
          <div className="rounded-md border border-bmo-border bg-surface-elev p-3">
            <div className="text-xs uppercase tracking-wide text-slate-600">
              Reply
            </div>
            <p className="mt-1 whitespace-pre-wrap text-sm text-bmo-dark">
              {result.reply}
            </p>
          </div>

          {fb ? (
            <div className="rounded-md border border-bmo-mouth/40 bg-bmo-screen p-3">
              <div className="text-xs uppercase tracking-wide text-bmo-mouth">
                Device Feedback
              </div>
              <dl className="mt-1 grid grid-cols-3 gap-2 text-sm">
                <div>
                  <dt className="text-xs text-bmo-mouth">Face</dt>
                  <dd className="font-medium text-bmo-dark">
                    {command?.face ?? "—"}
                  </dd>
                </div>
                <div>
                  <dt className="text-xs text-bmo-mouth">Sound</dt>
                  <dd className="font-medium text-bmo-dark">
                    {command?.sound ?? "—"}
                  </dd>
                </div>
                <div>
                  <dt className="text-xs text-bmo-mouth">Text</dt>
                  <dd className="font-medium text-bmo-dark">
                    {command?.text ?? "—"}
                  </dd>
                </div>
              </dl>
            </div>
          ) : null}

          {result.actions.length > 0 ? (
            <details className="rounded-md border border-bmo-border bg-surface-elev p-3">
              <summary className="text-xs font-medium text-slate-600 hover:text-bmo-dark">
                {result.actions.length} aksi terdeteksi
              </summary>
              <pre className="mt-2 overflow-x-auto rounded-md bg-bmo-screen/40 p-2 font-mono text-xs text-bmo-dark">
                {JSON.stringify(result.actions, null, 2)}
              </pre>
            </details>
          ) : null}
        </div>
      ) : null}
    </section>
  );
}
