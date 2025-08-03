'use client';

import React, { useState } from 'react';
import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { UserPlusIcon, ArrowLeftIcon } from '@heroicons/react/24/outline';
import RegisterForm from '../components/auth/RegisterForm';
import LoginForm from '../components/auth/LoginForm';
import { AuthProvider } from '../contexts/AuthContext';

function RegisterPageContent() {
  const router = useRouter();
  const [showLogin, setShowLogin] = useState(false);

  const handleSwitchToLogin = () => {
    setShowLogin(true);
  };

  const handleSwitchToRegister = () => {
    setShowLogin(false);
  };

  const handleRegistrationSuccess = () => {
    // Redirect to get-started page for wallet setup
    setTimeout(() => {
      router.push('/get-started');
    }, 2000);
  };

  if (showLogin) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
        {/* Navigation */}
        <nav className="bg-gray-800/50 border-b border-gray-700">
          <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
            <div className="flex justify-between items-center py-4">
              <div className="flex items-center space-x-4">
                <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                  <UserPlusIcon className="h-6 w-6 text-white" />
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
              Welcome Back
            </h1>
            <p className="text-gray-300">
              Sign in to your Sulwestake account
            </p>
          </div>

          {/* Login Form */}
          <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-8">
            <LoginForm 
              onSwitchToRegister={handleSwitchToRegister} 
              onForgotPassword={() => {
                // Handle forgot password - could redirect to a dedicated page
                console.log('Forgot password clicked');
              }}
            />
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
                <UserPlusIcon className="h-6 w-6 text-white" />
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
            Create Your Account
          </h1>
          <p className="text-xl text-gray-300 max-w-3xl mx-auto">
            Join thousands of users already earning rewards on the Nilotic blockchain. 
            Create your account and start your staking journey today.
          </p>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 items-start">
          {/* Registration Form */}
          <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-8">
            <div className="text-center mb-6">
              <h2 className="text-2xl font-semibold text-white mb-2">
                Get Started
              </h2>
              <p className="text-gray-400">
                Create your account to access the platform
              </p>
            </div>
            <RegisterForm onSwitchToLogin={handleSwitchToLogin} />
          </div>

          {/* Benefits Section */}
          <div className="space-y-6">
            <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-6">
              <h3 className="text-xl font-semibold text-white mb-4">Why Choose Sulwestake?</h3>
              <div className="space-y-4">
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">Secure Staking</h4>
                    <p className="text-gray-400 text-sm">Your tokens are secured by advanced blockchain cryptography</p>
                  </div>
                </div>
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">High Rewards</h4>
                    <p className="text-gray-400 text-sm">Earn competitive APY rates with optimized staking algorithms</p>
                  </div>
                </div>
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">Instant Access</h4>
                    <p className="text-gray-400 text-sm">Stake and unstake instantly with real-time blockchain integration</p>
                  </div>
                </div>
                <div className="flex items-start space-x-3">
                  <div className="h-6 w-6 bg-green-600 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5">
                    <svg className="h-3 w-3 text-white" fill="currentColor" viewBox="0 0 20 20">
                      <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                    </svg>
                  </div>
                  <div>
                    <h4 className="text-white font-medium">User-Friendly</h4>
                    <p className="text-gray-400 text-sm">Intuitive interface designed for both beginners and experts</p>
                  </div>
                </div>
              </div>
            </div>

            {/* Demo Account */}
            <div className="bg-gray-800/30 rounded-lg border border-gray-700 p-6">
              <h3 className="text-lg font-semibold text-white mb-2">Try Demo Account</h3>
              <p className="text-gray-400 text-sm mb-4">
                Test the platform with our demo account before creating your own
              </p>
              <div className="bg-gray-700/50 rounded p-3 text-sm">
                <p className="text-gray-300">
                  <strong>Email:</strong> demo@nilotic.com<br />
                  <strong>Password:</strong> password123
                </p>
              </div>
            </div>

            {/* Already have account */}
            <div className="text-center">
              <p className="text-gray-300">
                Already have an account?{' '}
                <button
                  onClick={handleSwitchToLogin}
                  className="text-indigo-400 hover:text-indigo-300 font-medium transition-colors duration-200"
                >
                  Sign in here
                </button>
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default function RegisterPage() {
  return (
    <AuthProvider>
      <RegisterPageContent />
    </AuthProvider>
  );
} 