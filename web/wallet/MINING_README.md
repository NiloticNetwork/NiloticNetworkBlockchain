# 🚀 Nilotic Blockchain Miner Guide

This guide shows you how to run a miner in the integrated terminal to constantly validate transactions.

## 📋 Prerequisites

1. **Blockchain Server Running**: Make sure your blockchain server is running on `http://localhost:5500`
2. **Python Environment**: Ensure you have Python 3.6+ installed
3. **Required Packages**: Install the required packages

```bash
pip install requests
```

## 🔧 Setup Steps

### Step 1: Get a Wallet Address for Mining

First, you need a wallet address to receive mining rewards. Run:

```bash
python get_wallet_address.py
```

This will:
- Check if the blockchain server is running
- Create a new wallet for mining
- Display the wallet address
- Show you the mining command to use

### Step 2: Start the Miner

Use the wallet address from Step 1 to start mining:

```bash
python miner.py --address YOUR_WALLET_ADDRESS
```

## 🎯 Mining Commands

### Basic Mining
```bash
python miner.py --address NILda9879380c1efaff4aede80339f2e35fac
```

### Custom Mining Interval (5 seconds)
```bash
python miner.py --address NILda9879380c1efaff4aede80339f2e35fac --interval 5
```

### Check Mining Status
```bash
python miner.py --address NILda9879380c1efaff4aede80339f2e35fac --status
```

### Different Blockchain URL
```bash
python miner.py --url http://localhost:8080 --address YOUR_WALLET_ADDRESS
```

## 📊 What the Miner Does

The miner will:

1. **Connect to Blockchain**: Verify connection to the blockchain server
2. **Monitor Transactions**: Check for pending transactions
3. **Mine Blocks**: Create new blocks with pending transactions
4. **Validate Transactions**: Process and validate all transactions
5. **Show Statistics**: Display mining progress and statistics

## 📈 Expected Output

When running the miner, you'll see output like:

```
🚀 Starting miner with address: NILda9879380c1efaff4aede80339f2e35fac
🔗 Blockchain URL: http://localhost:5500
⏱️  Mining interval: 10 seconds
============================================================
📊 14:30:25 - Chain Height: 15, Pending TX: 3
⛏️ 14:30:25 - Mining new block...
✅ 14:30:26 - Block mined successfully! Hash: a1b2c3d4e5f6...
📈 Mining Stats: 5 blocks, 12.5 blocks/hour
------------------------------------------------------------
```

## 🛑 Stopping the Miner

To stop the miner, press `Ctrl+C` in the terminal. You'll see final statistics:

```
📊 Final Mining Statistics:
   Blocks mined: 15
   Total time: 7200.0 seconds
   Average rate: 7.5 blocks/hour
```

## 🔍 Troubleshooting

### Connection Issues
If you get connection errors:
1. Make sure the blockchain server is running
2. Check the URL in the command
3. Verify the server is accessible

### No Pending Transactions
If the miner shows "No pending transactions":
- This is normal when there are no transactions to validate
- The miner will still create empty blocks periodically
- Send some transactions from the wallet to see mining activity

### Mining Fails
If mining fails:
1. Check the blockchain server logs
2. Verify the miner address is valid
3. Check if the mining endpoint is working

## 🎮 Using the Wallet Script

You can also use the existing wallet script for mining:

```bash
# Mine a single block
python nilotic_wallet.py --address YOUR_ADDRESS mine

# Check balance
python nilotic_wallet.py --address YOUR_ADDRESS balance

# Send transaction
python nilotic_wallet.py --address YOUR_ADDRESS send RECIPIENT_ADDRESS 10.0
```

## 📝 Tips for Continuous Mining

1. **Use a Dedicated Terminal**: Keep the miner running in a separate terminal
2. **Monitor Logs**: Watch for any errors or connection issues
3. **Check Balance**: Periodically check your mining rewards
4. **Adjust Interval**: Use shorter intervals for more frequent mining
5. **Multiple Miners**: You can run multiple miners with different addresses

## 🔄 Integration with Wallet

The miner works with your web wallet:
1. Create a wallet in the web interface
2. Use that wallet address for mining
3. Mining rewards will appear in your wallet balance
4. Transactions sent from the wallet will be validated by the miner

Happy Mining! ⛏️💎 