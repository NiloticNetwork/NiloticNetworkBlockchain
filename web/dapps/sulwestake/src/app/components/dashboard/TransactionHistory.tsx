'use client';

import React from 'react';
import { ArrowUpIcon, ArrowDownIcon, ClockIcon, CheckCircleIcon, XCircleIcon } from '@heroicons/react/24/outline';

interface Transaction {
  id: string;
  hash: string;
  from: string;
  to: string;
  amount: number;
  type: string;
  status: 'pending' | 'confirmed' | 'failed';
  timestamp: string;
  description?: string;
  blockNumber?: number;
  gasUsed?: number;
  fee?: number;
}

interface TransactionHistoryProps {
  transactions: Transaction[];
  userWallets: any[];
}

export default function TransactionHistory({ transactions, userWallets }: TransactionHistoryProps) {
  const formatAmount = (amount: number) => {
    return `${amount.toFixed(2)} NIL`;
  };

  const formatAddress = (address: string) => {
    return `${address.slice(0, 8)}...${address.slice(-6)}`;
  };

  const formatDate = (timestamp: string) => {
    return new Date(timestamp).toLocaleDateString('en-US', {
      year: 'numeric',
      month: 'short',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    });
  };

  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'confirmed':
        return <CheckCircleIcon className="h-4 w-4 text-green-400" />;
      case 'pending':
        return <ClockIcon className="h-4 w-4 text-yellow-400" />;
      case 'failed':
        return <XCircleIcon className="h-4 w-4 text-red-400" />;
      default:
        return <ClockIcon className="h-4 w-4 text-gray-400" />;
    }
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

  const isOutgoing = (transaction: Transaction) => {
    return userWallets.some(wallet => wallet.address === transaction.from);
  };

  const getTransactionType = (transaction: Transaction) => {
    if (isOutgoing(transaction)) {
      return 'Sent';
    }
    return 'Received';
  };

  const getTransactionIcon = (transaction: Transaction) => {
    if (isOutgoing(transaction)) {
      return <ArrowUpIcon className="h-4 w-4 text-red-400" />;
    }
    return <ArrowDownIcon className="h-4 w-4 text-green-400" />;
  };

  const getTransactionColor = (transaction: Transaction) => {
    if (isOutgoing(transaction)) {
      return 'text-red-400';
    }
    return 'text-green-400';
  };

  if (transactions.length === 0) {
    return (
      <div className="text-center py-12">
        <div className="mx-auto h-16 w-16 bg-gray-700 rounded-full flex items-center justify-center mb-4">
          <ClockIcon className="h-8 w-8 text-gray-400" />
        </div>
        <h3 className="text-lg font-medium text-white mb-2">No transactions yet</h3>
        <p className="text-gray-400 text-sm">
          Your transaction history will appear here once you start sending or receiving NIL tokens.
        </p>
      </div>
    );
  }

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h3 className="text-lg font-medium text-white">Recent Transactions</h3>
        <span className="text-sm text-gray-400">{transactions.length} transactions</span>
      </div>

      <div className="space-y-3">
        {transactions.map((transaction) => (
          <div
            key={transaction.id}
            className="bg-gray-800 rounded-lg p-4 border border-gray-700 hover:border-gray-600 transition-colors"
          >
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center space-x-3">
                {getTransactionIcon(transaction)}
                <div>
                  <p className={`text-sm font-medium ${getTransactionColor(transaction)}`}>
                    {getTransactionType(transaction)}
                  </p>
                  <p className="text-xs text-gray-400">
                    {formatDate(transaction.timestamp)}
                  </p>
                </div>
              </div>
              <div className="flex items-center space-x-2">
                {getStatusIcon(transaction.status)}
                <span className={`text-xs font-medium ${getStatusColor(transaction.status)}`}>
                  {transaction.status}
                </span>
              </div>
            </div>

            <div className="space-y-2">
              <div className="flex justify-between items-center">
                <span className="text-sm text-gray-400">Amount:</span>
                <span className={`text-sm font-medium ${getTransactionColor(transaction)}`}>
                  {isOutgoing(transaction) ? '-' : '+'}{formatAmount(transaction.amount)}
                </span>
              </div>

              <div className="flex justify-between items-center">
                <span className="text-sm text-gray-400">
                  {isOutgoing(transaction) ? 'To:' : 'From:'}
                </span>
                <span className="text-sm text-white font-mono">
                  {isOutgoing(transaction) 
                    ? formatAddress(transaction.to)
                    : formatAddress(transaction.from)
                  }
                </span>
              </div>

              {transaction.description && (
                <div className="flex justify-between items-center">
                  <span className="text-sm text-gray-400">Description:</span>
                  <span className="text-sm text-white">{transaction.description}</span>
                </div>
              )}

              {transaction.blockNumber && (
                <div className="flex justify-between items-center">
                  <span className="text-sm text-gray-400">Block:</span>
                  <span className="text-sm text-white">#{transaction.blockNumber}</span>
                </div>
              )}

              {transaction.fee && (
                <div className="flex justify-between items-center">
                  <span className="text-sm text-gray-400">Fee:</span>
                  <span className="text-sm text-white">{formatAmount(transaction.fee)}</span>
                </div>
              )}

              <div className="pt-2 border-t border-gray-700">
                <div className="flex justify-between items-center">
                  <span className="text-xs text-gray-400">Transaction Hash:</span>
                  <span className="text-xs text-gray-400 font-mono">
                    {formatAddress(transaction.hash)}
                  </span>
                </div>
              </div>
            </div>
          </div>
        ))}
      </div>

      {transactions.length > 10 && (
        <div className="text-center pt-4">
          <button className="px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors">
            View All Transactions
          </button>
        </div>
      )}
    </div>
  );
} 