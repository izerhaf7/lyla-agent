import { BmoMascot } from "../bmo/BmoMascot";

export function PublicFooter() {
  return (
    <footer className="relative border-t border-bmo-border bg-bmo-dark text-bmo-screen/80">
      <div className="mx-auto flex max-w-6xl flex-col gap-4 px-4 py-8 md:flex-row md:items-center md:justify-between">
        <div className="flex items-center gap-3">
          <BmoMascot size={48} />
          <div>
            <p className="text-sm font-medium text-bmo-screen">Taskbot</p>
            <p className="text-xs text-bmo-screen/60">
              Dibuat untuk skripsi · © {new Date().getFullYear()}
            </p>
          </div>
        </div>
        <div className="flex gap-4 text-sm">
          <a
            href="https://github.com/izerhaf7/lyla-agent"
            target="_blank"
            rel="noopener noreferrer"
            className="cursor-pointer transition-colors duration-200 hover:text-bmo-body"
          >
            GitHub
          </a>
          <a
            href="mailto:demo@taskbot.local"
            className="cursor-pointer transition-colors duration-200 hover:text-bmo-body"
          >
            Email
          </a>
        </div>
      </div>
    </footer>
  );
}
