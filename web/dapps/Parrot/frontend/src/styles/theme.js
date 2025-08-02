const theme = {
  colors: {
    // Primary colors inspired by Nilotic heritage and stars
    primary: {
      50: '#f0f9ff',
      100: '#e0f2fe',
      200: '#bae6fd',
      300: '#7dd3fc',
      400: '#38bdf8',
      500: '#0ea5e9', // Main blue (star color)
      600: '#0284c7',
      700: '#0369a1',
      800: '#075985',
      900: '#0c4a6e',
    },
    
    // Gold/yellow for star motifs
    star: {
      50: '#fffbeb',
      100: '#fef3c7',
      200: '#fde68a',
      300: '#fcd34d',
      400: '#fbbf24',
      500: '#f59e0b', // Main gold
      600: '#d97706',
      700: '#b45309',
      800: '#92400e',
      900: '#78350f',
    },
    
    // Dark theme colors
    dark: {
      50: '#f8fafc',
      100: '#f1f5f9',
      200: '#e2e8f0',
      300: '#cbd5e1',
      400: '#94a3b8',
      500: '#64748b',
      600: '#475569',
      700: '#334155',
      800: '#1e293b',
      900: '#0f172a', // Main dark
      950: '#020617',
    },
    
    // Accent colors
    accent: {
      purple: '#8b5cf6',
      pink: '#ec4899',
      green: '#10b981',
      red: '#ef4444',
      orange: '#f97316',
    },
    
    // Semantic colors
    success: '#10b981',
    warning: '#f59e0b',
    error: '#ef4444',
    info: '#3b82f6',
    
    // Background colors
    background: {
      primary: '#0f172a',
      secondary: '#1e293b',
      tertiary: '#334155',
      card: '#1e293b',
      modal: '#0f172a',
    },
    
    // Text colors
    text: {
      primary: '#f8fafc',
      secondary: '#cbd5e1',
      tertiary: '#94a3b8',
      muted: '#64748b',
      inverse: '#0f172a',
    },
    
    // Border colors
    border: {
      primary: '#334155',
      secondary: '#475569',
      accent: '#0ea5e9',
    },
  },
  
  // Typography
  fonts: {
    primary: "'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif",
    secondary: "'Poppins', sans-serif",
    mono: "'JetBrains Mono', 'Fira Code', monospace",
  },
  
  fontSizes: {
    xs: '0.75rem',
    sm: '0.875rem',
    base: '1rem',
    lg: '1.125rem',
    xl: '1.25rem',
    '2xl': '1.5rem',
    '3xl': '1.875rem',
    '4xl': '2.25rem',
    '5xl': '3rem',
    '6xl': '3.75rem',
  },
  
  fontWeights: {
    light: 300,
    normal: 400,
    medium: 500,
    semibold: 600,
    bold: 700,
    extrabold: 800,
  },
  
  // Spacing
  spacing: {
    0: '0',
    1: '0.25rem',
    2: '0.5rem',
    3: '0.75rem',
    4: '1rem',
    5: '1.25rem',
    6: '1.5rem',
    8: '2rem',
    10: '2.5rem',
    12: '3rem',
    16: '4rem',
    20: '5rem',
    24: '6rem',
    32: '8rem',
    40: '10rem',
    48: '12rem',
    56: '14rem',
    64: '16rem',
  },
  
  // Border radius
  borderRadius: {
    none: '0',
    sm: '0.125rem',
    base: '0.25rem',
    md: '0.375rem',
    lg: '0.5rem',
    xl: '0.75rem',
    '2xl': '1rem',
    '3xl': '1.5rem',
    full: '9999px',
  },
  
  // Shadows
  shadows: {
    sm: '0 1px 2px 0 rgba(0, 0, 0, 0.05)',
    base: '0 1px 3px 0 rgba(0, 0, 0, 0.1), 0 1px 2px 0 rgba(0, 0, 0, 0.06)',
    md: '0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06)',
    lg: '0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05)',
    xl: '0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 10px 10px -5px rgba(0, 0, 0, 0.04)',
    '2xl': '0 25px 50px -12px rgba(0, 0, 0, 0.25)',
    inner: 'inset 0 2px 4px 0 rgba(0, 0, 0, 0.06)',
    glow: '0 0 20px rgba(14, 165, 233, 0.3)',
    starGlow: '0 0 30px rgba(245, 158, 11, 0.4)',
  },
  
  // Breakpoints
  breakpoints: {
    xs: '320px',
    sm: '640px',
    md: '768px',
    lg: '1024px',
    xl: '1280px',
    '2xl': '1536px',
  },
  
  // Transitions
  transitions: {
    fast: '0.15s ease-in-out',
    base: '0.2s ease-in-out',
    slow: '0.3s ease-in-out',
    slower: '0.5s ease-in-out',
  },
  
  // Z-index
  zIndex: {
    hide: -1,
    auto: 'auto',
    base: 0,
    docked: 10,
    dropdown: 1000,
    sticky: 1100,
    banner: 1200,
    overlay: 1300,
    modal: 1400,
    popover: 1500,
    skipLink: 1600,
    toast: 1700,
    tooltip: 1800,
  },
  
  // Gradients
  gradients: {
    primary: 'linear-gradient(135deg, #0ea5e9 0%, #3b82f6 100%)',
    star: 'linear-gradient(135deg, #f59e0b 0%, #fbbf24 100%)',
    dark: 'linear-gradient(135deg, #0f172a 0%, #1e293b 100%)',
    card: 'linear-gradient(135deg, #1e293b 0%, #334155 100%)',
    glow: 'linear-gradient(135deg, rgba(14, 165, 233, 0.1) 0%, rgba(59, 130, 246, 0.1) 100%)',
  },
  
  // Animations
  animations: {
    fadeIn: 'fadeIn 0.3s ease-in-out',
    slideUp: 'slideUp 0.3s ease-out',
    slideDown: 'slideDown 0.3s ease-out',
    scaleIn: 'scaleIn 0.2s ease-out',
    pulse: 'pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite',
    shimmer: 'shimmer 2s linear infinite',
    starTwinkle: 'starTwinkle 3s ease-in-out infinite',
  },
  
  // Keyframes
  keyframes: {
    fadeIn: {
      '0%': { opacity: 0 },
      '100%': { opacity: 1 },
    },
    slideUp: {
      '0%': { transform: 'translateY(100%)', opacity: 0 },
      '100%': { transform: 'translateY(0)', opacity: 1 },
    },
    slideDown: {
      '0%': { transform: 'translateY(-100%)', opacity: 0 },
      '100%': { transform: 'translateY(0)', opacity: 1 },
    },
    scaleIn: {
      '0%': { transform: 'scale(0.9)', opacity: 0 },
      '100%': { transform: 'scale(1)', opacity: 1 },
    },
    pulse: {
      '0%, 100%': { opacity: 1 },
      '50%': { opacity: 0.5 },
    },
    shimmer: {
      '0%': { transform: 'translateX(-100%)' },
      '100%': { transform: 'translateX(100%)' },
    },
    starTwinkle: {
      '0%, 100%': { opacity: 1, transform: 'scale(1)' },
      '50%': { opacity: 0.7, transform: 'scale(1.1)' },
    },
  },
};

export default theme; 