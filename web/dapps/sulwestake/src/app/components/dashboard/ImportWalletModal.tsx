'use client';

import React, { useState, useRef } from 'react';
import { XMarkIcon, WalletIcon, EyeIcon, EyeSlashIcon, KeyIcon, DocumentArrowUpIcon } from '@heroicons/react/24/outline';

interface ImportWalletModalProps {
  isOpen: boolean;
  onClose: () => void;
  onWalletImported: (wallet: any) => void;
}

type ImportMethod = 'privateKey' | 'seedPhrase' | 'fileUpload';

export default function ImportWalletModal({ isOpen, onClose, onWalletImported }: ImportWalletModalProps) {
  const [importMethod, setImportMethod] = useState<ImportMethod>('seedPhrase');
  const [formData, setFormData] = useState({
    name: '',
    privateKey: '',
    seedPhrase: '',
    password: '',
  });
  const [showPrivateKey, setShowPrivateKey] = useState(false);
  const [showPassword, setShowPassword] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [uploadedFileName, setUploadedFileName] = useState<string | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    
    if (!formData.name) {
      setError('Wallet name is required');
      return;
    }

    if (importMethod === 'privateKey' && !formData.privateKey) {
      setError('Private key is required');
      return;
    }

    if (importMethod === 'seedPhrase' && !formData.seedPhrase) {
      setError('Seed phrase is required');
      return;
    }

    if (importMethod === 'fileUpload' && !formData.seedPhrase) {
      setError('Please upload a seed phrase file');
      return;
    }

    if (!formData.password) {
      setError('Password is required');
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
      const response = await fetch('/api/blockchain/wallet/import', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${localStorage.getItem('auth_token')}`,
        },
        body: JSON.stringify({
          name: formData.name,
          importMethod: importMethod === 'fileUpload' ? 'seedPhrase' : importMethod,
          privateKey: importMethod === 'privateKey' ? formData.privateKey : undefined,
          seedPhrase: (importMethod === 'seedPhrase' || importMethod === 'fileUpload') ? formData.seedPhrase : undefined,
          password: formData.password,
        }),
      });

      const data = await response.json();

      if (!response.ok) {
        throw new Error(data.error || 'Failed to import wallet');
      }

      setSuccess('Wallet imported successfully!');
      
      // Call the callback with the imported wallet data
      onWalletImported(data);
      
      // Reset form
      setFormData({
        name: '',
        privateKey: '',
        seedPhrase: '',
        password: '',
      });
      setUploadedFileName(null);

      // Close modal after a short delay
      setTimeout(() => {
        onClose();
        setSuccess(null);
      }, 2000);

    } catch (error) {
      console.error('Wallet import failed:', error);
      setError(error instanceof Error ? error.message : 'Failed to import wallet');
    } finally {
      setLoading(false);
    }
  };

  const handleInputChange = (field: string, value: string) => {
    setFormData(prev => ({ ...prev, [field]: value }));
    if (error) setError(null);
  };

  const handleFileUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file) {
      setUploadedFileName(file.name);
      
      const reader = new FileReader();
      reader.onload = (e) => {
        const content = e.target?.result as string;
        // Clean up the content (remove extra whitespace, newlines, etc.)
        const cleanedContent = content.trim().replace(/\s+/g, ' ');
        setFormData(prev => ({ ...prev, seedPhrase: cleanedContent }));
      };
      reader.readAsText(file);
    }
  };

  const triggerFileUpload = () => {
    fileInputRef.current?.click();
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-gray-800 rounded-xl p-6 w-full max-w-md mx-4 border border-gray-700">
        {/* Header */}
        <div className="flex items-center justify-between mb-6">
          <div className="flex items-center space-x-3">
            <div className="h-10 w-10 bg-green-600 rounded-full flex items-center justify-center">
              <KeyIcon className="h-6 w-6 text-white" />
            </div>
            <div>
              <h2 className="text-xl font-bold text-white">Import Wallet</h2>
              <p className="text-sm text-gray-400">Import an existing blockchain wallet</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="text-gray-400 hover:text-white transition-colors duration-200"
          >
            <XMarkIcon className="h-6 w-6" />
          </button>
        </div>

        {/* Import Method Selection */}
        <div className="mb-6">
          <label className="block text-sm font-medium text-gray-300 mb-3">
            Import Method
          </label>
          <div className="grid grid-cols-3 gap-2">
            <button
              type="button"
              onClick={() => setImportMethod('privateKey')}
              className={`px-3 py-2 rounded-lg border transition-colors duration-200 text-sm ${
                importMethod === 'privateKey'
                  ? 'border-indigo-500 bg-indigo-600 text-white'
                  : 'border-gray-600 bg-gray-700 text-gray-300 hover:border-gray-500'
              }`}
            >
              Private Key
            </button>
            <button
              type="button"
              onClick={() => setImportMethod('seedPhrase')}
              className={`px-3 py-2 rounded-lg border transition-colors duration-200 text-sm ${
                importMethod === 'seedPhrase'
                  ? 'border-indigo-500 bg-indigo-600 text-white'
                  : 'border-gray-600 bg-gray-700 text-gray-300 hover:border-gray-500'
              }`}
            >
              Seed Phrase
            </button>
            <button
              type="button"
              onClick={() => setImportMethod('fileUpload')}
              className={`px-3 py-2 rounded-lg border transition-colors duration-200 text-sm ${
                importMethod === 'fileUpload'
                  ? 'border-indigo-500 bg-indigo-600 text-white'
                  : 'border-gray-600 bg-gray-700 text-gray-300 hover:border-gray-500'
              }`}
            >
              Upload File
            </button>
          </div>
        </div>

        {/* Form */}
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

          {importMethod === 'privateKey' ? (
            <div>
              <label htmlFor="privateKey" className="block text-sm font-medium text-gray-300 mb-2">
                Private Key
              </label>
              <div className="relative">
                <textarea
                  id="privateKey"
                  value={formData.privateKey}
                  onChange={(e) => handleInputChange('privateKey', e.target.value)}
                  className="w-full px-3 py-2 pr-10 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent resize-none"
                  placeholder="Enter your private key"
                  rows={3}
                  required
                />
                <button
                  type="button"
                  className="absolute top-2 right-2 p-1"
                  onClick={() => setShowPrivateKey(!showPrivateKey)}
                >
                  {showPrivateKey ? (
                    <EyeSlashIcon className="h-4 w-4 text-gray-400" />
                  ) : (
                    <EyeIcon className="h-4 w-4 text-gray-400" />
                  )}
                </button>
              </div>
              <p className="text-xs text-gray-400 mt-1">
                Enter your 64-character private key (without 0x prefix)
              </p>
              <p className="text-xs text-yellow-400 mt-1">
                💡 Most users have seed phrases, not private keys. Try the "Seed Phrase" option instead.
              </p>
            </div>
          ) : importMethod === 'seedPhrase' ? (
            <div>
              <label htmlFor="seedPhrase" className="block text-sm font-medium text-gray-300 mb-2">
                Seed Phrase
              </label>
              <textarea
                id="seedPhrase"
                value={formData.seedPhrase}
                onChange={(e) => handleInputChange('seedPhrase', e.target.value)}
                className="w-full px-3 py-2 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent resize-none"
                placeholder="Enter your 12 or 24 word seed phrase"
                rows={3}
                required
              />
              <p className="text-xs text-gray-400 mt-1">
                Enter your 12 or 24 word seed phrase separated by spaces
              </p>
            </div>
          ) : (
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Upload Seed Phrase File
              </label>
              <div className="border-2 border-dashed border-gray-600 rounded-lg p-6 text-center hover:border-gray-500 transition-colors duration-200">
                <input
                  ref={fileInputRef}
                  type="file"
                  accept=".txt,.json"
                  onChange={handleFileUpload}
                  className="hidden"
                />
                <div className="space-y-3">
                  <DocumentArrowUpIcon className="mx-auto h-12 w-12 text-gray-400" />
                  <div>
                    <p className="text-white font-medium">
                      {uploadedFileName ? uploadedFileName : 'Click to upload or drag and drop'}
                    </p>
                    <p className="text-sm text-gray-400 mt-1">
                      Supports .txt and .json files containing your seed phrase
                    </p>
                  </div>
                  <button
                    type="button"
                    onClick={triggerFileUpload}
                    className="px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white rounded-lg transition-colors duration-200"
                  >
                    Choose File
                  </button>
                </div>
              </div>
              {uploadedFileName && (
                <p className="text-xs text-green-400 mt-2">
                  ✓ File uploaded: {uploadedFileName}
                </p>
              )}
            </div>
          )}

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

          {/* Security Warning */}
          <div className="bg-yellow-900/30 border border-yellow-600 text-yellow-200 px-4 py-3 rounded-lg">
            <div className="flex items-start">
              <svg className="h-5 w-5 mr-2 mt-0.5 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
              </svg>
              <div className="text-sm">
                <p className="font-medium">Security Notice</p>
                <p className="mt-1">
                  Your private key and seed phrase are sensitive information. Never share them with anyone and ensure you're on a secure connection.
                </p>
              </div>
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

          {/* Success Message */}
          {success && (
            <div className="bg-green-900/50 border border-green-500 text-green-200 px-4 py-3 rounded-lg">
              <div className="flex items-center">
                <svg className="h-5 w-5 mr-2" fill="currentColor" viewBox="0 0 20 20">
                  <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                </svg>
                {success}
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
              className="flex-1 px-4 py-2 bg-green-600 hover:bg-green-700 text-white rounded-lg transition-colors duration-200 disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {loading ? (
                <div className="flex items-center justify-center">
                  <svg className="animate-spin -ml-1 mr-3 h-5 w-5 text-white" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                    <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                    <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                  </svg>
                  Importing...
                </div>
              ) : (
                'Import Wallet'
              )}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
} 