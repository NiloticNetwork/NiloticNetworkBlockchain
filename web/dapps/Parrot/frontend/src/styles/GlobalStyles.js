import { createGlobalStyle } from 'styled-components';

const GlobalStyles = createGlobalStyle`
  * {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
  }

  html {
    font-size: 16px;
    scroll-behavior: smooth;
  }

  body {
    font-family: ${({ theme }) => theme.fonts.primary};
    font-size: ${({ theme }) => theme.fontSizes.base};
    font-weight: ${({ theme }) => theme.fontWeights.normal};
    line-height: 1.6;
    color: ${({ theme }) => theme.colors.text.primary};
    background: ${({ theme }) => theme.colors.background.primary};
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    overflow-x: hidden;
  }

  /* Typography */
  h1, h2, h3, h4, h5, h6 {
    font-family: ${({ theme }) => theme.fonts.secondary};
    font-weight: ${({ theme }) => theme.fontWeights.bold};
    line-height: 1.2;
    margin-bottom: ${({ theme }) => theme.spacing[4]};
  }

  h1 {
    font-size: ${({ theme }) => theme.fontSizes['5xl']};
    background: ${({ theme }) => theme.gradients.primary};
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }

  h2 {
    font-size: ${({ theme }) => theme.fontSizes['4xl']};
    color: ${({ theme }) => theme.colors.text.primary};
  }

  h3 {
    font-size: ${({ theme }) => theme.fontSizes['3xl']};
    color: ${({ theme }) => theme.colors.text.primary};
  }

  h4 {
    font-size: ${({ theme }) => theme.fontSizes['2xl']};
    color: ${({ theme }) => theme.colors.text.primary};
  }

  h5 {
    font-size: ${({ theme }) => theme.fontSizes.xl};
    color: ${({ theme }) => theme.colors.text.secondary};
  }

  h6 {
    font-size: ${({ theme }) => theme.fontSizes.lg};
    color: ${({ theme }) => theme.colors.text.secondary};
  }

  p {
    margin-bottom: ${({ theme }) => theme.spacing[4]};
    color: ${({ theme }) => theme.colors.text.secondary};
  }

  a {
    color: ${({ theme }) => theme.colors.primary[400]};
    text-decoration: none;
    transition: ${({ theme }) => theme.transitions.base};
    
    &:hover {
      color: ${({ theme }) => theme.colors.primary[300]};
      text-decoration: underline;
    }
  }

  /* Layout */
  .App {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
  }

  .main-content {
    flex: 1;
    padding-top: 80px; /* Account for fixed navbar */
  }

  /* Container */
  .container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 0 ${({ theme }) => theme.spacing[4]};
    
    @media (min-width: ${({ theme }) => theme.breakpoints.md}) {
      padding: 0 ${({ theme }) => theme.spacing[6]};
    }
  }

  /* Buttons */
  .btn {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    padding: ${({ theme }) => `${theme.spacing[3]} ${theme.spacing[6]}`};
    border: none;
    border-radius: ${({ theme }) => theme.borderRadius.lg};
    font-family: ${({ theme }) => theme.fonts.primary};
    font-size: ${({ theme }) => theme.fontSizes.base};
    font-weight: ${({ theme }) => theme.fontWeights.medium};
    text-decoration: none;
    cursor: pointer;
    transition: ${({ theme }) => theme.transitions.base};
    position: relative;
    overflow: hidden;
    
    &:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    
    &:focus {
      outline: 2px solid ${({ theme }) => theme.colors.primary[400]};
      outline-offset: 2px;
    }
  }

  .btn-primary {
    background: ${({ theme }) => theme.gradients.primary};
    color: white;
    box-shadow: ${({ theme }) => theme.shadows.md};
    
    &:hover:not(:disabled) {
      transform: translateY(-2px);
      box-shadow: ${({ theme }) => theme.shadows.lg};
    }
  }

  .btn-secondary {
    background: transparent;
    color: ${({ theme }) => theme.colors.text.primary};
    border: 2px solid ${({ theme }) => theme.colors.border.primary};
    
    &:hover:not(:disabled) {
      border-color: ${({ theme }) => theme.colors.primary[400]};
      color: ${({ theme }) => theme.colors.primary[400]};
    }
  }

  .btn-star {
    background: ${({ theme }) => theme.gradients.star};
    color: ${({ theme }) => theme.colors.dark[900]};
    box-shadow: ${({ theme }) => theme.shadows.starGlow};
    
    &:hover:not(:disabled) {
      transform: translateY(-2px);
      box-shadow: ${({ theme }) => theme.shadows.starGlow}, ${({ theme }) => theme.shadows.lg};
    }
  }

  /* Cards */
  .card {
    background: ${({ theme }) => theme.colors.background.card};
    border: 1px solid ${({ theme }) => theme.colors.border.primary};
    border-radius: ${({ theme }) => theme.borderRadius.xl};
    padding: ${({ theme }) => theme.spacing[6]};
    box-shadow: ${({ theme }) => theme.shadows.md};
    transition: ${({ theme }) => theme.transitions.base};
    
    &:hover {
      transform: translateY(-4px);
      box-shadow: ${({ theme }) => theme.shadows.lg};
    }
  }

  .card-glow {
    background: ${({ theme }) => theme.colors.background.card};
    border: 1px solid ${({ theme }) => theme.colors.border.accent};
    border-radius: ${({ theme }) => theme.borderRadius.xl};
    padding: ${({ theme }) => theme.spacing[6]};
    box-shadow: ${({ theme }) => theme.shadows.glow};
    transition: ${({ theme }) => theme.transitions.base};
    
    &:hover {
      transform: translateY(-4px);
      box-shadow: ${({ theme }) => theme.shadows.glow}, ${({ theme }) => theme.shadows.lg};
    }
  }

  /* Forms */
  .form-group {
    margin-bottom: ${({ theme }) => theme.spacing[6]};
  }

  .form-label {
    display: block;
    margin-bottom: ${({ theme }) => theme.spacing[2]};
    font-weight: ${({ theme }) => theme.fontWeights.medium};
    color: ${({ theme }) => theme.colors.text.primary};
  }

  .form-input {
    width: 100%;
    padding: ${({ theme }) => theme.spacing[3]};
    border: 2px solid ${({ theme }) => theme.colors.border.primary};
    border-radius: ${({ theme }) => theme.borderRadius.lg};
    background: ${({ theme }) => theme.colors.background.secondary};
    color: ${({ theme }) => theme.colors.text.primary};
    font-family: ${({ theme }) => theme.fonts.primary};
    font-size: ${({ theme }) => theme.fontSizes.base};
    transition: ${({ theme }) => theme.transitions.base};
    
    &:focus {
      outline: none;
      border-color: ${({ theme }) => theme.colors.primary[400]};
      box-shadow: 0 0 0 3px ${({ theme }) => theme.colors.primary[400]}20;
    }
    
    &::placeholder {
      color: ${({ theme }) => theme.colors.text.tertiary};
    }
  }

  .form-textarea {
    resize: vertical;
    min-height: 120px;
  }

  /* Utilities */
  .text-center { text-align: center; }
  .text-left { text-align: left; }
  .text-right { text-align: right; }
  
  .text-primary { color: ${({ theme }) => theme.colors.primary[400]}; }
  .text-secondary { color: ${({ theme }) => theme.colors.text.secondary}; }
  .text-muted { color: ${({ theme }) => theme.colors.text.muted}; }
  .text-success { color: ${({ theme }) => theme.colors.success}; }
  .text-warning { color: ${({ theme }) => theme.colors.warning}; }
  .text-error { color: ${({ theme }) => theme.colors.error}; }
  
  .bg-primary { background: ${({ theme }) => theme.colors.background.primary}; }
  .bg-secondary { background: ${({ theme }) => theme.colors.background.secondary}; }
  .bg-card { background: ${({ theme }) => theme.colors.background.card}; }
  
  .border { border: 1px solid ${({ theme }) => theme.colors.border.primary}; }
  .border-accent { border: 1px solid ${({ theme }) => theme.colors.border.accent}; }
  
  .rounded { border-radius: ${({ theme }) => theme.borderRadius.base}; }
  .rounded-lg { border-radius: ${({ theme }) => theme.borderRadius.lg}; }
  .rounded-xl { border-radius: ${({ theme }) => theme.borderRadius.xl}; }
  .rounded-full { border-radius: ${({ theme }) => theme.borderRadius.full}; }
  
  .shadow { box-shadow: ${({ theme }) => theme.shadows.base}; }
  .shadow-lg { box-shadow: ${({ theme }) => theme.shadows.lg}; }
  .shadow-glow { box-shadow: ${({ theme }) => theme.shadows.glow}; }
  
  /* Spacing utilities */
  .p-0 { padding: ${({ theme }) => theme.spacing[0]}; }
  .p-1 { padding: ${({ theme }) => theme.spacing[1]}; }
  .p-2 { padding: ${({ theme }) => theme.spacing[2]}; }
  .p-3 { padding: ${({ theme }) => theme.spacing[3]}; }
  .p-4 { padding: ${({ theme }) => theme.spacing[4]}; }
  .p-6 { padding: ${({ theme }) => theme.spacing[6]}; }
  .p-8 { padding: ${({ theme }) => theme.spacing[8]}; }
  
  .m-0 { margin: ${({ theme }) => theme.spacing[0]}; }
  .m-1 { margin: ${({ theme }) => theme.spacing[1]}; }
  .m-2 { margin: ${({ theme }) => theme.spacing[2]}; }
  .m-3 { margin: ${({ theme }) => theme.spacing[3]}; }
  .m-4 { margin: ${({ theme }) => theme.spacing[4]}; }
  .m-6 { margin: ${({ theme }) => theme.spacing[6]}; }
  .m-8 { margin: ${({ theme }) => theme.spacing[8]}; }
  
  .mt-0 { margin-top: ${({ theme }) => theme.spacing[0]}; }
  .mt-1 { margin-top: ${({ theme }) => theme.spacing[1]}; }
  .mt-2 { margin-top: ${({ theme }) => theme.spacing[2]}; }
  .mt-3 { margin-top: ${({ theme }) => theme.spacing[3]}; }
  .mt-4 { margin-top: ${({ theme }) => theme.spacing[4]}; }
  .mt-6 { margin-top: ${({ theme }) => theme.spacing[6]}; }
  .mt-8 { margin-top: ${({ theme }) => theme.spacing[8]}; }
  
  .mb-0 { margin-bottom: ${({ theme }) => theme.spacing[0]}; }
  .mb-1 { margin-bottom: ${({ theme }) => theme.spacing[1]}; }
  .mb-2 { margin-bottom: ${({ theme }) => theme.spacing[2]}; }
  .mb-3 { margin-bottom: ${({ theme }) => theme.spacing[3]}; }
  .mb-4 { margin-bottom: ${({ theme }) => theme.spacing[4]}; }
  .mb-6 { margin-bottom: ${({ theme }) => theme.spacing[6]}; }
  .mb-8 { margin-bottom: ${({ theme }) => theme.spacing[8]}; }

  /* Flexbox utilities */
  .flex { display: flex; }
  .inline-flex { display: inline-flex; }
  .flex-col { flex-direction: column; }
  .flex-row { flex-direction: row; }
  .items-center { align-items: center; }
  .items-start { align-items: flex-start; }
  .items-end { align-items: flex-end; }
  .justify-center { justify-content: center; }
  .justify-between { justify-content: space-between; }
  .justify-start { justify-content: flex-start; }
  .justify-end { justify-content: flex-end; }
  .flex-1 { flex: 1; }
  .flex-wrap { flex-wrap: wrap; }

  /* Grid utilities */
  .grid { display: grid; }
  .grid-cols-1 { grid-template-columns: repeat(1, minmax(0, 1fr)); }
  .grid-cols-2 { grid-template-columns: repeat(2, minmax(0, 1fr)); }
  .grid-cols-3 { grid-template-columns: repeat(3, minmax(0, 1fr)); }
  .grid-cols-4 { grid-template-columns: repeat(4, minmax(0, 1fr)); }
  .gap-2 { gap: ${({ theme }) => theme.spacing[2]}; }
  .gap-4 { gap: ${({ theme }) => theme.spacing[4]}; }
  .gap-6 { gap: ${({ theme }) => theme.spacing[6]}; }
  .gap-8 { gap: ${({ theme }) => theme.spacing[8]}; }

  /* Responsive utilities */
  .hidden { display: none; }
  .block { display: block; }
  .inline-block { display: inline-block; }
  
  @media (min-width: ${({ theme }) => theme.breakpoints.sm}) {
    .sm\\:block { display: block; }
    .sm\\:hidden { display: none; }
  }
  
  @media (min-width: ${({ theme }) => theme.breakpoints.md}) {
    .md\\:block { display: block; }
    .md\\:hidden { display: none; }
  }
  
  @media (min-width: ${({ theme }) => theme.breakpoints.lg}) {
    .lg\\:block { display: block; }
    .lg\\:hidden { display: none; }
  }

  /* Animations */
  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  @keyframes slideUp {
    from { transform: translateY(100%); opacity: 0; }
    to { transform: translateY(0); opacity: 1; }
  }

  @keyframes slideDown {
    from { transform: translateY(-100%); opacity: 0; }
    to { transform: translateY(0); opacity: 1; }
  }

  @keyframes scaleIn {
    from { transform: scale(0.9); opacity: 0; }
    to { transform: scale(1); opacity: 1; }
  }

  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.5; }
  }

  @keyframes shimmer {
    0% { transform: translateX(-100%); }
    100% { transform: translateX(100%); }
  }

  @keyframes starTwinkle {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.7; transform: scale(1.1); }
  }

  /* Scrollbar styling */
  ::-webkit-scrollbar {
    width: 8px;
  }

  ::-webkit-scrollbar-track {
    background: ${({ theme }) => theme.colors.background.secondary};
  }

  ::-webkit-scrollbar-thumb {
    background: ${({ theme }) => theme.colors.border.primary};
    border-radius: ${({ theme }) => theme.borderRadius.full};
  }

  ::-webkit-scrollbar-thumb:hover {
    background: ${({ theme }) => theme.colors.border.accent};
  }

  /* Selection styling */
  ::selection {
    background: ${({ theme }) => theme.colors.primary[400]};
    color: white;
  }

  /* Focus styles */
  *:focus {
    outline: 2px solid ${({ theme }) => theme.colors.primary[400]};
    outline-offset: 2px;
  }

  /* Loading states */
  .loading {
    opacity: 0.6;
    pointer-events: none;
  }

  .skeleton {
    background: linear-gradient(90deg, 
      ${({ theme }) => theme.colors.background.secondary} 25%, 
      ${({ theme }) => theme.colors.background.tertiary} 50%, 
      ${({ theme }) => theme.colors.background.secondary} 75%
    );
    background-size: 200% 100%;
    animation: shimmer 2s linear infinite;
  }
`;

export default GlobalStyles; 