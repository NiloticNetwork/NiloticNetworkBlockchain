'use client';

import React, { useState } from 'react';
import { XMarkIcon, ClipboardDocumentIcon, CheckIcon } from '@heroicons/react/24/outline';

interface ReceiveModalProps {
  isOpen: boolean;
  onClose: () => void;
  wallet: any;
}

export default function ReceiveModal({ isOpen, onClose, wallet }: ReceiveModalProps) {
  const [copied, setCopied] = useState(false);

  const copyToClipboard = async () => {
    try {
      await navigator.clipboard.writeText(wallet.address);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch (error) {
      console.error('Failed to copy address:', error);
    }
  };

  const formatAddress = (address: string) => {
    return `${address.slice(0, 8)}...${address.slice(-6)}`;
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-gray-800 rounded-xl p-6 w-full max-w-md mx-4">
        {/* Header */}
        <div className="flex items-center justify-between mb-6">
          <h2 className="text-xl font-semibold text-white">Receive NIL</h2>
          <button
            onClick={onClose}
            className="text-gray-400 hover:text-white"
          >
            <XMarkIcon className="h-6 w-6" />
          </button>
        </div>

        {/* Wallet Info */}
        <div className="bg-gray-700 rounded-lg p-4 mb-6">
          <p className="text-sm text-gray-400 mb-1">Wallet</p>
          <p className="text-white font-medium">{wallet.name}</p>
          <p className="text-xs text-gray-400 font-mono">{wallet.address}</p>
        </div>

        {/* QR Code Placeholder */}
        <div className="bg-white rounded-lg p-8 mb-6 flex items-center justify-center">
          <div className="text-center">
            <div className="w-32 h-32 bg-gray-200 rounded-lg flex items-center justify-center mb-4">
              <span className="text-gray-500 text-sm">QR Code</span>
            </div>
            <p className="text-xs text-gray-500">QR code for {wallet.address}</p>
          </div>
        </div>

        {/* Address Display */}
        <div className="bg-gray-700 rounded-lg p-4 mb-6">
          <p className="text-sm text-gray-400 mb-2">Your Address</p>
          <div className="flex items-center justify-between">
            <code className="text-sm text-white font-mono break-all">{wallet.address}</code>
            <button
              onClick={copyToClipboard}
              className="ml-3 p-2 text-gray-400 hover:text-white transition-colors"
              title="Copy address"
            >
              {copied ? (
                <CheckIcon className="h-5 w-5 text-green-400" />
              ) : (
                <ClipboardDocumentIcon className="h-5 w-5" />
              )}
            </button>
          </div>
          {copied && (
            <p className="text-xs text-green-400 mt-2">Address copied to clipboard!</p>
          )}
        </div>

        {/* Instructions */}
        <div className="bg-blue-900/20 border border-blue-500/50 rounded-lg p-4 mb-6">
          <h3 className="text-sm font-medium text-blue-200 mb-2">How to receive NIL</h3>
          <ul className="text-xs text-blue-200 space-y-1">
            <li>• Share your address with the sender</li>
            <li>• They can scan the QR code or copy the address</li>
            <li>• Funds will appear in your wallet once confirmed</li>
            <li>• Only send NIL tokens to this address</li>
          </ul>
        </div>

        {/* Security Notice */}
        <div className="bg-yellow-900/20 border border-yellow-500/50 rounded-lg p-4 mb-6">
          <h3 className="text-sm font-medium text-yellow-200 mb-2">Security Notice</h3>
          <p className="text-xs text-yellow-200">
            Never share your private key or seed phrase. Only share your public address for receiving payments.
          </p>
        </div>

        {/* Action Button */}
        <button
          onClick={onClose}
          className="w-full px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors"
        >
          Close
        </button>
      </div>
    </div>
  );
} 