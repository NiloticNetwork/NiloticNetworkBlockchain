# Proof of Resource Contribution (PoRC) Implementation

## 🚀 Quick Start

The Proof of Resource Contribution (PoRC) system has been successfully integrated into the Nilotic Blockchain. This innovative consensus mechanism leverages bandwidth as a resource for transaction processing, enabling wallets to earn rewards by contributing to network infrastructure.

### Prerequisites

- C++17 compiler (GCC 7+ or Clang 5+)
- CMake 3.10+
- OpenSSL development libraries
- SQLite3 development libraries
- Make or Ninja build system

### Building the Project

```bash
# Clone the repository (if not already done)
git clone <repository-url>
cd nilotic-blockchain-clean

# Build the project with PoRC support
./build.sh

# The build script will automatically include PoRC components
```

### Running the Blockchain with PoRC

```bash
# Start the blockchain server with PoRC enabled
./build/nilotic_blockchain --port 5000 --debug

# You should see output like:
# Starting PoRC (Proof of Resource Contribution) system...
# PoRC system started successfully
# Starting API server on port 5000
```

### Testing the PoRC System

```bash
# Run the comprehensive PoRC test suite
./scripts/test_porc.sh

# This will test all PoRC endpoints and functionality
```

## 📁 Implementation Structure

### Core Files

```
include/core/
├── porc.h                 # PoRC system header file
└── api.h                  # Updated API with PoRC endpoints

src/core/
├── porc.cpp               # PoRC system implementation
├── api.cpp                # Updated API with PoRC integration
└── main.cpp               # Updated main with PoRC startup

scripts/
└── test_porc.sh          # PoRC testing script

docs/
├── PORC_DOCUMENTATION.md  # Comprehensive documentation
└── PORC_IMPLEMENTATION_README.md  # This file
```

### Key Components

1. **PoRCSystem**: Main system class managing the entire PoRC mechanism
2. **PoRCTask**: Represents bandwidth-intensive tasks assigned to wallets
3. **PoRCContribution**: Records of bandwidth contributions with cryptographic proofs
4. **PoRCPool**: Rotating pools of wallets for load distribution
5. **PoRCWalletStatus**: Status tracking for individual wallets
6. **PoRCStats**: System-wide statistics and metrics

## 🔧 Configuration

### PoRC Parameters

The system is configured through constants in `include/core/porc.h`:

```cpp
namespace PoRCConfig {
    constexpr uint64_t MIN_BALANCE = 5;                    // Minimum NIL balance
    constexpr uint64_t MIN_ACTIVITY = 1;                   // Minimum transactions
    constexpr uint64_t DAILY_REWARD_POOL = 500;            // Daily reward pool
    constexpr uint64_t BLOCKS_PER_DAY = 36000;             // Blocks per day
    constexpr double BONDING_CURVE_EARLY = 1.5;            // Early adopter bonus
    constexpr uint64_t EARLY_ADOPTER_LIMIT = 1000;         // Early adopter limit
    constexpr uint64_t MAX_REWARD_PER_BLOCK = 0.5;         // Max reward per block
    constexpr uint64_t POOL_SIZE = 100;                    // Wallets per pool
    constexpr uint64_t POOL_ROTATION_BLOCKS = 10;          // Pool rotation frequency
    constexpr double BURN_RATE = 0.5;                      // Transaction fee burn rate
    constexpr double TRANSACTION_FEE = 0.001;              // Transaction fee
    constexpr uint64_t RESOURCE_POINT_MB = 1;              // Points per MB
    constexpr uint64_t RESOURCE_POINT_TX = 10;             // Points per 10 transactions
}
```

## 🌐 API Endpoints

### Enable PoRC for a Wallet

```bash
curl -X POST http://localhost:5000/porc/enable \
  -H "Content-Type: application/json" \
  -d '{
    "address": "wallet_address_here",
    "bandwidthLimit": 50
  }'
```

**Response:**
```json
{
  "success": true,
  "message": "PoRC enabled successfully"
}
```

### Get PoRC Statistics

```bash
curl http://localhost:5000/porc/stats
```

**Response:**
```json
{
  "success": true,
  "stats": {
    "totalWallets": 1,
    "activeWallets": 1,
    "totalResourcePoints": 150,
    "totalRewardsDistributed": 0,
    "totalBurned": 0,
    "currentBlockReward": 0,
    "activePools": 1,
    "averageBandwidth": 100.0,
    "averageUptime": 300.0
  }
}
```

### Submit Contribution Log

```bash
curl -X POST http://localhost:5000/porc/submit_log \
  -H "Content-Type: application/json" \
  -d '{
    "walletAddress": "wallet_address_here",
    "taskId": "task_1234567890_123456",
    "timestamp": 1234567890,
    "blockHeight": 1000,
    "bandwidthUsed": 100,
    "transactionsRelayed": 50,
    "uptimeSeconds": 300,
    "proofHash": "sha256_hash_here",
    "signature": "ecdsa_signature_here"
  }'
```

