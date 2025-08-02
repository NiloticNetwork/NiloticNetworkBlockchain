# Nilotic Blockchain API Documentation

This document describes the API endpoints available for interacting with the Nilotic Blockchain. These endpoints can be used by wallet applications, mining software, or any other third-party tools that need to communicate with the blockchain.

## Base URL

All API endpoints are relative to the base URL where the blockchain node is running. By default, this is:

```
http://localhost:5500
```

## General Information

- All requests and responses are in JSON format, unless otherwise specified
- HTTP status codes are used to indicate success or failure
- All timestamp values are Unix timestamps (seconds since epoch)
- Blockchain addresses are string identifiers

## Authentication

The current implementation does not include authentication. In a production environment, you would want to implement authentication mechanisms such as API keys or JWT tokens.

## Endpoints

### GET /

Returns general information about the blockchain.

**Request Parameters**: None

**Response Example**:
```json
{
  "status": "running",
  "node_address": "node1",
  "version": "0.1.0",
  "peers": 0,
  "current_height": 12
}
```

### GET /chain

Returns the entire blockchain.

**Request Parameters**: None

**Response Example**:
```json
[
  {
    "index": 0,
    "timestamp": 1701123456,
    "previous_hash": "0",
    "hash": "000a8c1750e94e743302873d114dcea194eb977b92f3ba0c6e7be509aa8ffe78",
    "data": "Genesis Block",
    "nonce": 0,
    "difficulty": 1
  },
  {
    "index": 1,
    "timestamp": 1701123487,
    "previous_hash": "000a8c1750e94e743302873d114dcea194eb977b92f3ba0c6e7be509aa8ffe78",
    "hash": "000c9e7821f5ee473b93e1db936d59ef45bc53a843bac59ced0c94acb625405a",
    "data": "{\"type\":\"mining_reward\",\"miner\":\"alice\",\"reward\":100,\"timestamp\":1701123487}",
    "nonce": 8547,
    "difficulty": 3
  }
]
```

### GET /balance

Returns the balance for a given address.

**Request Parameters**:
- `address` (query parameter): The blockchain address to check

**Response Example**:
```json
{
  "address": "alice",
  "balance": 250.0
}
```

### POST /transaction

Creates a new transaction on the blockchain.

**Request Body**:
```json
{
  "sender": "alice",
  "recipient": "bob",
  "amount": 25.5
}
```

**Response Example**:
```json
{
  "message": "Transaction added to pending transactions",
  "transaction": {
    "sender": "alice",
    "recipient": "bob",
    "amount": 25.5,
    "timestamp": 1701123789,
    "signature": "",
    "transaction_id": "78a9df3c5b3c48f5b8c0e1a6d8f9e2c1",
    "type": 0,
    "network": 0
  }
}
```

### POST /mine

Mines a new block and awards the mining reward to the specified address.

**Request Body**:
```json
{
  "address": "alice"
}
```

**Response Example**:
```json
{
  "message": "Block mined successfully",
  "block": {
    "index": 12,
    "timestamp": 1701124001,
    "previous_hash": "000f7e6d5c4b3a2918d7c6b5a4938271a1d9e8f7a6b5c4d3e2f1a0b9c8d7e6f",
    "hash": "0002f1e0d9c8b7a6958473625140a3b2918d7c6b5a4a3b2c1d0e9f8g7h6i5j4",
    "data": "{\"type\":\"mining_reward\",\"miner\":\"alice\",\"reward\":100,\"timestamp\":1701124001}",
    "nonce": 12345,
    "difficulty": 3
  },
  "reward": 100
}
```

### POST /create_odero

Creates a new Odero SLW token for offline payments.

**Request Body**:
```json
{
  "sender": "alice",
  "amount": 50.0,
  "expiry": 1701220000
}
```

