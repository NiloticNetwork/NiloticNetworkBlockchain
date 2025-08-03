'use client';

import React, { useState } from 'react';
import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { UserIcon, ArrowLeftIcon } from '@heroicons/react/24/outline';
import LoginForm from '../components/auth/LoginForm';
import RegisterForm from '../components/auth/RegisterForm';
import { AuthProvider } from '../contexts/AuthContext';

function LoginPageContent() {
  const router = useRouter();
  const [showRegister, setShowRegister] = useState(false);

  const handleSwitchToRegister = () => {
    setShowRegister(true);
  };

  const handleSwitchToLogin = () => {
    setShowRegister(false);
  };

  if (showRegister) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
        {/* Navigation */}
        <nav className="bg-gray-800/50 border-b border-gray-700">
          <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
            <div className="flex justify-between items-center py-4">
              <div className="flex items-center space-x-4">
                <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                  <UserIcon className="h-6 w-6 text-white" />
                </div>
                <span className="text-xl font-bold text-white">Sulwestake</span>
              </div>
              <div className="flex items-center space-x-4">
                <Link href="/landing" className="text-gray-300 hover:text-white px-3 py-2">
                  <ArrowLeftIcon className="h-4 w-4 inline mr-1" />
                  Back to Home
                </Link>
              </div>
            </div>
          </div>
        </nav>

        <div className="max-w-md mx-auto px-4 sm:px-6 lg:px-8 py-12">
          <div className="text-center mb-8">
            <h1 className="text-3xl font-bold text-white mb-4">
              Create Account
            </h1>
            <p className="text-gray-300">
              Join Sulwestake and start your staking journey
            </p>
          </div>

          {/* Registration Form */}
          <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-8">
            <RegisterForm onSwitchToLogin={handleSwitchToLogin} />
          </div>

          {/* Additional Info */}
          <div className="mt-8 text-center">
            <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-6">
              <h3 className="text-lg font-semibold text-white mb-2">Demo Account</h3>
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

  return (
    <div className="min-h-screen bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
      {/* Navigation */}
      <nav className="bg-gray-800/50 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                <UserIcon className="h-6 w-6 text-white" />
              </div>
              <span className="text-xl font-bold text-white">Sulwestake</span>
            </div>
            <div className="flex items-center space-x-4">
              <Link href="/landing" className="text-gray-300 hover:text-white px-3 py-2">
                <ArrowLeftIcon className="h-4 w-4 inline mr-1" />
                Back to Home
              </Link>
            </div>
          </div>
        </div>
      </nav>

      <div className="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        {/* Header Section */}
        <div className="text-center mb-12">
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-6">
            Welcome Back
          </h1>
          <p className="text-xl text-gray-300 max-w-3xl mx-auto">
            Sign in to your Sulwestake account and continue your staking journey. 
            Access your dashboard, manage your stakes, and track your rewards.
          </p>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 items-start">
          {/* Login Form */}
          <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-8">
            <div className="text-center mb-6">
              <h2 className="text-2xl font-semibold text-white mb-2">
                Sign In
              </h2>
              <p className="text-gray-400">
                Access your account and dashboard
              </p>
            </div>
            <LoginForm 
              onSwitchToRegister={handleSwitchToRegister} 
              onForgotPassword={() => {
                console.log('Forgot password clicked');
              }}
            />
          </div>

          {/* Features Section */}
          <div className="space-y-6">
            <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-6">
              <h3 className="text-xl font-semibold text-white mb-4">Platform Features</h3>
              <div className="space-y-4">
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">Dashboard Access</h4>
                    <p className="text-gray-400 text-sm">View your staking portfolio and performance metrics</p>
                  </div>
                </div>
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">Wallet Management</h4>
                    <p className="text-gray-400 text-sm">Create and manage multiple wallets for staking</p>
                  </div>
                </div>
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">Real-time Analytics</h4>
                    <p className="text-gray-400 text-sm">Track your rewards and network statistics</p>
                  </div>
                </div>
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">Secure Transactions</h4>
                    <p className="text-gray-400 text-sm">All operations are secured by blockchain technology</p>
                  </div>
                </div>
              </div>
            </div>

            {/* Demo Account */}
            <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-6">
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

            {/* Don't have account */}
            <div className="text-center">
              <p className="text-gray-300">
                Don't have an account?{' '}
                <button
                  onClick={handleSwitchToRegister}
                  className="text-indigo-400 hover:text-indigo-300 font-medium transition-colors duration-200"
                >
                  Create one here
                </button>
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default function LoginPage() {
  return (
    <AuthProvider>
      <LoginPageContent />
    </AuthProvider>
  );
} 