**Response:**
```json
{
  "success": true,
  "message": "Contribution submitted successfully"
}
```

### Get Wallet Status

```bash
curl http://localhost:5000/porc/wallet/wallet_address_here
```

**Response:**
```json
{
  "success": true,
  "status": {
    "address": "wallet_address_here",
    "isEnabled": true,
    "totalResourcePoints": 150,
    "totalRewards": 0,
    "lastContribution": 1234567890,
    "reputationScore": 1000,
    "bandwidthLimit": 50,
    "isEarlyAdopter": true,
    "poolIndex": 0
  }
}
```

### Get Active Pools

```bash
curl http://localhost:5000/porc/pools
```

**Response:**
```json
{
  "success": true,
  "pools": [
    {
      "poolIndex": 0,
      "walletAddresses": ["wallet_address_here"],
      "totalResourcePoints": 150,
      "blockStart": 1000,
      "blockEnd": 1010,
      "isActive": true
    }
  ]
}
```

## 🗄️ Database Schema

The PoRC system uses SQLite for persistent storage. The database file is created automatically as `porc.db` in the project root.

### Tables

#### wallet_status
Stores wallet configuration and status information.

```sql
CREATE TABLE wallet_status (
    address TEXT PRIMARY KEY,
    is_enabled INTEGER,
    total_resource_points INTEGER,
    total_rewards INTEGER,
    last_contribution INTEGER,
    reputation_score INTEGER,
    bandwidth_limit INTEGER,
    is_early_adopter INTEGER,
    pool_index INTEGER,
    created_at INTEGER,
    updated_at INTEGER
);
```

#### contributions
Stores contribution logs with cryptographic proofs.

```sql
CREATE TABLE contributions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    wallet_address TEXT,
    task_id TEXT,
    timestamp INTEGER,
    block_height INTEGER,
    bandwidth_used INTEGER,
    transactions_relayed INTEGER,
    uptime_seconds INTEGER,
    proof_hash TEXT,
    signature TEXT,
    resource_points INTEGER,
    created_at INTEGER
);
```

#### pools
Manages rotating pools of wallets.

```sql
CREATE TABLE pools (
    pool_index INTEGER PRIMARY KEY,
    wallet_addresses TEXT,
    total_resource_points INTEGER,
    block_start INTEGER,
    block_end INTEGER,
    is_active INTEGER,
    created_at INTEGER
);
```

#### tasks
Tracks assigned tasks and their status.

```sql
CREATE TABLE tasks (
    task_id TEXT PRIMARY KEY,
    type INTEGER,
    assigned_wallet TEXT,
    timestamp INTEGER,
    block_height INTEGER,
    data TEXT,
    estimated_bandwidth INTEGER,
    estimated_transactions INTEGER,
    status INTEGER,
    created_at INTEGER
);
```

## 🔍 Monitoring and Debugging

### Viewing Database Contents

```bash
# List all tables
sqlite3 porc.db '.tables'

# View wallet status
sqlite3 porc.db 'SELECT * FROM wallet_status;'

# View recent contributions
sqlite3 porc.db 'SELECT * FROM contributions ORDER BY created_at DESC LIMIT 10;'

# View active pools
sqlite3 porc.db 'SELECT * FROM pools WHERE is_active = 1;'

# View task assignments
sqlite3 porc.db 'SELECT * FROM tasks ORDER BY created_at DESC LIMIT 10;'
```

### Logging

The PoRC system provides comprehensive logging. Look for these log messages:

```
[INFO] Starting PoRC (Proof of Resource Contribution) system...
[INFO] PoRC system started successfully
[INFO] PoRC enabled for wallet: wallet_address_here
[INFO] Contribution submitted by wallet_address_here - Points: 150
[INFO] PoRC pools rotated - 1 active pools
```

### Performance Monitoring

Monitor system performance with these queries:

```sql
-- Total rewards distributed
SELECT SUM(total_rewards) FROM wallet_status;

-- Average resource points per wallet
SELECT AVG(total_resource_points) FROM wallet_status WHERE is_enabled = 1;

-- Top contributors
SELECT address, total_resource_points, total_rewards 
FROM wallet_status 
WHERE is_enabled = 1 
ORDER BY total_resource_points DESC 
LIMIT 10;

-- Daily contribution statistics
SELECT 
    DATE(created_at, 'unixepoch') as date,
    COUNT(*) as contributions,
    SUM(bandwidth_used) as total_bandwidth,
    SUM(transactions_relayed) as total_transactions
FROM contributions 
GROUP BY DATE(created_at, 'unixepoch')
ORDER BY date DESC;
```

## 🧪 Testing

### Automated Testing

Run the comprehensive test suite:

```bash
./scripts/test_porc.sh
```

This script tests:
- ✅ PoRC system startup
- ✅ API endpoint functionality
- ✅ Wallet enrollment
- ✅ Contribution submission
- ✅ Statistics retrieval
- ✅ Error handling
- ✅ Database operations

