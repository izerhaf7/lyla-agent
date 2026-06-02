import { Link } from "react-router-dom";
import { PublicNavbar } from "../components/landing/PublicNavbar";
import { LandingAdventureHero } from "../components/landing/LandingAdventureHero";
import { FeatureCard } from "../components/landing/FeatureCard";
import { FaqAccordion } from "../components/landing/FaqAccordion";
import { BmoButton } from "../components/bmo/BmoButton";
import { PublicFooter } from "../components/landing/PublicFooter";

const FEATURES = [
  {
    icon: (
      <svg
        width="20"
        height="20"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
        strokeLinecap="round"
        strokeLinejoin="round"
        aria-hidden="true"
      >
        <path d="M9 11l3 3L22 4" />
        <path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11" />
      </svg>
    ),
    title: "Catat Tugas",
    description:
      "Taskbot membantu mencatat tugas kuliah, deadline, dan pengingat dengan cepat lewat satu dashboard.",
    accent: "#1C4B3B",
  },
  {
    icon: (
      <svg
        width="20"
        height="20"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
        strokeLinecap="round"
        strokeLinejoin="round"
        aria-hidden="true"
      >
        <line x1="12" y1="1" x2="12" y2="23" />
        <path d="M17 5H9.5a3.5 3.5 0 0 0 0 7h5a3.5 3.5 0 0 1 0 7H6" />
      </svg>
    ),
    title: "Voice Log",
    description:
      "Semua perintah suara tersimpan rapi agar kamu bisa meninjau aktivitas terbaru dengan mudah.",
    accent: "#313F98",
  },
  {
    icon: (
      <svg
        width="20"
        height="20"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
        strokeLinecap="round"
        strokeLinejoin="round"
        aria-hidden="true"
      >
        <path d="M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9" />
        <path d="M13.73 21a2 2 0 0 1-3.46 0" />
      </svg>
    ),
    title: "Pengeluaran",
    description:
      "Catat uang jajan, transport, dan pengeluaran harian dalam panel yang ringkas dan nyaman dibaca.",
    accent: "#ED306A",
  },
  {
    icon: (
      <svg
        width="20"
        height="20"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
        strokeLinecap="round"
        strokeLinejoin="round"
        aria-hidden="true"
      >
        <rect x="3" y="4" width="18" height="16" rx="3" />
        <circle cx="9" cy="10" r="1.5" />
        <circle cx="15" cy="10" r="1.5" />
        <path d="M8 15c1 .8 2.3 1.2 4 1.2S14.9 15.8 16 15" />
      </svg>
    ),
    title: "Pair Device",
    description:
      "Hubungkan perangkat ESP32 dan lihat feedback device BMO langsung dari dashboard.",
    accent: "#0EA5E9",
  },
];

const FAQ = [
  {
    q: "Apakah Taskbot hanya bisa dipakai lewat dashboard?",
    a: "Tidak. Taskbot bisa dipakai lewat dashboard web, dan juga bisa dihubungkan ke perangkat ESP32 untuk pengalaman input suara dan feedback device.",
  },
  {
    q: "Apa saja yang bisa dicatat oleh Taskbot?",
    a: "Taskbot dirancang untuk mencatat tugas, reminder, pengeluaran, serta menyimpan voice log agar aktivitas kamu tetap mudah ditinjau.",
  },
  {
    q: "Apakah saya harus login untuk memakai fiturnya?",
    a: "Ya. Login dipakai untuk masuk ke dashboard utama agar kamu bisa mengakses fitur tugas, log suara, pengeluaran, device, dan observability.",
  },
  {
    q: "Apakah perangkat BMO wajib untuk mencoba Taskbot?",
    a: "Tidak wajib. Dashboard web tetap bisa dipakai sendiri. Perangkat BMO menambah pengalaman interaksi fisik dan feedback yang lebih seru.",
  },
];

