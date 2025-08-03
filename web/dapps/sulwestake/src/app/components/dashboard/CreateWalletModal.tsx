'use client';

import React, { useState } from 'react';
import { XMarkIcon, WalletIcon, EyeIcon, EyeSlashIcon, ClipboardIcon, DocumentArrowDownIcon, CheckIcon } from '@heroicons/react/24/outline';

interface CreateWalletModalProps {
  isOpen: boolean;
  onClose: () => void;
  onWalletCreated: (wallet: any) => void;
}

export default function CreateWalletModal({ isOpen, onClose, onWalletCreated }: CreateWalletModalProps) {
  const [formData, setFormData] = useState({
    name: '',
    password: '',
    confirmPassword: '',
  });
  const [showPassword, setShowPassword] = useState(false);
  const [showConfirmPassword, setShowConfirmPassword] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [walletCreated, setWalletCreated] = useState(false);
  const [createdWallet, setCreatedWallet] = useState<any>(null);
  const [seedPhraseCopied, setSeedPhraseCopied] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    
    if (formData.password !== formData.confirmPassword) {
      setError('Passwords do not match');
      return;
    }

    if (formData.password.length < 6) {
      setError('Password must be at least 6 characters long');
      return;
    }

    setLoading(true);
    setError(null);
    setSuccess(null);

    try {
      const response = await fetch('/api/blockchain/wallet/create', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
        },
        body: JSON.stringify({
          name: formData.name,
          password: formData.password,
        }),
      });

      const data = await response.json();

      if (!response.ok) {
        throw new Error(data.error || 'Failed to create wallet');
      }

      setCreatedWallet(data.wallet);
      setWalletCreated(true);
      setSuccess('Wallet created successfully! Please save your seed phrase securely.');
      
      // Reset form
      setFormData({
        name: '',
        password: '',
        confirmPassword: '',
      });

    } catch (error) {
      console.error('Wallet creation failed:', error);
      setError(error instanceof Error ? error.message : 'Failed to create wallet');
    } finally {
      setLoading(false);
    }
  };

  const handleInputChange = (field: string, value: string) => {
    setFormData(prev => ({ ...prev, [field]: value }));
    if (error) setError(null);
  };

  const copySeedPhrase = async () => {
    if (createdWallet?.seedPhrase) {
      try {
        await navigator.clipboard.writeText(createdWallet.seedPhrase);
        setSeedPhraseCopied(true);
        setTimeout(() => setSeedPhraseCopied(false), 2000);
      } catch (error) {
        console.error('Failed to copy seed phrase:', error);
      }
    }
  };

  const downloadSeedPhrase = () => {
    if (createdWallet?.seedPhrase) {
      const blob = new Blob([createdWallet.seedPhrase], { type: 'text/plain' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `${createdWallet.name}_seed_phrase.txt`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }
  };

  const handleFinish = () => {
    if (createdWallet) {
      onWalletCreated(createdWallet);
    }
    setWalletCreated(false);
    setCreatedWallet(null);
    setSeedPhraseCopied(false);
    onClose();
  };

  const handleClose = () => {
    if (walletCreated) {
      // If wallet was created, confirm before closing
      if (window.confirm('Are you sure you want to close? Make sure you have saved your seed phrase securely.')) {
        handleFinish();
      }
    } else {
      onClose();
    }
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-gray-800 rounded-xl p-6 w-full max-w-md mx-4 border border-gray-700">
        {/* Header */}
        <div className="flex items-center justify-between mb-6">
          <div className="flex items-center space-x-3">
            <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
              <WalletIcon className="h-6 w-6 text-white" />
            </div>
            <div>
              <h2 className="text-xl font-bold text-white">
                {walletCreated ? 'Wallet Created Successfully!' : 'Create New Wallet'}
              </h2>
              <p className="text-sm text-gray-400">
                {walletCreated ? 'Save your seed phrase securely' : 'Generate a new blockchain wallet'}
              </p>
            </div>
          </div>
          <button
            onClick={handleClose}
            className="text-gray-400 hover:text-white transition-colors duration-200"
          >
            <XMarkIcon className="h-6 w-6" />
          </button>
        </div>

        {!walletCreated ? (
          /* Wallet Creation Form */
          <form onSubmit={handleSubmit} className="space-y-4">
            <div>
              <label htmlFor="walletName" className="block text-sm font-medium text-gray-300 mb-2">
                Wallet Name
              </label>
              <input
                id="walletName"
                type="text"
                value={formData.name}
                onChange={(e) => handleInputChange('name', e.target.value)}
                className="w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent"
                placeholder="Enter wallet name"
                required
              />
            </div>

            <div>
              <label htmlFor="walletPassword" className="block text-sm font-medium text-gray-300 mb-2">
                Wallet Password
              </label>
              <div className="relative">
                <input
                  id="walletPassword"
                  type={showPassword ? 'text' : 'password'}
                  value={formData.password}
                  onChange={(e) => handleInputChange('password', e.target.value)}
                  className="w-full px-3 py-2 pr-10 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent"
                  placeholder="Enter wallet password"
                  required
                />
                <button
                  type="button"
                  className="absolute inset-y-0 right-0 pr-3 flex items-center"
                  onClick={() => setShowPassword(!showPassword)}
                >
                  {showPassword ? (
                    <EyeSlashIcon className="h-5 w-5 text-gray-400" />
                  ) : (
                    <EyeIcon className="h-5 w-5 text-gray-400" />
                  )}
                </button>
              </div>
              <p className="text-xs text-gray-400 mt-1">Minimum 6 characters</p>
            </div>

            <div>
              <label htmlFor="confirmPassword" className="block text-sm font-medium text-gray-300 mb-2">
                Confirm Password
              </label>
              <div className="relative">
                <input
                  id="confirmPassword"
                  type={showConfirmPassword ? 'text' : 'password'}
                  value={formData.confirmPassword}
                  onChange={(e) => handleInputChange('confirmPassword', e.target.value)}
                  className="w-full px-3 py-2 pr-10 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent"
                  placeholder="Confirm wallet password"
                  required
                />
                <button
                  type="button"
                  className="absolute inset-y-0 right-0 pr-3 flex items-center"
                  onClick={() => setShowConfirmPassword(!showConfirmPassword)}
                >
                  {showConfirmPassword ? (
                    <EyeSlashIcon className="h-5 w-5 text-gray-400" />
                  ) : (
                    <EyeIcon className="h-5 w-5 text-gray-400" />
                  )}
                </button>
              </div>
            </div>

            {/* Error Message */}
            {error && (
              <div className="bg-red-900/50 border border-red-500 text-red-200 px-4 py-3 rounded-lg">
                <div className="flex items-center">
                  <svg className="h-5 w-5 mr-2" fill="currentColor" viewBox="0 0 20 20">
                    <path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7 4a1 1 0 11-2 0 1 1 0 012 0zm-1-9a1 1 0 00-1 1v4a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
                  </svg>
                  {error}
                </div>
              </div>
            )}

            {/* Action Buttons */}
            <div className="flex space-x-3 pt-4">
              <button
                type="button"
                onClick={onClose}
                className="flex-1 px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors duration-200"
                disabled={loading}
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={loading}
                className="flex-1 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200 disabled:opacity-50 disabled:cursor-not-allowed"
              >
                {loading ? (
                  <div className="flex items-center justify-center">
                    <svg className="animate-spin -ml-1 mr-3 h-5 w-5 text-white" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                      <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                      <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                    </svg>
                    Creating...
                  </div>
                ) : (
                  'Create Wallet'
                )}
              </button>
            </div>
          </form>
        ) : (
          /* Seed Phrase Display */
          <div className="space-y-6">
            {/* Security Warning */}
            <div className="bg-yellow-900/30 border border-yellow-600 text-yellow-200 px-4 py-3 rounded-lg">
              <div className="flex items-start">
                <svg className="h-5 w-5 mr-2 mt-0.5 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20">
                  <path fillRule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
                </svg>
                <div className="text-sm">
                  <p className="font-medium">Important: Save Your Seed Phrase</p>
                  <p className="mt-1">
                    Write down your seed phrase and store it securely. You'll need it to recover your wallet if you lose access.
                  </p>
                </div>
              </div>
            </div>

            {/* Wallet Info */}
            <div className="bg-gray-700 rounded-lg p-4">
              <h3 className="text-white font-semibold mb-2">Wallet Details</h3>
              <div className="space-y-2 text-sm">
                <div className="flex justify-between">
                  <span className="text-gray-400">Name:</span>
                  <span className="text-white">{createdWallet?.name}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-400">Address:</span>
                  <span className="text-white font-mono text-xs">{createdWallet?.address}</span>
                </div>
              </div>
            </div>

            {/* Seed Phrase */}
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Your Seed Phrase
              </label>
              <div className="bg-gray-700 border border-gray-600 rounded-lg p-4">
                <p className="text-white font-mono text-sm leading-relaxed">
                  {createdWallet?.seedPhrase}
                </p>
              </div>
              <p className="text-xs text-gray-400 mt-2">
                Write this down and keep it safe. Never share it with anyone.
              </p>
            </div>

            {/* Action Buttons */}
            <div className="flex space-x-3">
              <button
                onClick={copySeedPhrase}
                className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors duration-200"
              >
                {seedPhraseCopied ? (
                  <>
                    <CheckIcon className="h-5 w-5" />
                    <span>Copied!</span>
                  </>
                ) : (
                  <>
                    <ClipboardIcon className="h-5 w-5" />
                    <span>Copy</span>
                  </>
                )}
              </button>
              <button
                onClick={downloadSeedPhrase}
                className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-green-600 hover:bg-green-700 text-white rounded-lg transition-colors duration-200"
              >
                <DocumentArrowDownIcon className="h-5 w-5" />
                <span>Download</span>
              </button>
            </div>

            {/* Finish Button */}
            <button
              onClick={handleFinish}
              className="w-full px-4 py-3 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200 font-medium"
            >
              I've Saved My Seed Phrase
            </button>
          </div>
        )}
      </div>
    </div>
  );
} 