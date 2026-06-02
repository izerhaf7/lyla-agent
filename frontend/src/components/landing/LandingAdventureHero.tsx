import { Link } from "react-router-dom";
import { BmoButton } from "../bmo/BmoButton";

export function LandingAdventureHero() {
  return (
    <section className="relative overflow-hidden pb-8 pt-8 sm:pb-12 sm:pt-10">
      <div
        className="absolute inset-0 bg-[linear-gradient(180deg,rgba(136,223,255,0.95)_0%,rgba(116,219,255,0.9)_45%,rgba(171,220,135,0.88)_100%)]"
        aria-hidden="true"
      />

      <div
        className="absolute inset-0 bg-[url('/landing_background.png')] bg-cover bg-[center_top] opacity-55 mix-blend-soft-light"
        aria-hidden="true"
      />

      <div
        className="pointer-events-none absolute inset-x-0 bottom-0 h-1/3 bg-[linear-gradient(180deg,transparent_0%,rgba(143,197,81,0.35)_45%,rgba(99,157,56,0.45)_100%)]"
        aria-hidden="true"
      />

      <div className="relative mx-auto max-w-7xl px-4">
        <div className="grid items-end gap-10 lg:grid-cols-[1.05fr_0.95fr] lg:gap-12">
          <div className="max-w-2xl pb-4 text-left text-bmo-dark">
            <div className="mb-4 inline-flex items-center gap-2 rounded-full border border-bmo-dark/20 bg-white/25 px-3 py-1 text-xs font-bold uppercase tracking-[0.25em] text-bmo-dark/80 backdrop-blur-sm">
              <span className="h-2 w-2 rounded-full bg-bmo-red" />
              Taskbot siap membantu
            </div>

            <h1 className="max-w-xl text-[clamp(3rem,7vw,6.4rem)] font-black leading-[0.92] tracking-[-0.06em] text-bmo-dark drop-shadow-[0_3px_0_rgba(255,255,255,0.2)]">
              Catat tugas,
              <br />
              atur reminder,
              <br />
              pantau pengeluaran
            </h1>

            <p className="mt-6 max-w-xl text-[1.05rem] leading-relaxed text-bmo-dark/80 sm:text-[1.15rem]">
              Taskbot membantu mencatat tugas, mengatur pengingat, memantau pengeluaran, dan membaca log suara dalam satu dashboard yang ramah.
            </p>

            <div className="mt-8 flex flex-wrap items-center gap-4">
              <Link to="/login">
                <BmoButton
                  variant="primary"
                  className="rounded-full px-6 py-3 text-base shadow-[0_6px_0_0_rgba(153,51,86,0.38)]"
                >
                  Buka Dashboard
                </BmoButton>
              </Link>
              <a href="#adventure">
                <BmoButton
                  variant="secondary"
                  className="rounded-full border-2 border-bmo-dark/40 bg-white/35 px-6 py-3 text-base text-bmo-dark backdrop-blur-sm"
                >
                  Lihat Fitur
                </BmoButton>
              </a>
            </div>
          </div>

          <div className="relative flex min-h-[34rem] items-end justify-center lg:justify-end">
            <img
              src="/bmo_running.png"
              alt="BMO di ilustrasi landing page"
              className="absolute bottom-[11%] left-[8%] z-20 w-[44%] max-w-[18rem] drop-shadow-[0_20px_24px_rgba(17,63,95,0.25)] sm:w-[38%]"
            />
            <img
              src="/finn_hi.png"
              alt="Finn sebagai dekorasi ilustrasi"
              className="absolute bottom-0 left-0 z-10 w-[28%] max-w-[11rem] drop-shadow-[0_12px_16px_rgba(17,63,95,0.18)]"
            />
            <img
              src="/jake_hi.png"
              alt="Jake sebagai dekorasi ilustrasi"
              className="absolute bottom-[3%] left-[21%] z-10 w-[22%] max-w-[10rem] drop-shadow-[0_12px_16px_rgba(17,63,95,0.18)]"
            />

            <div className="absolute right-0 top-[2%] z-0 hidden w-[88%] lg:block">
              <img
                src="/landing_background.png"
                alt="Adventure background"
                className="w-full rounded-[2rem] object-cover opacity-95 shadow-[0_24px_50px_rgba(17,63,95,0.18)]"
              />
            </div>

            <div className="absolute right-0 top-[10%] z-30 hidden w-[40%] rounded-[1.75rem] border-2 border-bmo-dark/20 bg-[#F5F1BE] px-5 py-4 shadow-[0_12px_0_0_rgba(28,75,59,0.18)] lg:block">
              <p className="text-2xl font-black tracking-[-0.04em] text-bmo-dark">BMO</p>
              <p className="mt-1 text-[0.95rem] leading-snug text-bmo-dark/85">
                Aku BMO!
                <br />
                Teman bantu tugasmu!
              </p>
              <div className="mt-2 flex justify-end text-bmo-red">❤</div>
            </div>

            <div className="absolute bottom-[11%] right-[8%] z-20 hidden rounded-[1.35rem] border-2 border-bmo-dark/20 bg-bmo-screen/90 px-4 py-3 shadow-[0_12px_0_0_rgba(28,75,59,0.18)] lg:block">
              <p className="text-sm font-bold uppercase tracking-[0.2em] text-bmo-dark/70">Dashboard</p>
              <p className="mt-1 max-w-[11rem] text-[0.95rem] leading-snug text-bmo-dark">
                Tugas, reminder, pengeluaran, dan feedback device dalam satu tempat.
              </p>
            </div>

            <div className="pointer-events-none absolute inset-x-0 bottom-0 z-10 h-40 bg-[linear-gradient(180deg,rgba(163,213,98,0)_0%,rgba(128,188,68,0.55)_55%,rgba(88,151,41,0.9)_100%)]" />
          </div>
        </div>
      </div>
    </section>
  );
}
