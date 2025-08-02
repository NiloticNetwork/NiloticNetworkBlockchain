# Parrot DApp Deployment Guide

## Overview

This guide provides step-by-step instructions for deploying the Parrot AI-driven SocialFi platform on the Nilotic blockchain. The deployment includes smart contracts, backend API, frontend application, and AI services.

## Prerequisites

### System Requirements
- Node.js 18+ 
- npm or yarn
- Git
- Python 3.8+ (for AI services)
- Redis (for caching)
- MongoDB (for analytics)

### Development Tools
- MetaMask or compatible wallet
- Hardhat (for smart contract deployment)
- IPFS node or Pinata account

## Step 1: Environment Setup

### 1.1 Clone the Repository
```bash
git clone <repository-url>
cd DApps/Parrot
```

### 1.2 Install Dependencies

#### Smart Contracts
```bash
cd contracts
npm install
```

#### Backend
```bash
cd backend
npm install
```

#### Frontend
```bash
cd frontend
npm install
```

#### AI Services
```bash
cd ai
pip install -r requirements.txt
```

### 1.3 Environment Configuration

Create environment files for each component:

#### Smart Contracts (.env)
```env
# Blockchain Configuration
NILOTIC_RPC_URL=https://testnet.nilotic.chain
NILOTIC_CHAIN_ID=1234
PRIVATE_KEY=your_private_key_here

# IPFS Configuration
PINATA_API_KEY=your_pinata_api_key
PINATA_SECRET_KEY=your_pinata_secret_key

# Explorer Configuration
NILOTIC_EXPLORER_API_KEY=your_explorer_api_key
```

#### Backend (.env)
```env
# Server Configuration
PORT=3001
NODE_ENV=production

# Blockchain Configuration
NILOTIC_RPC_URL=https://testnet.nilotic.chain
NILOTIC_CHAIN_ID=1234
SULWE_TOKEN_ADDRESS=deployed_token_address
PARROT_PROFILE_ADDRESS=deployed_profile_address
PARROT_POSTS_ADDRESS=deployed_posts_address
PARROT_NFT_ADDRESS=deployed_nft_address
PARROT_DAO_ADDRESS=deployed_dao_address

# Database Configuration
MONGODB_URI=your_mongodb_uri
REDIS_URL=your_redis_url

# JWT Configuration
JWT_SECRET=your_jwt_secret

# IPFS Configuration
PINATA_API_KEY=your_pinata_api_key
PINATA_SECRET_KEY=your_pinata_secret_key

# AI Configuration
AI_MODEL_PATH=./ai/models/recommendation_model
AI_API_KEY=your_ai_api_key

# CORS Configuration
FRONTEND_URL=https://your-frontend-domain.com
```

#### Frontend (.env)
```env
REACT_APP_API_URL=https://your-backend-domain.com
REACT_APP_CHAIN_ID=1234
REACT_APP_RPC_URL=https://testnet.nilotic.chain
REACT_APP_EXPLORER_URL=https://testnet-explorer.nilotic.chain
REACT_APP_CONTRACT_ADDRESSES={"sulweToken":"...","parrotProfile":"...","parrotPosts":"...","parrotNFT":"...","parrotDAO":"..."}
```

## Step 2: Smart Contract Deployment

### 2.1 Compile Contracts
```bash
cd contracts
npx hardhat compile
```

### 2.2 Deploy to Testnet
```bash
npx hardhat run scripts/deploy.js --network nilotic-testnet
```

### 2.3 Verify Contracts
```bash
npx hardhat verify --network nilotic-testnet <contract_address> <constructor_args>
```

### 2.4 Update Contract Addresses
After deployment, update the contract addresses in:
- Backend `.env` file
- Frontend `.env` file
- Deployment configuration files

## Step 3: Backend Deployment

### 3.1 Local Development
```bash
cd backend
npm run dev
```

### 3.2 Production Deployment (AWS)

#### Using Docker
```dockerfile
# Dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
EXPOSE 3001
CMD ["npm", "start"]
```

#### Using PM2
```bash
npm install -g pm2
pm2 start ecosystem.config.js
```

#### Using AWS ECS
```bash
# Build and push Docker image
docker build -t parrot-backend .
docker tag parrot-backend:latest your-ecr-repo/parrot-backend:latest
docker push your-ecr-repo/parrot-backend:latest

# Deploy to ECS
aws ecs update-service --cluster parrot-cluster --service parrot-backend --force-new-deployment
```

### 3.3 Environment Variables Setup
```bash
# Set environment variables in your deployment platform
aws ssm put-parameter --name "/parrot/backend/env" --value "$(cat .env)" --type "SecureString"
```

## Step 4: Frontend Deployment

### 4.1 Build for Production
```bash
cd frontend
npm run build
```

### 4.2 Deploy to Vercel
```bash
# Install Vercel CLI
npm i -g vercel

# Deploy
vercel --prod
```

### 4.3 Deploy to AWS S3 + CloudFront
```bash
# Sync to S3
aws s3 sync build/ s3://your-parrot-bucket --delete

# Invalidate CloudFront cache
aws cloudfront create-invalidation --distribution-id YOUR_DISTRIBUTION_ID --paths "/*"
```

### 4.4 Deploy to Netlify
```bash
# Install Netlify CLI
npm install -g netlify-cli

# Deploy
netlify deploy --prod --dir=build
```

## Step 5: AI Services Setup

