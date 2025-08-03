# Blockchain Sync System

## Overview

The Blockchain Sync System ensures that wallet balances and transactions are constantly updated in the database for consistency with the Nilotic blockchain. This system provides real-time synchronization, background processing, and comprehensive error handling.

## Architecture

### 1. BlockchainSyncService (`src/lib/blockchain-sync-service.ts`)

**Purpose**: Manages real-time synchronization for individual users.

**Key Features**:
- **Real-time Sync**: Updates every 30 seconds for active users
- **Wallet Discovery**: Automatically discovers new wallets from blockchain transactions
- **Balance Updates**: Syncs wallet balances, staked amounts, and rewards
- **Transaction Tracking**: Monitors and updates transaction status
- **Error Handling**: Comprehensive retry logic and error reporting

**Core Methods**:
```typescript
// Start real-time sync for a user
static async startUserSync(userId: string): Promise<void>

// Stop real-time sync for a user
static stopUserSync(userId: string): void

// Force immediate sync
static async forceUserSync(userId: string): Promise<void>

// Get sync status
static getUserSyncStatus(userId: string): SyncStatus | null
```

### 2. BackgroundSyncService (`src/lib/background-sync-service.ts`)

**Purpose**: Server-side background service that ensures all active users are synced.

**Key Features**:
- **Automatic Startup**: Starts automatically in production
- **Batch Processing**: Processes users in batches to prevent system overload
- **Retry Logic**: Implements exponential backoff for failed syncs
- **Configurable**: Adjustable intervals, batch sizes, and retry attempts

**Configuration**:
```typescript
{
  enabled: true,
  interval: 60000, // 1 minute
  maxConcurrentUsers: 10,
  retryAttempts: 3,
  retryDelay: 5000 // 5 seconds
}
```

### 3. API Endpoints

#### User Sync API (`/api/blockchain/sync`)

**POST** - Manage user sync:
```json
{
  "action": "start|stop|force",
  "userId": "user-id"
}
```

**GET** - Get sync status:
```
GET /api/blockchain/sync?userId=user-id
```

#### Admin Background Sync API (`/api/admin/background-sync`)

**POST** - Manage background service:
```json
{
  "action": "start|stop|force|config",
  "config": {
    "interval": 60000,
    "maxConcurrentUsers": 10
  }
}
```

**GET** - Get service status and stats:
```
GET /api/admin/background-sync?stats=true
```

## Sync Process

### 1. Wallet Synchronization

```typescript
// Process for each user wallet:
1. Get current blockchain balance
2. Get staking data (staked amount, rewards)
3. Compare with database values
4. Update if changes detected
5. Log changes for audit trail
```

### 2. Transaction Synchronization

```typescript
// Process for each transaction:
1. Fetch blockchain transactions
2. Filter for user's wallet addresses
3. Check if transaction exists in database
4. Create new transactions or update existing ones
5. Update transaction status (pending → confirmed)
```

### 3. Staking Data Synchronization

```typescript
// Process for user staking data:
1. Aggregate staking data from all user wallets
2. Calculate total staked amount
3. Calculate total rewards earned
4. Update user's staking profile
5. Calculate next reward estimates
```

## Integration Points

### 1. Authentication Context

The sync system integrates with the authentication context:

```typescript
// Auto-start sync on login
const login = async (credentials: LoginCredentials) => {
  // ... login logic ...
  
  // Start blockchain sync for the user
  await fetch('/api/blockchain/sync', {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body: JSON.stringify({ action: 'start', userId: user.id })
  });
};

// Auto-stop sync on logout
const logout = () => {
  // Stop blockchain sync
  fetch('/api/blockchain/sync', {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body: JSON.stringify({ action: 'stop', userId: user.id })
  });
  
  // ... logout logic ...
};
```

### 2. User Dashboard

The dashboard displays real-time sync status:

```typescript
// Sync status indicators
- Green dot: Synced
- Yellow dot: Syncing
- Gray dot: Not synced

// Sync controls
- Start Sync: Begin real-time sync
- Force Sync: Immediate sync
- Last sync time display
```

## Error Handling

### 1. Retry Logic

```typescript
// Exponential backoff for failed syncs
for (let attempt = 1; attempt <= maxRetries; attempt++) {
  try {
    await syncUserData(userId);
    return; // Success
  } catch (error) {
    if (attempt < maxRetries) {
      await delay(retryDelay * Math.pow(2, attempt - 1));
    }
  }
}
```

### 2. Error Reporting

```typescript
// Comprehensive error logging
console.error(`Sync failed for user ${userId}:`, error);
console.log(`Sync completed for user ${userId}: ${walletCount} wallets, ${transactionCount} transactions`);
```

### 3. Graceful Degradation

