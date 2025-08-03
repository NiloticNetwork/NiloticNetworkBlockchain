#!/bin/bash

# Sulwestake DApp Installation Script
echo "🚀 Installing Sulwestake DApp..."

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "❌ Node.js is not installed. Please install Node.js 18+ first."
    echo "Visit https://nodejs.org/ to download and install Node.js"
    exit 1
fi

# Check Node.js version
NODE_VERSION=$(node -v | cut -d'v' -f2 | cut -d'.' -f1)
if [ "$NODE_VERSION" -lt 18 ]; then
    echo "❌ Node.js version 18+ is required. Current version: $(node -v)"
    echo "Please upgrade Node.js to version 18 or higher"
    exit 1
fi

echo "✅ Node.js version: $(node -v)"

# Check if npm is installed
if ! command -v npm &> /dev/null; then
    echo "❌ npm is not installed. Please install npm first."
    exit 1
fi

echo "✅ npm version: $(npm -v)"

# Clear npm cache
echo "🧹 Clearing npm cache..."
npm cache clean --force

# Remove existing node_modules and package-lock.json if they exist
if [ -d "node_modules" ]; then
    echo "🗑️ Removing existing node_modules..."
    rm -rf node_modules
fi

if [ -f "package-lock.json" ]; then
    echo "🗑️ Removing existing package-lock.json..."
    rm -f package-lock.json
fi

# Install dependencies
echo "📦 Installing dependencies..."
npm install

# Check if installation was successful
if [ $? -eq 0 ]; then
    echo "✅ Dependencies installed successfully!"
else
    echo "❌ Installation failed. Trying with legacy peer deps..."
    npm install --legacy-peer-deps
    
    if [ $? -eq 0 ]; then
        echo "✅ Dependencies installed successfully with legacy peer deps!"
    else
        echo "❌ Installation failed. Trying with force flag..."
        npm install --force
        
        if [ $? -eq 0 ]; then
            echo "✅ Dependencies installed successfully with force flag!"
        else
            echo "❌ Installation failed. Please check the error messages above."
            exit 1
        fi
    fi
fi

# Create environment file if it doesn't exist
if [ ! -f ".env.local" ]; then
    echo "📝 Creating environment file..."
    if [ -f "config/env.example" ]; then
        cp config/env.example .env.local
        echo "✅ Environment file created from config/env.example"
        echo "⚠️  Please update .env.local with your configuration"
    else
        echo "⚠️  config/env.example not found. Please create .env.local manually"
    fi
else
    echo "✅ Environment file already exists"
fi

# Check if installation was successful
if [ -d "node_modules" ]; then
    echo ""
    echo "🎉 Installation completed successfully!"
    echo ""
    echo "Next steps:"
    echo "1. Update .env.local with your configuration"
    echo "2. Run 'npm run dev' to start the development server"
    echo "3. Open http://localhost:3000 in your browser"
    echo ""
    echo "For detailed setup instructions, see SETUP.md"
    echo ""
    echo "Note: This version uses a simplified wallet connection."
    echo "Make sure you have MetaMask installed in your browser."
else
    echo "❌ Installation failed. Please check the error messages above."
    exit 1
fi 