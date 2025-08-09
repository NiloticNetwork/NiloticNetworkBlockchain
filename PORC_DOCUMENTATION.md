# Proof of Resource Contribution (PoRC) Documentation

## Overview

Proof of Resource Contribution (PoRC) is an innovative consensus mechanism integrated into the Nilotic Blockchain that leverages bandwidth as a resource for transaction processing. This system enables wallets with NIL tokens to contribute bandwidth (data relayed for transaction propagation or block dissemination) and earn rewards proportional to their contribution.

## Key Features

- **Bandwidth-Based Rewards**: Wallets earn rewards by relaying transactions and propagating blocks
- **Rotating Pools**: Wallets are organized into rotating pools to distribute network load
- **Bonding Curve**: Early adopters receive 1.5x rewards to incentivize early participation
- **Deflationary Mechanism**: 50% of transaction fees are burned to offset reward issuance
- **Low Resource Requirements**: Works on basic smartphones with 3G/4G connectivity
- **Automatic Task Assignment**: System automatically assigns bandwidth-intensive tasks to eligible wallets

## Technical Architecture

### Core Components

1. **PoRCSystem**: Main system class managing the entire PoRC mechanism
2. **PoRCTask**: Represents bandwidth-intensive tasks assigned to wallets
3. **PoRCContribution**: Records of bandwidth contributions with cryptographic proofs
4. **PoRCPool**: Rotating pools of wallets for load distribution
5. **PoRCWalletStatus**: Status tracking for individual wallets
6. **PoRCStats**: System-wide statistics and metrics

### Database Schema

The PoRC system uses SQLite for persistent storage with the following tables:

```sql
-- Wallet status and configuration
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

-- Contribution logs
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

-- Pool management
CREATE TABLE pools (
    pool_index INTEGER PRIMARY KEY,
    wallet_addresses TEXT,
    total_resource_points INTEGER,
    block_start INTEGER,
    block_end INTEGER,
    is_active INTEGER,
    created_at INTEGER
);

-- Task tracking
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

## Configuration Parameters

```cpp
namespace PoRCConfig {
    constexpr uint64_t MIN_BALANCE = 5;                    // Minimum NIL balance required
    constexpr uint64_t MIN_ACTIVITY = 1;                   // Minimum transactions in 30 days
    constexpr uint64_t DAILY_REWARD_POOL = 500;            // Total daily reward pool in NIL
    constexpr uint64_t BLOCKS_PER_DAY = 36000;             // Blocks per day (2.4s block time)
    constexpr double BONDING_CURVE_EARLY = 1.5;            // Early adopter multiplier
    constexpr uint64_t EARLY_ADOPTER_LIMIT = 1000;         // First 1000 wallets get 1.5x
    constexpr uint64_t MAX_REWARD_PER_BLOCK = 0.5;         // Max reward per wallet per block
    constexpr uint64_t POOL_SIZE = 100;                    // Wallets per rotating pool
    constexpr uint64_t POOL_ROTATION_BLOCKS = 10;          // Rotate pools every 10 blocks
    constexpr double BURN_RATE = 0.5;                      // 50% of transaction fees burned
    constexpr double TRANSACTION_FEE = 0.001;              // Transaction fee in NIL
    constexpr uint64_t RESOURCE_POINT_MB = 1;              // 1 point per MB relayed
    constexpr uint64_t RESOURCE_POINT_TX = 10;             // 1 point per 10 transactions
}
```

## Mathematical Model

### Reward Calculation

The reward for wallet `i` per block is calculated as:

```
R_i = B × (C_i / T) × R × min(1, S_i / S_min) × min(1, A_i / A_min)
```

Where:
- `B` = Bonding curve multiplier (1.5 for early adopters, 1.0 for later participants)
- `C_i` = Contribution of wallet `i` (resource points)
- `T` = Total resource points contributed by all wallets in a block
- `R` = Total PoRC reward pool per block (0.01389 NIL)
- `S_i` = NIL balance of wallet `i` (minimum 5 NIL)
- `A_i` = Activity score of wallet `i` (number of transactions in past 30 days)
- `S_min` = 5 NIL (minimum balance)
- `A_min` = 1 transaction (minimum activity)

### Resource Points Calculation

Resource points are calculated as:
```
Points = (Bandwidth Used × 1) + (Transactions Relayed ÷ 10)
```

### Economic Model

- **Daily Reward Pool**: 500 NIL (1.38% of current 36,300 NIL supply annually)
- **Per-Block Reward**: 0.01389 NIL
- **Burn Rate**: 50% of transaction fees (0.0005 NIL per transaction)
- **Net Issuance**: ~320 NIL/day (deflationary pressure)

## API Endpoints

### Enable PoRC
```
POST /porc/enable
Content-Type: application/json

