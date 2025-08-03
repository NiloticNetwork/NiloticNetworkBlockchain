'use client';

import React, { useState, useEffect } from 'react';
import Link from 'next/link';
import { useAuth } from '../../contexts/AuthContext';
import { UserProfile, WalletInfo, Transaction } from '../../types/user';
import CreateWalletModal from './CreateWalletModal';
import ImportWalletModal from './ImportWalletModal';
import SendModal from './SendModal';
import ReceiveModal from './ReceiveModal';
import {
  UserIcon,
  WalletIcon,
  ChartBarIcon,
  CogIcon,
  BellIcon,
  ArrowRightIcon,
  PlusIcon,
  EyeIcon,
  EyeSlashIcon,
  KeyIcon,
} from '@heroicons/react/24/outline';

interface UserDashboardProps {
  blockchainConnected: boolean;
}

export default function UserDashboard({ blockchainConnected }: UserDashboardProps) {
  const { user, logout } = useAuth();
  const [userProfile, setUserProfile] = useState<UserProfile | null>(null);
  const [userWallets, setUserWallets] = useState<any[]>([]);
  const [loading, setLoading] = useState(true);
  const [showBalance, setShowBalance] = useState(true);
  const [activeTab, setActiveTab] = useState<'overview' | 'wallets' | 'transactions' | 'profile'>('overview');
  const [error, setError] = useState<string | null>(null);
  const [showCreateWalletModal, setShowCreateWalletModal] = useState(false);
  const [showImportWalletModal, setShowImportWalletModal] = useState(false);
  const [showSendModal, setShowSendModal] = useState(false);
  const [showReceiveModal, setShowReceiveModal] = useState(false);
  const [selectedWallet, setSelectedWallet] = useState<any>(null);
  const [syncStatus, setSyncStatus] = useState<any>(null);
  const [isSyncing, setIsSyncing] = useState(false);
  const [lastSync, setLastSync] = useState<Date | null>(null);

  const fetchUserProfile = async () => {
    if (!user) return;
    
    try {
      setLoading(true);
      setError(null);
      
      const [profileResponse, walletsResponse] = await Promise.all([
        fetch('/api/auth/profile', {
          headers: {
            'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
          },
        }),
        fetch('/api/auth/wallets', {
          headers: {
            'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
          },
        }),
      ]);

      if (!profileResponse.ok) {
        throw new Error('Failed to fetch user profile');
      }

      const profile: UserProfile = await profileResponse.json();
      setUserProfile(profile);

      if (walletsResponse.ok) {
        const walletsData = await walletsResponse.json();
        setUserWallets(walletsData.wallets || []);
      } else {
        console.error('Failed to fetch wallets:', walletsResponse.status, walletsResponse.statusText);
      }
    } catch (error) {
      console.error('Error fetching user data:', error);
      setError('Failed to load user data. Please try again.');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchUserProfile();
  }, [user]);

  // Sync management functions
  const startSync = async () => {
    if (!user) return;
    
    try {
      setIsSyncing(true);
      const response = await fetch('/api/blockchain/sync', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
        },
        body: JSON.stringify({
          action: 'start',
          userId: user.id,
        }),
      });

      if (response.ok) {
        console.log('Sync started successfully');
        // Refresh data after starting sync
        setTimeout(() => {
          fetchUserProfile();
        }, 2000);
      } else {
        throw new Error('Failed to start sync');
      }
    } catch (error) {
      console.error('Error starting sync:', error);
      setError('Failed to start sync');
    } finally {
      setIsSyncing(false);
    }
  };

  const forceSync = async () => {
    if (!user) return;
    
    try {
      setIsSyncing(true);
      const response = await fetch('/api/blockchain/sync', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
        },
        body: JSON.stringify({
          action: 'force',
          userId: user.id,
        }),
      });

      if (response.ok) {
        console.log('Force sync completed');
        setLastSync(new Date());
        // Refresh data after force sync
        setTimeout(() => {
          fetchUserProfile();
        }, 1000);
      } else {
        throw new Error('Failed to force sync');
      }
    } catch (error) {
      console.error('Error force syncing:', error);
      setError('Failed to force sync');
    } finally {
      setIsSyncing(false);
    }
  };

  const getSyncStatus = async () => {
    if (!user) return;
    
    try {
      const response = await fetch(`/api/blockchain/sync?userId=${user.id}`, {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
        },
      });

      if (response.ok) {
        const data = await response.json();
        setSyncStatus(data.status);
        if (data.status?.lastSync) {
          setLastSync(new Date(data.status.lastSync));
        }
      }
    } catch (error) {
      console.error('Error getting sync status:', error);
    }
  };

  const manualSyncWallets = async () => {
    if (!user || !userWallets.length) return;
    
    try {
      setIsSyncing(true);
      setError(null);

      // Sync each wallet manually
      for (const wallet of userWallets) {
        console.log(`Manually syncing wallet: ${wallet.address}`);
        
        const response = await fetch('/api/blockchain/manual-sync', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
          },
          body: JSON.stringify({
            address: wallet.address,
            userId: user.id,
          }),
        });

        if (response.ok) {
          const result = await response.json();
          console.log(`Manual sync result for ${wallet.address}:`, result);
        } else {
          console.error(`Failed to sync wallet ${wallet.address}`);
        }
      }

      // Refresh user data after manual sync
      setTimeout(() => {
        fetchUserProfile();
      }, 1000);

    } catch (error) {
      console.error('Error during manual sync:', error);
      setError('Manual sync failed. Please try again.');
    } finally {
      setIsSyncing(false);
    }
  };

  // Monitor sync status
  useEffect(() => {
    if (user) {
      getSyncStatus();
      // Check sync status every 10 seconds
      const interval = setInterval(getSyncStatus, 10000);
      return () => clearInterval(interval);
    }
  }, [user]);

  const handleWalletCreated = async (newWallet: any) => {
    // Refresh the user profile to include the new wallet
    await fetchUserProfile();
  };

  const handleWalletImported = async (importedWallet: any) => {
    // Refresh the user profile to include the imported wallet
    await fetchUserProfile();
  };

  const handleSendClick = (wallet: any) => {
    setSelectedWallet(wallet);
    setShowSendModal(true);
  };

  const handleReceiveClick = (wallet: any) => {
    setSelectedWallet(wallet);
    setShowReceiveModal(true);
  };

  const handleTransactionSent = async (transaction: any) => {
    console.log('Transaction sent:', transaction);
    // Refresh user data to show updated balances
    setTimeout(() => {
      fetchUserProfile();
    }, 2000);
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center min-h-screen">
        <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-indigo-600"></div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-indigo-900 via-purple-900 to-pink-900">
        <div className="text-center max-w-md mx-auto p-8">
          <div className="bg-red-900/50 border border-red-500 text-red-200 px-4 py-3 rounded-md mb-4">
            {error}
          </div>
          <button
            onClick={() => window.location.reload()}
            className="px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-md"
          >
            Retry
          </button>
        </div>
      </div>
    );
  }

  if (!userProfile) {
    return (
      <div className="text-center py-12">
        <p className="text-gray-500">Unable to load user profile</p>
      </div>
    );
  }

  const formatAmount = (amount: number | undefined | null) => {
    if (amount === undefined || amount === null) {
      return showBalance ? '0.00 NIL' : '••••••';
    }
    return showBalance ? `${amount.toFixed(2)} NIL` : '••••••';
  };

  const formatAddress = (address: string) => {
    return `${address.slice(0, 8)}...${address.slice(-6)}`;
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'confirmed':
        return 'text-green-400';
      case 'pending':
        return 'text-yellow-400';
      case 'failed':
        return 'text-red-400';
      default:
        return 'text-gray-400';
    }
  };

  const getStakingLevelColor = (level: string) => {
    switch (level) {
      case 'platinum':
        return 'text-purple-400';
      case 'gold':
        return 'text-yellow-400';
      case 'silver':
        return 'text-gray-400';
      case 'bronze':
        return 'text-orange-400';
      default:
        return 'text-gray-400';
    }
  };

  return (
    <div className="min-h-screen bg-gray-900">
      {/* Header */}
      <div className="bg-gray-800 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <div className="flex-shrink-0">
                <img
                  className="h-10 w-10 rounded-full"
                  src={userProfile.user?.avatar || '/default-avatar.png'}
                  alt={userProfile.user?.firstName || 'User'}
                />
              </div>
              <div>
                <h1 className="text-xl font-semibold text-white">
                  Welcome back, {userProfile.user?.firstName || 'User'}!
                </h1>
                <p className="text-sm text-gray-400">
                  {userProfile.stats?.accountAge || 0} days with Sulwestake
                </p>
              </div>
            </div>
            <div className="flex items-center space-x-4">
              {/* Blockchain Connection Status */}
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${blockchainConnected ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span className="text-sm text-gray-400">
                  {blockchainConnected ? 'Connected' : 'Disconnected'}
                </span>
              </div>

              {/* Sync Status */}
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${syncStatus?.isSyncing ? 'bg-yellow-500' : syncStatus ? 'bg-green-500' : 'bg-gray-500'}`}></div>
                <span className="text-sm text-gray-400">
                  {syncStatus?.isSyncing ? 'Syncing...' : syncStatus ? 'Synced' : 'Not Synced'}
                </span>
                {lastSync && (
                  <span className="text-xs text-gray-500">
                    {new Date(lastSync).toLocaleTimeString()}
                  </span>
                )}
              </div>

              {/* Sync Controls */}
              <div className="flex items-center space-x-2">
                <button
                  onClick={startSync}
                  disabled={isSyncing}
                  className="px-2 py-1 text-xs bg-indigo-600 hover:bg-indigo-700 disabled:opacity-50 text-white rounded"
                >
                  {isSyncing ? 'Starting...' : 'Start Sync'}
                </button>
                <button
                  onClick={forceSync}
                  disabled={isSyncing}
                  className="px-2 py-1 text-xs bg-green-600 hover:bg-green-700 disabled:opacity-50 text-white rounded"
                >
                  {isSyncing ? 'Syncing...' : 'Force Sync'}
                </button>
                <button
                  onClick={manualSyncWallets}
                  disabled={isSyncing}
                  className="px-2 py-1 text-xs bg-purple-600 hover:bg-purple-700 disabled:opacity-50 text-white rounded"
                >
                  {isSyncing ? 'Syncing...' : 'Manual Sync'}
                </button>
              </div>

              <Link href="/network" className="p-2 text-gray-400 hover:text-white">
                <ChartBarIcon className="h-6 w-6" />
              </Link>
              <button className="p-2 text-gray-400 hover:text-white">
                <BellIcon className="h-6 w-6" />
              </button>
              <button className="p-2 text-gray-400 hover:text-white">
                <CogIcon className="h-6 w-6" />
              </button>
              <button
                onClick={logout}
                className="px-4 py-2 text-sm font-medium text-white bg-red-600 hover:bg-red-700 rounded-md"
              >
                Logout
              </button>
            </div>
          </div>
        </div>
      </div>

      {/* Navigation Tabs */}
      <div className="bg-gray-800 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <nav className="flex space-x-8">
            {[
              { id: 'overview', name: 'Overview', icon: ChartBarIcon },
              { id: 'wallets', name: 'Wallets', icon: WalletIcon },
              { id: 'transactions', name: 'Transactions', icon: ChartBarIcon },
              { id: 'profile', name: 'Profile', icon: UserIcon },
            ].map((tab) => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id as any)}
                className={`py-4 px-1 border-b-2 font-medium text-sm flex items-center space-x-2 ${
                  activeTab === tab.id
                    ? 'border-indigo-500 text-indigo-400'
                    : 'border-transparent text-gray-400 hover:text-gray-300 hover:border-gray-300'
                }`}
              >
                <tab.icon className="h-5 w-5" />
                <span>{tab.name}</span>
              </button>
            ))}
          </nav>
        </div>
      </div>

      {/* Main Content */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {activeTab === 'overview' && (
          <div className="space-y-6">
            {/* Balance Overview */}
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="flex justify-between items-center mb-4">
                <h2 className="text-lg font-semibold text-white">Total Balance</h2>
                <button
                  onClick={() => setShowBalance(!showBalance)}
                  className="p-2 text-gray-400 hover:text-white"
                >
                  {showBalance ? <EyeSlashIcon className="h-5 w-5" /> : <EyeIcon className="h-5 w-5" />}
                </button>
              </div>
              <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
                <div className="bg-gray-700 rounded-lg p-4">
                  <div className="flex items-center justify-between">
                    <div>
                      <p className="text-sm text-gray-400">Available Balance</p>
                      <p className="text-2xl font-bold text-white">
                        {formatAmount(userProfile.stats?.totalBalance)}
                      </p>
                    </div>
                    <div className="h-12 w-12 bg-green-500 rounded-full flex items-center justify-center">
                      <WalletIcon className="h-6 w-6 text-white" />
                    </div>
                  </div>
                </div>
                <div className="bg-gray-700 rounded-lg p-4">
                  <div className="flex items-center justify-between">
                    <div>
                      <p className="text-sm text-gray-400">Total Staked</p>
                      <p className="text-2xl font-bold text-white">
                        {formatAmount(userProfile.stats?.totalStaked)}
                      </p>
                    </div>
                    <div className="h-12 w-12 bg-blue-500 rounded-full flex items-center justify-center">
                      <ChartBarIcon className="h-6 w-6 text-white" />
                    </div>
                  </div>
                </div>
                <div className="bg-gray-700 rounded-lg p-4">
                  <div className="flex items-center justify-between">
                    <div>
                      <p className="text-sm text-gray-400">Total Rewards</p>
                      <p className="text-2xl font-bold text-white">
                        {formatAmount(userProfile.stats?.totalRewards)}
                      </p>
                    </div>
                    <div className="h-12 w-12 bg-yellow-500 rounded-full flex items-center justify-center">
                      <ChartBarIcon className="h-6 w-6 text-white" />
                    </div>
                  </div>
                </div>
              </div>
            </div>

            {/* Staking Overview */}
            <div className="bg-gray-800 rounded-lg p-6">
              <h2 className="text-lg font-semibold text-white mb-4">Staking Overview</h2>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                <div className="space-y-4">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Staking Level</span>
                    <span className={`font-semibold ${getStakingLevelColor(userProfile.stakingData?.stakingLevel || 'bronze')}`}>
                      {(userProfile.stakingData?.stakingLevel || 'bronze').toUpperCase()}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">APY</span>
                    <span className="text-white font-semibold">{userProfile.stakingData?.apy || 0}%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Next Reward Estimate</span>
                    <span className="text-white font-semibold">
                      {formatAmount(userProfile.stakingData?.nextRewardEstimate)}
                    </span>
                  </div>
                </div>
                <div className="space-y-4">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Staking Start</span>
                    <span className="text-white">
                      {new Date(userProfile.stakingData?.stakingStartDate || Date.now()).toLocaleDateString()}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Last Reward</span>
                    <span className="text-white">
                      {new Date(userProfile.stakingData?.lastRewardDate || Date.now()).toLocaleDateString()}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Total Transactions</span>
                    <span className="text-white">{userProfile.stats?.totalTransactions || 0}</span>
                  </div>
                </div>
              </div>
            </div>

            {/* Recent Activity */}
            <div className="bg-gray-800 rounded-lg p-6">
              <h2 className="text-lg font-semibold text-white mb-4">Recent Activity</h2>
              {userProfile.recentTransactions?.length > 0 ? (
                <div className="space-y-3">
                  {userProfile.recentTransactions?.slice(0, 5).map((tx) => (
                    <div key={tx.hash} className="flex items-center justify-between p-3 bg-gray-700 rounded-lg">
                      <div className="flex items-center space-x-3">
                        <div className={`h-8 w-8 rounded-full flex items-center justify-center ${
                          tx.type === 'stake' ? 'bg-blue-500' : 'bg-green-500'
                        }`}>
                          <ChartBarIcon className="h-4 w-4 text-white" />
                        </div>
                        <div>
                          <p className="text-white font-medium">{tx.type.charAt(0).toUpperCase() + tx.type.slice(1)}</p>
                          <p className="text-sm text-gray-400">{formatAddress(tx.hash)}</p>
                        </div>
                      </div>
                      <div className="text-right">
                        <p className={`text-sm font-medium ${getStatusColor(tx.status)}`}>
                          {tx.status.toUpperCase()}
                        </p>
                        <p className="text-white">{formatAmount(tx.amount)}</p>
                      </div>
                    </div>
                  ))}
                </div>
              ) : (
                <div className="text-center py-8">
                  <p className="text-gray-400">No recent transactions</p>
                </div>
              )}
            </div>
          </div>
        )}

        {activeTab === 'wallets' && (
          <div className="space-y-6">
            {/* Wallet Header */}
            <div className="flex justify-between items-center">
              <div>
                <h2 className="text-2xl font-bold text-white">Your Wallets</h2>
                <p className="text-gray-400 mt-1">Manage your blockchain wallets and balances</p>
              </div>
              <div className="flex space-x-2">
                <button 
                  onClick={() => setShowCreateWalletModal(true)}
                  className="flex items-center space-x-2 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200 shadow-lg"
                >
                  <PlusIcon className="h-5 w-5" />
                  <span>Create Wallet</span>
                </button>
                <button 
                  onClick={() => setShowImportWalletModal(true)}
                  className="flex items-center space-x-2 px-4 py-2 bg-green-600 hover:bg-green-700 text-white rounded-lg transition-colors duration-200 shadow-lg"
                >
                  <KeyIcon className="h-5 w-5" />
                  <span>Import Wallet</span>
                </button>
              </div>
            </div>

            {/* Wallet Stats */}
            <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
              <div className="bg-gradient-to-r from-green-600 to-green-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-green-100">Total Balance</p>
                    <p className="text-2xl font-bold text-white">
                      {formatAmount(userProfile.stats?.totalBalance)}
                    </p>
                  </div>
                  <WalletIcon className="h-8 w-8 text-green-100" />
                </div>
              </div>
              <div className="bg-gradient-to-r from-blue-600 to-blue-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-blue-100">Total Staked</p>
                    <p className="text-2xl font-bold text-white">
                      {formatAmount(userProfile.stats?.totalStaked)}
                    </p>
                  </div>
                  <ChartBarIcon className="h-8 w-8 text-blue-100" />
                </div>
              </div>
              <div className="bg-gradient-to-r from-yellow-600 to-yellow-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-yellow-100">Total Rewards</p>
                    <p className="text-2xl font-bold text-white">
                      {formatAmount(userProfile.stats?.totalRewards)}
                    </p>
                  </div>
                  <ChartBarIcon className="h-8 w-8 text-yellow-100" />
                </div>
              </div>
                                <div className="bg-gradient-to-r from-purple-600 to-purple-700 rounded-lg p-4">
                    <div className="flex items-center justify-between">
                      <div>
                        <p className="text-sm text-purple-100">Active Wallets</p>
                        <p className="text-2xl font-bold text-white">
                          {userWallets.length}
                        </p>
                      </div>
                      <WalletIcon className="h-8 w-8 text-purple-100" />
                    </div>
                  </div>
            </div>



            {/* Wallets Grid */}
            {userWallets.length > 0 ? (
              <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
                {userWallets.map((wallet) => (
                                      <div key={wallet.id} className="bg-gray-800 rounded-xl p-6 border border-gray-700 hover:border-gray-600 transition-all duration-200 shadow-lg hover:shadow-xl">
                    {/* Wallet Header */}
                    <div className="flex items-center justify-between mb-4">
                      <div className="flex items-center space-x-3">
                        <div className="h-12 w-12 bg-gradient-to-r from-indigo-500 to-purple-600 rounded-full flex items-center justify-center">
                          <WalletIcon className="h-6 w-6 text-white" />
                        </div>
                        <div>
                          <h3 className="text-white font-semibold text-lg">{wallet.name}</h3>
                          <div className="flex items-center space-x-2">
                            <p className="text-sm text-gray-400 capitalize">{wallet.type}</p>
                            {wallet.type === 'imported' && (
                              <span className="px-2 py-1 text-xs bg-green-600 text-white rounded-full">
                                Imported
                              </span>
                            )}
                          </div>
                        </div>
                      </div>
                      {wallet.isPrimary && (
                        <span className="px-3 py-1 text-xs bg-green-600 text-white rounded-full font-medium">Primary</span>
                      )}
                    </div>

                    {/* Wallet Address */}
                    <div className="mb-4 p-3 bg-gray-700 rounded-lg">
                      <p className="text-xs text-gray-400 mb-1">Address</p>
                      <div className="flex items-center justify-between">
                        <code className="text-sm text-white font-mono">{formatAddress(wallet.address)}</code>
                        <button className="text-indigo-400 hover:text-indigo-300 text-xs">
                          Copy
                        </button>
                      </div>
                    </div>

                    {/* Wallet Balances */}
                    <div className="space-y-3 mb-4">
                      <div className="flex justify-between items-center p-3 bg-gray-700 rounded-lg">
                        <div className="flex items-center space-x-2">
                          <div className="h-3 w-3 bg-green-500 rounded-full"></div>
                          <span className="text-gray-300 text-sm">Available</span>
                        </div>
                        <span className="text-white font-semibold">{formatAmount(wallet.balance)}</span>
                      </div>
                      <div className="flex justify-between items-center p-3 bg-gray-700 rounded-lg">
                        <div className="flex items-center space-x-2">
                          <div className="h-3 w-3 bg-blue-500 rounded-full"></div>
                          <span className="text-gray-300 text-sm">Staked</span>
                        </div>
                        <span className="text-white font-semibold">{formatAmount(wallet.staked)}</span>
                      </div>
                      <div className="flex justify-between items-center p-3 bg-gray-700 rounded-lg">
                        <div className="flex items-center space-x-2">
                          <div className="h-3 w-3 bg-yellow-500 rounded-full"></div>
                          <span className="text-gray-300 text-sm">Rewards</span>
                        </div>
                        <span className="text-white font-semibold">{formatAmount(wallet.rewards)}</span>
                      </div>
                    </div>

                    {/* Wallet Actions */}
                    <div className="flex space-x-2">
                      <button 
                        onClick={() => handleSendClick(wallet)}
                        disabled={wallet.balance <= 0}
                        className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 disabled:opacity-50 disabled:cursor-not-allowed text-white rounded-lg transition-colors duration-200"
                      >
                        <span>Send</span>
                      </button>
                      <button 
                        onClick={() => handleReceiveClick(wallet)}
                        className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors duration-200"
                      >
                        <span>Receive</span>
                      </button>
                      <button className="px-3 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors duration-200">
                        <ArrowRightIcon className="h-4 w-4" />
                      </button>
                    </div>

                    {/* Last Activity */}
                    <div className="mt-4 pt-4 border-t border-gray-700">
                      <p className="text-xs text-gray-400">
                        Last activity: {new Date(wallet.lastActivity).toLocaleDateString()}
                      </p>
                    </div>
                  </div>
                ))}
              </div>
            ) : (
              <div className="text-center py-16">
                <div className="mx-auto h-24 w-24 bg-gray-700 rounded-full flex items-center justify-center mb-6">
                  <WalletIcon className="h-12 w-12 text-gray-400" />
                </div>
                <h3 className="text-xl font-medium text-white mb-2">No wallets found</h3>
                <p className="text-gray-400 mb-6 max-w-md mx-auto">
                  Create a new wallet or import an existing one to start managing your blockchain assets and participate in staking
                </p>
                <div className="flex justify-center space-x-4">
                  <button 
                    onClick={() => setShowCreateWalletModal(true)}
                    className="px-6 py-3 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200 shadow-lg"
                  >
                    Create New Wallet
                  </button>
                  <button 
                    onClick={() => setShowImportWalletModal(true)}
                    className="px-6 py-3 bg-green-600 hover:bg-green-700 text-white rounded-lg transition-colors duration-200 shadow-lg"
                  >
                    Import Existing Wallet
                  </button>
                </div>
              </div>
            )}
          </div>
        )}

        {activeTab === 'transactions' && (
          <div className="space-y-6">
            {/* Transaction Header */}
            <div className="flex justify-between items-center">
              <div>
                <h2 className="text-2xl font-bold text-white">Transaction History</h2>
                <p className="text-gray-400 mt-1">View all your blockchain transactions and activities</p>
              </div>
              <div className="flex space-x-2">
                <button className="px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors duration-200">
                  Export
                </button>
                <button className="px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200">
                  Filter
                </button>
              </div>
            </div>

            {/* Transaction Stats */}
            <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
              <div className="bg-gradient-to-r from-green-600 to-green-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-green-100">Total Transactions</p>
                    <p className="text-2xl font-bold text-white">
                      {userProfile.stats.totalTransactions}
                    </p>
                  </div>
                  <ChartBarIcon className="h-8 w-8 text-green-100" />
                </div>
              </div>
              <div className="bg-gradient-to-r from-blue-600 to-blue-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-blue-100">Successful</p>
                    <p className="text-2xl font-bold text-white">
                      {userProfile.recentTransactions.filter(tx => tx.status === 'confirmed').length}
                    </p>
                  </div>
                  <div className="h-8 w-8 bg-blue-100 rounded-full flex items-center justify-center">
                    <div className="h-4 w-4 bg-blue-600 rounded-full"></div>
                  </div>
                </div>
              </div>
              <div className="bg-gradient-to-r from-yellow-600 to-yellow-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-yellow-100">Pending</p>
                    <p className="text-2xl font-bold text-white">
                      {userProfile.recentTransactions.filter(tx => tx.status === 'pending').length}
                    </p>
                  </div>
                  <div className="h-8 w-8 bg-yellow-100 rounded-full flex items-center justify-center">
                    <div className="h-4 w-4 bg-yellow-600 rounded-full"></div>
                  </div>
                </div>
              </div>
              <div className="bg-gradient-to-r from-purple-600 to-purple-700 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="text-sm text-purple-100">This Month</p>
                    <p className="text-2xl font-bold text-white">
                      {userProfile.recentTransactions.filter(tx => {
                        const txDate = new Date(tx.timestamp);
                        const now = new Date();
                        return txDate.getMonth() === now.getMonth() && txDate.getFullYear() === now.getFullYear();
                      }).length}
                    </p>
                  </div>
                  <ChartBarIcon className="h-8 w-8 text-purple-100" />
                </div>
              </div>
            </div>

            {/* Transactions Table */}
            {userProfile.recentTransactions.length > 0 ? (
              <div className="bg-gray-800 rounded-xl overflow-hidden border border-gray-700">
                <div className="overflow-x-auto">
                  <table className="min-w-full divide-y divide-gray-700">
                    <thead className="bg-gray-700">
                      <tr>
                        <th className="px-6 py-4 text-left text-xs font-medium text-gray-300 uppercase tracking-wider">
                          Transaction
                        </th>
                        <th className="px-6 py-4 text-left text-xs font-medium text-gray-300 uppercase tracking-wider">
                          Amount
                        </th>
                        <th className="px-6 py-4 text-left text-xs font-medium text-gray-300 uppercase tracking-wider">
                          Status
                        </th>
                        <th className="px-6 py-4 text-left text-xs font-medium text-gray-300 uppercase tracking-wider">
                          Date
                        </th>
                        <th className="px-6 py-4 text-left text-xs font-medium text-gray-300 uppercase tracking-wider">
                          Hash
                        </th>
                        <th className="px-6 py-4 text-left text-xs font-medium text-gray-300 uppercase tracking-wider">
                          Actions
                        </th>
                      </tr>
                    </thead>
                    <tbody className="bg-gray-800 divide-y divide-gray-700">
                      {userProfile.recentTransactions.map((tx) => (
                        <tr key={tx.hash} className="hover:bg-gray-750 transition-colors duration-150">
                          <td className="px-6 py-4 whitespace-nowrap">
                            <div className="flex items-center">
                              <div className={`h-10 w-10 rounded-full flex items-center justify-center ${
                                tx.type === 'stake' ? 'bg-blue-500' : 
                                tx.type === 'unstake' ? 'bg-red-500' : 
                                tx.type === 'reward' ? 'bg-yellow-500' : 'bg-green-500'
                              }`}>
                                <ChartBarIcon className="h-5 w-5 text-white" />
                              </div>
                              <div className="ml-4">
                                <div className="text-sm font-medium text-white capitalize">
                                  {tx.type}
                                </div>
                                <div className="text-sm text-gray-400">
                                  {tx.from === userProfile.user.primaryWalletAddress ? 'Sent' : 'Received'}
                                </div>
                              </div>
                            </div>
                          </td>
                          <td className="px-6 py-4 whitespace-nowrap">
                            <div className="text-sm font-medium text-white">
                              {formatAmount(tx.amount)}
                            </div>
                            <div className="text-sm text-gray-400">
                              {tx.type === 'stake' ? 'Staked' : tx.type === 'unstake' ? 'Unstaked' : 'Reward'}
                            </div>
                          </td>
                          <td className="px-6 py-4 whitespace-nowrap">
                            <span className={`inline-flex px-3 py-1 text-xs font-semibold rounded-full ${getStatusColor(tx.status)}`}>
                              {tx.status.charAt(0).toUpperCase() + tx.status.slice(1)}
                            </span>
                          </td>
                          <td className="px-6 py-4 whitespace-nowrap">
                            <div className="text-sm text-white">
                              {new Date(tx.timestamp).toLocaleDateString()}
                            </div>
                            <div className="text-sm text-gray-400">
                              {new Date(tx.timestamp).toLocaleTimeString()}
                            </div>
                          </td>
                          <td className="px-6 py-4 whitespace-nowrap">
                            <div className="flex items-center space-x-2">
                              <code className="text-sm text-gray-300 font-mono">
                                {formatAddress(tx.hash)}
                              </code>
                              <button className="text-indigo-400 hover:text-indigo-300 text-xs">
                                Copy
                              </button>
                            </div>
                          </td>
                          <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-400">
                            <div className="flex space-x-2">
                              <button className="text-indigo-400 hover:text-indigo-300">
                                View
                              </button>
                              <button className="text-gray-400 hover:text-gray-300">
                                Details
                              </button>
                            </div>
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </div>
            ) : (
              <div className="text-center py-16">
                <div className="mx-auto h-24 w-24 bg-gray-700 rounded-full flex items-center justify-center mb-6">
                  <ChartBarIcon className="h-12 w-12 text-gray-400" />
                </div>
                <h3 className="text-xl font-medium text-white mb-2">No transactions found</h3>
                <p className="text-gray-400 mb-6 max-w-md mx-auto">
                  Your transaction history will appear here once you start making transactions on the blockchain
                </p>
                <div className="flex justify-center space-x-4">
                  <button className="px-6 py-3 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200">
                    Make Your First Transaction
                  </button>
                  <button className="px-6 py-3 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors duration-200">
                    Learn More
                  </button>
                </div>
              </div>
            )}
          </div>
        )}

        {activeTab === 'profile' && (
          <div className="space-y-6">
            <h2 className="text-lg font-semibold text-white">Profile Settings</h2>
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                <div>
                  <h3 className="text-lg font-medium text-white mb-4">Personal Information</h3>
                  <div className="space-y-4">
                    <div>
                      <label className="block text-sm font-medium text-gray-300">First Name</label>
                      <input
                        type="text"
                        defaultValue={userProfile.user.firstName}
                        className="mt-1 block w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500"
                      />
                    </div>
                    <div>
                      <label className="block text-sm font-medium text-gray-300">Last Name</label>
                      <input
                        type="text"
                        defaultValue={userProfile.user.lastName}
                        className="mt-1 block w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500"
                      />
                    </div>
                    <div>
                      <label className="block text-sm font-medium text-gray-300">Email</label>
                      <input
                        type="email"
                        defaultValue={userProfile.user.email}
                        className="mt-1 block w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500"
                      />
                    </div>
                    <div>
                      <label className="block text-sm font-medium text-gray-300">Username</label>
                      <input
                        type="text"
                        defaultValue={userProfile.user.username}
                        className="mt-1 block w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500"
                      />
                    </div>
                  </div>
                </div>
                <div>
                  <h3 className="text-lg font-medium text-white mb-4">Preferences</h3>
                  <div className="space-y-4">
                    <div>
                      <label className="block text-sm font-medium text-gray-300">Theme</label>
                      <select className="mt-1 block w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500">
                        <option value="dark">Dark</option>
                        <option value="light">Light</option>
                        <option value="auto">Auto</option>
                      </select>
                    </div>
                    <div>
                      <label className="block text-sm font-medium text-gray-300">Language</label>
                      <select className="mt-1 block w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500">
                        <option value="en">English</option>
                        <option value="es">Spanish</option>
                        <option value="fr">French</option>
                      </select>
                    </div>
                    <div className="space-y-2">
                      <label className="block text-sm font-medium text-gray-300">Notifications</label>
                      <div className="space-y-2">
                        <label className="flex items-center">
                          <input type="checkbox" defaultChecked className="h-4 w-4 text-indigo-600 focus:ring-indigo-500 border-gray-600 rounded bg-gray-700" />
                          <span className="ml-2 text-sm text-gray-300">Email notifications</span>
                        </label>
                        <label className="flex items-center">
                          <input type="checkbox" defaultChecked className="h-4 w-4 text-indigo-600 focus:ring-indigo-500 border-gray-600 rounded bg-gray-700" />
                          <span className="ml-2 text-sm text-gray-300">Push notifications</span>
                        </label>
                        <label className="flex items-center">
                          <input type="checkbox" className="h-4 w-4 text-indigo-600 focus:ring-indigo-500 border-gray-600 rounded bg-gray-700" />
                          <span className="ml-2 text-sm text-gray-300">SMS notifications</span>
                        </label>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
              <div className="mt-6 flex justify-end">
                <button className="px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-md">
                  Save Changes
                </button>
              </div>
            </div>
          </div>
        )}
      </div>

      {/* Create Wallet Modal */}
      <CreateWalletModal
        isOpen={showCreateWalletModal}
        onClose={() => setShowCreateWalletModal(false)}
        onWalletCreated={handleWalletCreated}
      />

      {/* Import Wallet Modal */}
      <ImportWalletModal
        isOpen={showImportWalletModal}
        onClose={() => setShowImportWalletModal(false)}
        onWalletImported={handleWalletImported}
      />

      {/* Send Modal */}
      {showSendModal && selectedWallet && (
        <SendModal
          isOpen={showSendModal}
          onClose={() => setShowSendModal(false)}
          wallet={selectedWallet}
          onTransactionSent={handleTransactionSent}
        />
      )}

      {/* Receive Modal */}
      {showReceiveModal && selectedWallet && (
        <ReceiveModal
          isOpen={showReceiveModal}
          onClose={() => setShowReceiveModal(false)}
          wallet={selectedWallet}
        />
      )}
    </div>
  );
} 