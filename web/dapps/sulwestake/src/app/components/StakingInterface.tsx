'use client';

import { useState } from 'react';
import { motion } from 'framer-motion';
import { LockClosedIcon, LockOpenIcon } from '@heroicons/react/24/outline';
import { ArrowUpIcon, ArrowDownIcon } from '@heroicons/react/24/solid';
import toast from 'react-hot-toast';
import { stakingAPI } from '../utils/blockchain';

interface StakingInterfaceProps {
  blockchainConnected?: boolean;
  onStake: (amount: number) => void;
  onUnstake: (amount: number) => void;
  onClaimRewards: () => void;
}

export default function StakingInterface({
  address,
  totalStaked,
  apy,
  onStake,
  onUnstake,
  isLoading,
  blockchainConnected = false
}: StakingInterfaceProps) {
  const [stakeAmount, setStakeAmount] = useState('');
  const [unstakeAmount, setUnstakeAmount] = useState('');

  const handleStake = async () => {
    if (!stakeAmount || parseFloat(stakeAmount) <= 0) {
      toast.error('Please enter a valid amount to stake');
      return;
    }
    
    if (!address) {
      toast.error('Please connect your wallet first');
      return;
    }

    try {
      toast.success(`Staking ${stakeAmount} NIL tokens...`);
      
      // Submit transaction to blockchain
      const transaction = await stakingAPI.stakeTokens(parseFloat(stakeAmount), address);
      console.log('Staking transaction:', transaction);
      
      onStake(parseFloat(stakeAmount));
      setStakeAmount('');
      toast.success('Tokens staked successfully!');
    } catch (error) {
      console.error('Error staking tokens:', error);
      toast.error('Failed to stake tokens');
    }
  };

  const handleUnstake = async () => {
    if (!unstakeAmount || parseFloat(unstakeAmount) <= 0) {
      toast.error('Please enter a valid amount to unstake');
      return;
    }
    
    if (!address) {
      toast.error('Please connect your wallet first');
      return;
    }

    if (parseFloat(unstakeAmount) > totalStaked) {
      toast.error('Cannot unstake more than you have staked');
      return;
    }

    try {
      toast.success(`Unstaking ${unstakeAmount} NIL tokens...`);
      
      // Submit transaction to blockchain
      const transaction = await stakingAPI.unstakeTokens(parseFloat(unstakeAmount), address);
      console.log('Unstaking transaction:', transaction);
      
      onUnstake(parseFloat(unstakeAmount));
      setUnstakeAmount('');
      toast.success('Tokens unstaked successfully!');
    } catch (error) {
      console.error('Error unstaking tokens:', error);
      toast.error('Failed to unstake tokens');
    }
  };

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.5 }}
      className="grid grid-cols-1 lg:grid-cols-2 gap-8"
    >
      {/* Stake */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="flex items-center space-x-2 mb-6">
          <LockClosedIcon className="w-6 h-6 text-emerald-500" />
          <h3 className="text-xl font-bold text-white">Stake Tokens</h3>
        </div>
        
        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-300 mb-2">
              Amount to Stake (NIL)
            </label>
            <input
              type="number"
              value={stakeAmount}
              onChange={(e) => setStakeAmount(e.target.value)}
              placeholder="0.0"
              disabled={isLoading}
              className="w-full bg-gray-700 border border-gray-600 rounded-lg px-4 py-3 text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-emerald-500 disabled:opacity-50"
            />
          </div>
          
          <div className="flex justify-between text-sm text-gray-400">
            <span>Available: 1000 NIL</span>
            <span>APY: {apy}%</span>
          </div>
          
          <button
            onClick={handleStake}
            disabled={isLoading || !stakeAmount}
            className="w-full bg-emerald-600 hover:bg-emerald-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-white font-semibold py-3 px-6 rounded-lg transition-colors flex items-center justify-center space-x-2"
          >
            <ArrowUpIcon className="w-5 h-5" />
            <span>{isLoading ? 'Processing...' : 'Stake Tokens'}</span>
          </button>
        </div>
      </div>

      {/* Unstake */}
      <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700">
        <div className="flex items-center space-x-2 mb-6">
          <LockOpenIcon className="w-6 h-6 text-red-500" />
          <h3 className="text-xl font-bold text-white">Unstake Tokens</h3>
        </div>
        
        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-300 mb-2">
              Amount to Unstake (NIL)
            </label>
            <input
              type="number"
              value={unstakeAmount}
              onChange={(e) => setUnstakeAmount(e.target.value)}
              placeholder="0.0"
              disabled={isLoading}
              className="w-full bg-gray-700 border border-gray-600 rounded-lg px-4 py-3 text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-red-500 disabled:opacity-50"
            />
          </div>
          
          <div className="flex justify-between text-sm text-gray-400">
            <span>Staked: {totalStaked} NIL</span>
            <span>Lock Period: 7 days</span>
          </div>
          
          <button
            onClick={handleUnstake}
            disabled={isLoading || !unstakeAmount}
            className="w-full bg-red-600 hover:bg-red-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-white font-semibold py-3 px-6 rounded-lg transition-colors flex items-center justify-center space-x-2"
          >
            <ArrowDownIcon className="w-5 h-5" />
            <span>{isLoading ? 'Processing...' : 'Unstake Tokens'}</span>
          </button>
        </div>
      </div>
    </motion.div>
  );
} 