export function LandingPage() {
  return (
    <div className="flex min-h-screen flex-col bg-[#e3f7d8] text-bmo-dark">
      <PublicNavbar />
      <main className="flex-1">
        <LandingAdventureHero />

        <section id="fitur" className="mx-auto max-w-7xl px-4 py-8 sm:py-12">
          <div className="grid gap-4 rounded-[2rem] border-2 border-bmo-dark/20 bg-[#d5efcf] p-5 shadow-[0_12px_0_0_rgba(28,75,59,0.14)] lg:grid-cols-[1.1fr_1.9fr] lg:p-7">
            <div className="space-y-3 rounded-[1.5rem] bg-[#e7f8da] p-5 shadow-[0_8px_0_0_rgba(28,75,59,0.08)] lg:p-6">
              <h2 className="text-[2.2rem] font-black tracking-[-0.05em] text-bmo-dark sm:text-[2.7rem]">
                What’s inside?
              </h2>
              <div
                className="h-1 w-16 rounded-full bg-bmo-mouth"
                aria-hidden="true"
              />
              <p className="max-w-sm text-base leading-relaxed text-bmo-dark/80">
                Simpan tugas, reminder, voice log, pengeluaran, dan status
                device dalam kontrol center yang fun dan mudah dipakai.
              </p>
            </div>
            <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
              {FEATURES.map((f) => (
                <FeatureCard key={f.title} {...f} />
              ))}
            </div>
          </div>
        </section>

        <section
          id="adventure"
          className="mx-auto max-w-7xl px-4 py-6 sm:py-10"
        >
          <div className="grid gap-5 rounded-[2rem] border-2 border-[#0d4c5d] bg-[#0f7389] p-5 shadow-[0_14px_0_0_rgba(13,27,42,0.18)] lg:grid-cols-[1.15fr_0.85fr] lg:p-7">
            <div className="space-y-4 text-white">
              <h2 className="text-[2.1rem] font-black tracking-[-0.05em] text-[#fff8d7] sm:text-[2.8rem]">
                Aktivitas & Ringkasan
              </h2>
              <div className="grid gap-4 lg:grid-cols-[1.2fr_0.8fr]">
                <div className="relative overflow-hidden rounded-[1.5rem] border-2 border-white/20 bg-[#7dc8e8] shadow-[0_10px_0_0_rgba(13,27,42,0.2)]">
                  <img
                    src="/bmo_running.png"
                    alt="Ilustrasi BMO untuk ringkasan fitur Taskbot"
                    className="h-full w-full object-cover"
                  />
                  <div className="absolute inset-x-4 bottom-4 rounded-[1.1rem] border border-white/25 bg-white/20 px-4 py-3 text-white backdrop-blur-sm">
                    <p className="text-sm font-bold uppercase tracking-[0.2em] text-white/80">
                      Taskbot Overview
                    </p>
                    <p className="mt-1 text-sm leading-relaxed text-white/90">
                      Lihat tugas terbaru, reminder aktif, dan aktivitas suara
                      dalam satu tampilan ringkas.
                    </p>
                  </div>
                </div>
                <div className="flex flex-col justify-between rounded-[1.5rem] border-2 border-white/15 bg-[#0d8ca2] p-5 shadow-[0_10px_0_0_rgba(13,27,42,0.18)]">
                  <div>
                    <p className="text-sm font-bold uppercase tracking-[0.3em] text-white/80">
                      Ringkasan terbaru
                    </p>
                    <p className="mt-3 max-w-sm text-base leading-relaxed text-white/90">
                      Ringkasan aktivitas terbaru: lihat suara yang terekam,
                      tindakan yang terdeteksi, dan respons Taskbot dengan gaya
                      panel yang mudah dipindai.
                    </p>
                  </div>
                  <div className="mt-4 flex items-center gap-4">
                    <Link to="/login">
                      <BmoButton
                        variant="primary"
                        className="rounded-full bg-[#f7dc71] text-bmo-dark hover:bg-[#f5d65c]"
                      >
                        Masuk sekarang
                      </BmoButton>
                    </Link>
                    <div className="hidden rounded-[1.25rem] border-2 border-white/15 bg-white/15 p-3 text-white/90 lg:block">
                      <p className="text-sm font-bold">Voice + Dashboard</p>
                      <p className="mt-1 text-sm leading-snug">
                        Input suara dan panel web bekerja bersama.
                      </p>
                    </div>
                  </div>
                </div>
              </div>
            </div>

            <div
              id="gallery"
              className="grid gap-4 rounded-[1.5rem] bg-[#9de3d0] p-4 sm:grid-cols-2 lg:grid-cols-1"
            >
              <div className="overflow-hidden rounded-[1.35rem] border-2 border-bmo-dark/15 bg-[#bcefd7] p-3 shadow-[0_8px_0_0_rgba(28,75,59,0.12)]">
                <img
                  src="/bmo_chillin.png"
                  alt="BMO chillin"
                  className="h-52 w-full object-contain"
                />
                <div className="rounded-[1rem] bg-[#fff4c4] p-3 text-bmo-dark shadow-[0_4px_0_0_rgba(28,75,59,0.08)]">
                  <p className="text-2xl font-black tracking-[-0.04em]">BMO</p>
                  <p className="mt-1 text-sm leading-snug">
                    Aku bantu kamu mencatat tugas, reminder, dan aktivitas
                    harian.
                  </p>
                </div>
              </div>
              <div className="rounded-[1.35rem] border-2 border-bmo-dark/15 bg-white/35 p-4 text-bmo-dark shadow-[0_8px_0_0_rgba(28,75,59,0.12)]">
                <p className="text-sm font-bold uppercase tracking-[0.25em] text-bmo-dark/70">
                  Karakter pendamping
                </p>
                <div className="mt-3 grid grid-cols-2 gap-3">
                  <img
                    src="/finn_hi.png"
                    alt="Finn"
                    className="rounded-[1rem] bg-white/50 p-2"
                  />
                  <img
                    src="/jake_pointing.png"
                    alt="Jake"
                    className="rounded-[1rem] bg-white/50 p-2"
                  />
                  <img
                    src="/bmo_hi.png"
                    alt="BMO hi"
                    className="rounded-[1rem] bg-white/50 p-2"
                  />
                  <img
                    src="/fin_running.png"
                    alt="Finn running"
                    className="rounded-[1rem] bg-white/50 p-2"
                  />
                </div>
              </div>
            </div>
          </div>
        </section>

        <section id="tentang" className="mx-auto max-w-7xl px-4 py-8 sm:py-12">
          <div className="rounded-[2rem] border-2 border-[#9bd1a6] bg-[#dff4d5] px-5 py-8 shadow-[0_12px_0_0_rgba(28,75,59,0.12)] sm:px-8">
            <div className="grid gap-6 lg:grid-cols-[1fr_0.9fr] lg:items-center">
              <div>
                <h2 className="text-[2rem] font-black tracking-[-0.05em] text-bmo-dark sm:text-[2.5rem]">
                  Jelajahi Taskbot lebih lanjut
                </h2>
                <p className="mt-3 max-w-xl text-base leading-relaxed text-bmo-dark/80">
                  Hubungi admin untuk mencoba proyek ini, semua dokumentasi
                  disajikan secara opensource pada github di bawah
                </p>
                <div className="mt-6 flex max-w-xl flex-col gap-3 sm:flex-row">
                  <a
                    href="#fitur"
                    className="min-h-12 rounded-full border-2 border-bmo-body bg-white px-5 py-3 text-sm font-medium text-bmo-dark transition-colors duration-200 hover:bg-bmo-screen"
                  >
                    Lihat bagian Fitur
                  </a>
                  <Link to="/login">
                    <BmoButton
                      variant="primary"
                      className="rounded-full px-6 py-3"
                    >
                      Buka Dashboard
                    </BmoButton>
                  </Link>
                </div>
                <div className="mt-5 flex gap-3 text-bmo-dark/80">
                  <a
                    href="https://github.com/izerhaf7/lyla-agent"
                    target="_blank"
                    rel="noopener noreferrer"
                    className="grid h-10 w-10 place-items-center rounded-full bg-white/75 shadow-[0_4px_0_0_rgba(28,75,59,0.08)] transition-transform duration-200 hover:-translate-y-0.5"
                    aria-label="GitHub Taskbot"
                  >
                    gh
                  </a>
                  <a
                    href="mailto:alessymilandra4@gmail.com"
                    className="grid h-10 w-10 place-items-center rounded-full bg-white/75 shadow-[0_4px_0_0_rgba(28,75,59,0.08)] transition-transform duration-200 hover:-translate-y-0.5"
                    aria-label="Email Taskbot"
                  >
                    @
                  </a>
                </div>
              </div>

              <div className="grid gap-4 sm:grid-cols-2">
                <div className="rounded-[1.25rem] border-2 border-bmo-dark/15 bg-white/60 p-4">
                  <p className="text-sm font-bold uppercase tracking-[0.25em] text-bmo-dark/70">
                    Fitur
                  </p>
                  <p className="mt-2 text-sm leading-relaxed text-bmo-dark/80">
                    Fitur taskbot ditenagai oleh agent runtime "Lyla" dengan
                    memanfaatkan ADK Kit dari gogle
                  </p>
                </div>
                <div className="rounded-[1.25rem] border-2 border-bmo-dark/15 bg-[#fff3c6] p-4 text-bmo-dark">
                  <p className="text-sm font-bold uppercase tracking-[0.25em] text-bmo-dark/70">
                    Cara kerja & FAQ
                  </p>
                  <p className="mt-2 text-sm leading-relaxed">
                    Inpu suara masuk diterjemahkan dengan STT lalu dideteksi
                    oleh agent dan dikeluarkan dengan TTS
                  </p>
                </div>
              </div>
            </div>
          </div>
        </section>

        <section id="faq" className="mx-auto max-w-7xl px-4 py-8 sm:py-12">
          <div className="rounded-[2rem] border-2 border-bmo-dark/15 bg-[#f7f2c9] px-5 py-8 shadow-[0_12px_0_0_rgba(28,75,59,0.12)] sm:px-8">
            <div className="mb-6 max-w-2xl">
              <p className="text-sm font-bold uppercase tracking-[0.28em] text-bmo-dark/65">
                FAQ
              </p>
              <h2 className="mt-2 text-[2rem] font-black tracking-[-0.05em] text-bmo-dark sm:text-[2.6rem]">
                Pertanyaan yang sering ditanyakan
              </h2>
              <p className="mt-3 text-base leading-relaxed text-bmo-dark/80">
                Kalau kamu baru pertama kali lihat Taskbot, bagian ini
                menjelaskan alur dasar pemakaian dan apa saja yang benar-benar
                tersedia di proyek ini.
              </p>
            </div>

            <FaqAccordion items={FAQ} />
          </div>
        </section>
      </main>
      <PublicFooter />
    </div>
  );
}
