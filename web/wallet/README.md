# Nilotic Blockchain Web Wallet

A comprehensive, production-ready web wallet interface for the Nilotic Blockchain platform. This wallet provides a modern, secure interface for managing blockchain operations including wallet creation, transactions, mining, and network monitoring.

## 🚀 Features

### Core Wallet Functionality
- **Wallet Creation**: Create new wallets with secure key generation
- **Wallet Import**: Import existing wallets using private keys
- **Transaction Management**: Send and receive NIL tokens
- **Balance Tracking**: Real-time balance monitoring
- **Address Management**: Secure address generation and storage

### Mining Operations
- **Mining Controls**: Start and stop mining operations
- **Mining Status**: Real-time mining status monitoring
- **Hash Rate Display**: Current mining performance metrics
- **Difficulty Tracking**: Dynamic difficulty adjustment monitoring

### Network Management
- **Network Status**: Monitor P2P network health
- **Peer Management**: View active connections and peers
- **Message Tracking**: Monitor network message flow
- **Connection Status**: Real-time network connectivity

### Advanced Features
- **Token Creation**: Create OderoSLW tokens
- **Smart Contract Deployment**: Deploy and interact with smart contracts
- **Real-time Updates**: Live blockchain data synchronization
- **Responsive Design**: Mobile-friendly interface

## 📁 File Structure

```
web/wallet/
├── index.html              # Main wallet interface
├── enhanced_wallet.html    # Advanced wallet with all features
├── dapp_sdk.js            # Blockchain SDK for dApp development
├── dapp_template.html     # Template for dApp development
├── optimized_wallet.html  # Performance-optimized wallet
├── README.md              # This documentation
├── requirements.txt       # Python dependencies (if using Python backend)
└── nilotic_wallet.py     # Python wallet backend (optional)
```

## 🔧 API Compatibility

The web wallet is fully compatible with the Nilotic Blockchain API endpoints:

### Core Endpoints
- `GET /info` - Blockchain information
- `GET /balance/{address}` - Wallet balance
- `POST /transaction` - Send transaction
- `GET /block/latest` - Latest block information
- `GET /block/{index}` - Block by index

### Wallet Endpoints
- `POST /wallet/create` - Create new wallet
- `POST /wallet/import` - Import existing wallet
- `POST /wallet/sign` - Sign transaction

### Mining Endpoints
- `GET /mining/status` - Mining status
- `POST /mining/start` - Start mining
- `POST /mining/stop` - Stop mining

### Network Endpoints
- `GET /network/status` - Network status
- `GET /network/peers` - Peer list
- `POST /network/connect` - Connect to peer
- `POST /network/disconnect` - Disconnect from peer

### Token Endpoints
- `POST /token` - Create OderoSLW token

## 🚀 Quick Start

### 1. Start the Blockchain
```bash
cd nilotic-blockchain-clean
./build.sh
cd build
./nilotic_blockchain --port 5010 --debug
```

### 2. Open the Web Wallet
Open `index.html` in your web browser or serve it using a local server:

```bash
# Using Python
python -m http.server 8080

# Using Node.js
npx serve .

# Using PHP
php -S localhost:8080
```

### 3. Connect to Blockchain
The wallet will automatically attempt to connect to `http://localhost:5010`. Ensure the blockchain is running on this port.

## 💻 Usage Guide

### Creating a Wallet
1. Enter a wallet name and password
2. Click "Create Wallet"
3. The wallet address will be displayed
4. Store your private key securely (not shown in interface for security)

### Importing a Wallet
1. Enter wallet name and password
2. Click "Import Wallet"
3. Paste your private key in PEM format
4. Click "Import"

### Sending Transactions
1. Ensure you have a wallet loaded
2. Enter recipient address
3. Enter amount in NIL
4. Click "Send Transaction"

### Mining Operations
1. Load a wallet (required for mining)
2. Click "Start Mining" to begin
3. Monitor hash rate and difficulty
4. Click "Stop Mining" to halt

### Network Monitoring
- View active connections and peers
- Monitor message flow
- Check network health status
- Refresh network data manually

## 🔒 Security Features

### Client-Side Security
- Private keys are never stored in the interface
- All sensitive operations use secure APIs
- Password-based wallet encryption
- Secure transaction signing

### Network Security
- HTTPS-ready (configure for production)
- CORS headers for cross-origin requests
- Input validation and sanitization
- Error handling and logging

## 🛠️ Development

### SDK Usage
The wallet includes a comprehensive SDK for dApp development:

```javascript
// Initialize the SDK
const dapp = new NiloticDApp('http://localhost:5010');

// Connect to blockchain
await dapp.connect();

// Create wallet
const wallet = await dapp.createWallet('MyWallet', 'password123');

// Send transaction
await dapp.sendTransaction('recipient_address', 10.5);

// Start mining
await dapp.startMining(wallet.address);

// Get network status
const networkStatus = await dapp.getNetworkStatus();
```

### Customization
The wallet interface can be customized by modifying:
- CSS variables in the `:root` selector
- API endpoints in the JavaScript code
- UI components and layouts
- Color schemes and themes

## 📱 Responsive Design

The wallet is fully responsive and works on:
- Desktop computers
- Tablets
- Mobile phones
- Various screen sizes and orientations

## 🔧 Configuration

### API Endpoint
Change the blockchain URL by modifying:
```javascript
let blockchainUrl = 'http://localhost:5010';
```

### Update Intervals
Adjust real-time update intervals:
```javascript
setInterval(updateNetworkStatus, 10000); // 10 seconds
```

### Notification Duration
Modify notification display time:
```javascript
setTimeout(() => {
    // Remove notification
}, 5000); // 5 seconds
```

## 🚀 Production Deployment

### Security Considerations
1. Use HTTPS in production
2. Implement proper CORS policies
3. Add rate limiting
4. Enable secure headers
5. Use environment variables for configuration

### Performance Optimization
1. Minify CSS and JavaScript
2. Enable gzip compression
3. Use CDN for external resources
4. Implement caching strategies
5. Optimize images and assets

### Monitoring
1. Add error tracking (e.g., Sentry)
2. Implement analytics
3. Monitor API response times
4. Track user interactions
5. Log security events

## 🐛 Troubleshooting

### Connection Issues
- Ensure blockchain is running on correct port
- Check firewall settings
- Verify CORS configuration
- Check browser console for errors

### Transaction Failures
- Verify wallet has sufficient balance
- Check recipient address format
- Ensure network connectivity
- Review transaction parameters

### Mining Issues
- Confirm wallet is loaded
- Check mining difficulty
- Verify network connectivity
- Monitor system resources

## 📄 License

This project is part of the Nilotic Blockchain platform. See the main project license for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For support and questions:
- Check the main project documentation
- Review API documentation
- Test with the provided examples
- Report issues through the project repository

---

**Note**: This web wallet is designed for the Nilotic Blockchain platform and may not be compatible with other blockchain networks without modification.