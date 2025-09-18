# Nilotic Blockchain API Endpoints

This document provides a comprehensive list of all implemented API endpoints for the Nilotic Blockchain.

## Base URL
```
http://localhost:5500
```

## Authentication
Currently, the API does not require authentication. All endpoints are publicly accessible.

## Response Format
All API responses are returned in JSON format.

## Core Blockchain Endpoints

### GET /
Get basic information about the blockchain node.

**Response:**
```json
{
  "status": "Nilotic Blockchain API is running",
  "version": "1.0.0",
  "chain_height": 1,
  "pending_transactions": 0,
  "difficulty": 4,
  "mining_reward": 100.0,
  "success": true
}
```

### GET /info
Get detailed blockchain information.

**Response:**
```json
{
  "chainId": "nilotic-chain-1",
  "chainHeight": 1,
  "blockCount": 1,
  "isValid": true,
  "pendingTransactions": 0,
  "difficulty": 4,
  "miningReward": 100.0,
  "status": "success"
}
```

### GET /chain
Get blockchain data including recent blocks.

**Response:**
```json
{
  "chainHeight": 1,
  "blockCount": 1,
  "status": "success",
  "blocks": [
    {
      "index": 0,
      "timestamp": "2025-01-09T17:00:00Z",
      "transactions": [...],
      "proof": 100,
      "previous_hash": "1"
    }
  ]
}
```

### GET /balance/{address}
Get wallet balance for a specific address.

**Response:**
```json
{
  "address": "test_address",
  "balance": 1000.0,
  "stake": 0.0,
  "status": "success"
}
```

### GET /block/latest
Get the latest block information.

**Response:**
```json
{
  "index": 1,
  "timestamp": "2025-01-09T17:00:00Z",
  "transactions": [...],
  "proof": 100,
  "previous_hash": "0000...",
  "hash": "0000..."
}
```

### GET /block/{index}
Get block information by index.

**Response:**
```json
{
  "index": 0,
  "timestamp": "2025-01-09T17:00:00Z",
  "transactions": [...],
  "proof": 100,
  "previous_hash": "1"
}
```

## Transaction Endpoints

### POST /transaction
Create a new transaction.

**Request Body:**
```json
{
  "sender": "sender_address",
  "recipient": "recipient_address",
  "amount": 10.5,
  "type": "transfer"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Transaction added to pending pool",
  "transaction_id": "tx_hash"
}
```

### GET /transactions
Get all pending transactions.

**Response:**
```json
{
  "transactions": [
    {
      "sender": "sender_address",
      "recipient": "recipient_address",
      "amount": 10.5,
      "hash": "tx_hash",
      "timestamp": 1754749778
    }
  ],
  "count": 1,
  "status": "success"
}
```

### GET /transactions/pending
Get pending transactions count.

**Response:**
```json
{
  "pending_count": 1,
  "status": "success"
}
```

### GET /transactions/confirmed
Get confirmed transactions count.

**Response:**
```json
{
  "confirmed_count": 10,
  "status": "success"
}
```

## Mining Endpoints

### POST /mine
Mine a new block.

**Request Body:**
```json
{
  "miner_address": "miner_address"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Block mined successfully",
  "block_index": 2,
  "block_hash": "0000...",
  "miner_address": "miner_address",
  "difficulty": 4,
  "reward": 100.0
}
```

### GET /mining/status
Get mining status and statistics.

**Response:**
```json
{
  "status": "success",
  "isMining": false,
  "currentDifficulty": 4,
  "hashRate": 0,
  "estimatedTimeToNextBlock": 0,
  "pendingTransactions": 0,
  "miningStats": {...}
}
```

### POST /mining/start
Start mining process.

**Request Body:**
```json
{
  "miner_address": "miner_address"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Mining started successfully",
  "miner_address": "miner_address",
  "difficulty": 4
}
```

### POST /mining/stop
Stop mining process.

**Response:**
```json
{
  "status": "success",
  "message": "Mining stopped successfully",
  "isMining": false
}
```

## Consensus Endpoints

### POST /consensus/join
Join consensus as a participant.

**Request Body:**
```json
{
  "address": "participant_address",
  "method": "pow"
}
```

**Methods:**
- `"pow"` - Proof of Work
- `"pos"` - Proof of Stake (requires `stake` field)
- `"porc"` - Proof of Resource Contribution (requires `bandwidth` field)

**Response:**
```json
{
  "success": true,
  "message": "Successfully joined consensus"
}
```

### POST /consensus/leave
Leave consensus.

**Request Body:**
```json
{
  "address": "participant_address"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Successfully left consensus"
}
```

### GET /consensus/stats
Get consensus statistics.

