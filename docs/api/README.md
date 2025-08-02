# Nilotic Blockchain API Documentation

This document provides comprehensive documentation for the Nilotic Blockchain REST API.

## Base URL

```
http://localhost:5500
```

## Authentication

Currently, the API does not require authentication. All endpoints are publicly accessible.

## Response Format

All API responses are returned in JSON format with the following structure:

```json
{
  "status": "success|error",
  "data": { ... },
  "message": "Description of the response"
}
```

## Core Endpoints

### GET /

Get basic information about the blockchain node.

**Response:**
```json
{
  "chain_height": 0,
  "difficulty": 4,
  "mining_reward": 100.0,
  "pending_transactions": 0,
  "status": "Nilotic Blockchain API is running",
  "version": "0.1.0"
}
```

### GET /chain

Get the current blockchain data.

**Response:**
```json
{
  "chain_height": 1,
  "blocks": [
    {
      "index": 0,
      "timestamp": "2025-01-01T00:00:00Z",
      "transactions": [],
      "proof": 100,
      "previous_hash": "1"
    }
  ]
}
```

### GET /balance

Get the balance of a specific wallet address.

**Parameters:**
- `address` (string, required): The wallet address to check

**Example:**
```
GET /balance?address=test_wallet
```

**Response:**
```json
{
  "address": "test_wallet",
  "balance": 100.0
}
```

### POST /mine

Mine a new block and add it to the blockchain.

**Request Body:**
```json
{
  "miner_address": "test_wallet"
}
```

**Response:**
```json
{
  "block_hash": "0000ed770f399aea45d43a5624000ba3cb81d70e8e8a03b66f56c3209a07f5d2",
  "block_index": 1,
  "message": "Block mined successfully",
  "success": true
}
```

### POST /transaction

Create a new transaction.

**Request Body:**
```json
{
  "sender": "sender_address",
  "recipient": "recipient_address",
  "amount": 10.5
}
```

**Response:**
```json
{
  "message": "Transaction added to pending transactions",
  "transaction_id": "tx_123456789"
}
```

## Odero SLW Token Endpoints

### POST /odero/create

Create a new Odero SLW token for offline payments.

**Request Body:**
```json
{
  "creator": "wallet_address",
  "amount": 5.0,
  "pin": "optional_pin"
}
```

**Response:**
```json
{
  "message": "Odero SLW token created",
  "tokenId": "ODERO-1234567890abcdef",
  "qrCode": "data:image/png;base64,..."
}
```

### POST /odero/verify

Verify an Odero SLW token.

**Request Body:**
```json
{
  "tokenId": "ODERO-1234567890abcdef"
}
```

**Response:**
```json
{
  "valid": true,
  "token": {
    "tokenId": "ODERO-1234567890abcdef",
    "amount": 5.0,
    "creator": "wallet_address",
    "creationTime": "2025-01-01T00:00:00Z"
  }
}
```

### POST /odero/redeem

Redeem an Odero SLW token.

**Request Body:**
```json
{
  "tokenId": "ODERO-1234567890abcdef",
  "redeemer": "redeemer_address",
  "pin": "optional_pin"
}
```

**Response:**
```json
{
  "message": "Odero SLW token redemption pending",
  "tokenId": "ODERO-1234567890abcdef"
}
```

## Proof-of-Stake Endpoints

### POST /stake

Stake tokens for validation.

**Request Body:**
```json
{
  "address": "wallet_address",
  "amount": 100.0
}
```

**Response:**
```json
{
  "message": "Tokens staked successfully",
  "staked_amount": 100.0
}
```

### POST /unstake

Remove staked tokens.

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
  "message": "Tokens unstaked successfully",
  "unstaked_amount": 50.0
}
```

## Error Responses

When an error occurs, the API returns an error response with the following structure:

```json
{
  "error": true,
  "message": "Description of the error"
}
```

### Common Error Codes

- `400 Bad Request`: Invalid request parameters
- `404 Not Found`: Endpoint not found
- `500 Internal Server Error`: Server-side error

## Rate Limiting

Currently, there are no rate limits implemented. However, it's recommended to:

- Limit requests to reasonable frequencies
- Implement proper error handling
- Use appropriate timeouts

## Examples

### Using curl

```bash
# Get blockchain info
curl http://localhost:5500/

# Get wallet balance
curl http://localhost:5500/balance?address=test_wallet

# Mine a block
curl -X POST -H "Content-Type: application/json" \
  -d '{"miner_address":"test_wallet"}' \
  http://localhost:5500/mine

# Send a transaction
curl -X POST -H "Content-Type: application/json" \
  -d '{"sender":"alice","recipient":"bob","amount":10.5}' \
  http://localhost:5500/transaction
```

### Using Python

```python
import requests

# Get blockchain info
response = requests.get('http://localhost:5500/')
print(response.json())

# Mine a block
response = requests.post('http://localhost:5500/mine', 
                        json={'miner_address': 'test_wallet'})
print(response.json())
```

### Using JavaScript

```javascript
// Get blockchain info
fetch('http://localhost:5500/')
  .then(response => response.json())
  .then(data => console.log(data));

// Mine a block
fetch('http://localhost:5500/mine', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    miner_address: 'test_wallet'
  })
})
.then(response => response.json())
.then(data => console.log(data));
```

## SDK Libraries

The following SDK libraries are available for easier integration:

- [Python SDK](https://github.com/nilotic/python-sdk)
- [JavaScript SDK](https://github.com/nilotic/js-sdk)
- [Go SDK](https://github.com/nilotic/go-sdk)

## Support

For API support and questions:

- GitHub Issues: [Create an issue](https://github.com/nilotic/blockchain/issues)
- Documentation: [Full documentation](https://docs.nilotic.io)
- Community: [Discord server](https://discord.gg/nilotic) 