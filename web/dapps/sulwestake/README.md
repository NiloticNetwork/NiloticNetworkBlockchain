# 🚀 Sulwestake - Nilotic Blockchain Staking Platform

A modern, decentralized staking platform for the Nilotic blockchain ecosystem. Built with Next.js 15, React 19, and TypeScript.

## ✨ Features

### 🔐 **Wallet Management**
- **MetaMask Integration** - Connect existing MetaMask wallets
- **Nilotic Wallet Creation** - Generate new wallets for the Nilotic blockchain
- **Mock Mode** - Test the application without real wallets
- **Authentication System** - Sign up/sign in with email and password

### 📊 **Staking Interface**
- **Stake Tokens** - Lock your NIL tokens to earn rewards
- **Unstake Tokens** - Withdraw your staked tokens (with lock period)
- **Real-time APY** - View current annual percentage yield
- **Rewards Tracking** - Monitor your earned rewards

### 🎯 **Dashboard & Analytics**
- **Portfolio Overview** - Total staked, rewards, and performance metrics
- **Blockchain Status** - Real-time connection to Nilotic blockchain
- **Rewards Chart** - Visual representation of earnings over time
- **Quick Actions** - Fast access to common operations

### 🏆 **Gamification & Social**
- **Leaderboard** - Compete with other stakers
- **User Levels** - Bronze, Silver, Gold, Platinum tiers
- **Staking Pools** - Community and charity pools
- **Challenges** - Participate in staking competitions

### 🌐 **Modern UI/UX**
- **Responsive Design** - Works on desktop, tablet, and mobile
- **Dark Theme** - Beautiful dark mode interface
- **Smooth Animations** - Framer Motion powered transitions
- **Real-time Updates** - Live blockchain data integration

## 🏗️ Architecture

### **Frontend Components**
```
src/app/
├── components/
│   ├── WalletManager.tsx      # Wallet connection & auth
│   ├── Dashboard.tsx          # Main dashboard view
│   └── StakingInterface.tsx   # Stake/unstake interface
├── api/                       # Backend API routes
│   ├── auth/
│   │   ├── signup/route.ts    # User registration
│   │   └── signin/route.ts    # User authentication
│   └── wallet/
│       └── create/route.ts    # Wallet creation
├── utils/
│   └── blockchain.ts          # Blockchain API utilities
└── page.tsx                   # Main application page
```

### **Backend APIs**
- **Authentication** - JWT-based user management
- **Wallet Creation** - Cryptographic key generation
- **Blockchain Integration** - Nilotic blockchain API
- **Staking Operations** - Transaction submission

## 🚀 Quick Start

### **Prerequisites**
- Node.js 18+ 
- npm or yarn
- MetaMask extension (optional)

### **Installation**

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd sulwestake
   ```

2. **Install dependencies**
   ```bash
   npm install
   ```

3. **Set up environment variables**
   ```bash
   cp config/env.example .env.local
   ```
   
   Edit `.env.local` with your configuration:
   ```env
   # Nilotic Blockchain
   NEXT_PUBLIC_BLOCKCHAIN_BASE_URL=http://localhost:5500
   NEXT_PUBLIC_NILOTIC_CHAIN_ID=1337
   
   # JWT Secret (change in production)
   JWT_SECRET=your-secret-key-here
   ```

4. **Start the development server**
   ```bash
   npm run dev
   ```

5. **Open your browser**
   ```
   http://localhost:3000
   ```

## 🔧 Configuration

### **Environment Variables**

```env
# =============================================================================
# Nilotic Blockchain Configuration
# =============================================================================
NEXT_PUBLIC_NILOTIC_CHAIN_ID=1337
NEXT_PUBLIC_BLOCKCHAIN_BASE_URL=http://localhost:5500
NEXT_PUBLIC_NILOTIC_RPC_URL=http://localhost:5500
NEXT_PUBLIC_MINING_ENDPOINT=http://localhost:5500/mine
NEXT_PUBLIC_METRICS_ENDPOINT=http://localhost:5500/metrics
NEXT_PUBLIC_STAKING_CONTRACT_ADDRESS=0x1234567890123456789012345678901234567890
NEXT_PUBLIC_NILOTIC_TOKEN_ADDRESS=0x1234567890123456789012345678901234567890
NEXT_PUBLIC_TOKEN_SYMBOL=NIL
NEXT_PUBLIC_TOKEN_DECIMALS=18
NEXT_PUBLIC_TOKEN_NAME=Nilotic Token

# =============================================================================
# Authentication
# =============================================================================
JWT_SECRET=your-super-secret-jwt-key-change-in-production
JWT_EXPIRES_IN=7d

