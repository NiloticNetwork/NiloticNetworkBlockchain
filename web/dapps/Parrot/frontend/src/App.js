import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import { QueryClient, QueryClientProvider } from 'react-query';
import { Toaster } from 'react-hot-toast';
import { ThemeProvider } from 'styled-components';
import { Helmet } from 'react-helmet';

// Components
import Navbar from './components/layout/Navbar';
import Footer from './components/layout/Footer';
import LoadingSpinner from './components/common/LoadingSpinner';

// Pages
import Home from './pages/Home';
import Feed from './pages/Feed';
import Profile from './pages/Profile';
import CreatePost from './pages/CreatePost';
import NFTMarketplace from './pages/NFTMarketplace';
import CreateNFT from './pages/CreateNFT';
import Governance from './pages/Governance';
import Analytics from './pages/Analytics';
import ConnectWallet from './pages/ConnectWallet';

// Context
import { WalletProvider } from './contexts/WalletContext';
import { ThemeProvider as CustomThemeProvider } from './contexts/ThemeContext';

// Styles
import GlobalStyles from './styles/GlobalStyles';
import theme from './styles/theme';

// Create a client
const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      retry: 1,
      refetchOnWindowFocus: false,
    },
  },
});

function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <CustomThemeProvider>
        <ThemeProvider theme={theme}>
          <WalletProvider>
            <Router>
              <Helmet>
                <title>Parrot - AI-Driven SocialFi Platform</title>
                <meta name="description" content="Decentralized social networking with AI-powered content curation on the Nilotic blockchain" />
                <meta name="keywords" content="blockchain, socialfi, ai, nilotic, sulwe, nft, dao" />
                <link rel="icon" href="/favicon.ico" />
              </Helmet>
              
              <GlobalStyles />
              <Toaster 
                position="top-right"
                toastOptions={{
                  duration: 4000,
                  style: {
                    background: '#1a1a1a',
                    color: '#fff',
                    border: '1px solid #333',
                  },
                }}
              />
              
              <div className="App">
                <Navbar />
                <main className="main-content">
                  <Routes>
                    <Route path="/" element={<Home />} />
                    <Route path="/feed" element={<Feed />} />
                    <Route path="/profile/:address" element={<Profile />} />
                    <Route path="/create-post" element={<CreatePost />} />
                    <Route path="/nft-marketplace" element={<NFTMarketplace />} />
                    <Route path="/create-nft" element={<CreateNFT />} />
                    <Route path="/governance" element={<Governance />} />
                    <Route path="/analytics" element={<Analytics />} />
                    <Route path="/connect" element={<ConnectWallet />} />
                  </Routes>
                </main>
                <Footer />
              </div>
            </Router>
          </WalletProvider>
        </ThemeProvider>
      </CustomThemeProvider>
    </QueryClientProvider>
  );
}

export default App; 