{
    "address": "wallet_address",
    "bandwidthLimit": 50
}
```

### Get Statistics
```
GET /porc/stats
```

### Submit Contribution Log
```
POST /porc/submit_log
Content-Type: application/json

{
    "walletAddress": "wallet_address",
    "taskId": "task_id",
    "timestamp": 1234567890,
    "blockHeight": 1000,
    "bandwidthUsed": 100,
    "transactionsRelayed": 50,
    "uptimeSeconds": 300,
    "proofHash": "sha256_hash",
    "signature": "ecdsa_signature"
}
```

### Get Wallet Status
```
GET /porc/wallet/{address}
```

### Get Active Pools
```
GET /porc/pools
```

## Integration with Nilotic Blockchain

### System Startup

The PoRC system is automatically started when the blockchain server starts:

```cpp
// In main.cpp
API api(blockchain);

// Start PoRC system
Logger::info("Starting PoRC (Proof of Resource Contribution) system...");
if (api.getPoRCSystem().start()) {
    Logger::info("PoRC system started successfully");
} else {
    Logger::error("Failed to start PoRC system");
}
```

### Block Mining Integration

The PoRC system integrates with the blockchain's block mining process:

```cpp
void PoRCSystem::onBlockMined(uint64_t blockHeight) {
    currentBlockHeight = blockHeight;
    
    // Rotate pools if needed
    if (blockHeight % PoRCConfig::POOL_ROTATION_BLOCKS == 0) {
        rotatePools();
    }
    
    // Process contributions and distribute rewards
    distributeRewards();
}
```

### Transaction Processing

When new transactions are created, the PoRC system generates new tasks:

```cpp
void PoRCSystem::onTransactionCreated(const std::string& transactionId) {
    // Generate new tasks for transaction relay
    generateTasks();
}
```

## Task Types

### 1. Transaction Relay
- **Purpose**: Relay pending transactions to other nodes
- **Bandwidth**: ~10 MB per task
- **Transactions**: ~50 transactions per task
- **Reward**: Proportional to bandwidth used

### 2. Block Propagation
- **Purpose**: Assist in propagating newly mined blocks
- **Bandwidth**: ~5 MB per task
- **Transactions**: 0 (block data only)
- **Reward**: Proportional to bandwidth used

### 3. Data Caching
- **Purpose**: Cache data for offline users (Odero SLW integration)
- **Bandwidth**: ~20 MB per task
- **Transactions**: 0
- **Reward**: Proportional to bandwidth used

### 4. Peer Verification
- **Purpose**: Verify peer connectivity and health
- **Bandwidth**: ~2 MB per task
- **Transactions**: 0
- **Reward**: Proportional to bandwidth used

## Security Features

### Cryptographic Proofs
- All contributions are signed with ECDSA signatures
- Proof-of-usage logs are cryptographically verified
- SHA-256 hashing for data integrity

### Anti-Sybil Measures
- Minimum NIL balance requirement (5 NIL)
- Minimum transaction activity requirement (1 transaction in 30 days)
- Reputation scoring based on historical contributions

### Rate Limiting
- Maximum reward per wallet per block (0.5 NIL)
- Bandwidth limits per wallet (configurable, default 50 MB/day)
- Pool rotation to prevent concentration

## Performance Characteristics

### Scalability
- **Rotating Pools**: Limit active contributors per block to 100-500 wallets
- **Caching**: Use existing caching layer to minimize redundant data transfers
- **Layer-2 Potential**: Future integration with rollups for high transaction volumes

### Resource Requirements
- **Hardware**: Minimal; supports low-end devices (e.g., 3G smartphones with 10 KB/s bandwidth)
- **Software**: C++17, SQLite 3.0+, OpenSSL
- **Network**: P2P layer extended to track bandwidth metrics

### Network Efficiency
- **Bandwidth Measurement**: Lightweight benchmarking embedded in wallets
- **Fallback Mode**: Offline mode (Odero SLW) for low-connectivity users
- **Compression**: Optional data compression for bandwidth optimization

## Comparison with Other Consensus Mechanisms

| Mechanism | PoRC (Nilotic) | PoW (Bitcoin) | PoS (Ethereum) | DPoS (EOS) |
|-----------|----------------|---------------|----------------|------------|
| Resource Used | Bandwidth | Computational power | Token stake | Delegated stake |
| Energy Efficiency | High | Low | High | High |
| Accessibility | High | Low | Moderate | Low |
| Decentralization | High | Moderate | Moderate | Low |
| Scalability | High | Low | Moderate | High |
| Reward Fairness | High | Moderate | Moderate | Low |
| Security | Strong | Strong | Strong | Moderate |
| Innovation | Novel | Established | Established | Established |

## Future Enhancements

### Planned Features
1. **Layer-2 Integration**: Support for rollups and sidechains
2. **Additional Resource Types**: Storage and computation contributions
3. **Mobile Provider Partnerships**: Subsidized data costs for African markets
4. **Advanced Analytics**: Real-time performance monitoring and optimization

### Research Areas
1. **Dynamic Pool Sizing**: Adaptive pool sizes based on network load
2. **Cross-Chain PoRC**: Interoperability with other blockchains
3. **AI-Powered Task Assignment**: Machine learning for optimal task distribution
4. **Zero-Knowledge Proofs**: Privacy-preserving contribution verification

## Development and Testing

### Building with PoRC Support

```bash
# Clone the repository
git clone <repository-url>
cd nilotic-blockchain-clean

