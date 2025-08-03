'use client';

import React, { useState, useEffect } from 'react';
import Link from 'next/link';
import { WalletIcon, ChartBarIcon, CurrencyDollarIcon, UsersIcon, ClockIcon } from '@heroicons/react/24/outline';
import { blockchainAPI, stakingAPI } from '../utils/blockchain';

interface NetworkStats {
  chainHeight: number;
  totalBlocks: number;
  totalTransactions: number;
  pendingTransactions: number;
  difficulty: number;
  miningRate: number;
  networkHashRate: number;
  averageBlockTime: number;
  lastBlockHash: string;
  lastBlockTime: string;
}

interface TokenMetrics {
  totalSupply: number;
  circulatingSupply: number;
  availableForMining: number;
  totalMined: number;
  miningReward: number;
  totalStaked: number;
  totalStakers: number;
  averageAPY: number;
}

interface StakingMetrics {
  totalStaked: number;
  totalStakers: number;
  averageAPY: number;
  totalRewards: number;
  chainHeight: number;
  lastBlockHash: string;
}

export default function NetworkPage() {
  const [networkStats, setNetworkStats] = useState<NetworkStats | null>(null);
  const [tokenMetrics, setTokenMetrics] = useState<TokenMetrics | null>(null);
  const [stakingMetrics, setStakingMetrics] = useState<StakingMetrics | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [lastUpdate, setLastUpdate] = useState<Date>(new Date());

  useEffect(() => {
    const fetchNetworkData = async () => {
      try {
        setLoading(true);
        setError(null);

        const [analytics, stakingData] = await Promise.all([
          blockchainAPI.getBlockchainAnalytics(),
          stakingAPI.getStakingMetrics(),
        ]);

        setNetworkStats({
          chainHeight: analytics.chainHeight,
          totalBlocks: analytics.totalBlocks,
          totalTransactions: analytics.totalTransactions,
          pendingTransactions: analytics.pendingTransactions,
          difficulty: analytics.difficulty,
          miningRate: analytics.miningRate,
          networkHashRate: analytics.networkHashRate,
          averageBlockTime: analytics.averageBlockTime,
          lastBlockHash: analytics.lastBlockHash,
          lastBlockTime: analytics.lastBlockTime,
        });

        setTokenMetrics(analytics.tokenMetrics);
        setStakingMetrics(stakingData);
        setLastUpdate(new Date());
      } catch (error) {
        console.error('Error fetching network data:', error);
        setError('Failed to load network statistics');
      } finally {
        setLoading(false);
      }
    };

    fetchNetworkData();

    // Refresh data every 30 seconds
    const interval = setInterval(fetchNetworkData, 30000);
    return () => clearInterval(interval);
  }, []);

  const formatNumber = (num: number) => {
    return new Intl.NumberFormat().format(num);
  };

  const formatHashRate = (hashRate: number) => {
    if (hashRate >= 1e9) return `${(hashRate / 1e9).toFixed(2)} GH/s`;
    if (hashRate >= 1e6) return `${(hashRate / 1e6).toFixed(2)} MH/s`;
    if (hashRate >= 1e3) return `${(hashRate / 1e3).toFixed(2)} KH/s`;
    return `${hashRate.toFixed(2)} H/s`;
  };

  const formatTime = (seconds: number) => {
    if (seconds < 60) return `${seconds.toFixed(1)}s`;
    const minutes = Math.floor(seconds / 60);
    return `${minutes}m ${(seconds % 60).toFixed(0)}s`;
  };

  if (loading) {
    return (
      <div className="min-h-screen bg-gray-900 flex items-center justify-center">
        <div className="text-center">
          <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-indigo-500 mx-auto"></div>
          <p className="text-white mt-4">Loading network statistics...</p>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="min-h-screen bg-gray-900 flex items-center justify-center">
        <div className="text-center">
          <div className="text-red-500 text-xl mb-4">⚠️</div>
          <p className="text-white mb-4">{error}</p>
          <button
            onClick={() => window.location.reload()}
            className="px-4 py-2 bg-indigo-600 text-white rounded hover:bg-indigo-700"
          >
            Try Again
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-900">
      {/* Navigation */}
      <nav className="bg-gray-800 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <Link href="/" className="flex items-center space-x-2">
                <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                  <WalletIcon className="h-6 w-6 text-white" />
                </div>
                <span className="text-xl font-bold text-white">Sulwestake</span>
              </Link>
            </div>
            <div className="flex items-center space-x-4">
              <Link href="/" className="text-gray-300 hover:text-white px-3 py-2">
                Home
              </Link>
              <Link href="/about" className="text-gray-300 hover:text-white px-3 py-2">
                About
              </Link>
              <Link
                href="/get-started"
                className="bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg"
              >
                Get Started
              </Link>
            </div>
          </div>
        </div>
      </nav>

      {/* Header */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <div className="flex justify-between items-center mb-8">
          <div>
            <h1 className="text-3xl font-bold text-white mb-2">Network Statistics</h1>
            <p className="text-gray-400">
              Real-time data from the Nilotic blockchain network
            </p>
          </div>
          <div className="flex items-center space-x-2 text-sm text-gray-400">
            <ClockIcon className="h-4 w-4" />
            <span>Last updated: {lastUpdate.toLocaleTimeString()}</span>
          </div>
        </div>

        {/* Network Overview */}
        {networkStats && (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="flex items-center justify-between mb-4">
                <div className="h-10 w-10 bg-blue-600 rounded-full flex items-center justify-center">
                  <ChartBarIcon className="h-5 w-5 text-white" />
                </div>
                <span className="text-green-400 text-sm">Live</span>
              </div>
              <h3 className="text-2xl font-bold text-white">{formatNumber(networkStats.chainHeight)}</h3>
              <p className="text-gray-400">Block Height</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="flex items-center justify-between mb-4">
                <div className="h-10 w-10 bg-green-600 rounded-full flex items-center justify-center">
                  <CurrencyDollarIcon className="h-5 w-5 text-white" />
                </div>
                <span className="text-green-400 text-sm">Active</span>
              </div>
              <h3 className="text-2xl font-bold text-white">{formatNumber(networkStats.totalTransactions)}</h3>
              <p className="text-gray-400">Total Transactions</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="flex items-center justify-between mb-4">
                <div className="h-10 w-10 bg-yellow-600 rounded-full flex items-center justify-center">
                  <UsersIcon className="h-5 w-5 text-white" />
                </div>
                <span className="text-yellow-400 text-sm">{networkStats.difficulty}</span>
              </div>
              <h3 className="text-2xl font-bold text-white">{formatHashRate(networkStats.networkHashRate)}</h3>
              <p className="text-gray-400">Network Hash Rate</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="flex items-center justify-between mb-4">
                <div className="h-10 w-10 bg-purple-600 rounded-full flex items-center justify-center">
                  <ClockIcon className="h-5 w-5 text-white" />
                </div>
                <span className="text-purple-400 text-sm">Avg</span>
              </div>
              <h3 className="text-2xl font-bold text-white">{formatTime(networkStats.averageBlockTime)}</h3>
              <p className="text-gray-400">Block Time</p>
            </div>
          </div>
        )}

        {/* Token Metrics */}
        {tokenMetrics && (
          <div className="mb-8">
            <h2 className="text-2xl font-bold text-white mb-6">SLW Token Metrics</h2>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Total Supply</h3>
                <p className="text-2xl font-bold text-indigo-400">{formatNumber(tokenMetrics.totalSupply)}</p>
                <p className="text-gray-400">SLW Tokens</p>
              </div>
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Circulating Supply</h3>
                <p className="text-2xl font-bold text-green-400">{formatNumber(tokenMetrics.circulatingSupply)}</p>
                <p className="text-gray-400">SLW Tokens</p>
              </div>
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Total Mined</h3>
                <p className="text-2xl font-bold text-yellow-400">{formatNumber(tokenMetrics.totalMined)}</p>
                <p className="text-gray-400">SLW Tokens</p>
              </div>
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Available for Mining</h3>
                <p className="text-2xl font-bold text-purple-400">{formatNumber(tokenMetrics.availableForMining)}</p>
                <p className="text-gray-400">SLW Tokens</p>
              </div>
            </div>
          </div>
        )}

        {/* Staking Metrics */}
        {stakingMetrics && (
          <div className="mb-8">
            <h2 className="text-2xl font-bold text-white mb-6">Staking Statistics</h2>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Total Staked</h3>
                <p className="text-2xl font-bold text-blue-400">{formatNumber(stakingMetrics.totalStaked)}</p>
                <p className="text-gray-400">SLW Tokens</p>
              </div>
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Active Stakers</h3>
                <p className="text-2xl font-bold text-green-400">{formatNumber(stakingMetrics.totalStakers)}</p>
                <p className="text-gray-400">Users</p>
              </div>
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Average APY</h3>
                <p className="text-2xl font-bold text-yellow-400">{stakingMetrics.averageAPY.toFixed(2)}%</p>
                <p className="text-gray-400">Annual Yield</p>
              </div>
              <div className="bg-gray-800 rounded-lg p-6">
                <h3 className="text-lg font-semibold text-white mb-2">Total Rewards</h3>
                <p className="text-2xl font-bold text-purple-400">{formatNumber(stakingMetrics.totalRewards)}</p>
                <p className="text-gray-400">SLW Distributed</p>
              </div>
            </div>
          </div>
        )}

        {/* Network Details */}
        {networkStats && (
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
            <div className="bg-gray-800 rounded-lg p-6">
              <h3 className="text-xl font-semibold text-white mb-4">Network Information</h3>
              <div className="space-y-3">
                <div className="flex justify-between">
                  <span className="text-gray-400">Total Blocks:</span>
                  <span className="text-white">{formatNumber(networkStats.totalBlocks)}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Pending Transactions:</span>
                  <span className="text-white">{formatNumber(networkStats.pendingTransactions)}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Mining Rate:</span>
                  <span className="text-white">{networkStats.miningRate.toFixed(2)} blocks/min</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Network Difficulty:</span>
                  <span className="text-white">{networkStats.difficulty}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Last Block Time:</span>
                  <span className="text-white">{new Date(networkStats.lastBlockTime).toLocaleString()}</span>
                </div>
              </div>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <h3 className="text-xl font-semibold text-white mb-4">Latest Block</h3>
              <div className="space-y-3">
                <div>
                  <span className="text-gray-400 text-sm">Block Hash:</span>
                  <p className="text-white font-mono text-sm break-all">{networkStats.lastBlockHash}</p>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Block Height:</span>
                  <span className="text-white">{formatNumber(networkStats.chainHeight)}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Total Transactions:</span>
                  <span className="text-white">{formatNumber(networkStats.totalTransactions)}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Average Block Time:</span>
                  <span className="text-white">{formatTime(networkStats.averageBlockTime)}</span>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* CTA Section */}
        <div className="mt-12 text-center">
          <h2 className="text-2xl font-bold text-white mb-4">Ready to Participate?</h2>
          <p className="text-gray-400 mb-6">
            Join the network and start earning rewards through staking.
          </p>
          <div className="flex flex-col sm:flex-row gap-4 justify-center">
            <Link
              href="/get-started"
              className="bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-3 rounded-lg font-medium"
            >
              Start Staking
            </Link>
            <Link
              href="/about"
              className="border border-gray-600 text-gray-300 hover:text-white px-6 py-3 rounded-lg font-medium"
            >
              Learn More
            </Link>
          </div>
        </div>
      </div>
    </div>
  );
} 