import { useCallback, useEffect, useMemo, useState } from "react";
import {
  DashboardSummary,
  Device,
  Expense,
  Task,
  VoiceCommandLog,
} from "../lib/types";
import * as api from "../lib/api";
import { isReady } from "../lib/env";
import { LoadingState } from "../components/LoadingState";
import { ErrorState } from "../components/ErrorState";
import { StatCard } from "../components/StatCard";
import { AgentCommandBox } from "../components/AgentCommandBox";
import { VoiceLogList } from "../components/VoiceLogList";
import { EmptyState } from "../components/EmptyState";
import { formatCurrencyIDR } from "../lib/format";

const ICONS = {
  task: (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      <path d="M9 11l3 3L22 4" />
      <path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11" />
    </svg>
  ),
  clock: (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      <circle cx="12" cy="12" r="9" />
      <path d="M12 7v5l3 2" />
    </svg>
  ),
  money: (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      <line x1="12" y1="1" x2="12" y2="23" />
      <path d="M17 5H9.5a3.5 3.5 0 0 0 0 7h5a3.5 3.5 0 0 1 0 7H6" />
    </svg>
  ),
  calendar: (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      <rect x="3" y="4" width="18" height="18" rx="2" />
      <path d="M16 2v4M8 2v4M3 10h18" />
    </svg>
  ),
  device: (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      <rect x="6" y="2" width="12" height="20" rx="2" />
      <line x1="10" y1="18" x2="14" y2="18" />
    </svg>
  ),
} as const;

interface Snapshot {
  summary: DashboardSummary;
  pendingTasks: Task[];
  expenses: Expense[];
  logs: VoiceCommandLog[];
  devices: Device[];
}

const sumMonthExpenses = (expenses: Expense[]): number => {
  const now = new Date();
  const y = now.getFullYear();
  const m = now.getMonth();
  return expenses.reduce((acc, e) => {
    const d = new Date(e.spent_at);
    if (Number.isNaN(d.getTime())) return acc;
    return d.getFullYear() === y && d.getMonth() === m ? acc + e.amount : acc;
  }, 0);
};

export function DashboardPage() {
  const ready = isReady();
  const userId = ready.ok ? ready.userId : null;

  const [data, setData] = useState<Snapshot | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [refreshKey, setRefreshKey] = useState(0);

  const load = useCallback(async (uid: string) => {
    setLoading(true);
    setError(null);
    try {
      const [summary, pendingTasks, expenses, logs, devices] =
        await Promise.all([
          api.getSummary(uid),
          api.getTasks(uid, "pending"),
          api.getExpenses(uid),
          api.getLogs(uid),
          api.getDevices(uid),
        ]);
      setData({ summary, pendingTasks, expenses, logs, devices });
    } catch (err) {
      setError(err instanceof Error ? err : new Error(String(err)));
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    if (userId) void load(userId);
  }, [userId, refreshKey, load]);

  const monthExpenses = useMemo(
    () => (data ? sumMonthExpenses(data.expenses) : 0),
    [data],
  );

  const recentLogs = useMemo(
    () => (data ? data.logs.slice(0, 5) : []),
    [data],
  );

  return (
    <section className="space-y-6">
      <header>
        <h1 className="text-2xl font-medium text-bmo-dark">Ringkasan</h1>
        <p className="text-sm text-slate-500">
          Snapshot aktivitas hari ini.
        </p>
      </header>

      {loading && !data ? <LoadingState /> : null}
      {error ? (
        <ErrorState
          error={error}
          onRetry={() => userId && void load(userId)}
        />
      ) : null}

      {data ? (
        <>
          <div className="grid grid-cols-2 gap-3 md:grid-cols-3 xl:grid-cols-5">
            <StatCard
              label="Tugas pending"
              value={data.pendingTasks.length}
              tone={data.pendingTasks.length > 0 ? "warn" : "good"}
              icon={ICONS.task}
            />
            <StatCard
              label="Jatuh tempo hari ini"
              value={data.summary.tasks_due_today}
              tone={data.summary.tasks_due_today > 0 ? "warn" : "neutral"}
              icon={ICONS.clock}
            />
            <StatCard
              label="Pengeluaran hari ini"
              value={formatCurrencyIDR(data.summary.total_expenses_today)}
              icon={ICONS.money}
            />
            <StatCard
              label="Pengeluaran bulan ini"
              value={formatCurrencyIDR(monthExpenses)}
              icon={ICONS.calendar}
            />
            <StatCard
              label="Devices terdaftar"
              value={data.devices.length}
              icon={ICONS.device}
            />
          </div>

          <AgentCommandBox onSuccess={() => setRefreshKey((k) => k + 1)} />

          <section className="space-y-2">
            <h2 className="text-sm font-medium uppercase tracking-wide text-slate-600">
              Aktivitas terkini
            </h2>
            {recentLogs.length === 0 ? (
              <EmptyState
                face="idle"
                title="Belum ada aktivitas hari ini"
                description="Coba jalankan perintah lewat Agent Command di atas."
              />
            ) : (
              <VoiceLogList logs={recentLogs} />
            )}
          </section>
        </>
      ) : null}
    </section>
  );
}