# Build the project
./build.sh

# Run the blockchain with PoRC
./build/nilotic_blockchain --port 5000 --debug
```

### Testing PoRC Endpoints

```bash
# Enable PoRC for a wallet
curl -X POST http://localhost:5000/porc/enable \
  -H "Content-Type: application/json" \
  -d '{"address": "test_wallet_address", "bandwidthLimit": 50}'

# Get PoRC statistics
curl http://localhost:5000/porc/stats

# Submit a contribution log
curl -X POST http://localhost:5000/porc/submit_log \
  -H "Content-Type: application/json" \
  -d '{
    "walletAddress": "test_wallet_address",
    "taskId": "task_1234567890_123456",
    "timestamp": 1234567890,
    "blockHeight": 1000,
    "bandwidthUsed": 100,
    "transactionsRelayed": 50,
    "uptimeSeconds": 300,
    "proofHash": "abc123...",
    "signature": "def456..."
  }'
```

### Monitoring and Debugging

The PoRC system provides comprehensive logging:

```cpp
Logger::info("PoRC enabled for wallet: " + address);
Logger::info("Contribution submitted by " + address + " - Points: " + std::to_string(points));
Logger::info("PoRC pools rotated - " + std::to_string(pools.size()) + " active pools");
```

## Conclusion

The Proof of Resource Contribution (PoRC) system represents a significant innovation in blockchain consensus mechanisms. By leveraging bandwidth as a resource for transaction processing, PoRC provides:

1. **Accessibility**: Enables participation from low-resource devices
2. **Efficiency**: High energy efficiency compared to PoW
3. **Fairness**: Proportional rewards with bonding curve incentives
4. **Scalability**: Rotating pools and layer-2 potential
5. **Cultural Alignment**: Supports the Nilotic Blockchain's African focus

The system is designed to be sustainable, secure, and scalable while providing generous yet controlled rewards to incentivize participation without inflating the NIL token supply. The integration with the existing hybrid PoW/PoS system creates a comprehensive consensus mechanism that addresses the limitations of traditional approaches.

For more information, refer to the technical specification and mathematical model provided in the whitepaper section.
