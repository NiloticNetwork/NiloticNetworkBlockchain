'use client';

import React, { useState } from 'react';
import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { WalletIcon, UserPlusIcon, UserIcon, KeyIcon, ArrowRightIcon } from '@heroicons/react/24/outline';
import LoginForm from '../components/auth/LoginForm';
import RegisterForm from '../components/auth/RegisterForm';
import CreateWalletModal from '../components/dashboard/CreateWalletModal';
import ImportWalletModal from '../components/dashboard/ImportWalletModal';
import { AuthProvider } from '../contexts/AuthContext';

type AuthMode = 'login' | 'register';
type OnboardingStep = 'auth' | 'wallet-setup';

function GetStartedPageContent() {
  const [authMode, setAuthMode] = useState<AuthMode>('login');
  const [onboardingStep, setOnboardingStep] = useState<OnboardingStep>('auth');
  const [showForgotPassword, setShowForgotPassword] = useState(false);

  const handleSwitchToRegister = () => {
    setAuthMode('register');
  };

  const handleSwitchToLogin = () => {
    setAuthMode('login');
  };

  const handleForgotPassword = () => {
    setShowForgotPassword(true);
  };

  const handleAuthSuccess = () => {
    setOnboardingStep('wallet-setup');
  };

  if (onboardingStep === 'wallet-setup') {
    return <WalletSetupStep onBack={() => setOnboardingStep('auth')} />;
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
      {/* Navigation */}
      <nav className="bg-gray-800/50 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                <WalletIcon className="h-6 w-6 text-white" />
              </div>
              <span className="text-xl font-bold text-white">Sulwestake</span>
            </div>
            <div className="flex items-center space-x-4">
              <Link href="/landing" className="text-gray-300 hover:text-white px-3 py-2">
                Back to Home
              </Link>
            </div>
          </div>
        </div>
      </nav>

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        {/* Header Section */}
        <div className="text-center mb-12">
          <div className="flex justify-center mb-6">
            <div className="flex items-center space-x-4">
              <div className="flex items-center">
                <div className="h-8 w-8 bg-indigo-600 rounded-full flex items-center justify-center">
                  <UserIcon className="h-4 w-4 text-white" />
                </div>
                <span className="ml-2 text-sm text-white font-medium">Account Setup</span>
              </div>
              <div className="h-px w-8 bg-gray-600"></div>
              <div className="flex items-center">
                <div className="h-8 w-8 bg-gray-600 rounded-full flex items-center justify-center">
                  <WalletIcon className="h-4 w-4 text-gray-400" />
                </div>
                <span className="ml-2 text-sm text-gray-400">Wallet Setup</span>
              </div>
            </div>
          </div>
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-6">
            Get Started with Sulwestake
          </h1>
          <p className="text-xl text-gray-300 max-w-3xl mx-auto">
            Join the future of decentralized staking on the Nilotic blockchain. 
            Create your account, set up your wallet, and start earning rewards.
          </p>
        </div>

        {/* Auth Mode Toggle */}
        <div className="flex justify-center mb-8">
          <div className="bg-gray-800/50 rounded-lg p-1 border border-gray-700">
            <button
              onClick={() => setAuthMode('login')}
              className={`px-6 py-2 rounded-md text-sm font-medium transition-colors duration-200 ${
                authMode === 'login'
                  ? 'bg-indigo-600 text-white'
                  : 'text-gray-300 hover:text-white'
              }`}
            >
              <UserIcon className="h-4 w-4 inline mr-2" />
              Sign In
            </button>
            <button
              onClick={() => setAuthMode('register')}
              className={`px-6 py-2 rounded-md text-sm font-medium transition-colors duration-200 ${
                authMode === 'register'
                  ? 'bg-indigo-600 text-white'
                  : 'text-gray-300 hover:text-white'
              }`}
            >
              <UserPlusIcon className="h-4 w-4 inline mr-2" />
              Create Account
            </button>
          </div>
        </div>

        {/* Auth Forms */}
        <div className="max-w-md mx-auto">
          {authMode === 'login' ? (
            <LoginForm
              onSwitchToRegister={handleSwitchToRegister}
              onForgotPassword={handleForgotPassword}
            />
          ) : (
            <RegisterForm onSwitchToLogin={handleSwitchToLogin} />
          )}
        </div>

        {/* Features Section */}
        <div className="mt-16 grid grid-cols-1 md:grid-cols-3 gap-8">
          <div className="text-center p-6 bg-gray-800/30 rounded-lg border border-gray-700">
            <div className="h-12 w-12 bg-indigo-600 rounded-full flex items-center justify-center mx-auto mb-4">
              <WalletIcon className="h-6 w-6 text-white" />
            </div>
            <h3 className="text-lg font-semibold text-white mb-2">Secure Wallet</h3>
            <p className="text-gray-400 text-sm">
              Create or import your wallet with industry-standard security protocols
            </p>
          </div>
          <div className="text-center p-6 bg-gray-800/30 rounded-lg border border-gray-700">
            <div className="h-12 w-12 bg-green-600 rounded-full flex items-center justify-center mx-auto mb-4">
              <KeyIcon className="h-6 w-6 text-white" />
            </div>
            <h3 className="text-lg font-semibold text-white mb-2">Easy Staking</h3>
            <p className="text-gray-400 text-sm">
              Start staking your SLW tokens with just a few clicks
            </p>
          </div>
          <div className="text-center p-6 bg-gray-800/30 rounded-lg border border-gray-700">
            <div className="h-12 w-12 bg-purple-600 rounded-full flex items-center justify-center mx-auto mb-4">
              <ArrowRightIcon className="h-6 w-6 text-white" />
            </div>
            <h3 className="text-lg font-semibold text-white mb-2">Instant Rewards</h3>
            <p className="text-gray-400 text-sm">
              Earn competitive APY rates and track your rewards in real-time
            </p>
          </div>
        </div>

        {/* Demo Account Info */}
        <div className="mt-12 text-center">
          <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-6 max-w-md mx-auto">
            <h3 className="text-lg font-semibold text-white mb-2">Try Demo Account</h3>
            <p className="text-gray-400 text-sm mb-4">
              Test the platform with our demo account
            </p>
            <div className="bg-gray-700/50 rounded p-3 text-sm">
              <p className="text-gray-300">
                <strong>Email:</strong> demo@nilotic.com<br />
                <strong>Password:</strong> password123
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

function WalletSetupStep({ onBack }: { onBack: () => void }) {
  const router = useRouter();
  const [showCreateModal, setShowCreateModal] = useState(false);
  const [showImportModal, setShowImportModal] = useState(false);
  const [successMessage, setSuccessMessage] = useState<string | null>(null);

  return (
    <div className="min-h-screen bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
      {/* Navigation */}
      <nav className="bg-gray-800/50 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                <WalletIcon className="h-6 w-6 text-white" />
              </div>
              <span className="text-xl font-bold text-white">Sulwestake</span>
            </div>
            <button
              onClick={onBack}
              className="text-gray-300 hover:text-white px-3 py-2"
            >
              ← Back
            </button>
          </div>
        </div>
      </nav>

      <div className="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        <div className="text-center mb-12">
          <div className="flex justify-center mb-6">
            <div className="flex items-center space-x-4">
              <div className="flex items-center">
                <div className="h-8 w-8 bg-green-600 rounded-full flex items-center justify-center">
                  <svg className="h-4 w-4 text-white" fill="currentColor" viewBox="0 0 20 20">
                    <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                  </svg>
                </div>
                <span className="ml-2 text-sm text-gray-300">Account Created</span>
              </div>
              <div className="h-px w-8 bg-gray-600"></div>
              <div className="flex items-center">
                <div className="h-8 w-8 bg-indigo-600 rounded-full flex items-center justify-center">
                  <WalletIcon className="h-4 w-4 text-white" />
                </div>
                <span className="ml-2 text-sm text-white font-medium">Wallet Setup</span>
              </div>
            </div>
          </div>
          <h1 className="text-4xl font-bold text-white mb-6">
            Set Up Your Wallet
          </h1>
          <p className="text-xl text-gray-300">
            Choose how you'd like to set up your wallet for staking
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-8 max-w-4xl mx-auto mb-8">
            <div 
              onClick={() => setShowCreateModal(true)}
              className="bg-gray-800/30 rounded-lg border border-gray-700 p-8 cursor-pointer hover:border-indigo-500 transition-colors duration-200"
            >
              <div className="text-center">
                <div className="h-16 w-16 bg-indigo-600 rounded-full flex items-center justify-center mx-auto mb-6">
                  <UserPlusIcon className="h-8 w-8 text-white" />
                </div>
                <h3 className="text-2xl font-semibold text-white mb-4">Create New Wallet</h3>
                <p className="text-gray-400 mb-6">
                  Generate a new wallet with a secure seed phrase. Perfect for new users.
                </p>
                <div className="space-y-3 text-sm text-gray-300">
                  <div className="flex items-center">
                    <div className="h-2 w-2 bg-green-500 rounded-full mr-3"></div>
                    Generate secure seed phrase
                  </div>
                  <div className="flex items-center">
                    <div className="h-2 w-2 bg-green-500 rounded-full mr-3"></div>
                    Full control over your keys
                  </div>
                  <div className="flex items-center">
                    <div className="h-2 w-2 bg-green-500 rounded-full mr-3"></div>
                    Ready for staking immediately
                  </div>
                </div>
              </div>
            </div>

            <div 
              onClick={() => setShowImportModal(true)}
              className="bg-gray-800/30 rounded-lg border border-gray-700 p-8 cursor-pointer hover:border-indigo-500 transition-colors duration-200"
            >
              <div className="text-center">
                <div className="h-16 w-16 bg-green-600 rounded-full flex items-center justify-center mx-auto mb-6">
                  <KeyIcon className="h-8 w-8 text-white" />
                </div>
                <h3 className="text-2xl font-semibold text-white mb-4">Import Existing Wallet</h3>
                <p className="text-gray-400 mb-6">
                  Import your existing wallet using a seed phrase or private key.
                </p>
                <div className="space-y-3 text-sm text-gray-300">
                  <div className="flex items-center">
                    <div className="h-2 w-2 bg-green-500 rounded-full mr-3"></div>
                    Use existing seed phrase
                  </div>
                  <div className="flex items-center">
                    <div className="h-2 w-2 bg-green-500 rounded-full mr-3"></div>
                    Import private key
                  </div>
                  <div className="flex items-center">
                    <div className="h-2 w-2 bg-green-500 rounded-full mr-3"></div>
                    Access existing funds
                  </div>
                </div>
              </div>
            </div>
          </div>

        {/* Skip Option */}
        <div className="text-center mt-8">
          <button
            onClick={() => {
              setSuccessMessage('Skipping wallet setup. Redirecting to dashboard...');
              setTimeout(() => {
                router.push('/dashboard');
              }, 2000);
            }}
            className="text-gray-400 hover:text-white transition-colors duration-200"
          >
            Skip for now - I'll set up my wallet later
          </button>
        </div>

        {/* Wallet Modals */}
        <CreateWalletModal
          isOpen={showCreateModal}
          onClose={() => setShowCreateModal(false)}
          onWalletCreated={(wallet) => {
            console.log('Wallet created:', wallet);
            setShowCreateModal(false);
            setSuccessMessage('Wallet created successfully! Redirecting to dashboard...');
            setTimeout(() => {
              router.push('/dashboard');
            }, 2000);
          }}
        />

        <ImportWalletModal
          isOpen={showImportModal}
          onClose={() => setShowImportModal(false)}
          onWalletImported={(wallet) => {
            console.log('Wallet imported:', wallet);
            setShowImportModal(false);
            setSuccessMessage('Wallet imported successfully! Redirecting to dashboard...');
            setTimeout(() => {
              router.push('/dashboard');
            }, 2000);
          }}
        />

        {/* Success Message */}
        {successMessage && (
          <div className="fixed top-4 right-4 bg-green-900/90 border border-green-500 text-green-200 px-6 py-4 rounded-lg shadow-lg z-50">
            <div className="flex items-center">
              <svg className="h-5 w-5 mr-2" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
              </svg>
              {successMessage}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

export default function GetStartedPage() {
  return (
    <AuthProvider>
      <GetStartedPageContent />
    </AuthProvider>
  );
} 