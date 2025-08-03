'use client';

import React, { useState, useEffect } from 'react';
import { useRouter } from 'next/navigation';
import { AuthProvider, useAuth } from '../contexts/AuthContext';
import LoginForm from '../components/auth/LoginForm';
import RegisterForm from '../components/auth/RegisterForm';
import UserDashboard from '../components/dashboard/UserDashboard';
import { blockchainAPI } from '../utils/blockchain';

type AuthMode = 'login' | 'register';

function DashboardContent() {
  const { isAuthenticated, user, loading } = useAuth();
  const router = useRouter();
  const [blockchainConnected, setBlockchainConnected] = useState(false);
  const [blockchainLoading, setBlockchainLoading] = useState(true);
  const [authMode, setAuthMode] = useState<AuthMode>('login');

  useEffect(() => {
    const checkBlockchainConnection = async () => {
      setBlockchainLoading(true);
      try {
        const isValid = await blockchainAPI.validateBlockchain();
        setBlockchainConnected(isValid);
      } catch (error) {
        console.error('Blockchain connection check failed:', error);
        setBlockchainConnected(false);
      } finally {
        setBlockchainLoading(false);
      }
    };

    checkBlockchainConnection();
  }, []);

  // Redirect to landing if not authenticated and not loading
  useEffect(() => {
    if (!loading && !isAuthenticated) {
      router.push('/landing');
    }
  }, [isAuthenticated, loading, router]);

  // Show loading screen while checking authentication and blockchain
  if (loading || blockchainLoading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
        <div className="text-center">
          <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-white mx-auto"></div>
          <p className="mt-4 text-white text-lg">
            {loading ? 'Loading application...' : 'Checking blockchain connection...'}
          </p>
          <p className="mt-2 text-gray-300 text-sm">
            Please wait while we set up your experience
          </p>
        </div>
      </div>
    );
  }

  // Show authentication forms if not authenticated
  if (!isAuthenticated) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
        {/* Blockchain Status Banner */}
        {!blockchainConnected && (
          <div className="bg-yellow-900/50 border-b border-yellow-500 text-yellow-200 px-4 py-2 text-center">
            <div className="flex items-center justify-center">
              <svg className="h-5 w-5 mr-2" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
              </svg>
              Blockchain connection unavailable. Some features may be limited.
            </div>
          </div>
        )}

        {authMode === 'login' ? (
          <LoginForm
            onSwitchToRegister={() => setAuthMode('register')}
            onForgotPassword={() => {
              // Handle forgot password
              alert('Forgot password functionality coming soon!');
            }}
          />
        ) : (
          <RegisterForm onSwitchToLogin={() => setAuthMode('login')} />
        )}
      </div>
    );
  }

  // Show user dashboard if authenticated
  return <UserDashboard blockchainConnected={blockchainConnected} />;
}

export default function DashboardPage() {
  return (
    <AuthProvider>
      <DashboardContent />
    </AuthProvider>
  );
} 