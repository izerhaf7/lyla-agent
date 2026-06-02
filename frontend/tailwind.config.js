/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        bmo: {
          body: "#9FD5B1",
          screen: "#C5E3BF",
          mouth: "#1F8941",
          dark: "#1C4B3B",
          yellow: "#F7E72F",
          blue: "#313F98",
          "blue-light": "#C8CFFF",
          cyan: "#77CFDB",
          red: "#ED306A",
          purple: "#b297c7",
          "screen-dark": "#0D1B2A",
        },
        surface: {
          DEFAULT: "#F8FAF6",
          elev: "#FFFFFF",
        },
        "bmo-border": "#D1E0CC",
      },
      fontFamily: {
        mono: [
          "JetBrains Mono",
          "ui-monospace",
          "SFMono-Regular",
          "Menlo",
          "Consolas",
          "monospace",
        ],
      },
      boxShadow: {
        bmo: "-2px 2px 0 2px #639975",
        "bmo-lg": "-4px 4px 0 4px #639975",
      },
      keyframes: {
        "bmo-float": {
          "0%, 100%": { transform: "translateY(0)" },
          "50%": { transform: "translateY(-8px)" },
        },
      },
      animation: {
        "bmo-float": "bmo-float 3s ease-in-out infinite",
      },
      backgroundImage: {
        "bmo-pixel":
          "repeating-linear-gradient(0deg, transparent, transparent 3px, rgba(28,75,59,0.04) 3px, rgba(28,75,59,0.04) 4px), repeating-linear-gradient(90deg, transparent, transparent 3px, rgba(28,75,59,0.04) 3px, rgba(28,75,59,0.04) 4px)",
      },
    },
  },
  plugins: [],
};