**Response:**
```json
{
  "success": true,
  "stats": {
    "totalRounds": 25,
    "totalValidations": 0,
    "methodWins": null,
    "methodRewards": null,
    "totalRewardsDistributed": 0.0,
    "averageRoundTime": 29,
    "activeParticipants": 0
  }
}
```

### GET /consensus/rounds
Get active consensus rounds.

**Response:**
```json
{
  "success": true,
  "rounds": [
    {
      "roundId": 16,
      "blockHeight": 1,
      "tasks": [],
      "participants": {
        "0": ["test"]
      },
      "methodRewards": null,
      "startTime": 1754749748,
      "endTime": 1754749778,
      "isCompleted": false
    }
  ]
}
```

### GET /consensus/participants
Get active consensus participants.

**Response:**
```json
{
  "success": true,
  "participants": [
    {
      "address": "test",
      "method": 0,
      "status": 2,
      "stake": 0.0,
      "bandwidth": 0,
      "hashRate": 0,
      "reputationScore": 0.95,
      "lastActivity": 1754749733,
      "totalRewards": 0,
      "successfulValidations": 0,
      "failedValidations": 0
    }
  ]
}
```

### POST /consensus/submit_transaction
Submit transaction for validation.

**Request Body:**
```json
{
  "transaction_id": "tx_hash"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Transaction submitted for validation"
}
```

### POST /consensus/submit_result
Submit validation result.

**Request Body:**
```json
{
  "task_id": "task_id",
  "participant_address": "participant_address",
  "success": true
}
```

**Response:**
```json
{
  "success": true,
  "message": "Validation result submitted"
}
```

## PoRC (Proof of Resource Contribution) Endpoints

### POST /porc/enable
Enable PoRC for a wallet.

**Request Body:**
```json
{
  "address": "wallet_address",
  "bandwidth": 100
}
```

**Response:**
```json
{
  "success": true,
  "message": "PoRC enabled successfully"
}
```

### GET /porc/stats
Get PoRC statistics.

**Response:**
```json
{
  "success": true,
  "stats": {
    "totalWallets": 0,
    "activeWallets": 0,
    "totalResourcePoints": 0,
    "totalRewardsDistributed": 0,
    "totalBurned": 0,
    "averageBandwidth": 0.0,
    "averageUptime": 0.0,
    "activePools": 0,
    "currentBlockReward": 0
  }
}
```

### POST /porc/submit_log
Submit contribution log.

**Request Body:**
```json
{
  "address": "wallet_address",
  "bandwidth": 100,
  "uptime": 3600,
  "proof": "cryptographic_proof"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Contribution log submitted"
}
```

### GET /porc/wallet/{address}
Get wallet PoRC status.

**Response:**
```json
{
  "success": true,
  "wallet": {
    "address": "wallet_address",
    "isEnabled": true,
    "bandwidth": 100,
    "totalContributions": 0,
    "totalRewards": 0.0
  }
}
```

### GET /porc/pools
Get active pools.

**Response:**
```json
{
  "success": true,
  "pools": []
}
```

## Staking Endpoints

### POST /stake
Stake tokens.

**Request Body:**
```json
{
  "address": "wallet_address",
  "amount": 100.0,
  "type": "validator"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Staking request received",
  "address": "wallet_address",
  "amount": 100.0,
  "type": "validator",
  "stake_id": "stake_1754749778"
}
```

### POST /stake/unstake
Unstake tokens.

**Request Body:**
```json
{
  "address": "wallet_address",
  "amount": 50.0
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Unstaking request received",
  "address": "wallet_address",
  "amount": 50.0
}
```

### POST /stake/rewards
Claim staking rewards.

**Request Body:**
```json
{
  "address": "wallet_address"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Reward claim request received",
  "address": "wallet_address",
  "reward_amount": 0.0
}
```

### GET /stake/status
Get staking status.

**Response:**
```json
{
  "status": "success",
  "total_staked": 0.0,
  "active_validators": 0,
  "staking_rewards_pool": 0.0
}
```

## Network Endpoints

### GET /network/status
Get network status.

**Response:**
```json
{
  "status": "success",
  "isRunning": false,
  "activeConnections": 0,
  "totalPeers": 0,
  "totalMessagesReceived": 0,
  "totalMessagesSent": 0,
  "listenPort": 8333
}
```

### GET /network/peers
Get peer list.

**Response:**
```json
{
  "status": "success",
  "peers": [],
  "total_peers": 0,
  "connected_peers": 0
}
```

### POST /network/connect
Connect to peer.

**Request Body:**
```json
{
  "address": "peer_address",
  "port": 8333
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Connection request sent",
  "address": "peer_address",
  "port": 8333
}
```

### POST /network/disconnect
Disconnect from peer.

