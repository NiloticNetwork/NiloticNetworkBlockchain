'use client';

import { motion } from 'framer-motion';
import { 
  LockClosedIcon, 
  GiftIcon,
  FireIcon,
  StarIcon,
  UsersIcon,
  TrophyIcon
} from '@heroicons/react/24/outline';
import { XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, AreaChart, Area } from 'recharts';
import CountUp from 'react-countup';
import { BlockchainMetrics } from '../utils/blockchain';

interface DashboardProps {
  address?: string;
  blockchainConnected?: boolean;
}

// Mock data for demonstration
const mockStakingData = [
  { time: '00:00', rewards: 0 },
  { time: '04:00', rewards: 2.5 },
  { time: '08:00', rewards: 5.2 },
  { time: '12:00', rewards: 8.1 },
  { time: '16:00', rewards: 12.3 },
  { time: '20:00', rewards: 15.7 },
  { time: '24:00', rewards: 18.9 },
];

export default function Dashboard({
  totalStaked,
  totalRewards,
  apy,
  userLevel,
  blockchainMetrics,
  onClaimRewards,
  isLoading,
  address,
  blockchainConnected = false
}: DashboardProps) {
  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.5 }}
    >
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
        <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-gray-400 text-sm">Total Staked</p>
              <p className="text-2xl font-bold text-white">
                <CountUp end={totalStaked} decimals={2} /> NIL
              </p>
            </div>
            <LockClosedIcon className="w-8 h-8 text-emerald-500" />
          </div>
        </div>

        <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-gray-400 text-sm">Total Rewards</p>
              <p className="text-2xl font-bold text-white">
                <CountUp end={totalRewards} decimals={2} /> NIL
              </p>
            </div>
            <GiftIcon className="w-8 h-8 text-yellow-500" />
          </div>
        </div>

        <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-gray-400 text-sm">APY Rate</p>
              <p className="text-2xl font-bold text-white">{apy}%</p>
            </div>
            <FireIcon className="w-8 h-8 text-orange-500" />
          </div>
        </div>

        <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-gray-400 text-sm">Your Level</p>
              <p className="text-lg font-bold text-white">{userLevel}</p>
            </div>
            <StarIcon className="w-8 h-8 text-purple-500" />
          </div>
        </div>
      </div>

      {/* Blockchain Status */}
      {blockchainMetrics && (
        <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700 mb-8">
          <h3 className="text-xl font-bold text-white mb-4">Blockchain Status</h3>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-6">
            <div>
              <p className="text-gray-400 text-sm">Chain Height</p>
              <p className="text-2xl font-bold text-white">{blockchainMetrics.chainHeight}</p>
            </div>
            <div>
              <p className="text-gray-400 text-sm">Total Blocks</p>
              <p className="text-2xl font-bold text-white">{blockchainMetrics.totalBlocks}</p>
            </div>
            <div>
              <p className="text-gray-400 text-sm">Transactions</p>
              <p className="text-2xl font-bold text-white">{blockchainMetrics.totalTransactions}</p>
            </div>
            <div>
              <p className="text-gray-400 text-sm">Difficulty</p>
              <p className="text-2xl font-bold text-white">{blockchainMetrics.difficulty}</p>
            </div>
          </div>
        </div>
      )}

      {/* Rewards Chart */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700 mb-8">
        <h3 className="text-xl font-bold text-white mb-4">Rewards Over Time</h3>
        <div className="h-64">
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={mockStakingData}>
              <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
              <XAxis dataKey="time" stroke="#9CA3AF" />
              <YAxis stroke="#9CA3AF" />
              <Tooltip 
                contentStyle={{ 
                  backgroundColor: '#1f2937', 
                  border: '1px solid #374151',
                  borderRadius: '8px'
                }}
              />
              <Area 
                type="monotone" 
                dataKey="rewards" 
                stroke="#10b981" 
                fill="#10b981" 
                fillOpacity={0.3}
              />
            </AreaChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Quick Actions */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <button
          onClick={onClaimRewards}
          disabled={totalRewards <= 0 || isLoading}
          className="bg-emerald-600 hover:bg-emerald-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-white font-semibold py-3 px-6 rounded-lg transition-colors flex items-center justify-center space-x-2"
        >
          <GiftIcon className="w-5 h-5" />
          <span>{isLoading ? 'Processing...' : 'Claim Rewards'}</span>
        </button>
        
        <button className="bg-blue-600 hover:bg-blue-700 text-white font-semibold py-3 px-6 rounded-lg transition-colors flex items-center justify-center space-x-2">
          <UsersIcon className="w-5 h-5" />
          <span>Join Pool</span>
        </button>
        
        <button className="bg-purple-600 hover:bg-purple-700 text-white font-semibold py-3 px-6 rounded-lg transition-colors flex items-center justify-center space-x-2">
          <TrophyIcon className="w-5 h-5" />
          <span>View Challenges</span>
        </button>
      </div>
    </motion.div>
  );
} 