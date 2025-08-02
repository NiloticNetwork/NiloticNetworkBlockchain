# Nilotic Blockchain Documentation

## Overview

The Nilotic Blockchain is a comprehensive blockchain implementation written in C++ that supports smart contracts, advanced synchronization, robust security features, Proof-of-Stake consensus, token management, and Odero SLW tokens for offline payments. This document provides detailed information about the design, architecture, and implementation of the Nilotic Blockchain system.

## Table of Contents

1. [Architecture](#architecture)
2. [Core Components](#core-components)
3. [Blockchain Data Structure](#blockchain-data-structure)
4. [Consensus Mechanism](#consensus-mechanism)
5. [Transaction System](#transaction-system)
6. [Wallet System](#wallet-system)
7. [Odero SLW Token System](#odero-slw-token-system)
8. [Persistence Layer](#persistence-layer)
9. [API Interface](#api-interface)
10. [Security Features](#security-features)
11. [Performance Optimizations](#performance-optimizations)
12. [Deployment Considerations](#deployment-considerations)

## Architecture

The Nilotic Blockchain follows a layered architecture designed for modularity, scalability, and maintainability:

```
+-------------------------------------------------------------+
|                       Application Layer                      |
|        (API, CLI Interface, Configuration Management)        |
+-------------------------------------------------------------+
|                       Business Layer                         |
| (Core Blockchain Logic, Consensus, Transaction Processing)   |
+-------------------------------------------------------------+
|                        Data Layer                            |
|   (Block Storage, Chain Management, Transaction Pool)        |
+-------------------------------------------------------------+
|                    Persistence Layer                         |
|            (SQLite Database, File Storage)                   |
+-------------------------------------------------------------+
```

### Communication Flow

1. User requests are received through the Application Layer
2. Requests are processed by the Business Layer, which applies appropriate business logic
3. The Data Layer manages in-memory blockchain data structures
4. The Persistence Layer ensures data durability and persistence

## Core Components

The Nilotic Blockchain consists of several core components:

1. **Block**: The fundamental unit of the blockchain, containing transaction data
2. **Blockchain**: Manages the chain of blocks and ensures integrity
3. **Transaction**: Represents transfers of value between addresses
4. **Wallet**: Manages cryptographic keys and addresses
5. **Consensus**: Implements the Proof-of-Stake algorithm
6. **OderoSLW**: Manages offline payment token creation and redemption
7. **API**: Provides HTTP interface for interaction with the blockchain
8. **Persistence**: Manages data storage and retrieval

## Blockchain Data Structure

### Block Structure

Each block in the Nilotic Blockchain contains the following elements:

- **Index**: The position of the block in the chain
- **Timestamp**: The time when the block was created
- **Transactions**: A list of transactions included in the block
- **Previous Hash**: The hash of the previous block
- **Hash**: The current block's hash value
- **Nonce**: A value used for the consensus mechanism

```cpp
struct Block {
    int index;
    time_t timestamp;
    std::vector<Transaction> transactions;
    std::string previous_hash;
    std::string hash;
    int nonce;
    
    // Additional PoS-related fields
    std::string validator_address;
    double stake_amount;
};
```

### Chain Validation

The blockchain maintains integrity through continuous validation:

1. Each block's hash is verified
2. Block indices are checked for sequential integrity
3. Previous hash references are validated
4. Transaction validity is confirmed
5. Consensus rules are enforced

## Consensus Mechanism

The Nilotic Blockchain uses a Proof-of-Stake (PoS) consensus mechanism to achieve distributed agreement on the state of the blockchain.

### Proof-of-Stake (PoS)

Unlike Proof-of-Work systems that require computational work, PoS selects validators based on the number of coins they have staked:

1. Users stake tokens to participate in validation
2. Validators are chosen with probability proportional to their stake
3. Selected validators verify transactions and create new blocks
4. Validators receive transaction fees and staking rewards

### Validator Selection

```
P(validator) = stake_amount / total_staked
```

Where:
- P(validator) is the probability of being selected as a validator
- stake_amount is the amount of tokens staked by a user
- total_staked is the total amount of tokens staked across all users

### Advantages of PoS

- Energy efficient compared to Proof-of-Work
- Reduces the risk of 51% attacks
- Encourages token holders to participate in consensus
- Faster transaction finality

## Transaction System

### Transaction Structure

Each transaction contains:

- **Sender**: Address of the sender
- **Recipient**: Address of the recipient
- **Amount**: Amount of tokens transferred
- **Timestamp**: Time of transaction creation
- **Signature**: Cryptographic signature
- **Transaction ID**: Unique identifier

```cpp
struct Transaction {
    std::string sender;
    std::string recipient;
    double amount;
    time_t timestamp;
    std::string signature;
    std::string transaction_id;
    
    // Additional fields for special transaction types
    int type;
    std::map<std::string, std::string> additional_data;
};
```

### Transaction Types

The system supports multiple transaction types:

1. **Standard Transfers**: Regular token transfers between addresses
2. **Mining Rewards**: System rewards for block validators
3. **Staking Transactions**: Tokens staked or unstaked for consensus
4. **Odero Token Creation**: Creates offline payment tokens
5. **Odero Token Redemption**: Redeems offline payment tokens

### Transaction Validation

Transactions are validated through:

1. Signature verification using public key cryptography
2. Balance verification to prevent double-spending
3. Format validation to ensure all required fields are present
4. Special validation rules for different transaction types

## Wallet System

The wallet system manages cryptographic keys, addresses, and transaction signing:

### Key Generation

- Uses elliptic curve cryptography (ECC) for key generation
- Generates key pairs (private key, public key)
- Derives addresses from public keys

### Transaction Signing

```
signature = sign(transaction_data, private_key)
```

### Address Format

Addresses are derived from public keys using:
1. Hash the public key with SHA-256
2. Apply RIPEMD-160 to the result
3. Add version byte and checksum
4. Encode in Base58

## Odero SLW Token System

The Odero SLW (Secure Local Wallet) token system enables offline payments and value transfers:

### Token Creation

1. User specifies an amount to lock in a token
2. System generates a unique token identifier
3. User's balance is reduced by the token amount
4. Token details are recorded in the blockchain

### Token Verification

1. Token ID is used to query the blockchain
2. System verifies the token's existence and status
3. Returns token details including amount and status

### Token Redemption

1. Token holder presents the token ID to a recipient
2. Recipient submits the token ID for redemption
3. System verifies the token is valid and unredeemed
4. Transfers the token amount to the recipient
5. Marks the token as redeemed

### Offline Usage Scenario

1. Alice creates an Odero token for 10 coins
2. Alice gives Bob the token ID (can be transmitted offline via QR code, text, etc.)
3. Bob verifies the token's validity online
4. Bob accepts the token as payment
5. Bob redeems the token online to receive the funds

## Persistence Layer

The Nilotic Blockchain uses SQLite for persistent storage of blockchain data:

### Database Schema

```sql
CREATE TABLE blocks (
    index INTEGER PRIMARY KEY,
    timestamp INTEGER,
    previous_hash TEXT,
    hash TEXT,
    nonce INTEGER,
    validator_address TEXT,
    stake_amount REAL
);

CREATE TABLE transactions (
    transaction_id TEXT PRIMARY KEY,
    block_index INTEGER,
    sender TEXT,
    recipient TEXT,
    amount REAL,
    timestamp INTEGER,
    signature TEXT,
    type INTEGER,
    FOREIGN KEY (block_index) REFERENCES blocks(index)
);

CREATE TABLE odero_tokens (
    token_id TEXT PRIMARY KEY,
    sender TEXT,
    amount REAL,
    timestamp INTEGER,
    status TEXT,
    redeemed_by TEXT,
    redemption_time INTEGER
);

CREATE TABLE stakes (
    stake_id TEXT PRIMARY KEY,
    address TEXT,
    amount REAL,
    timestamp INTEGER,
    status TEXT
);
```

### Data Persistence Operations

- **Load Chain**: Loads the blockchain from the database on startup
- **Save Block**: Saves a new block to the database
- **Query Transactions**: Retrieves transactions for various queries
- **Manage Tokens**: Stores and updates Odero SLW token status
- **Track Stakes**: Manages stake information for the PoS consensus

## API Interface

The API provides a RESTful interface for interacting with the blockchain:

### Implementation

- Uses a lightweight HTTP server implementation
- Parses JSON requests and generates JSON responses
- Implements threading for handling concurrent requests
- Provides endpoints for all blockchain operations

### Key Endpoints

- Core blockchain operations (mining, transactions, balance checks)
- Odero SLW token management (creation, verification, redemption)
- Proof-of-Stake operations (staking, unstaking, validation)

For a complete list of endpoints and usage examples, see [API_DOCUMENTATION.md](API_DOCUMENTATION.md).

## Security Features

The Nilotic Blockchain implements several security features:

### Cryptographic Security

- Uses SHA-256 for hashing blocks and data
- Employs elliptic curve cryptography for digital signatures
- Implements secure random number generation for keys

### Double-Spending Prevention

- Maintains a transaction pool with pending transactions
- Verifies sender balances before including transactions in blocks
- Prevents the same funds from being spent multiple times

### Immutability

- Blocks are linked through cryptographic hashes
- Any change to a block invalidates all subsequent blocks
- Full chain validation occurs regularly

### Attack Prevention

- Stake-based consensus reduces the risk of 51% attacks
- Rate limiting prevents denial-of-service attacks
- Input validation prevents injection attacks

## Libraries and Dependencies

### JSON Library
The Nilotic Blockchain uses the `nlohmann/json` library for JSON parsing and serialization. We've created a centralized wrapper (json.hpp) that abstracts the library implementation, making it easier to:
1. Update the JSON library without changing application code
2. Provide consistent JSON handling throughout the codebase
3. Add custom serialization/deserialization methods if needed in the future

The wrapper is located at `include/json.hpp` and should be included instead of directly including the nlohmann/json library.

## Performance Optimizations

The Nilotic Blockchain includes several optimizations:

### Multi-threading

- Mining and validation operations run in separate threads
- HTTP server uses thread pooling for concurrent request handling
- Database operations are optimized to minimize blocking

### Efficient Data Structures

- Uses hash maps for fast transaction lookups
- Implements in-memory caching for frequently accessed data
- Optimizes memory usage for long-running operations

### Database Optimizations

- Uses indexed queries for efficient data retrieval
- Implements transaction batching for improved throughput
- Optimizes schema for common query patterns

## Deployment Considerations

### System Requirements

- C++17-compatible compiler
- Sufficient memory for the in-memory blockchain copy
- Adequate storage for the blockchain database
- Network connectivity for peer synchronization

### Scaling Strategy

- Horizontal scaling through a network of nodes
- Vertical scaling through optimized resource usage
- Database partitioning for large-scale deployments

### Monitoring and Maintenance

- Logging system for tracking operations and errors
- Performance metrics for system optimization
- Backup and recovery procedures for data safety

### Security Best Practices

- Regular software updates to address vulnerabilities
- Proper network configuration to limit exposure
- Key management practices for secure operation