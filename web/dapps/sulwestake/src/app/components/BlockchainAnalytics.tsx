'use client';

import { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import { 
  ChartBarIcon, 
  CurrencyDollarIcon,
  CogIcon,
  ClockIcon,
  FireIcon,
  UserGroupIcon,
  TrendingUpIcon,
  CubeIcon
} from '@heroicons/react/24/outline';
import { BlockchainAnalytics, TokenMetrics } from '../utils/blockchain';

interface BlockchainAnalyticsProps {
  blockchainConnected: boolean;
}

export default function BlockchainAnalytics({ blockchainConnected }: BlockchainAnalyticsProps) {
  const [analytics, setAnalytics] = useState<BlockchainAnalytics | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    const fetchAnalytics = async () => {
      if (!blockchainConnected) return;
      
      try {
        setIsLoading(true);
        const response = await fetch('/api/blockchain/analytics');
        if (response.ok) {
          const data = await response.json();
          setAnalytics(data);
        }
      } catch (error) {
        console.error('Error fetching analytics:', error);
      } finally {
        setIsLoading(false);
      }
    };

    fetchAnalytics();
    const interval = setInterval(fetchAnalytics, 30000); // Update every 30 seconds
    
    return () => clearInterval(interval);
  }, [blockchainConnected]);

  if (!blockchainConnected) {
    return (
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="text-center">
          <CogIcon className="w-12 h-12 text-gray-400 mx-auto mb-4" />
          <h3 className="text-lg font-semibold text-white mb-2">Blockchain Analytics</h3>
          <p className="text-gray-400">Connect to blockchain to view analytics</p>
        </div>
      </div>
    );
  }

  if (isLoading) {
    return (
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="text-center">
          <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-emerald-500 mx-auto mb-4"></div>
          <p className="text-gray-400">Loading analytics...</p>
        </div>
      </div>
    );
  }

  if (!analytics) {
    return (
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="text-center">
          <ChartBarIcon className="w-12 h-12 text-gray-400 mx-auto mb-4" />
          <h3 className="text-lg font-semibold text-white mb-2">Blockchain Analytics</h3>
          <p className="text-gray-400">No analytics data available</p>
        </div>
      </div>
    );
  }

  const formatNumber = (num: number) => {
    return new Intl.NumberFormat().format(num);
  };

  const formatHashRate = (hashRate: number) => {
    if (hashRate >= 1000000) return `${(hashRate / 1000000).toFixed(2)} MH/s`;
    if (hashRate >= 1000) return `${(hashRate / 1000).toFixed(2)} KH/s`;
    return `${hashRate.toFixed(2)} H/s`;
  };

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.5 }}
      className="space-y-6"
    >
      {/* Blockchain Overview */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="flex items-center space-x-2 mb-6">
          <ChartBarIcon className="w-6 h-6 text-emerald-500" />
          <h3 className="text-xl font-bold text-white">Blockchain Overview</h3>
        </div>
        
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <CubeIcon className="w-5 h-5 text-blue-500" />
              <span className="text-gray-400 text-sm">Chain Height</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.chainHeight)}</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <CogIcon className="w-5 h-5 text-orange-500" />
              <span className="text-gray-400 text-sm">Difficulty</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.difficulty)}</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <ClockIcon className="w-5 h-5 text-purple-500" />
              <span className="text-gray-400 text-sm">Block Time</span>
            </div>
            <p className="text-2xl font-bold text-white">{analytics.averageBlockTime}s</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <FireIcon className="w-5 h-5 text-red-500" />
              <span className="text-gray-400 text-sm">Hash Rate</span>
            </div>
            <p className="text-lg font-bold text-white">{formatHashRate(analytics.networkHashRate)}</p>
          </div>
        </div>
      </div>

      {/* Token Metrics */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="flex items-center space-x-2 mb-6">
          <CurrencyDollarIcon className="w-6 h-6 text-yellow-500" />
          <h3 className="text-xl font-bold text-white">Token Metrics</h3>
        </div>
        
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <TrendingUpIcon className="w-5 h-5 text-green-500" />
              <span className="text-gray-400 text-sm">Total Supply</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.tokenMetrics.totalSupply)} NIL</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <CurrencyDollarIcon className="w-5 h-5 text-blue-500" />
              <span className="text-gray-400 text-sm">Circulating</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.tokenMetrics.circulatingSupply)} NIL</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <CogIcon className="w-5 h-5 text-orange-500" />
              <span className="text-gray-400 text-sm">Available for Mining</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.tokenMetrics.availableForMining)} NIL</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <FireIcon className="w-5 h-5 text-red-500" />
              <span className="text-gray-400 text-sm">Total Mined</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.tokenMetrics.totalMined)} NIL</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <UserGroupIcon className="w-5 h-5 text-purple-500" />
              <span className="text-gray-400 text-sm">Total Stakers</span>
            </div>
            <p className="text-2xl font-bold text-white">{formatNumber(analytics.tokenMetrics.totalStakers)}</p>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center space-x-2">
              <TrendingUpIcon className="w-5 h-5 text-green-500" />
              <span className="text-gray-400 text-sm">Average APY</span>
            </div>
            <p className="text-2xl font-bold text-white">{analytics.tokenMetrics.averageAPY.toFixed(2)}%</p>
          </div>
        </div>
      </div>

      {/* Network Statistics */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="flex items-center space-x-2 mb-6">
          <ChartBarIcon className="w-6 h-6 text-emerald-500" />
          <h3 className="text-xl font-bold text-white">Network Statistics</h3>
        </div>
        
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-gray-400 text-sm">Total Transactions</p>
                <p className="text-2xl font-bold text-white">{formatNumber(analytics.totalTransactions)}</p>
              </div>
              <div className="text-right">
                <p className="text-gray-400 text-sm">Pending</p>
                <p className="text-lg font-semibold text-orange-500">{formatNumber(analytics.pendingTransactions)}</p>
              </div>
            </div>
          </div>
          
          <div className="bg-gray-700/50 rounded-lg p-4">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-gray-400 text-sm">Mining Rate</p>
                <p className="text-2xl font-bold text-white">{analytics.miningRate.toFixed(2)} blocks/min</p>
              </div>
              <div className="text-right">
                <p className="text-gray-400 text-sm">Reward</p>
                <p className="text-lg font-semibold text-green-500">{analytics.tokenMetrics.miningReward} NIL</p>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Last Block Info */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="flex items-center space-x-2 mb-4">
          <CubeIcon className="w-6 h-6 text-blue-500" />
          <h3 className="text-lg font-bold text-white">Latest Block</h3>
        </div>
        
        <div className="space-y-2">
          <div className="flex justify-between">
            <span className="text-gray-400">Hash:</span>
            <span className="text-white font-mono text-sm">{analytics.lastBlockHash.slice(0, 20)}...</span>
          </div>
          <div className="flex justify-between">
            <span className="text-gray-400">Time:</span>
            <span className="text-white">{new Date(analytics.lastBlockTime).toLocaleString()}</span>
          </div>
        </div>
      </div>
    </motion.div>
  );
} 