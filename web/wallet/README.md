# Nilotic Blockchain Wallet Applications

This directory contains wallet applications that interface with the Nilotic Blockchain. These applications demonstrate how third-party tools can connect to and interact with the blockchain through its API.

## Web Wallet

The web wallet (`index.html`) provides a browser-based interface for interacting with the Nilotic Blockchain. It's a single HTML file with embedded JavaScript and CSS, making it easy to deploy and use.

### Features:
- View wallet balance
- Send transactions
- Mine new blocks
- View transaction history
- Explore the blockchain

### Usage:
1. Make sure the Nilotic Blockchain is running (default: http://localhost:5500)
2. Open `index.html` in a web browser
3. The wallet will automatically connect to the blockchain
4. Use the tabs to navigate between different functions

## Command-Line Wallet

The command-line wallet (`nilotic_wallet.py`) provides a Python-based interface for interacting with the Nilotic Blockchain from the terminal.

### Features:
- Get blockchain info
- Check wallet balance
- Send transactions
- Mine blocks
- View blockchain data
- List transactions for an address

### Requirements:
- Python 3.6+
- requests library (`pip install requests`)

### Usage:

```bash
# Make executable
chmod +x nilotic_wallet.py

# Get help
./nilotic_wallet.py --help

# Set blockchain URL and default wallet address
./nilotic_wallet.py --url http://localhost:5500 --address my_wallet_address info

# Get balance
./nilotic_wallet.py --address my_wallet_address balance

# Send a transaction
./nilotic_wallet.py --address my_wallet_address send recipient_address 10.5

# Mine a block
./nilotic_wallet.py --address my_wallet_address mine

# Get all transactions for an address
./nilotic_wallet.py --address my_wallet_address transactions
```

## Test Scripts

The root directory contains several test scripts that can be used to test the functionality of the Nilotic Blockchain and the wallet integration:

### `test_nilotic_blockchain.sh`

This script tests the basic blockchain functionality, including balance checks, transactions, and mining.

```bash
# Run with default URL (http://localhost:5500)
./test_nilotic_blockchain.sh

# Run with custom URL
./test_nilotic_blockchain.sh http://your-blockchain-host:port
```

### `test_odero_slw.sh`

This script tests the Odero SLW token functionality, including creation, verification, and redemption.

```bash
# Run with default URL (http://localhost:5500)
./test_odero_slw.sh

# Run with custom URL
./test_odero_slw.sh http://your-blockchain-host:port
```

### `test_wallet.cpp`

This is a C++ example that demonstrates how to create a wallet application that connects to the Nilotic Blockchain. It includes:

- Basic wallet functionality (balance checking, transactions, mining)
- Transaction tracking
- Error handling

To build and run:

```bash
g++ -std=c++17 -o test_wallet test_wallet.cpp
./test_wallet
```

## Connecting to Your Local Blockchain

By default, both wallet applications connect to `http://localhost:5500`. If your blockchain is running on a different host or port, you can change this:

- **Web Wallet**: Edit the `apiUrl` parameter in the `BlockchainWallet` constructor in the JavaScript code.
- **CLI Wallet**: Use the `--url` command-line parameter.
- **Test Scripts**: Pass the URL as a command-line argument.

## Security Considerations

These wallet applications are for demonstration purposes and lack several security features needed for a production environment:

1. **No Encryption**: Private keys and sensitive data are not encrypted
2. **No Authentication**: The applications do not implement user authentication
3. **No Signature Verification**: Transactions are not cryptographically signed
4. **No Input Validation**: Limited validation of user inputs

## Extending the Wallets

These wallets can be extended with additional features:

1. **Key Management**: Add support for generating and managing cryptographic keys
2. **Transaction Signing**: Implement digital signatures for transactions
3. **Mnemonic Phrases**: Add support for BIP39 mnemonic seed phrases
4. **Address Book**: Store frequently used addresses
5. **Transaction Metadata**: Add support for attaching metadata to transactions
6. **Offline Signing**: Enable signing transactions in an offline environment
7. **Multi-signature Support**: Implement multi-signature transactions
8. **Mobile Applications**: Create mobile wallet apps using frameworks like React Native or Flutter
9. **Desktop Applications**: Create desktop wallet apps using frameworks like Electron