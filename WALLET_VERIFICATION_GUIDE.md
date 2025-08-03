# Wallet Verification Guide - Nilotic Blockchain

## 🔍 How to Verify Real Wallet Creation

This guide shows you how to verify that real wallets are being created in the Nilotic blockchain and how to test their functionality.

## 🚀 Quick Verification Steps

### 1. **Check Blockchain is Running**
```bash
curl http://localhost:5500/
```
**Expected Response:**
```json
{
    "chain_height": 1,
    "difficulty": 4,
    "mining_reward": 100.0,
    "pending_transactions": 0,
    "status": "Nilotic Blockchain API is running"
}
```

### 2. **Create a Real Wallet**
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"name":"test_wallet","password":"test123"}' \
  http://localhost:5500/wallet/create
```
**Expected Response:**
```json
{
    "status": "success",
    "message": "Wallet created successfully",
    "address": "NIL0bb56c507b17d5fd746cc741b79eeb0746",
    "name": "test_wallet"
}
```

### 3. **Verify Wallet Balance**
```bash
curl "http://localhost:5500/balance/NIL0bb56c507b17d5fd746cc741b79eeb0746"
```
**Expected Response:**
```json
{
    "address": "NIL0bb56c507b17d5fd746cc741b79eeb0746",
    "balance": 0.0,
    "stake": 0.0
}
```

### 4. **Test Wallet Transaction**
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"sender":"NIL0bb56c507b17d5fd746cc741b79eeb0746","recipient":"NIL0test123456789","amount":10.0}' \
  http://localhost:5500/transaction
```

## 📊 Verification Methods

### Method 1: **Command Line Testing**

#### Step 1: Create Multiple Wallets
```bash
# Create wallet 1
curl -X POST -H "Content-Type: application/json" \
  -d '{"name":"wallet1","password":"pass1"}' \
  http://localhost:5500/wallet/create

# Create wallet 2
curl -X POST -H "Content-Type: application/json" \
  -d '{"name":"wallet2","password":"pass2"}' \
  http://localhost:5500/wallet/create

# Create wallet 3
curl -X POST -H "Content-Type: application/json" \
  -d '{"name":"wallet3","password":"pass3"}' \
  http://localhost:5500/wallet/create
```

#### Step 2: Verify All Wallets
```bash
# Check each wallet balance
curl "http://localhost:5500/balance/WALLET_ADDRESS_1"
curl "http://localhost:5500/balance/WALLET_ADDRESS_2"
curl "http://localhost:5500/balance/WALLET_ADDRESS_3"
```

#### Step 3: Test Transactions
```bash
# Send transaction between wallets
curl -X POST -H "Content-Type: application/json" \
  -d '{"sender":"WALLET_ADDRESS_1","recipient":"WALLET_ADDRESS_2","amount":5.0}' \
  http://localhost:5500/transaction
```

### Method 2: **Using the Verification Script**

```bash
cd web/dapps/sulwestake
node verify-wallets.js
```

This script will:
- ✅ Create 3 test wallets
- ✅ Verify wallet balances
- ✅ Test transactions between wallets
- ✅ Check blockchain state
- ✅ Provide detailed verification report

### Method 3: **Monitor Blockchain Logs**

Watch the blockchain logs for wallet activity:
```bash
# The logs show real wallet activity like:
[INFO] Request: GET /balance?address=0x05a057df22e78ae6b096191483c6153a4938f81e
[INFO] Request: GET /balance?address=0xfb1d8c9958566e5476e28221e59b012937d381b7
[INFO] Request: GET /balance?address=0xf93103a637b3154d8a77a5c72f80f2f63461c806
```

## 🔍 What to Look For

### ✅ **Successful Wallet Creation**
- **Response includes**: `status: "success"` or `message: "Wallet created successfully"`
- **Address format**: Starts with `NIL0` followed by alphanumeric characters
- **Unique addresses**: Each wallet gets a unique address

### ✅ **Successful Balance Verification**
- **Response includes**: `address`, `balance`, `stake` fields
- **Address matches**: The returned address matches the requested address
- **Valid balance**: Balance is a number (can be 0 for new wallets)

### ✅ **Successful Transactions**
- **Response includes**: `status: "success"` or transaction confirmation
- **Transaction ID**: Unique transaction hash is generated
- **Pending transactions**: Transaction appears in pending pool

## 🧪 Advanced Verification

### 1. **Test Wallet Import**
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"name":"import_test","private_key":"PRIVATE_KEY","password":"pass123"}' \
  http://localhost:5500/wallet/import
```

### 2. **Test Transaction Signing**
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '{"private_key":"PRIVATE_KEY","password":"pass123","transaction_data":"DATA"}' \
  http://localhost:5500/wallet/sign
```

### 3. **Check Blockchain State**
```bash
# Get full blockchain data
curl http://localhost:5500/chain

# Get latest block
curl http://localhost:5500/block/latest

# Get mining status
curl http://localhost:5500/mining/status
```

## 📈 Real-World Testing

### 1. **Use Wallets in Sulwestake App**
1. Create wallets using the blockchain API
2. Use the wallet addresses in the Sulwestake app
3. Test staking operations with real wallets
4. Verify transactions appear in blockchain

### 2. **Monitor Real Activity**
```bash
# Watch blockchain logs in real-time
tail -f blockchain.log

# Check for wallet-related activity:
# - Wallet creation requests
# - Balance queries
# - Transaction submissions
# - Mining rewards
```

### 3. **Verify Data Persistence**
```bash
# Check if wallet data persists after restart
# 1. Create wallets
# 2. Restart blockchain
# 3. Verify wallets still exist
# 4. Check balances are preserved
```

## 🚨 Troubleshooting

### **Wallet Creation Fails**
- Check blockchain is running: `curl http://localhost:5500/`
- Verify JSON format: `{"name":"wallet","password":"pass"}`
- Check blockchain logs for errors

### **Balance Check Fails**
- Verify address format: Should start with `NIL0`
- Check address exists: Try creating wallet first
- Verify endpoint: Use `/balance/ADDRESS` format

### **Transaction Fails**
- Check sender has sufficient balance
- Verify address formats are correct
- Check blockchain logs for validation errors

## 📊 Expected Results

### **Successful Verification Should Show:**

1. **Wallet Creation**
   ```
   ✅ Wallet created successfully!
   Address: NIL0bb56c507b17d5fd746cc741b79eeb0746
   Name: test_wallet
   ```

2. **Balance Verification**
   ```
   ✅ Balance verified!
   Address: NIL0bb56c507b17d5fd746cc741b79eeb0746
   Balance: 0 NIL
   Staked: 0 NIL
   ```

3. **Transaction Processing**
   ```
   ✅ Transaction submitted successfully!
   Transaction ID: tx_123456789
   Message: Transaction added to pending pool
   ```

4. **Blockchain State**
   ```
   ✅ Blockchain Status:
   Chain Height: 1
   Difficulty: 4
   Mining Reward: 100.0
   Pending Transactions: 1
   ```

## 🎯 Conclusion

When you see these results, you have **verified that real wallets are being created** in the Nilotic blockchain:

- ✅ **Real wallet addresses** are generated
- ✅ **Wallet balances** can be queried
- ✅ **Transactions** can be submitted
- ✅ **Blockchain state** is maintained
- ✅ **Data persistence** works correctly

This confirms that the Sulwestake application is working with **real blockchain wallets** and not just mock data! 