### 5.1 Model Training
```bash
cd ai
python train_models.py
```

### 5.2 Deploy AI Service
```bash
# Using Docker
docker build -t parrot-ai .
docker run -p 5000:5000 parrot-ai

# Using AWS SageMaker
aws sagemaker create-model --model-name parrot-recommendation --primary-container Image=your-ecr-repo/parrot-ai:latest
```

## Step 6: Database Setup

### 6.1 MongoDB Setup
```bash
# Install MongoDB
sudo apt-get install mongodb

# Create database and collections
mongo
use parrot
db.createCollection('users')
db.createCollection('posts')
db.createCollection('analytics')
```

### 6.2 Redis Setup
```bash
# Install Redis
sudo apt-get install redis-server

# Configure Redis
sudo nano /etc/redis/redis.conf
# Set maxmemory and maxmemory-policy
```

## Step 7: IPFS Configuration

### 7.1 Pinata Setup
1. Create Pinata account
2. Get API keys
3. Configure in environment variables

### 7.2 Local IPFS Node (Optional)
```bash
# Install IPFS
wget https://dist.ipfs.io/go-ipfs/v0.12.0/go-ipfs_v0.12.0_linux-amd64.tar.gz
tar -xvzf go-ipfs_v0.12.0_linux-amd64.tar.gz
cd go-ipfs
sudo bash install.sh

# Initialize and start
ipfs init
ipfs daemon
```

## Step 8: Monitoring and Analytics

### 8.1 Application Monitoring
```bash
# Install monitoring tools
npm install -g pm2
pm2 install pm2-logrotate
pm2 install pm2-server-monit
```

### 8.2 Logging Setup
```bash
# Configure log rotation
sudo nano /etc/logrotate.d/parrot
```

### 8.3 Health Checks
```bash
# Backend health check
curl https://your-backend-domain.com/health

# Frontend health check
curl https://your-frontend-domain.com
```

## Step 9: Security Configuration

### 9.1 SSL/TLS Setup
```bash
# Using Let's Encrypt
sudo certbot --nginx -d your-domain.com

# Using AWS Certificate Manager
aws acm import-certificate --certificate fileb://certificate.pem --private-key fileb://private-key.pem
```

### 9.2 Firewall Configuration
```bash
# Configure UFW
sudo ufw allow 22
sudo ufw allow 80
sudo ufw allow 443
sudo ufw enable
```

### 9.3 Environment Security
```bash
# Encrypt sensitive data
aws kms encrypt --key-id your-kms-key --plaintext "sensitive-data"
```

## Step 10: Testing and Validation

### 10.1 Smart Contract Testing
```bash
cd contracts
npx hardhat test
npx hardhat coverage
```

### 10.2 Backend Testing
```bash
cd backend
npm test
npm run test:integration
```

### 10.3 Frontend Testing
```bash
cd frontend
npm test
npm run test:coverage
```

### 10.4 End-to-End Testing
```bash
# Using Cypress
npm install cypress
npx cypress run
```

## Step 11: Performance Optimization

### 11.1 Backend Optimization
```bash
# Enable compression
npm install compression

# Configure caching
npm install redis

# Optimize database queries
npm install mongoose-cache
```

### 11.2 Frontend Optimization
```bash
# Enable code splitting
npm install @loadable/component

# Optimize images
npm install imagemin

# Enable service worker
npm install workbox-webpack-plugin
```

## Step 12: Maintenance and Updates

### 12.1 Automated Deployments
```bash
# GitHub Actions workflow
# .github/workflows/deploy.yml
name: Deploy Parrot DApp
on:
  push:
    branches: [main]
jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Deploy to production
        run: |
          # Deployment steps
```

### 12.2 Backup Strategy
```bash
# Database backup
mongodump --db parrot --out /backup/$(date +%Y%m%d)

# File backup
aws s3 sync /app/uploads s3://parrot-backups/uploads/$(date +%Y%m%d)
```

## Troubleshooting

### Common Issues

#### Smart Contract Deployment Fails
```bash
# Check gas settings
npx hardhat deploy --network nilotic-testnet --gas-price 20000000000

# Verify network configuration
npx hardhat console --network nilotic-testnet
```

#### Backend Connection Issues
```bash
# Check environment variables
echo $NILOTIC_RPC_URL

# Test blockchain connection
curl -X POST $NILOTIC_RPC_URL -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"id":1}'
```

#### Frontend Build Issues
```bash
# Clear cache
rm -rf node_modules package-lock.json
npm install

# Check environment variables
echo $REACT_APP_API_URL
```

## Support and Documentation

### Useful Commands
```bash
# Check deployment status
pm2 status

# View logs
pm2 logs parrot-backend

# Monitor resources
htop

# Check disk space
df -h
```

### Contact Information
- Technical Support: support@parrot.nilotic.chain
- Documentation: docs.parrot.nilotic.chain
- Community: discord.gg/parrot-nilotic

## Next Steps

1. **Monitor Performance**: Set up monitoring dashboards
2. **Scale Infrastructure**: Add load balancers and auto-scaling
3. **Security Audits**: Regular security assessments
4. **Feature Updates**: Continuous development and deployment
5. **Community Building**: Engage with users and gather feedback

## Conclusion

The Parrot DApp is now deployed and ready for users. Remember to:
- Monitor system health regularly
- Keep dependencies updated
- Backup data frequently
- Engage with the community
- Iterate based on user feedback

For additional support, refer to the project documentation or contact the development team. 