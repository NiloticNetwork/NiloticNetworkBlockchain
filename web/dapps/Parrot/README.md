# Parrot - AI-Driven SocialFi Platform for Nilotic Blockchain

## Overview

Parrot is a decentralized social networking platform built on the Nilotic blockchain, combining AI-powered content curation with financial incentives using the native Sulwe (SLW) token. The platform celebrates Nilotic heritage and the "star" (Sulwe) concept while providing users with ownership of their data and monetization opportunities.

## Features

### Core Features
- **Decentralized Identity**: Wallet-based profiles with Nilotic-themed usernames
- **Social Networking**: Create, like, comment, and share posts with IPFS storage
- **AI-Powered Curation**: Personalized content recommendations and analytics
- **Tokenized Incentives**: SLW rewards for engagement and content creation
- **NFT Integration**: Mint and trade NFTs inspired by Nilotic culture
- **Community Governance**: DAO for platform decisions using SLW tokens
- **Cross-Chain Compatibility**: Bridge SLW tokens to other blockchains

### Technical Stack
- **Blockchain**: Nilotic (EVM-compatible)
- **Smart Contracts**: Solidity with OpenZeppelin
- **Frontend**: React with TypeScript
- **Backend**: Node.js with Express
- **AI**: TensorFlow.js for recommendations
- **Storage**: IPFS for decentralized content storage
- **Indexing**: The Graph for efficient data querying

## Project Structure

```
Parrot/
├── contracts/          # Smart contracts (Solidity)
├── frontend/          # React frontend application
├── backend/           # Node.js backend API
├── ai/               # AI models and scripts
├── tests/            # Test suites
├── docs/             # Documentation
└── deployment/       # Deployment scripts
```

## Quick Start

### Prerequisites
- Node.js 18+
- npm or yarn
- MetaMask or compatible wallet
- Git

### Installation

1. **Clone and Setup**
```bash
cd DApps/Parrot
npm install
```

2. **Environment Configuration**
```bash
cp .env.example .env
# Edit .env with your configuration
```

3. **Smart Contract Deployment**
```bash
cd contracts
npm install
npx hardhat compile
npx hardhat deploy --network nilotic-testnet
```

4. **Backend Setup**
```bash
cd backend
npm install
npm run dev
```

5. **Frontend Setup**
```bash
cd frontend
npm install
npm start
```

6. **AI Model Setup**
```bash
cd ai
pip install -r requirements.txt
python setup_models.py
```

## Configuration

### Environment Variables

Create a `.env` file in the root directory:

```env
# Blockchain Configuration
NILOTIC_RPC_URL=https://testnet.nilotic.chain
NILOTIC_CHAIN_ID=1234
PRIVATE_KEY=your_private_key_here

# IPFS Configuration
PINATA_API_KEY=your_pinata_api_key
PINATA_SECRET_KEY=your_pinata_secret_key

# AI Configuration
AI_MODEL_PATH=./ai/models/recommendation_model
AI_API_KEY=your_ai_api_key

# Database
DATABASE_URL=your_database_url

# JWT Secret
JWT_SECRET=your_jwt_secret
```

## Smart Contracts

### Core Contracts
- `ParrotProfile.sol`: User profile management
- `ParrotPosts.sol`: Post creation and interaction
- `SulweToken.sol`: SLW token implementation
- `ParrotNFT.sol`: NFT minting and marketplace
- `ParrotDAO.sol`: Governance and voting
- `ParrotRewards.sol`: Incentive system

### Deployment Addresses
- Mainnet: TBD
- Testnet: TBD

## API Reference

### Authentication
- `POST /api/auth/connect`: Connect wallet
- `POST /api/auth/verify`: Verify wallet signature

### Profiles
- `GET /api/profiles/:address`: Get user profile
- `POST /api/profiles`: Create/update profile
- `GET /api/profiles/:address/posts`: Get user posts

### Posts
- `GET /api/posts`: Get feed posts
- `POST /api/posts`: Create new post
- `POST /api/posts/:id/like`: Like post
- `POST /api/posts/:id/comment`: Comment on post

### AI & Analytics
- `GET /api/ai/recommendations`: Get personalized recommendations
- `GET /api/ai/analytics/:address`: Get creator analytics

### NFTs
- `GET /api/nfts`: Get NFT marketplace
- `POST /api/nfts/mint`: Mint new NFT
- `POST /api/nfts/:id/purchase`: Purchase NFT

## Testing

### Smart Contract Tests
```bash
cd contracts
npx hardhat test
```

### Frontend Tests
```bash
cd frontend
npm test
```

### Backend Tests
```bash
cd backend
npm test
```

## Deployment

### Smart Contracts
```bash
cd contracts
npx hardhat deploy --network nilotic-mainnet
```

### Frontend (Vercel)
```bash
cd frontend
vercel --prod
```

### Backend (AWS)
```bash
cd backend
npm run deploy
```

## Cultural Branding

Parrot incorporates Nilotic heritage and the "Sulwe" (star) concept:

- **Color Scheme**: Deep blues and golds representing the night sky and stars
- **UI Elements**: Star motifs and geometric patterns inspired by African art
- **NFT Themes**: Cultural artifacts, traditional stories, and star-inspired designs
- **Username System**: Nilotic names and star-related identifiers

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

MIT License - see LICENSE file for details

## Support

- Documentation: [docs.parrot.nilotic.chain](https://docs.parrot.nilotic.chain)
- Community: [Discord](https://discord.gg/parrot-nilotic)
- Issues: [GitHub Issues](https://github.com/nilotic/parrot/issues)

## Roadmap

- [ ] Mobile app development
- [ ] Advanced AI features
- [ ] Cross-chain NFT bridges
- [ ] Creator marketplace
- [ ] Advanced governance features
- [ ] Integration with other African blockchains 