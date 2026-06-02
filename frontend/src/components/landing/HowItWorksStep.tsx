import { BmoFace, BmoExpression } from "../bmo/BmoFace";

interface HowItWorksStepProps {
  step: number;
  expression: BmoExpression;
  title: string;
  description: string;
  showConnector?: boolean;
}

const EXPRESSION_COLORS: Record<string, string> = {
  idle: "bg-bmo-blue text-white",
  excited: "bg-bmo-yellow text-bmo-dark",
  happy: "bg-bmo-mouth text-white",
};

export function HowItWorksStep({
  step,
  expression,
  title,
  description,
  showConnector = false,
}: HowItWorksStepProps) {
  const badgeClass =
    EXPRESSION_COLORS[expression] ?? "bg-bmo-dark text-bmo-screen";

  return (
    <div className="relative flex flex-col items-center gap-3 rounded-lg border border-bmo-border bg-surface-elev p-5 text-center transition-all duration-200 hover:-translate-y-0.5 hover:shadow-bmo">
      <div
        className={`flex h-8 w-8 items-center justify-center rounded-full text-xs font-bold ${badgeClass}`}
      >
        {step}
      </div>
      <div className="transition-transform duration-200 hover:scale-105">
        <BmoFace expression={expression} size={100} />
      </div>
      <h4 className="text-sm font-medium text-bmo-dark">{title}</h4>
      <p className="text-xs leading-relaxed text-slate-600">{description}</p>

      {showConnector ? (
        <div className="absolute -right-3 top-1/2 hidden -translate-y-1/2 md:block" aria-hidden="true">
          <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
            <path d="M2 6h8M7 3l3 3-3 3" stroke="#1C4B3B" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
          </svg>
        </div>
      ) : null}
    </div>
  );
}
