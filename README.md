# Nilotic Blockchain

An open, lightweight C++ blockchain focused on inclusive finance in Africa: low‑bandwidth, offline‑first payments with transparent, programmable settlement and corridor FX. Built for real‑world constraints (feature phones, agents, and spotty connectivity).

## 🚀 Quick Start

```bash
# Clone the repository
git clone https://github.com/NiloticNetwork/NiloticNetworkBlockchain
cd nilotic-blockchain

# Build the application
./scripts/build/build.sh

# Run the blockchain
./build/nilotic_blockchain --port 5500 --debug

# Access the web wallet
open web/wallet/index.html
```

## 📋 Features

- **Offline-first payments**: Local acknowledgements and delayed on‑chain settlement
- **Low-bandwidth operation**: USSD/SMS agent flows, lightweight nodes
- **Proof-of-Stake**: Energy‑efficient consensus
- **Native Token**: Sulwe (SLW) with 1 SLW = 1,000,000 Lut precision
- **Odero SLW Tokens**: Offline payment capabilities for cash‑dominant contexts
- **Smart Contracts**: Basic programmable logic
- **API Interface**: RESTful API with OpenAPI spec
- **SQLite Persistence**: Reliable data storage (WAL recommended)

## 🏗️ Project Structure

```
nilotic-blockchain/
├── src/                    # Source code
│   ├── core/              # Core blockchain functionality
│   ├── api/               # API implementation
│   ├── wallet/            # Wallet functionality
│   └── persistence/       # Data persistence
├── include/               # Header files
│   ├── core/              # Core headers
│   ├── api/               # API headers
│   ├── wallet/            # Wallet headers
│   └── persistence/       # Persistence headers
├── web/                   # Web applications
│   ├── wallet/            # Web wallet interface
│   └── dapps/             # Decentralized applications
├── scripts/               # Build and deployment scripts
│   ├── build/             # Build scripts
│   ├── deploy/            # Deployment scripts
│   └── test/              # Testing scripts
├── docs/                  # Documentation
│   ├── api/               # API documentation
│   ├── user/              # User guides
│   └── developer/         # Developer documentation
├── tests/                 # Test suites
│   ├── unit/              # Unit tests
│   └── integration/       # Integration tests
├── examples/              # Example applications
├── config/                # Configuration files
└── lib/                   # Third-party libraries
```

## 🔧 Installation

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 6+, MSVC 2019+)
- CMake 3.10 or higher
- OpenSSL 1.1.1 or higher
- SQLite 3
- Git

### Build Instructions

#### Linux/macOS
```bash
# Install dependencies
sudo apt-get install build-essential cmake libssl-dev libsqlite3-dev

# Build the project
./scripts/build/build.sh

# Run the application
./build/nilotic_blockchain --port 5500 --debug
```

#### Windows
```bash
# Build using Visual Studio
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release

# Run the application
Release\nilotic_blockchain.exe --port 5500 --debug
```

## 🌐 Usage

### Command Line Interface

```bash
# Start the blockchain server
./build/nilotic_blockchain --port 5500 --debug

# Available options:
# --port <port>          # Set HTTP server port (default: 5000)
# --debug               # Enable debug logging
# --data-dir <path>     # Set data directory for storage
# --peers <ip:port>     # Comma-separated list of peers
# --no-mining           # Disable automatic mining
# --help                # Display usage information
```

### Web Wallet

1. Start the blockchain server
2. Open `web/wallet/index.html` in your browser
3. The wallet will automatically connect to the blockchain
4. Use the interface to:
   - View wallet balance
   - Send transactions
   - Mine new blocks
   - View transaction history

### API Interface

The blockchain exposes a RESTful API:

```bash
# Get blockchain info
curl http://localhost:5500/

# Get balance
curl http://localhost:5500/balance?address=test_wallet

# Mine a block
curl -X POST -H "Content-Type: application/json" \
  -d '{"miner_address":"test_wallet"}' \
  http://localhost:5500/mine

# Send transaction
curl -X POST -H "Content-Type: application/json" \
  -d '{"sender":"sender_address","recipient":"recipient_address","amount":10.5}' \
  http://localhost:5500/transaction
```

### Python Wallet

```bash
# Get blockchain info
python3 web/wallet/nilotic_wallet.py --url http://localhost:5500 info

# Check balance
python3 web/wallet/nilotic_wallet.py --url http://localhost:5500 balance --address test_wallet

# Mine a block
python3 web/wallet/nilotic_wallet.py --url http://localhost:5500 mine --address test_wallet

# Send transaction
python3 web/wallet/nilotic_wallet.py --url http://localhost:5500 send --address sender --recipient receiver --amount 10
```

## 🧪 Testing

```bash
# Run unit tests
./scripts/test/run_unit_tests.sh

# Run integration tests
./scripts/test/run_integration_tests.sh

# Run all tests
./scripts/test/run_all_tests.sh
```

## 📚 Documentation

- [Value Proposition](docs/VALUE_PROPOSITION.md)
- [Roadmap](docs/ROADMAP.md)
- [Business Model](docs/BUSINESS_MODEL.md)
- [Tokenomics](docs/TOKENOMICS.md)
- [Team](docs/TEAM.md)
- [Partnerships](docs/PARTNERSHIPS.md)
- [Compliance Strategy](docs/COMPLIANCE.md)
- [Security Audit Plan](docs/SECURITY_AUDIT_PLAN.md)
- [Traction Plan](docs/TRACION_PLAN.md)
- [Grants & Accelerators](docs/GRANTS_AND_ACCELERATORS.md)
- [OpenAPI Spec](docs/api/openapi.yaml)
- [API Documentation](docs/api/README.md)
- [User Guide](docs/user/README.md) - How to use the blockchain
- [Developer Guide](docs/developer/README.md) - How to extend the blockchain
- [Installation Guide](INSTALL.md) - Detailed installation instructions

## 🔐 Security

The Nilotic Blockchain implements several security features:

- SHA-256 hashing for block integrity
- Proof-of-Stake consensus mechanism
- Transaction validation
- Cryptographic token IDs for Odero SLW tokens
- Input validation and sanitization

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆘 Support

- **Issues**: Report bugs and feature requests on GitHub
- **Documentation**: Check the [docs](docs/) directory
- **Examples**: See the [examples](examples/) directory for usage examples

## 🗺️ Roadmap

- [ ] Enhanced smart contract system
- [ ] Mobile wallet applications
- [ ] Cross-chain interoperability
- [ ] Advanced consensus mechanisms
- [ ] Enterprise features

---

**Built with ❤️ for the blockchain community**
