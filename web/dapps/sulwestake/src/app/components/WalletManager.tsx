'use client';

import { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import { 
  WalletIcon, 
  PlusIcon,
  KeyIcon,
  EyeIcon,
  EyeSlashIcon,
  CheckIcon,
  XMarkIcon
} from '@heroicons/react/24/outline';
import { WalletInfo } from '../utils/blockchain';

interface WalletManagerProps {
  blockchainConnected?: boolean;
}

export default function WalletManager({ blockchainConnected = false }: WalletManagerProps) {
  const [wallets, setWallets] = useState<WalletInfo[]>([]);
  const [isCreating, setIsCreating] = useState(false);
  const [showCreateForm, setShowCreateForm] = useState(false);
  const [newWalletName, setNewWalletName] = useState('');
  const [newWalletPassword, setNewWalletPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [selectedWallet, setSelectedWallet] = useState<WalletInfo | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  // Load existing wallets from localStorage
  useEffect(() => {
    const savedWallets = localStorage.getItem('nilotic_wallets');
    if (savedWallets) {
      try {
        setWallets(JSON.parse(savedWallets));
      } catch (error) {
        console.error('Error loading saved wallets:', error);
      }
    }
  }, []);

  // Save wallets to localStorage
  useEffect(() => {
    if (wallets.length > 0) {
      localStorage.setItem('nilotic_wallets', JSON.stringify(wallets));
    }
  }, [wallets]);

  const createWallet = async () => {
    if (!newWalletName.trim() || !newWalletPassword.trim()) {
      alert('Please enter both name and password');
      return;
    }

    if (!blockchainConnected) {
      alert('Please connect to blockchain first');
      return;
    }

    try {
      setIsCreating(true);
      const response = await fetch('/api/blockchain/wallet/create', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          name: newWalletName,
          password: newWalletPassword,
        }),
      });

      if (response.ok) {
        const data = await response.json();
        const newWallet: WalletInfo = {
          address: data.wallet.address,
          name: data.wallet.name,
          balance: data.wallet.balance,
          staked: data.wallet.staked,
          rewards: data.wallet.rewards,
          createdAt: data.wallet.createdAt,
          lastActivity: data.wallet.lastActivity,
        };

        setWallets(prev => [...prev, newWallet]);
        setSelectedWallet(newWallet);
        setShowCreateForm(false);
        setNewWalletName('');
        setNewWalletPassword('');
        
        // Show success message
        alert(`Wallet created successfully!\nAddress: ${newWallet.address}`);
      } else {
        const errorData = await response.json();
        alert(`Failed to create wallet: ${errorData.error}`);
      }
    } catch (error) {
      console.error('Error creating wallet:', error);
      alert('Failed to create wallet. Please try again.');
    } finally {
      setIsCreating(false);
    }
  };

  const refreshWalletInfo = async (address: string) => {
    try {
      setIsLoading(true);
      const response = await fetch(`/api/blockchain/wallet/${address}`);
      if (response.ok) {
        const walletInfo = await response.json();
        setWallets(prev => 
          prev.map(wallet => 
            wallet.address === address ? walletInfo : wallet
          )
        );
      }
    } catch (error) {
      console.error('Error refreshing wallet info:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
    alert('Copied to clipboard!');
  };

  const formatAddress = (address: string) => {
    return `${address.slice(0, 8)}...${address.slice(-6)}`;
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleDateString();
  };

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.5 }}
      className="space-y-6"
    >
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <WalletIcon className="w-8 h-8 text-emerald-500" />
          <h2 className="text-2xl font-bold text-white">Wallet Manager</h2>
        </div>
        
        <button
          onClick={() => setShowCreateForm(true)}
          disabled={!blockchainConnected || isCreating}
          className="flex items-center space-x-2 bg-emerald-600 hover:bg-emerald-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-white px-4 py-2 rounded-lg transition-colors"
        >
          <PlusIcon className="w-5 h-5" />
          <span>Create Wallet</span>
        </button>
      </div>

      {/* Create Wallet Form */}
      {showCreateForm && (
        <motion.div
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          exit={{ opacity: 0, y: -20 }}
          className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700"
        >
          <div className="flex items-center justify-between mb-4">
            <h3 className="text-lg font-semibold text-white">Create New Wallet</h3>
            <button
              onClick={() => setShowCreateForm(false)}
              className="text-gray-400 hover:text-white"
            >
              <XMarkIcon className="w-6 h-6" />
            </button>
          </div>

          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Wallet Name
              </label>
              <input
                type="text"
                value={newWalletName}
                onChange={(e) => setNewWalletName(e.target.value)}
                placeholder="Enter wallet name"
                className="w-full bg-gray-700 border border-gray-600 rounded-lg px-4 py-2 text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-emerald-500"
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Password
              </label>
              <div className="relative">
                <input
                  type={showPassword ? "text" : "password"}
                  value={newWalletPassword}
                  onChange={(e) => setNewWalletPassword(e.target.value)}
                  placeholder="Enter password"
                  className="w-full bg-gray-700 border border-gray-600 rounded-lg px-4 py-2 pr-10 text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-emerald-500"
                />
                <button
                  type="button"
                  onClick={() => setShowPassword(!showPassword)}
                  className="absolute right-3 top-1/2 transform -translate-y-1/2 text-gray-400 hover:text-white"
                >
                  {showPassword ? <EyeSlashIcon className="w-5 h-5" /> : <EyeIcon className="w-5 h-5" />}
                </button>
              </div>
            </div>

            <div className="flex space-x-3">
              <button
                onClick={createWallet}
                disabled={isCreating || !newWalletName.trim() || !newWalletPassword.trim()}
                className="flex-1 bg-emerald-600 hover:bg-emerald-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-white px-4 py-2 rounded-lg transition-colors flex items-center justify-center space-x-2"
              >
                {isCreating ? (
                  <>
                    <div className="animate-spin rounded-full h-4 w-4 border-b-2 border-white"></div>
                    <span>Creating...</span>
                  </>
                ) : (
                  <>
                    <CheckIcon className="w-5 h-5" />
                    <span>Create Wallet</span>
                  </>
                )}
              </button>
              
              <button
                onClick={() => setShowCreateForm(false)}
                className="px-4 py-2 border border-gray-600 text-gray-300 hover:text-white rounded-lg transition-colors"
              >
                Cancel
              </button>
            </div>
          </div>
        </motion.div>
      )}

      {/* Wallet List */}
      <div className="space-y-4">
        {wallets.length === 0 ? (
          <div className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-8 border border-gray-700 text-center">
            <WalletIcon className="w-16 h-16 text-gray-400 mx-auto mb-4" />
            <h3 className="text-lg font-semibold text-white mb-2">No Wallets</h3>
            <p className="text-gray-400 mb-4">
              Create your first wallet to start using the blockchain
            </p>
            <button
              onClick={() => setShowCreateForm(true)}
              disabled={!blockchainConnected}
              className="bg-emerald-600 hover:bg-emerald-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-white px-6 py-2 rounded-lg transition-colors"
            >
              Create Wallet
            </button>
          </div>
        ) : (
          wallets.map((wallet, index) => (
            <motion.div
              key={wallet.address}
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: index * 0.1 }}
              className={`bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700 cursor-pointer transition-all hover:border-emerald-500 ${
                selectedWallet?.address === wallet.address ? 'border-emerald-500 bg-emerald-900/20' : ''
              }`}
              onClick={() => setSelectedWallet(wallet)}
            >
              <div className="flex items-center justify-between">
                <div className="flex items-center space-x-4">
                  <div className="bg-emerald-600 rounded-full p-3">
                    <KeyIcon className="w-6 h-6 text-white" />
                  </div>
                  
                  <div>
                    <h3 className="text-lg font-semibold text-white">{wallet.name}</h3>
                    <p className="text-gray-400 font-mono text-sm">
                      {formatAddress(wallet.address)}
                    </p>
                    <p className="text-gray-500 text-xs">
                      Created: {formatDate(wallet.createdAt)}
                    </p>
                  </div>
                </div>

                <div className="text-right">
                  <div className="flex items-center space-x-2 mb-2">
                    <span className="text-gray-400 text-sm">Balance:</span>
                    <span className="text-white font-semibold">{wallet.balance.toFixed(2)} NIL</span>
                  </div>
                  
                  <div className="flex items-center space-x-4 text-sm">
                    <div>
                      <span className="text-gray-400">Staked:</span>
                      <span className="text-blue-400 ml-1">{wallet.staked.toFixed(2)} NIL</span>
                    </div>
                    <div>
                      <span className="text-gray-400">Rewards:</span>
                      <span className="text-green-400 ml-1">{wallet.rewards.toFixed(2)} NIL</span>
                    </div>
                  </div>
                </div>
              </div>

              {/* Action Buttons */}
              <div className="flex items-center justify-between mt-4 pt-4 border-t border-gray-700">
                <div className="flex space-x-2">
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      copyToClipboard(wallet.address);
                    }}
                    className="text-gray-400 hover:text-white text-sm flex items-center space-x-1"
                  >
                    <span>Copy Address</span>
                  </button>
                  
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      refreshWalletInfo(wallet.address);
                    }}
                    disabled={isLoading}
                    className="text-gray-400 hover:text-white text-sm flex items-center space-x-1 disabled:opacity-50"
                  >
                    <span>{isLoading ? 'Refreshing...' : 'Refresh'}</span>
                  </button>
                </div>

                <div className="text-xs text-gray-500">
                  Last activity: {formatDate(wallet.lastActivity)}
                </div>
              </div>
            </motion.div>
          ))
        )}
      </div>

      {/* Selected Wallet Details */}
      {selectedWallet && (
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          className="bg-gray-800/50 backdrop-blur-sm rounded-xl p-6 border border-gray-700"
        >
          <h3 className="text-lg font-semibold text-white mb-4">Wallet Details</h3>
          
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label className="block text-sm font-medium text-gray-400 mb-1">Address</label>
              <div className="flex items-center space-x-2">
                <span className="font-mono text-white text-sm">{selectedWallet.address}</span>
                <button
                  onClick={() => copyToClipboard(selectedWallet.address)}
                  className="text-emerald-500 hover:text-emerald-400"
                >
                  <span className="text-xs">Copy</span>
                </button>
              </div>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-400 mb-1">Name</label>
              <span className="text-white">{selectedWallet.name}</span>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-400 mb-1">Balance</label>
              <span className="text-white font-semibold">{selectedWallet.balance.toFixed(2)} NIL</span>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-400 mb-1">Staked</label>
              <span className="text-blue-400 font-semibold">{selectedWallet.staked.toFixed(2)} NIL</span>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-400 mb-1">Rewards</label>
              <span className="text-green-400 font-semibold">{selectedWallet.rewards.toFixed(2)} NIL</span>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-400 mb-1">Created</label>
              <span className="text-white">{formatDate(selectedWallet.createdAt)}</span>
            </div>
          </div>
        </motion.div>
      )}
    </motion.div>
  );
} 