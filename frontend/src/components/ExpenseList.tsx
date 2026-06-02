import { Expense } from "../lib/types";
import { formatCurrencyIDR, formatDateTime } from "../lib/format";

interface ExpenseListProps {
  expenses: Expense[];
  onEdit?: (expense: Expense) => void;
  onDelete?: (expenseId: string) => void;
}

export function ExpenseList({ expenses, onEdit, onDelete }: ExpenseListProps) {
  if (expenses.length === 0) {
    return (
      <p className="rounded border border-dashed border-slate-300 bg-white p-4 text-sm text-slate-500">
        Belum ada pengeluaran.
      </p>
    );
  }

  return (
    <ul className="space-y-2">
      {expenses.map((e) => (
        <li
          key={e.id}
          className="rounded-lg border border-slate-200 bg-white p-3 shadow-sm"
        >
          <div className="flex items-start justify-between gap-3">
            <div>
              <div className="text-base font-semibold text-slate-900">
                {formatCurrencyIDR(e.amount)}
              </div>
              <div className="text-xs text-slate-500">
                {e.category ?? "—"} · {formatDateTime(e.spent_at)}
              </div>
            </div>
            <div className="flex gap-1">
              {onEdit ? (
                <button
                  type="button"
                  onClick={() => onEdit(e)}
                  className="rounded p-1 text-xs text-slate-400 hover:bg-slate-100 hover:text-slate-700"
                  title="Edit"
                >
                  ✏️
                </button>
              ) : null}
              {onDelete ? (
                <button
                  type="button"
                  onClick={() => onDelete(e.id)}
                  className="rounded p-1 text-xs text-slate-400 hover:bg-red-50 hover:text-red-600"
                  title="Hapus"
                >
                  🗑️
                </button>
              ) : null}
            </div>
          </div>
          {e.note ? (
            <p className="mt-2 line-clamp-2 text-sm text-slate-700">
              {e.note}
            </p>
          ) : null}
        </li>
      ))}
    </ul>
  );
}