**Response Example**:
```json
{
  "message": "Odero SLW token created successfully",
  "token": {
    "id": "odero_123456789abcdef",
    "sender": "alice",
    "amount": 50.0,
    "created_at": 1701124500,
    "expires_at": 1701220000,
    "status": "active",
    "signature": "af7c92b1e4a3d2f6..."
  }
}
```

### GET /verify_odero

Verifies an Odero SLW token without redeeming it.

**Request Parameters**:
- `token_id` (query parameter): The ID of the token to verify

**Response Example**:
```json
{
  "valid": true,
  "token": {
    "id": "odero_123456789abcdef",
    "sender": "alice",
    "amount": 50.0,
    "created_at": 1701124500,
    "expires_at": 1701220000,
    "status": "active",
    "signature": "af7c92b1e4a3d2f6..."
  }
}
```

### POST /redeem_odero

Redeems an Odero SLW token and credits the recipient.

**Request Body**:
```json
{
  "token_id": "odero_123456789abcdef",
  "recipient": "bob"
}
```

**Response Example**:
```json
{
  "message": "Token redeemed successfully",
  "transaction": {
    "sender": "alice",
    "recipient": "bob",
    "amount": 50.0,
    "timestamp": 1701125000,
    "signature": "",
    "transaction_id": "89b0df4c6b3c48f6b8c0e1a7d8f9e2d2",
    "type": 2,
    "network": 0
  }
}
```

## Error Handling

Errors are returned with appropriate HTTP status codes and include a message describing the error.

**Example Error Response**:
```json
{
  "error": "Invalid address format",
  "status": 400
}
```

Common error codes:
- 400: Bad Request - Invalid parameters or request body
- 404: Not Found - Resource not found
- 409: Conflict - Duplicate transaction or other conflict
- 500: Internal Server Error - Server-side error

## Rate Limiting

Currently, no rate limiting is implemented. In a production environment, you would want to implement rate limiting to prevent abuse.

## Versioning

API versioning is not currently implemented. Future versions of the API will include version information in the URL path (e.g., `/v1/chain`).

## Extensions

The API can be extended with additional endpoints as needed:

1. **Wallet Creation**: Endpoints for creating and managing wallets
2. **Transaction History**: Endpoints for retrieving transaction history for an address
3. **Peer Discovery**: Endpoints for finding and connecting to peers
4. **Smart Contracts**: Endpoints for deploying and interacting with smart contracts

## Examples

### JavaScript Example (using fetch)

```javascript
// Get blockchain info
fetch('http://localhost:5500/')
  .then(response => response.json())
  .then(data => console.log(data))
  .catch(error => console.error('Error:', error));

// Send a transaction
fetch('http://localhost:5500/transaction', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify({
    sender: 'alice',
    recipient: 'bob',
    amount: 25.5
  }),
})
  .then(response => response.json())
  .then(data => console.log(data))
  .catch(error => console.error('Error:', error));
```

### Python Example (using requests)

```python
import requests
import json

# Get blockchain info
response = requests.get('http://localhost:5500/')
print(response.json())

# Send a transaction
transaction = {
    'sender': 'alice',
    'recipient': 'bob',
    'amount': 25.5
}
response = requests.post(
    'http://localhost:5500/transaction',
    json=transaction,
    headers={'Content-Type': 'application/json'}
)
print(response.json())
```

## Security Recommendations

When implementing a wallet or other application that interfaces with the Nilotic Blockchain API, consider the following security recommendations:

1. **Secure Communication**: Use HTTPS for all API calls in production environments
2. **Input Validation**: Validate all inputs before sending them to the API
3. **Error Handling**: Implement robust error handling to gracefully handle API errors
4. **Rate Limiting**: Implement client-side rate limiting to prevent accidental abuse
5. **Logging**: Log all API interactions for debugging and audit purposes
6. **Timeout Handling**: Set appropriate timeouts for API calls and handle timeouts gracefully
7. **Retry Logic**: Implement exponential backoff for retrying failed API calls