- Sync failures don't affect user login
- Background sync continues for other users
- Database remains consistent even with sync errors

## Performance Optimizations

### 1. Batch Processing

```typescript
// Process users in small batches
const batchSize = 3;
for (let i = 0; i < users.length; i += batchSize) {
  const batch = users.slice(i, i + batchSize);
  await Promise.allSettled(batch.map(syncUser));
  await delay(1000); // Prevent overwhelming blockchain
}
```

### 2. Caching

```typescript
// Cache sync status to reduce API calls
const syncStatus = new Map<string, SyncStatus>();
```

### 3. Selective Updates

```typescript
// Only update if values have changed
const hasChanges = 
  oldBalance !== newBalance ||
  oldStaked !== newStaked ||
  oldRewards !== newRewards;

if (hasChanges) {
  await updateWalletBalance(walletId, newBalance, newStaked, newRewards);
}
```

## Monitoring and Logging

### 1. Sync Statistics

```typescript
{
  totalUsers: number,
  activeUsers: number,
  syncedUsers: number,
  failedUsers: number,
  lastSyncTime: Date
}
```

### 2. Real-time Monitoring

- Sync status per user
- Background service status
- Error rates and retry attempts
- Performance metrics

### 3. Audit Trail

```typescript
// Log all sync activities
console.log(`Updated wallet ${address}: balance ${oldBalance}→${newBalance}`);
console.log(`Created new transaction ${hash} for user ${userId}`);
console.log(`Updated staking data for user ${userId}: staked=${totalStaked}, rewards=${totalRewards}`);
```

## Security Considerations

### 1. Authentication

- All sync operations require valid JWT tokens
- Admin endpoints require admin role
- User-specific sync operations validate user ownership

### 2. Rate Limiting

- Background sync processes users in batches
- Delays between batches prevent blockchain overload
- Configurable intervals and retry limits

### 3. Data Integrity

- Transaction hashes prevent duplicate processing
- Atomic database updates ensure consistency
- Error handling prevents partial updates

## Configuration

### Environment Variables

```bash
# Sync intervals (in milliseconds)
SYNC_INTERVAL=30000
BACKGROUND_SYNC_INTERVAL=60000

# Retry configuration
MAX_RETRY_ATTEMPTS=3
RETRY_DELAY=5000

# Batch processing
MAX_CONCURRENT_USERS=10
BATCH_SIZE=3
```

### Service Configuration

```typescript
// Background sync configuration
{
  enabled: true,
  interval: 60000, // 1 minute
  maxConcurrentUsers: 10,
  retryAttempts: 3,
  retryDelay: 5000
}
```

## Usage Examples

### 1. Start Sync for User

```typescript
// Frontend
const startSync = async () => {
  const response = await fetch('/api/blockchain/sync', {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body: JSON.stringify({ action: 'start', userId: user.id })
  });
};
```

### 2. Force Immediate Sync

```typescript
// Frontend
const forceSync = async () => {
  const response = await fetch('/api/blockchain/sync', {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body: JSON.stringify({ action: 'force', userId: user.id })
  });
};
```

### 3. Monitor Sync Status

```typescript
// Frontend
const getSyncStatus = async () => {
  const response = await fetch(`/api/blockchain/sync?userId=${userId}`, {
    headers: { 'Authorization': `Bearer ${token}` }
  });
  const { status } = await response.json();
  return status;
};
```

### 4. Admin Background Sync Management

```typescript
// Admin panel
const manageBackgroundSync = async (action: string) => {
  const response = await fetch('/api/admin/background-sync', {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${adminToken}` },
    body: JSON.stringify({ action })
  });
};
```

## Troubleshooting

### Common Issues

1. **Sync Not Starting**
   - Check authentication token
   - Verify user exists in database
   - Check blockchain connection

2. **Sync Failing**
   - Review error logs
   - Check blockchain API availability
   - Verify database connectivity

3. **Performance Issues**
   - Reduce sync frequency
   - Increase batch processing delays
   - Monitor blockchain API limits

### Debug Commands

```typescript
// Check sync status
console.log(BlockchainSyncService.getAllSyncStatuses());

// Force background sync
await BackgroundSyncService.forceSync();

// Get sync statistics
const stats = await BackgroundSyncService.getSyncStats();
console.log(stats);
```

## Future Enhancements

1. **WebSocket Integration**: Real-time sync status updates
2. **Advanced Analytics**: Detailed sync performance metrics
3. **Multi-Blockchain Support**: Extend to other blockchain networks
4. **Machine Learning**: Predictive sync scheduling based on user patterns
5. **Distributed Sync**: Multiple sync nodes for high availability

This comprehensive sync system ensures that your dApp maintains perfect consistency with the blockchain while providing excellent user experience and robust error handling. 