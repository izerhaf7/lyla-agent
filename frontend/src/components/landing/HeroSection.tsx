import { Link } from "react-router-dom";
import { BmoMascot } from "../bmo/BmoMascot";
import { BmoFace } from "../bmo/BmoFace";
import { BmoButton } from "../bmo/BmoButton";

export function HeroSection() {
  return (
    <section className="relative overflow-hidden bg-gradient-to-b from-bmo-screen/20 via-bmo-purple/10 to-surface">
      <div
        className="pointer-events-none absolute inset-0 bg-bmo-pixel opacity-60"
        aria-hidden="true"
      />

      <div className="pointer-events-none absolute inset-0 hidden md:block" aria-hidden="true">
        <BmoFace
          expression="happy"
          size={48}
          className="absolute top-[15%] right-[12%] opacity-20 animate-bmo-float"
        />
        <BmoFace
          expression="excited"
          size={36}
          className="absolute bottom-[20%] right-[8%] opacity-15 animate-bmo-float [animation-delay:-1.5s]"
        />
        <BmoFace
          expression="idle"
          size={40}
          className="absolute top-[60%] left-[6%] opacity-15 animate-bmo-float [animation-delay:-0.8s]"
        />
      </div>

      <div className="relative mx-auto grid max-w-6xl items-center gap-8 px-4 py-16 md:grid-cols-[auto_1fr] md:gap-16 md:py-24">
        <div className="flex justify-center md:justify-start">
          <div className="animate-bmo-float">
            <BmoMascot size={160} />
          </div>
        </div>

        <div className="flex flex-col items-center gap-6 md:items-start">
          <div className="w-full max-w-xl rounded-xl border-2 border-bmo-body bg-bmo-screen/50 p-6 shadow-bmo-lg backdrop-blur-sm md:p-8">
            <h1 className="text-3xl font-medium leading-tight text-bmo-dark md:text-4xl">
              Asisten suara untuk pelajar Indonesia.
            </h1>
            <p className="mt-4 max-w-lg text-base leading-relaxed text-bmo-dark/70 md:text-lg">
              Catat tugas, ingatkan deadline, dan pantau pengeluaran — semua
              dengan suara. BMO mendengarkan, mencatat, dan mengingatkan
              kamu dengan ramah.
            </p>
            <div className="mt-6 flex flex-wrap gap-3">
              <Link to="/login">
                <BmoButton variant="primary">Mulai sekarang</BmoButton>
              </Link>
              <a href="#fitur">
                <BmoButton variant="secondary">Lihat fitur</BmoButton>
              </a>
            </div>
          </div>

          <div className="hidden items-center gap-6 md:flex" aria-hidden="true">
            <div className="h-4 w-4 rounded-full bg-bmo-red" />
            <div className="h-4 w-4 rounded-full bg-bmo-mouth" />
            <div className="flex gap-1">
              <div className="h-2 w-4 rounded-sm bg-bmo-blue" />
              <div className="h-2 w-4 rounded-sm bg-bmo-blue" />
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
