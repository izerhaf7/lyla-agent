import { ReactNode } from "react";

interface FeatureCardProps {
  icon: ReactNode;
  title: string;
  description: string;
  accent?: string;
}

export function FeatureCard({
  icon,
  title,
  description,
  accent = "#1C4B3B",
}: FeatureCardProps) {
  return (
    <div className="group flex flex-col gap-3 overflow-hidden rounded-[1.35rem] border-2 border-bmo-dark/15 bg-[#eaf9d9] transition-all duration-200 hover:-translate-y-0.5 hover:shadow-[0_12px_0_0_rgba(28,75,59,0.12)]">
      <div className="h-2" style={{ background: accent }} aria-hidden="true" />
      <div className="px-5 pb-5 pt-2">
        <div className="flex h-12 w-12 items-center justify-center rounded-[1rem] border-2 border-bmo-dark/15 bg-white/65 text-bmo-dark shadow-[0_4px_0_0_rgba(28,75,59,0.08)]">
          {icon}
        </div>
        <h3 className="mt-4 text-[1.35rem] font-black tracking-[-0.04em] text-bmo-dark">
          {title}
        </h3>
        <p className="mt-2 text-sm leading-relaxed text-bmo-dark/75">
          {description}
        </p>
      </div>
    </div>
  );
}
