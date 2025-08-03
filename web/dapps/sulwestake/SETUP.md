# Sulwestake DApp Setup Guide

This guide will help you set up the Sulwestake DApp on your local machine.

## 🚀 Prerequisites

### 1. Install Node.js

**Option A: Using Homebrew (macOS/Linux)**
```bash
# Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Node.js
brew install node

# Verify installation
node --version
npm --version
```

**Option B: Using Node Version Manager (nvm)**
```bash
# Install nvm
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash

# Restart your terminal or run
source ~/.bashrc

# Install Node.js 18
nvm install 18
nvm use 18

# Verify installation
node --version
npm --version
```

**Option C: Direct Download**
1. Visit [nodejs.org](https://nodejs.org/)
2. Download the LTS version (18.x or higher)
3. Install following the instructions for your OS

### 2. Install Git (if not already installed)
```bash
# macOS
brew install git

# Ubuntu/Debian
sudo apt-get install git

# Windows
# Download from https://git-scm.com/
```

## 📦 Installation Steps

### 1. Clone the Repository
```bash
git clone <repository-url>
cd sulwestake
```

### 2. Install Dependencies

**Option A: Use the installation script (recommended)**
```bash
./install.sh
```

**Option B: Manual installation**
```bash
npm install --legacy-peer-deps
```

**If you encounter dependency conflicts:**
```bash
# Clear npm cache
npm cache clean --force

# Remove node_modules and package-lock.json
rm -rf node_modules package-lock.json

# Reinstall dependencies with legacy peer deps
npm install --legacy-peer-deps
```

### 3. Configure Environment Variables
```bash
# Copy the environment example file
cp config/env.example .env.local

# Edit the file with your configuration
nano .env.local
# or
code .env.local
```

**Required Environment Variables:**
```env
# Get this from https://cloud.walletconnect.com/
NEXT_PUBLIC_WEB3MODAL_PROJECT_ID=your_project_id_here

# Update with actual Nilotic blockchain details
NEXT_PUBLIC_NILOTIC_CHAIN_ID=1337
NEXT_PUBLIC_STAKING_CONTRACT_ADDRESS=0x...
NEXT_PUBLIC_RPC_URL=http://localhost:8545
```

### 4. Start Development Server
```bash
npm run dev
```

### 5. Open in Browser
Navigate to [http://localhost:3000](http://localhost:3000)

## 🔧 Troubleshooting

### Common Issues

#### 1. Node.js Not Found
```bash
# Check if Node.js is installed
node --version

# If not found, install using one of the methods above
```

#### 2. Permission Errors
```bash
# Fix npm permissions
sudo chown -R $USER:$GROUP ~/.npm
sudo chown -R $USER:$GROUP ~/.config
```

#### 3. Port Already in Use
```bash
# Find process using port 3000
lsof -ti:3000

# Kill the process
kill -9 $(lsof -ti:3000)

# Or use a different port
npm run dev -- -p 3001
```

#### 4. Dependency Conflicts
```bash
# Use npm with legacy peer deps
npm install --legacy-peer-deps

# Or use yarn instead
npm install -g yarn
yarn install
```

#### 5. Build Errors
```bash
# Clear Next.js cache
rm -rf .next

# Rebuild
npm run build
```

### Platform-Specific Setup

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Node.js
brew install node
```

#### Ubuntu/Debian
```bash
# Update package list
sudo apt update

# Install Node.js
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Install build tools
sudo apt-get install -y build-essential
```

#### Windows
1. Download Node.js from [nodejs.org](https://nodejs.org/)
2. Run the installer
3. Open Command Prompt or PowerShell
4. Navigate to project directory
5. Run installation commands

#### WSL (Windows Subsystem for Linux)
```bash
# Update WSL
wsl --update

# Install Node.js in WSL
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs
```

## 🧪 Testing the Setup

### 1. Verify Installation
```bash
# Check Node.js version
node --version  # Should be 18.x or higher

# Check npm version
npm --version

# Check if dependencies are installed
ls node_modules
```

### 2. Test Development Server
```bash
# Start the server
npm run dev

# Check if it's running
curl http://localhost:3000
```

### 3. Test Wallet Connection
1. Open [http://localhost:3000](http://localhost:3000)
2. Click "Connect Wallet"
3. Install MetaMask if prompted
4. Connect your wallet

## 📱 Mobile Development

### iOS Simulator (macOS only)
```bash
# Install Xcode from App Store
# Install iOS Simulator
# Run the app in simulator
```

### Android Emulator
```bash
# Install Android Studio
# Set up Android Virtual Device
# Run the app in emulator
```

## 🚀 Production Setup

### 1. Build for Production
```bash
npm run build
```

### 2. Test Production Build
```bash
npm start
```

### 3. Deploy
See [DEPLOYMENT.md](./DEPLOYMENT.md) for detailed deployment instructions.

## 📚 Additional Resources

- [Node.js Documentation](https://nodejs.org/docs/)
- [Next.js Documentation](https://nextjs.org/docs)
- [Web3Modal Documentation](https://docs.walletconnect.com/web3modal)
- [Wagmi Documentation](https://wagmi.sh/)

## 🆘 Getting Help

If you encounter issues:

1. **Check the logs** for error messages
2. **Search existing issues** in the repository
3. **Create a new issue** with detailed information
4. **Join our Discord** for real-time support

### Useful Commands
```bash
# Check system information
node --version
npm --version
git --version

# Check project status
npm list --depth=0
npm outdated

# Debug mode
DEBUG=* npm run dev

# Check for security vulnerabilities
npm audit
npm audit fix
```

---

**Happy coding! 🚀** 