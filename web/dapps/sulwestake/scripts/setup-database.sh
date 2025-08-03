#!/bin/bash

# Database Setup Script for Sulwestake
# This script sets up PostgreSQL database and runs Prisma migrations

set -e

echo "🚀 Setting up Sulwestake Database..."

# Check if PostgreSQL is installed
if ! command -v psql &> /dev/null; then
    echo "❌ PostgreSQL is not installed. Please install PostgreSQL first."
    echo "   On macOS: brew install postgresql"
    echo "   On Ubuntu: sudo apt-get install postgresql postgresql-contrib"
    echo "   On Windows: Download from https://www.postgresql.org/download/windows/"
    exit 1
fi

# Check if .env file exists
if [ ! -f .env ]; then
    echo "📝 Creating .env file from template..."
    cp env.example .env
    echo "⚠️  Please edit .env file with your database credentials"
    echo "   Update DATABASE_URL with your PostgreSQL connection string"
    echo "   Update JWT_SECRET with a secure secret key"
    exit 1
fi

# Load environment variables
source .env

# Extract database connection details
DB_URL=${DATABASE_URL}
DB_NAME=$(echo $DB_URL | sed -n 's/.*\/\([^?]*\).*/\1/p')

echo "📊 Database URL: $DB_URL"
echo "📊 Database Name: $DB_NAME"

# Check if database exists
if psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME; then
    echo "✅ Database '$DB_NAME' already exists"
else
    echo "📝 Creating database '$DB_NAME'..."
    createdb $DB_NAME
    echo "✅ Database created successfully"
fi

# Install dependencies if not already installed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
fi

# Generate Prisma client
echo "🔧 Generating Prisma client..."
npx prisma generate

# Run database migrations
echo "🔄 Running database migrations..."
npx prisma migrate dev --name init

# Seed database with initial data (optional)
echo "🌱 Seeding database..."
npx prisma db seed

echo "✅ Database setup completed successfully!"
echo ""
echo "🎉 Next steps:"
echo "   1. Start the blockchain server: ./build/nilotic_blockchain --port 5500 --debug"
echo "   2. Start the application: npm run dev"
echo "   3. Open http://localhost:3000 in your browser"
echo ""
echo "📝 Default demo account:"
echo "   Email: demo@nilotic.com"
echo "   Password: password123" 