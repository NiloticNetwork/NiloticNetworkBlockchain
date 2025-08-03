import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./src/pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./src/components/**/*.{js,ts,jsx,tsx,mdx}",
    "./src/app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        background: "var(--background)",
        foreground: "var(--foreground)",
        emerald: {
          500: "#10b981",
          600: "#059669",
          700: "#047857",
        },
        gray: {
          300: "#d1d5db",
          400: "#9ca3af",
          600: "#4b5563",
          700: "#374151",
          800: "#1f2937",
          900: "#111827",
        },
        red: {
          500: "#ef4444",
          600: "#dc2626",
          700: "#b91c1c",
        },
        blue: {
          600: "#2563eb",
          700: "#1d4ed8",
        },
        purple: {
          500: "#a855f7",
          600: "#9333ea",
          700: "#7c3aed",
        },
        yellow: {
          500: "#eab308",
          600: "#ca8a04",
          700: "#a16207",
        },
        orange: {
          500: "#f97316",
        },
      },
      animation: {
        "fade-in": "fadeIn 0.5s ease-out",
        "pulse-slow": "pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite",
      },
      keyframes: {
        fadeIn: {
          "0%": { opacity: "0", transform: "translateY(10px)" },
          "100%": { opacity: "1", transform: "translateY(0)" },
        },
      },
    },
  },
  plugins: [],
};

export default config;
