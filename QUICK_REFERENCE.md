# 🚀 Nilotic Blockchain Quick Reference

## 🔗 API Endpoints

### Core Blockchain
```bash
# Get blockchain status
GET http://localhost:5000/

# Get chain information
GET http://localhost:5000/chain?include_blocks=true&limit=10

# Get balance
GET http://localhost:5000/balance?address=NILabandonaachievea
```

### Transactions
```bash
# Send transaction
POST http://localhost:5000/transaction
{
  "sender": "NILabandonaachievea",
  "recipient": "NIL2af6bf62441121f9df940a46fc0ee6a5b8",
  "amount": 100.0
}

# Deploy smart contract
POST http://localhost:5000/contract/deploy
{
  "sender": "NILabandonaachievea",
  "code": "function hello() { return \"Hello World\"; }",
  "language": "javascript"
}
```

### Mining
```bash
# Mine a block
POST http://localhost:5000/mine
{
  "miner_address": "NILda9879380c1efaff4aede80339f2e35fac"
}

# Start mining
POST http://localhost:5000/mining/start
{
  "miner_address": "NILda9879380c1efaff4aede80339f2e35fac"
}

# Get mining status
GET http://localhost:5000/mining/status
```

### Staking & PoS
```bash
# Stake tokens
POST http://localhost:5000/stake
{
  "address": "NILabandonaachievea",
  "amount": 100.0,
  "type": "validator"
}

# Validate block (PoS)
POST http://localhost:5000/consensus/validate
{
  "validator_address": "NILabandonaachievea",
  "block_hash": "0000a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef",
  "signature": "validator_signature_here"
}
```

### Offline Payments (Odero SLW)
```bash
# Create offline token
POST http://localhost:5000/odero/create
{
  "creator": "NILabandonaachievea",
  "amount": 50.0
}

# Redeem offline token
POST http://localhost:5000/odero/redeem
{
  "redeemer": "NIL2af6bf62441121f9df940a46fc0ee6a5b8",
  "tokenId": "OSLWf1a2b3c4d5e6f7"
}
```

## 📊 Sample Responses

### Blockchain Status
```json
{
  "status": "Nilotic Blockchain API is running",
  "version": "0.1.0",
  "chain_height": 5,
  "pending_transactions": 3,
  "difficulty": 4,
  "mining_reward": 100.0
}
```

### Transaction Response
```json
{
  "success": true,
  "message": "Transaction added to pending pool",
  "transaction_hash": "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef",
  "sender": "NILabandonaachievea",
  "recipient": "NIL2af6bf62441121f9df940a46fc0ee6a5b8",
  "amount": 100.0,
  "fee": 0.001,
  "timestamp": 1703123456
}
```

### Mining Response
```json
{
  "success": true,
  "message": "Block mined successfully",
  "block_hash": "0000a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef",
  "block_index": 5,
  "transactions_count": 3,
  "mining_time_ms": 2450,
  "difficulty": 4,
  "nonce": 12345
}
```

### Balance Response
```json
{
  "address": "NILabandonaachievea",
  "balance": 450.0,
  "staked_amount": 100.0,
  "pending_rewards": 12.5,
  "total_value": 562.5,
  "transaction_count": 15,
  "last_activity": "2024-12-20T10:30:00Z"
}
```

## 🧪 Testing Commands

### Run Complete Test Suite
```bash
./test_blockchain_interactions.sh
```

### Individual Test Commands
```bash
# Test blockchain status
curl -X GET http://localhost:5000/

# Test transaction
curl -X POST http://localhost:5000/transaction \
  -H "Content-Type: application/json" \
  -d '{"sender":"NILabandonaachievea","recipient":"NIL2af6bf62441121f9df940a46fc0ee6a5b8","amount":100.0}'

# Test mining
curl -X POST http://localhost:5000/mine \
  -H "Content-Type: application/json" \
  -d '{"miner_address":"NILda9879380c1efaff4aede80339f2e35fac"}'

# Test balance check
curl -X GET "http://localhost:5000/balance?address=NILabandonaachievea"
```

## 📈 Key Metrics

| Metric | Value | Description |
|--------|-------|-------------|
| **Block Time** | ~10 minutes | Target block generation time |
| **Difficulty** | 4 | Leading zeros required |
| **Mining Reward** | 100.0 NIL | Reward per block |
| **Transaction Fee** | 0.001 NIL | Fee per transaction |
| **Staking APY** | 18% | Validator rewards |
| **Regular Staking** | 12% | Standard staking rewards |
| **Max Tx/Block** | 10 | Maximum transactions per block |

## 🔧 Configuration

### Mining Configuration
```cpp
struct MiningConfig {
    uint64_t targetDifficulty = 4;
    uint64_t maxDifficulty = 8;
    uint64_t minDifficulty = 2;
    uint64_t targetBlockTime = 600;  // 10 minutes
    double miningReward = 100.0;
    double transactionFee = 0.001;
};
```

### Blockchain Parameters
- **Genesis Block Reward:** 1000.0 NIL
- **Block Reward:** 100.0 NIL
- **Transaction Fee:** 0.001 NIL
- **Minimum Stake:** 1000.0 NIL
- **Validator Minimum:** 10000.0 NIL

## 🎯 Common Use Cases

### 1. Send Money
```bash
curl -X POST http://localhost:5000/transaction \
  -H "Content-Type: application/json" \
  -d '{
    "sender": "NILabandonaachievea",
    "recipient": "NIL2af6bf62441121f9df940a46fc0ee6a5b8",
    "amount": 50.0
  }'
```

### 2. Mine for Rewards
```bash
curl -X POST http://localhost:5000/mine \
  -H "Content-Type: application/json" \
  -d '{"miner_address": "NILda9879380c1efaff4aede80339f2e35fac"}'
```

### 3. Stake for Validation
```bash
curl -X POST http://localhost:5000/stake \
  -H "Content-Type: application/json" \
  -d '{
    "address": "NILabandonaachievea",
    "amount": 100.0,
    "type": "validator"
  }'
```

### 4. Deploy Smart Contract
```bash
curl -X POST http://localhost:5000/contract/deploy \
  -H "Content-Type: application/json" \
  -d '{
    "sender": "NILabandonaachievea",
    "code": "function hello() { return \"Hello World\"; }",
    "language": "javascript"
  }'
```

### 5. Create Offline Payment
```bash
curl -X POST http://localhost:5000/odero/create \
  -H "Content-Type: application/json" \
  -d '{
    "creator": "NILabandonaachievea",
    "amount": 25.0
  }'
```

## ⚠️ Important Notes

1. **Security:** Current implementation uses demo keys - not for production
2. **Testing:** Use test addresses for development
3. **Mining:** Difficulty adjusts automatically based on block time
4. **Staking:** Tokens are locked for validation period
5. **Smart Contracts:** Support JavaScript, Python, and Solidity

## 🔍 Troubleshooting

### Common Issues
1. **Connection Refused:** Ensure blockchain server is running
2. **Invalid Transaction:** Check sender balance
3. **Mining Fails:** Verify miner address format
4. **Staking Fails:** Ensure sufficient balance

### Debug Commands
```bash
# Check server status
curl -X GET http://localhost:5000/

# Check pending transactions
curl -X GET http://localhost:5000/chain

# Check specific balance
curl -X GET "http://localhost:5000/balance?address=NILabandonaachievea"
```

---

**Quick Start:** Run `./test_blockchain_interactions.sh` to test all features! 