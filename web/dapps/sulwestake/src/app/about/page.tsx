'use client';

import React from 'react';
import Link from 'next/link';
import { 
  WalletIcon, 
  CheckCircleIcon
} from '@heroicons/react/24/outline';

export default function AboutPage() {
  return (
    <div className="min-h-screen bg-gray-900">
      {/* Navigation */}
      <nav className="bg-gray-800 border-b border-gray-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <Link href="/" className="flex items-center space-x-2">
                <div className="h-10 w-10 bg-indigo-600 rounded-full flex items-center justify-center">
                  <WalletIcon className="h-6 w-6 text-white" />
                </div>
                <span className="text-xl font-bold text-white">Sulwestake</span>
              </Link>
            </div>
            <div className="flex items-center space-x-4">
              <Link href="/" className="text-gray-300 hover:text-white px-3 py-2">
                Home
              </Link>
              <Link href="/network" className="text-gray-300 hover:text-white px-3 py-2">
                Network Stats
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
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-24">
        <div className="text-center mb-16">
          <h1 className="text-4xl md:text-6xl font-bold text-white mb-6">
            About <span className="text-indigo-400">Sulwestake</span>
          </h1>
          <p className="text-xl text-gray-300 max-w-3xl mx-auto">
            A decentralized staking platform built on the Nilotic blockchain, 
            enabling users to stake SLW tokens and earn competitive rewards.
          </p>
        </div>

        {/* Mission Section */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 items-center mb-24">
          <div>
            <h2 className="text-3xl font-bold text-white mb-6">Our Mission</h2>
            <p className="text-gray-300 mb-6">
              Sulwestake aims to democratize access to blockchain staking by providing a secure, 
              user-friendly platform that connects users directly to the Nilotic blockchain network.
            </p>
            <p className="text-gray-300 mb-6">
              We believe in the power of decentralized finance to create financial opportunities 
              for everyone, regardless of their technical expertise or geographic location.
            </p>
            <div className="flex flex-col sm:flex-row gap-4">
              <Link
                href="/get-started"
                className="bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-3 rounded-lg font-medium"
              >
                Start Staking
              </Link>
              <Link
                href="/network"
                className="border border-gray-600 text-gray-300 hover:text-white px-6 py-3 rounded-lg font-medium"
              >
                View Network Stats
              </Link>
            </div>
          </div>
          <div className="bg-gray-800 rounded-lg p-8">
            <h3 className="text-xl font-semibold text-white mb-4">Platform Features</h3>
            <div className="space-y-4">
              <div className="flex items-center space-x-3">
                <CheckCircleIcon className="h-6 w-6 text-green-400" />
                <span className="text-gray-300">Secure wallet management</span>
              </div>
              <div className="flex items-center space-x-3">
                <CheckCircleIcon className="h-6 w-6 text-green-400" />
                <span className="text-gray-300">Real-time staking rewards</span>
              </div>
              <div className="flex items-center space-x-3">
                <CheckCircleIcon className="h-6 w-6 text-green-400" />
                <span className="text-gray-300">Live blockchain analytics</span>
              </div>
              <div className="flex items-center space-x-3">
                <CheckCircleIcon className="h-6 w-6 text-green-400" />
                <span className="text-gray-300">Community-driven governance</span>
              </div>
              <div className="flex items-center space-x-3">
                <CheckCircleIcon className="h-6 w-6 text-green-400" />
                <span className="text-gray-300">Instant transaction processing</span>
              </div>
            </div>
          </div>
        </div>

        {/* Technology Stack */}
        <div className="mb-24">
          <h2 className="text-3xl font-bold text-white text-center mb-12">Technology Stack</h2>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="h-12 w-12 bg-blue-600 rounded-full flex items-center justify-center mb-4">
                <CheckCircleIcon className="h-6 w-6 text-white" />
              </div>
              <h3 className="text-xl font-semibold text-white mb-4">Nilotic Blockchain</h3>
              <p className="text-gray-400">
                Built on a custom C++ blockchain with Proof-of-Stake consensus, 
                providing security and scalability for decentralized applications.
              </p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="h-12 w-12 bg-green-600 rounded-full flex items-center justify-center mb-4">
                <CheckCircleIcon className="h-6 w-6 text-white" />
              </div>
              <h3 className="text-xl font-semibold text-white mb-4">Next.js Frontend</h3>
              <p className="text-gray-400">
                Modern React-based frontend with TypeScript, providing a smooth 
                and responsive user experience across all devices.
              </p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <div className="h-12 w-12 bg-purple-600 rounded-full flex items-center justify-center mb-4">
                <CheckCircleIcon className="h-6 w-6 text-white" />
              </div>
              <h3 className="text-xl font-semibold text-white mb-4">PostgreSQL Database</h3>
              <p className="text-gray-400">
                Robust relational database for user management, transaction history, 
                and real-time data synchronization with the blockchain.
              </p>
            </div>
          </div>
        </div>

        {/* Token Information */}
        <div className="mb-24">
          <h2 className="text-3xl font-bold text-white text-center mb-12">SLW Token</h2>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
            <div className="bg-gray-800 rounded-lg p-6">
              <h3 className="text-xl font-semibold text-white mb-4">Token Utility</h3>
              <ul className="space-y-3 text-gray-300">
                <li>• Staking rewards and governance</li>
                <li>• Network security through Proof-of-Stake</li>
                <li>• Transaction fee payments</li>
                <li>• Platform governance voting</li>
              </ul>
            </div>
            <div className="bg-gray-800 rounded-lg p-6">
              <h3 className="text-xl font-semibold text-white mb-4">Staking Benefits</h3>
              <ul className="space-y-3 text-gray-300">
                <li>• Earn competitive APY rates</li>
                <li>• Participate in network security</li>
                <li>• Vote on platform proposals</li>
                <li>• Receive staking rewards</li>
              </ul>
            </div>
          </div>
        </div>

        {/* Team Section */}
        <div className="mb-24">
          <h2 className="text-3xl font-bold text-white text-center mb-12">Development Team</h2>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-8">
            <div className="bg-gray-800 rounded-lg p-6 text-center">
              <div className="h-20 w-20 bg-indigo-600 rounded-full flex items-center justify-center mx-auto mb-4">
                <CheckCircleIcon className="h-10 w-10 text-white" />
              </div>
              <h3 className="text-xl font-semibold text-white mb-2">Blockchain Developers</h3>
              <p className="text-gray-400">
                Expert C++ developers building the core Nilotic blockchain infrastructure.
              </p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6 text-center">
              <div className="h-20 w-20 bg-green-600 rounded-full flex items-center justify-center mx-auto mb-4">
                <CheckCircleIcon className="h-10 w-10 text-white" />
              </div>
              <h3 className="text-xl font-semibold text-white mb-2">Frontend Engineers</h3>
              <p className="text-gray-400">
                React and Next.js specialists creating intuitive user interfaces.
              </p>
            </div>
            <div className="bg-gray-800 rounded-lg p-6 text-center">
              <div className="h-20 w-20 bg-purple-600 rounded-full flex items-center justify-center mx-auto mb-4">
                <CheckCircleIcon className="h-10 w-10 text-white" />
              </div>
              <h3 className="text-xl font-semibold text-white mb-2">Security Experts</h3>
              <p className="text-gray-400">
                Cybersecurity professionals ensuring platform and user security.
              </p>
            </div>
          </div>
        </div>

        {/* CTA Section */}
        <div className="text-center">
          <h2 className="text-3xl font-bold text-white mb-6">Ready to Get Started?</h2>
          <p className="text-xl text-gray-300 mb-8">
            Join the Sulwestake community and start earning rewards today.
          </p>
          <div className="flex flex-col sm:flex-row gap-4 justify-center">
            <Link
              href="/get-started"
              className="bg-indigo-600 hover:bg-indigo-700 text-white px-8 py-3 rounded-lg text-lg font-medium"
            >
              Create Account
            </Link>
            <Link
              href="/network"
              className="border border-gray-600 text-gray-300 hover:text-white px-8 py-3 rounded-lg text-lg font-medium"
            >
              View Network Stats
            </Link>
          </div>
        </div>
      </div>
    </div>
  );
} 