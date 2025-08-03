'use client';

import React from 'react';
import Link from 'next/link';
import { WalletIcon } from '@heroicons/react/24/outline';

export default function LandingPage() {
  return (
    <div className="min-h-screen bg-gray-900">
      {/* Navigation */}
      <nav className="bg-gray-800 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                <WalletIcon className="h-6 w-6 text-white" />
              </div>
              <span className="text-xl font-bold text-white">Sulwestake</span>
            </div>
            <div className="flex items-center space-x-4">
              <Link href="/network" className="text-gray-300 hover:text-white px-3 py-2">
                Network Stats
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

      {/* Hero Section */}
      <div className="relative overflow-hidden">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-24">
          <div className="text-center">
            <h1 className="text-4xl md:text-6xl font-bold text-white mb-6">
              Decentralized Staking on the
              <span className="text-indigo-400"> Nilotic Blockchain</span>
            </h1>
            <p className="text-xl text-gray-300 mb-8 max-w-3xl mx-auto">
              Stake your SLW tokens, earn rewards, and participate in the future of decentralized finance. 
              Built on the secure and scalable Nilotic blockchain.
            </p>
            <div className="flex flex-col sm:flex-row gap-4 justify-center">
              <Link
                href="/get-started"
                className="bg-indigo-600 hover:bg-indigo-700 text-white px-8 py-3 rounded-lg text-lg font-medium"
              >
                Start Staking
              </Link>
              <Link
                href="/about"
                className="border border-gray-600 text-gray-300 hover:text-white px-8 py-3 rounded-lg text-lg font-medium"
              >
                Learn More
              </Link>
            </div>
          </div>
        </div>
      </div>

      {/* Features Section */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-24">
        <h2 className="text-3xl font-bold text-white text-center mb-12">Why Choose Sulwestake?</h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
          <div className="text-center">
            <div className="h-16 w-16 bg-indigo-600 rounded-full flex items-center justify-center mx-auto mb-6">
              <WalletIcon className="h-8 w-8 text-white" />
            </div>
            <h3 className="text-xl font-semibold text-white mb-4">Secure Staking</h3>
            <p className="text-gray-400">
              Your tokens are secured by the Nilotic blockchain's advanced cryptography and consensus mechanisms.
            </p>
          </div>
          <div className="text-center">
            <div className="h-16 w-16 bg-green-600 rounded-full flex items-center justify-center mx-auto mb-6">
              <WalletIcon className="h-8 w-8 text-white" />
            </div>
            <h3 className="text-xl font-semibold text-white mb-4">High Rewards</h3>
            <p className="text-gray-400">
              Earn competitive APY rates with our optimized staking algorithms and network participation.
            </p>
          </div>
          <div className="text-center">
            <div className="h-16 w-16 bg-purple-600 rounded-full flex items-center justify-center mx-auto mb-6">
              <WalletIcon className="h-8 w-8 text-white" />
            </div>
            <h3 className="text-xl font-semibold text-white mb-4">Instant Access</h3>
            <p className="text-gray-400">
              Stake and unstake instantly with our streamlined interface and real-time blockchain integration.
            </p>
          </div>
        </div>
      </div>

      {/* CTA Section */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-24">
        <div className="text-center">
          <h2 className="text-3xl font-bold text-white mb-6">Ready to Start Staking?</h2>
          <p className="text-xl text-gray-400 mb-8">
            Join thousands of users already earning rewards on the Nilotic blockchain.
          </p>
          <Link
            href="/get-started"
            className="bg-indigo-600 hover:bg-indigo-700 text-white px-8 py-3 rounded-lg text-lg font-medium"
          >
            Get Started Now
          </Link>
        </div>
      </div>
    </div>
  );
} 