### Manual Testing

Test individual components:

```bash
# 1. Start the blockchain
./build/nilotic_blockchain --port 5000 --debug

# 2. In another terminal, test endpoints
curl http://localhost:5000/porc/stats

# 3. Enable PoRC for a test wallet
curl -X POST http://localhost:5000/porc/enable \
  -H "Content-Type: application/json" \
  -d '{"address": "test_wallet", "bandwidthLimit": 50}'

# 4. Submit a test contribution
curl -X POST http://localhost:5000/porc/submit_log \
  -H "Content-Type: application/json" \
  -d '{
    "walletAddress": "test_wallet",
    "taskId": "test_task_123",
    "timestamp": 1234567890,
    "blockHeight": 1000,
    "bandwidthUsed": 100,
    "transactionsRelayed": 50,
    "uptimeSeconds": 300,
    "proofHash": "test_hash",
    "signature": "test_signature"
  }'

# 5. Check updated statistics
curl http://localhost:5000/porc/stats
```

## 🔧 Troubleshooting

### Common Issues

#### 1. Build Errors

**Problem:** Compilation fails with SQLite errors
**Solution:** Ensure SQLite3 development libraries are installed:

```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

# macOS
brew install sqlite3

# CentOS/RHEL
sudo yum install sqlite-devel
```

#### 2. Runtime Errors

**Problem:** PoRC system fails to start
**Solution:** Check database permissions and dependencies:

```bash
# Check if porc.db is created
ls -la porc.db

# Check database integrity
sqlite3 porc.db 'PRAGMA integrity_check;'

# Recreate database if corrupted
rm porc.db
# Restart the blockchain (database will be recreated)
```

#### 3. API Connection Issues

**Problem:** API endpoints return 404 or connection refused
**Solution:** Verify the blockchain server is running:

```bash
# Check if server is listening
netstat -tlnp | grep :5000

# Check server logs
tail -f blockchain.log

# Restart server if needed
pkill nilotic_blockchain
./build/nilotic_blockchain --port 5000 --debug
```

#### 4. Database Lock Issues

**Problem:** SQLite database is locked
**Solution:** Check for multiple instances or file permissions:

```bash
# Check for multiple processes
ps aux | grep nilotic_blockchain

# Check database file permissions
ls -la porc.db

# Fix permissions if needed
chmod 644 porc.db
```

### Debug Mode

Enable detailed logging for troubleshooting:

```bash
# Start with debug logging
./build/nilotic_blockchain --port 5000 --debug

# Monitor logs in real-time
tail -f blockchain.log | grep -i porc
```

## 📈 Performance Optimization

### Database Optimization

```sql
-- Create indexes for better performance
CREATE INDEX IF NOT EXISTS idx_contributions_wallet ON contributions(wallet_address);
CREATE INDEX IF NOT EXISTS idx_contributions_block ON contributions(block_height);
CREATE INDEX IF NOT EXISTS idx_tasks_wallet ON tasks(assigned_wallet);
CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);

-- Analyze database for query optimization
ANALYZE;
```

### Memory Management

The PoRC system uses efficient memory management:
- Connection pooling for database operations
- Thread-safe data structures
- Automatic cleanup of old data
- Configurable cache sizes

### Network Optimization

- Rotating pools limit active contributors
- Bandwidth limits prevent resource exhaustion
- Task batching reduces overhead
- Compression for large data transfers

## 🔮 Future Enhancements

### Planned Features

1. **Layer-2 Integration**: Support for rollups and sidechains
2. **Advanced Analytics**: Real-time performance monitoring
3. **Mobile SDK**: Native mobile wallet integration
4. **Cross-Chain PoRC**: Interoperability with other blockchains

### Research Areas

1. **Dynamic Pool Sizing**: Adaptive pool sizes based on network load
2. **AI-Powered Task Assignment**: Machine learning for optimal distribution
3. **Zero-Knowledge Proofs**: Privacy-preserving contribution verification
4. **Federated Learning**: Distributed model training for task optimization

## 📚 Additional Resources

- [PoRC Technical Documentation](PORC_DOCUMENTATION.md)
- [API Reference](docs/api/README.md)
- [Blockchain Architecture](BLOCKCHAIN_ANALYSIS.md)
- [Development Guidelines](CONTRIBUTING.md)

## 🤝 Contributing

To contribute to the PoRC system:

1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Add tests for new functionality
5. Submit a pull request

### Development Guidelines

- Follow the existing code style
- Add comprehensive documentation
- Include unit tests for new features
- Update the test script for new endpoints
- Ensure backward compatibility

## 📄 License

This implementation is part of the Nilotic Blockchain project and follows the same licensing terms.

---

**🎉 Congratulations!** You now have a fully functional Proof of Resource Contribution (PoRC) system integrated into the Nilotic Blockchain. This innovative consensus mechanism provides a sustainable, scalable, and accessible way for users to contribute to the network and earn rewards.