# =============================================================================
# Application Settings
# =============================================================================
NEXT_PUBLIC_APP_NAME=Sulwestake
NEXT_PUBLIC_APP_VERSION=1.0.0
NODE_ENV=development
```

## 🎮 Usage Guide

### **1. Connect Wallet**
- **MetaMask**: Click "MetaMask" button to connect existing wallet
- **Create Wallet**: Click "Create Wallet" to generate a new Nilotic wallet
- **Mock Mode**: Click "Mock Mode" for testing without real wallets

### **2. Authentication**
- **Sign Up**: Create a new account with email and password
- **Sign In**: Log in with existing credentials
- **JWT Tokens**: Automatic token management for session persistence

### **3. Staking Operations**
- **Stake**: Enter amount and click "Stake Tokens"
- **Unstake**: Enter amount and click "Unstake Tokens"
- **Claim Rewards**: Click "Claim Rewards" to collect earnings

### **4. Dashboard Features**
- **Portfolio Overview**: View total staked, rewards, and APY
- **Blockchain Status**: Real-time connection to Nilotic blockchain
- **Quick Actions**: Fast access to common operations

## 🔒 Security Features

### **Wallet Security**
- **Cryptographic Key Generation** - Secure random key creation
- **Mnemonic Phrases** - BIP39 compatible seed phrases
- **Private Key Protection** - Never stored in plain text
- **Address Validation** - Nilotic blockchain address format

### **Authentication Security**
- **Password Hashing** - PBKDF2 with salt
- **JWT Tokens** - Secure session management
- **Input Validation** - Email and password validation
- **Rate Limiting** - Protection against brute force

### **Blockchain Security**
- **Transaction Signing** - Secure transaction submission
- **Error Handling** - Graceful failure handling
- **Fallback Mechanisms** - Default values when API fails

## 🛠️ Development

### **Component Structure**
```typescript
// Wallet Management
interface WalletManagerProps {
  onWalletConnect: (address: string, isMock: boolean) => void;
  onWalletDisconnect: () => void;
  isConnected: boolean;
  address: string;
  isMockMode: boolean;
}

// Dashboard
interface DashboardProps {
  totalStaked: number;
  totalRewards: number;
  apy: number;
  userLevel: string;
  blockchainMetrics: BlockchainMetrics | null;
  onClaimRewards: () => void;
  isLoading: boolean;
}

// Staking Interface
interface StakingInterfaceProps {
  address: string;
  totalStaked: number;
  apy: number;
  onStake: (amount: number) => void;
  onUnstake: (amount: number) => void;
  isLoading: boolean;
}
```

### **API Endpoints**
```typescript
// Authentication
POST /api/auth/signup    // Create new account
POST /api/auth/signin    // Sign in existing account

// Wallet Management
POST /api/wallet/create  // Create new Nilotic wallet

// Blockchain Integration
GET  /api/blockchain/status    // Get blockchain status
POST /api/blockchain/transaction // Submit transaction
```

## 🧪 Testing

### **Manual Testing**
1. **Wallet Connection**: Test MetaMask, wallet creation, and mock mode
2. **Authentication**: Test signup, signin, and session management
3. **Staking**: Test stake, unstake, and reward claiming
4. **UI/UX**: Test responsive design and animations

### **Automated Testing**
```bash
# Run linting
npm run lint

# Type checking
npx tsc --noEmit

# Build test
npm run build
```

## 🚀 Deployment

### **Vercel (Recommended)**
```bash
# Install Vercel CLI
npm i -g vercel

# Deploy
vercel --prod
```

### **Netlify**
```bash
# Build
npm run build

# Deploy to Netlify
# Upload dist folder to Netlify
```

### **Docker**
```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
RUN npm run build
EXPOSE 3000
CMD ["npm", "start"]
```

## 🔧 Troubleshooting

### **Common Issues**

1. **MetaMask Connection Failed**
   - Ensure MetaMask is installed and unlocked
   - Check if the correct network is selected
   - Try refreshing the page

2. **Blockchain API Errors**
   - Verify Nilotic blockchain is running on port 5500
   - Check environment variables
   - Review browser console for detailed errors

3. **Authentication Issues**
   - Clear browser cache and cookies
   - Check JWT_SECRET environment variable
   - Verify email format and password strength

4. **Build Errors**
   - Clear node_modules and reinstall: `rm -rf node_modules && npm install`
   - Check TypeScript errors: `npx tsc --noEmit`
   - Verify all dependencies are installed

### **Debug Mode**
```bash
# Enable debug logging
DEBUG=* npm run dev

# Check environment variables
echo $NEXT_PUBLIC_BLOCKCHAIN_BASE_URL
```

## 📈 Performance

### **Optimizations**
- **Component Splitting** - Modular architecture for better maintainability
- **Lazy Loading** - Components loaded on demand
- **Caching** - Blockchain data cached for 30 seconds
- **Error Boundaries** - Graceful error handling
- **TypeScript** - Type safety and better development experience

### **Monitoring**
- **Console Logging** - Detailed error and success messages
- **Toast Notifications** - User-friendly feedback
- **Loading States** - Visual feedback during operations

## 🤝 Contributing

### **Development Setup**
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### **Code Style**
- **TypeScript** - Strict type checking
- **ESLint** - Code linting and formatting
- **Prettier** - Consistent code formatting
- **Component Structure** - Modular and reusable components

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Nilotic Blockchain** - For the underlying blockchain technology
- **Next.js Team** - For the amazing React framework
- **Tailwind CSS** - For the utility-first CSS framework
- **Framer Motion** - For smooth animations
- **Heroicons** - For beautiful icons

---

**Built with ❤️ for the Nilotic blockchain ecosystem**
