import { FormEvent, useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { ApiError, Task } from "../lib/types";
import * as api from "../lib/api";
import { isReady } from "../lib/env";
import { LoadingState } from "../components/LoadingState";
import { ErrorState } from "../components/ErrorState";
import { TaskList } from "../components/TaskList";
import { EmptyState } from "../components/EmptyState";
import { BmoButton } from "../components/bmo/BmoButton";
import { BmoInput } from "../components/bmo/BmoInput";

const STATUS_OPTIONS: Array<{ value: string; label: string }> = [
  { value: "", label: "Semua" },
  { value: "pending", label: "Pending" },
  { value: "in_progress", label: "Dalam pengerjaan" },
  { value: "done", label: "Selesai" },
];

export function TasksPage() {
  const ready = isReady();
  const userId = ready.ok ? ready.userId : null;

  const [tasks, setTasks] = useState<Task[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [filter, setFilter] = useState<string>("");

  const [title, setTitle] = useState("");
  const [course, setCourse] = useState("");
  const [deadlineAt, setDeadlineAt] = useState("");
  const [priority, setPriority] = useState("");
  const [autoReminder, setAutoReminder] = useState(true);
  const [submitting, setSubmitting] = useState(false);
  const [formError, setFormError] = useState<string | null>(null);

  const load = async (uid: string, status: string) => {
    setLoading(true);
    setError(null);
    try {
      setTasks(await api.getTasks(uid, status || undefined));
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
    setCourse("");
    setDeadlineAt("");
    setPriority("");
    setAutoReminder(true);
    setFormError(null);
  };

  const handleSubmit = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    if (!userId || !title.trim()) return;
    setSubmitting(true);
    setFormError(null);
    try {
      const created = await api.createTask({
        user_id: userId,
        title: title.trim(),
        course: course.trim() || null,
        deadline_at: deadlineAt ? new Date(deadlineAt).toISOString() : null,
        priority: priority || null,
        auto_reminder: autoReminder,
      });
      setTasks((prev) => [created, ...prev]);
      resetForm();
    } catch (err) {
      setFormError(err instanceof ApiError ? err.message : String(err));
    } finally {
      setSubmitting(false);
    }
  };

  const handleUpdate = (updated: Task) => {
    setTasks((prev) => prev.map((t) => (t.id === updated.id ? updated : t)));
  };

  const handleDelete = (taskId: string) => {
    setTasks((prev) => prev.filter((t) => t.id !== taskId));
  };

  return (
    <section className="space-y-4">
      <header className="flex flex-wrap items-center justify-between gap-3">
        <h1 className="text-2xl font-medium text-bmo-dark">Tugas</h1>
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
          <span className="font-medium text-bmo-dark">Judul Tugas</span>
          <BmoInput
            type="text"
            value={title}
            onChange={(e) => setTitle(e.target.value)}
            placeholder="Tugas Matematika"
            required
          />
        </label>
        <label className="space-y-1 text-sm">
          <span className="font-medium text-bmo-dark">Mata Kuliah</span>
          <BmoInput
            type="text"
            value={course}
            onChange={(e) => setCourse(e.target.value)}
            placeholder="Matematika, Fisika, …"
          />
        </label>
        <label className="space-y-1 text-sm">
          <span className="font-medium text-bmo-dark">Deadline</span>
          <BmoInput
            type="datetime-local"
            value={deadlineAt}
            onChange={(e) => setDeadlineAt(e.target.value)}
          />
        </label>
        <label className="space-y-1 text-sm">
          <span className="font-medium text-bmo-dark">Prioritas</span>
          <select
            value={priority}
            onChange={(e) => setPriority(e.target.value)}
            className="w-full rounded-md border-2 border-bmo-body bg-surface-elev px-3 py-2 text-sm text-bmo-dark focus:border-bmo-mouth focus:outline-none focus:ring-2 focus:ring-bmo-mouth/20"
          >
            <option value="">Normal</option>
            <option value="low">Rendah</option>
            <option value="high">Tinggi</option>
          </select>
        </label>
        <label className="flex items-center gap-2 text-sm">
          <input
            type="checkbox"
            checked={autoReminder}
            onChange={(e) => setAutoReminder(e.target.checked)}
            className="rounded border-bmo-body"
          />
          <span className="font-medium text-bmo-dark">Auto Reminder</span>
        </label>
        <div className="flex items-end gap-2">
          <BmoButton type="submit" disabled={submitting || !userId}>
            {submitting ? "Menyimpan…" : "Tambah Tugas"}
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
        tasks.length === 0 ? (
          <EmptyState
            face="idle"
            title="Belum ada tugas"
            description="Buat tugas baru di atas, atau katakan ke BMO: catat tugas matematika besok jam 10 pagi."
            cta={
              <Link to="/app/reminders">
                <BmoButton variant="secondary">Lihat pengingat</BmoButton>
              </Link>
            }
          />
        ) : (
          <TaskList
            tasks={tasks}
            onUpdate={handleUpdate}
            onDelete={handleDelete}
          />
        )
      ) : null}
    </section>
  );
}
