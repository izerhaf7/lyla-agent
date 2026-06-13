import { FormEvent, useEffect, useState } from "react";
import * as api from "../lib/api";
import { isReady } from "../lib/env";
import { ApiError, ReminderOut } from "../lib/types";
import { LoadingState } from "../components/LoadingState";
import { ErrorState } from "../components/ErrorState";
import { EmptyState } from "../components/EmptyState";
import { BmoButton } from "../components/bmo/BmoButton";
import { BmoInput } from "../components/bmo/BmoInput";
import { ReminderCard } from "../components/reminders/ReminderCard";

const STATUS_OPTIONS: Array<{ value: string; label: string }> = [
  { value: "", label: "Semua" },
  { value: "scheduled", label: "Dijadwalkan" },
  { value: "sent", label: "Terkirim" },
  { value: "failed", label: "Gagal" },
  { value: "cancelled", label: "Dibatalkan" },
];

export function RemindersPage() {
  const ready = isReady();
  const userId = ready.ok ? ready.userId : null;

  const [reminders, setReminders] = useState<ReminderOut[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [filter, setFilter] = useState<string>("");
  const [busyId, setBusyId] = useState<string | null>(null);

  const [title, setTitle] = useState("");
  const [remindAt, setRemindAt] = useState("");
  const [channel, setChannel] = useState("both");
  const [submitting, setSubmitting] = useState(false);
  const [formError, setFormError] = useState<string | null>(null);

  const load = async (uid: string, status: string) => {
    setLoading(true);
    setError(null);
    try {
      setReminders(await api.getReminders(uid, status || undefined));
    } catch (err) {
      setError(err instanceof Error ? err : new Error(String(err)));
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (userId) void load(userId, filter);
  }, [userId, filter]);

  const resetForm = () => {
    setTitle("");
    setRemindAt("");
    setChannel("both");
    setFormError(null);
  };

  const handleSubmit = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    if (!userId || !title.trim() || !remindAt) return;
    setSubmitting(true);
    setFormError(null);
    try {
      await api.createReminder({
        user_id: userId,
        title: title.trim(),
        remind_at: new Date(remindAt).toISOString(),
        channel,
      });
      resetForm();
      await load(userId, filter);
    } catch (err) {
      setFormError(err instanceof ApiError ? err.message : String(err));
    } finally {
      setSubmitting(false);
    }
  };

  const handleCancel = async (reminderId: string) => {
    setBusyId(reminderId);
    try {
      await api.cancelReminder(reminderId);
      if (userId) await load(userId, filter);
    } catch (err) {
      setError(err instanceof Error ? err : new Error(String(err)));
    } finally {
      setBusyId(null);
    }
  };

  return (
    <section className="space-y-4">
      <header className="flex flex-wrap items-center justify-between gap-3">
        <div>
          <h1 className="text-2xl font-medium text-bmo-dark">Pengingat</h1>
          <p className="text-sm text-slate-500">
            Daftar reminder yang akan diputar BMO sebagai suara saat jatuh tempo.
          </p>
        </div>
        <div className="flex flex-wrap gap-2">
          <div className="flex flex-wrap gap-1 rounded-md border border-bmo-border bg-surface-elev p-1">
            {STATUS_OPTIONS.map((opt) => {
              const active = filter === opt.value;
              return (
                <button
                  key={opt.value}
                  type="button"
                  onClick={() => setFilter(opt.value)}
                  disabled={!userId || loading}
                  className={`cursor-pointer rounded px-3 py-1 text-xs font-medium transition-colors ${
                    active
                      ? "bg-bmo-dark text-bmo-screen"
                      : "text-slate-600 hover:bg-bmo-screen/40"
                  }`}
                >
                  {opt.label}
                </button>
              );
            })}
          </div>
          <BmoButton
            variant="secondary"
            size="sm"
            onClick={() => userId && load(userId, filter)}
            disabled={!userId || loading}
          >
            Refresh
          </BmoButton>
        </div>
      </header>

      <form
        onSubmit={handleSubmit}
        className="grid gap-3 rounded-lg border border-bmo-border bg-surface-elev p-4 md:grid-cols-2"
      >
        <label className="space-y-1 text-sm">
          <span className="font-medium text-bmo-dark">Judul Pengingat</span>
          <BmoInput
            type="text"
            value={title}
            onChange={(e) => setTitle(e.target.value)}
            placeholder="Ingatkan tugas matematika"
            required
          />
        </label>
        <label className="space-y-1 text-sm">
          <span className="font-medium text-bmo-dark">Waktu Pengingat</span>
          <BmoInput
            type="datetime-local"
            value={remindAt}
            onChange={(e) => setRemindAt(e.target.value)}
            required
          />
        </label>
        <label className="space-y-1 text-sm">
          <span className="font-medium text-bmo-dark">Channel</span>
          <select
            value={channel}
            onChange={(e) => setChannel(e.target.value)}
            className="w-full rounded-md border-2 border-bmo-body bg-surface-elev px-3 py-2 text-sm text-bmo-dark focus:border-bmo-mouth focus:outline-none focus:ring-2 focus:ring-bmo-mouth/20"
          >
            <option value="both">Semua (WhatsApp + Device)</option>
            <option value="whatsapp">WhatsApp</option>
            <option value="device">Device</option>
          </select>
        </label>
        <div className="flex items-end gap-2">
          <BmoButton type="submit" disabled={submitting || !userId}>
            {submitting ? "Menyimpan…" : "Tambah Pengingat"}
          </BmoButton>
        </div>
        {formError ? (
          <p className="md:col-span-2 rounded border border-bmo-red/40 bg-pink-50 p-2 text-xs text-bmo-red">
            {formError}
          </p>
        ) : null}
      </form>

      {loading ? <LoadingState /> : null}
      {error ? (
        <ErrorState
          error={error}
          onRetry={() => userId && load(userId, filter)}
        />
      ) : null}
      {!loading && !error ? (
        reminders.length === 0 ? (
          <EmptyState
            face="idle"
            title="Belum ada pengingat"
            description="Buat pengingat baru di atas, atau buat tugas dengan deadline untuk auto reminder."
          />
        ) : (
          <div className="grid grid-cols-1 gap-3 md:grid-cols-2 lg:grid-cols-3">
            {reminders.map((r) => (
              <ReminderCard
                key={r.id}
                reminder={r}
                onCancel={handleCancel}
                cancelling={busyId === r.id}
              />
            ))}
          </div>
        )
      ) : null}
    </section>
  );
}
