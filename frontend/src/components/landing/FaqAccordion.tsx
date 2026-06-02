interface FaqItem {
  q: string;
  a: string;
}

interface FaqAccordionProps {
  items: FaqItem[];
}

export function FaqAccordion({ items }: FaqAccordionProps) {
  return (
    <div className="space-y-2">
      {items.map((item, idx) => (
        <details
          key={idx}
          className="group rounded-lg border border-bmo-border bg-surface-elev transition-all duration-200 open:border-bmo-body open:bg-bmo-screen/30 open:shadow-bmo"
        >
          <summary className="flex cursor-pointer items-center gap-3 px-4 py-3 text-sm font-medium text-bmo-dark">
            <span
              className="flex h-5 w-5 shrink-0 items-center justify-center rounded bg-bmo-dark text-[10px] font-bold text-bmo-screen transition-colors duration-200 group-open:bg-bmo-mouth"
              aria-hidden="true"
            >
              {idx + 1}
            </span>
            <span className="flex-1">{item.q}</span>
            <svg
              width="16"
              height="16"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
              className="shrink-0 transition-transform duration-200 group-open:rotate-180"
              aria-hidden="true"
            >
              <polyline points="6 9 12 15 18 9" />
            </svg>
          </summary>
          <div className="border-t border-bmo-border px-4 py-3 pl-12 text-sm leading-relaxed text-slate-600">
            {item.a}
          </div>
        </details>
      ))}
    </div>
  );
}