**Request Body:**
```json
{
  "address": "peer_address"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Disconnection request sent",
  "address": "peer_address"
}
```

## Wallet Endpoints

### POST /wallet/create
Create new wallet.

**Request Body:**
```json
{
  "name": "wallet_name",
  "password": "password"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Wallet created successfully",
  "address": "wallet_address",
  "name": "wallet_name",
  "seedPhrase": "seed phrase words"
}
```

### POST /wallet/import
Import wallet.

**Request Body:**
```json
{
  "name": "wallet_name",
  "password": "password"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Wallet imported successfully",
  "address": "wallet_address",
  "name": "wallet_name"
}
```

### POST /wallet/sign
Sign transaction.

**Request Body:**
```json
{
  "private_key": "private_key_pem",
  "password": "password",
  "transaction_data": "transaction_data"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Transaction signed successfully",
  "signature": "signature",
  "address": "wallet_address"
}
```

## Analytics Endpoints

### GET /analytics/blockchain
Get blockchain analytics.

**Response:**
```json
{
  "status": "success",
  "total_blocks": 1,
  "total_transactions": 1,
  "total_volume": 1000.0,
  "average_block_time": 0,
  "current_difficulty": 4,
  "mining_reward": 100.0
}
```

### GET /analytics/mining
Get mining analytics.

**Response:**
```json
{
  "status": "success",
  "is_mining": false,
  "current_difficulty": 4,
  "hash_rate": 0,
  "estimated_time_to_next_block": 0,
  "mining_stats": {...}
}
```

### GET /analytics/consensus
Get consensus analytics.

**Response:**
```json
{
  "status": "success",
  "consensus_stats": {...},
  "active_rounds": 5,
  "active_participants": 1
}
```

### GET /analytics/porc
Get PoRC analytics.

**Response:**
```json
{
  "status": "success",
  "porc_stats": {...}
}
```

## Monitoring Endpoints

### GET /health
Health check endpoint.

**Response:**
```json
{
  "status": "healthy",
  "timestamp": 1754749778,
  "version": "1.0.0",
  "blockchain_height": 1,
  "pending_transactions": 0,
  "mining_active": false,
  "consensus_active": true,
  "porc_active": true
}
```

### GET /metrics
Metrics endpoint for monitoring.

**Response:**
```json
{
  "status": "success",
  "metrics": {
    "blockchain_height": 1,
    "pending_transactions": 0,
    "total_blocks": 1,
    "current_difficulty": 4,
    "mining_reward": 100.0,
    "mining_active": false,
    "consensus_active": true,
    "timestamp": 1754749778
  }
}
```

## Token Endpoints

### POST /token
Create token.

**Request Body:**
```json
{
  "token_id": "token_id",
  "amount": 100.0,
  "creator": "creator_address"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Token created successfully",
  "token_id": "token_id"
}
```

## Error Responses

When an error occurs, the API returns an error response with the following format:

```json
{
  "error": "Error message description"
}
```

Common HTTP status codes:
- `200` - Success
- `400` - Bad Request
- `404` - Not Found
- `500` - Internal Server Error

## CORS Support

The API includes CORS headers to support cross-origin requests:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type, Authorization`

## Rate Limiting

Currently, there is no rate limiting implemented. In a production environment, you should implement appropriate rate limiting.

## Security Considerations

1. **Authentication**: The current implementation does not include authentication. In production, implement proper authentication mechanisms.
2. **Input Validation**: All endpoints include basic input validation, but additional validation may be needed for production use.
3. **HTTPS**: The current implementation uses HTTP. In production, use HTTPS for secure communication.
4. **API Keys**: Consider implementing API key authentication for production use.

## Testing

You can test the endpoints using curl:

```bash
# Test health endpoint
curl http://localhost:5500/health

# Test consensus stats
curl http://localhost:5500/consensus/stats

# Test joining consensus
curl -X POST http://localhost:5500/consensus/join \
  -H "Content-Type: application/json" \
  -d '{"address":"test","method":"pow"}'

# Test creating a transaction
curl -X POST http://localhost:5500/transaction \
  -H "Content-Type: application/json" \
  -d '{"sender":"alice","recipient":"bob","amount":10.0}'
```

## Implementation Status

All endpoints listed above are fully implemented and functional. The blockchain includes:

- ✅ Core blockchain functionality
- ✅ Transaction processing
- ✅ Mining operations
- ✅ Unified consensus system (PoW, PoS, PoRC)
- ✅ PoRC (Proof of Resource Contribution) system
- ✅ Wallet management
- ✅ Staking operations
- ✅ Network operations
- ✅ Analytics and monitoring
- ✅ Health checks and metrics
- ✅ Token creation
- ✅ Comprehensive API documentation
