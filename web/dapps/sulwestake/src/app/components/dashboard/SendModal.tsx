'use client';

import React, { useState, useEffect } from 'react';
import { XMarkIcon, ExclamationTriangleIcon, CheckCircleIcon } from '@heroicons/react/24/outline';

interface SendModalProps {
  isOpen: boolean;
  onClose: () => void;
  wallet: any;
  onTransactionSent: (transaction: any) => void;
}

interface TransactionStatus {
  status: 'pending' | 'confirmed' | 'failed';
  hash?: string;
  message?: string;
}

export default function SendModal({ isOpen, onClose, wallet, onTransactionSent }: SendModalProps) {
  const [recipientAddress, setRecipientAddress] = useState('');
  const [amount, setAmount] = useState('');
  const [description, setDescription] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [transactionStatus, setTransactionStatus] = useState<TransactionStatus | null>(null);
  const [step, setStep] = useState<'form' | 'confirm' | 'processing' | 'success' | 'error'>('form');

  // Reset form when modal opens/closes
  useEffect(() => {
    if (isOpen) {
      setRecipientAddress('');
      setAmount('');
      setDescription('');
      setError(null);
      setTransactionStatus(null);
      setStep('form');
    }
  }, [isOpen]);

  const validateForm = () => {
    if (!recipientAddress.trim()) {
      setError('Recipient address is required');
      return false;
    }

    if (!amount || parseFloat(amount) <= 0) {
      setError('Amount must be greater than 0');
      return false;
    }

    if (parseFloat(amount) > wallet.balance) {
      setError('Insufficient balance');
      return false;
    }

    // Basic address validation (should match your blockchain format)
    if (!recipientAddress.startsWith('NIL') || recipientAddress.length < 10) {
      setError('Invalid recipient address format');
      return false;
    }

    if (recipientAddress === wallet.address) {
      setError('Cannot send to your own address');
      return false;
    }

    setError(null);
    return true;
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    
    if (!validateForm()) {
      return;
    }

    setStep('confirm');
  };

  const handleConfirm = async () => {
    try {
      setIsLoading(true);
      setStep('processing');
      setError(null);

      const authToken = localStorage.getItem('auth_token');
      console.log('Auth token available:', !!authToken);
      console.log('Sending transaction:', {
        fromAddress: wallet.address,
        toAddress: recipientAddress,
        amount: parseFloat(amount),
        description: description.trim() || undefined,
      });

      const response = await fetch('/api/blockchain/transactions/send', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${authToken}`,
        },
        body: JSON.stringify({
          fromAddress: wallet.address,
          toAddress: recipientAddress,
          amount: parseFloat(amount),
          description: description.trim() || undefined,
        }),
      });

      console.log('Response status:', response.status);
      const data = await response.json();
      console.log('Response data:', data);

      if (!response.ok) {
        throw new Error(data.error || 'Failed to send transaction');
      }

      setTransactionStatus({
        status: 'pending',
        hash: data.transaction.hash,
        message: 'Transaction submitted successfully',
      });

      // Notify parent component
      onTransactionSent(data.transaction);

      setStep('success');
    } catch (error) {
      console.error('Send transaction error:', error);
      setError(error instanceof Error ? error.message : 'Failed to send transaction');
      setStep('error');
    } finally {
      setIsLoading(false);
    }
  };

  const handleClose = () => {
    if (step === 'processing') {
      return; // Prevent closing during processing
    }
    onClose();
  };

  const formatAmount = (amount: number) => {
    return `${amount.toFixed(2)} NIL`;
  };

  const getFeeEstimate = () => {
    // Simple fee calculation (you can make this dynamic based on network conditions)
    return 0.001; // 0.001 NIL fee
  };

  const totalAmount = parseFloat(amount) + getFeeEstimate();

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-gray-800 rounded-xl p-6 w-full max-w-md mx-4">
        {/* Header */}
        <div className="flex items-center justify-between mb-6">
          <h2 className="text-xl font-semibold text-white">Send NIL</h2>
          <button
            onClick={handleClose}
            disabled={step === 'processing'}
            className="text-gray-400 hover:text-white disabled:opacity-50"
          >
            <XMarkIcon className="h-6 w-6" />
          </button>
        </div>

        {/* Form Step */}
        {step === 'form' && (
          <form onSubmit={handleSubmit} className="space-y-4">
            {/* Wallet Info */}
            <div className="bg-gray-700 rounded-lg p-4">
              <p className="text-sm text-gray-400 mb-1">From Wallet</p>
              <p className="text-white font-medium">{wallet.name}</p>
              <p className="text-xs text-gray-400 font-mono">{wallet.address}</p>
              <p className="text-sm text-gray-300 mt-2">
                Available: {formatAmount(wallet.balance)}
              </p>
            </div>

            {/* Recipient Address */}
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Recipient Address
              </label>
              <input
                type="text"
                value={recipientAddress}
                onChange={(e) => setRecipientAddress(e.target.value)}
                placeholder="NIL..."
                className="w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:border-indigo-500"
              />
            </div>

            {/* Amount */}
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Amount (NIL)
              </label>
              <input
                type="number"
                value={amount}
                onChange={(e) => setAmount(e.target.value)}
                placeholder="0.00"
                step="0.01"
                min="0"
                max={wallet.balance}
                className="w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:border-indigo-500"
              />
            </div>

            {/* Description */}
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Description (Optional)
              </label>
              <input
                type="text"
                value={description}
                onChange={(e) => setDescription(e.target.value)}
                placeholder="What's this payment for?"
                className="w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:border-indigo-500"
              />
            </div>

            {/* Transaction Summary */}
            {amount && parseFloat(amount) > 0 && (
              <div className="bg-gray-700 rounded-lg p-4">
                <h3 className="text-sm font-medium text-gray-300 mb-3">Transaction Summary</h3>
                <div className="space-y-2 text-sm">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Amount:</span>
                    <span className="text-white">{formatAmount(parseFloat(amount))}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Network Fee:</span>
                    <span className="text-white">{formatAmount(getFeeEstimate())}</span>
                  </div>
                  <div className="border-t border-gray-600 pt-2 flex justify-between">
                    <span className="text-gray-300 font-medium">Total:</span>
                    <span className="text-white font-medium">{formatAmount(totalAmount)}</span>
                  </div>
                </div>
              </div>
            )}

            {/* Error Display */}
            {error && (
              <div className="flex items-center space-x-2 p-3 bg-red-900/50 border border-red-500 rounded-lg">
                <ExclamationTriangleIcon className="h-5 w-5 text-red-400" />
                <span className="text-red-200 text-sm">{error}</span>
              </div>
            )}

            {/* Action Buttons */}
            <div className="flex space-x-3 pt-4">
              <button
                type="button"
                onClick={handleClose}
                className="flex-1 px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors"
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={!amount || parseFloat(amount) <= 0 || isLoading}
                className="flex-1 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 disabled:opacity-50 disabled:cursor-not-allowed text-white rounded-lg transition-colors"
              >
                Continue
              </button>
            </div>
          </form>
        )}

        {/* Confirmation Step */}
        {step === 'confirm' && (
          <div className="space-y-4">
            <div className="bg-yellow-900/20 border border-yellow-500/50 rounded-lg p-4">
              <div className="flex items-center space-x-2 mb-2">
                <ExclamationTriangleIcon className="h-5 w-5 text-yellow-400" />
                <span className="text-yellow-200 font-medium">Confirm Transaction</span>
              </div>
              <p className="text-yellow-200 text-sm">
                Please review the transaction details carefully. This action cannot be undone.
              </p>
            </div>

            <div className="bg-gray-700 rounded-lg p-4 space-y-3">
              <div className="flex justify-between">
                <span className="text-gray-400">To:</span>
                <span className="text-white font-mono text-sm">{recipientAddress}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Amount:</span>
                <span className="text-white">{formatAmount(parseFloat(amount))}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Fee:</span>
                <span className="text-white">{formatAmount(getFeeEstimate())}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Total:</span>
                <span className="text-white font-medium">{formatAmount(totalAmount)}</span>
              </div>
              {description && (
                <div className="flex justify-between">
                  <span className="text-gray-400">Description:</span>
                  <span className="text-white text-sm">{description}</span>
                </div>
              )}
            </div>

            <div className="flex space-x-3">
              <button
                onClick={() => setStep('form')}
                className="flex-1 px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors"
              >
                Back
              </button>
              <button
                onClick={handleConfirm}
                className="flex-1 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors"
              >
                Send Transaction
              </button>
            </div>
          </div>
        )}

        {/* Processing Step */}
        {step === 'processing' && (
          <div className="text-center space-y-4">
            <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-indigo-500 mx-auto"></div>
            <h3 className="text-lg font-medium text-white">Processing Transaction</h3>
            <p className="text-gray-400 text-sm">
              Please wait while we submit your transaction to the blockchain...
            </p>
          </div>
        )}

        {/* Success Step */}
        {step === 'success' && (
          <div className="text-center space-y-4">
            <div className="mx-auto h-12 w-12 bg-green-600 rounded-full flex items-center justify-center">
              <CheckCircleIcon className="h-8 w-8 text-white" />
            </div>
            <h3 className="text-lg font-medium text-white">Transaction Sent!</h3>
            <p className="text-gray-400 text-sm">
              Your transaction has been submitted to the blockchain.
            </p>
            {transactionStatus?.hash && (
              <div className="bg-gray-700 rounded-lg p-3">
                <p className="text-xs text-gray-400 mb-1">Transaction Hash:</p>
                <p className="text-xs text-white font-mono break-all">{transactionStatus.hash}</p>
              </div>
            )}
            <button
              onClick={handleClose}
              className="w-full px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors"
            >
              Close
            </button>
          </div>
        )}

        {/* Error Step */}
        {step === 'error' && (
          <div className="text-center space-y-4">
            <div className="mx-auto h-12 w-12 bg-red-600 rounded-full flex items-center justify-center">
              <ExclamationTriangleIcon className="h-8 w-8 text-white" />
            </div>
            <h3 className="text-lg font-medium text-white">Transaction Failed</h3>
            <p className="text-gray-400 text-sm">{error}</p>
            <div className="flex space-x-3">
              <button
                onClick={() => setStep('form')}
                className="flex-1 px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors"
              >
                Try Again
              </button>
              <button
                onClick={handleClose}
                className="flex-1 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors"
              >
                Close
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
} 