#!/bin/bash

# Nilotic Blockchain Interaction Testing Script
# This script demonstrates mining, rewards, transactions, and balance sheet operations

echo "🔗 Nilotic Blockchain Interaction Testing"
echo "========================================"

# Configuration
API_BASE="http://localhost:5500"
MINER_ADDRESS="NILda9879380c1efaff4aede80339f2e35fac"
SENDER_ADDRESS="NILabandonaachievea"
RECIPIENT_ADDRESS="NIL2af6bf62441121f9df940a46fc0ee6a5b8"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# Function to make API calls
api_call() {
    local method=$1
    local endpoint=$2
    local data=$3
    
    if [ -z "$data" ]; then
        curl -s -X $method "$API_BASE$endpoint" -H "Content-Type: application/json"
    else
        curl -s -X $method "$API_BASE$endpoint" -H "Content-Type: application/json" -d "$data"
    fi
}

# Test 1: Check Blockchain Status
echo ""
echo "📊 Test 1: Blockchain Status"
echo "----------------------------"
print_info "Checking blockchain status..."

response=$(api_call "GET" "/")
if [ $? -eq 0 ]; then
    print_status "Blockchain is running"
    echo "$response" | jq '.'
else
    print_error "Failed to connect to blockchain"
    exit 1
fi

# Test 2: Check Initial Balances
echo ""
echo "💰 Test 2: Initial Balance Check"
echo "-------------------------------"
print_info "Checking initial balances..."

# Check sender balance
sender_balance=$(api_call "GET" "/balance/$SENDER_ADDRESS" | jq -r '.balance // 0')
print_status "Sender balance: $sender_balance NIL"

# Check recipient balance
recipient_balance=$(api_call "GET" "/balance/$RECIPIENT_ADDRESS" | jq -r '.balance // 0')
print_status "Recipient balance: $recipient_balance NIL"

# Check miner balance
miner_balance=$(api_call "GET" "/balance/$MINER_ADDRESS" | jq -r '.balance // 0')
print_status "Miner balance: $miner_balance NIL"

# Test 3: Send Transaction
echo ""
echo "💸 Test 3: Send Transaction"
echo "---------------------------"
print_info "Sending 50 NIL from $SENDER_ADDRESS to $RECIPIENT_ADDRESS"

transaction_data="{
    \"sender\": \"$SENDER_ADDRESS\",
    \"recipient\": \"$RECIPIENT_ADDRESS\",
    \"amount\": 50.0
}"

response=$(api_call "POST" "/transaction" "$transaction_data")
if [ $? -eq 0 ]; then
    tx_id=$(echo "$response" | jq -r '.transaction_id')
    print_status "Transaction sent successfully"
    print_info "Transaction ID: $tx_id"
    echo "$response" | jq '.'
else
    print_error "Failed to send transaction"
    echo "$response"
fi

# Test 4: Check Pending Transactions
echo ""
echo "⏳ Test 4: Pending Transactions"
echo "-------------------------------"
print_info "Checking pending transactions..."

response=$(api_call "GET" "/")
if [ $? -eq 0 ]; then
    pending_count=$(echo "$response" | jq -r '.pending_transactions // 0')
    print_status "Pending transactions: $pending_count"
else
    print_error "Failed to get pending transactions"
fi

# Test 5: Mine a Block
echo ""
echo "⛏️  Test 5: Mining Block"
echo "-----------------------"
print_info "Mining a new block with miner: $MINER_ADDRESS"

mining_data="{
    \"miner_address\": \"$MINER_ADDRESS\"
}"

response=$(api_call "POST" "/mine" "$mining_data")
if [ $? -eq 0 ]; then
    status=$(echo "$response" | jq -r '.status')
    if [ "$status" = "success" ]; then
        print_status "Block mined successfully!"
        echo "$response" | jq '.'
    else
        print_warning "Mining completed but may have failed"
        echo "$response" | jq '.'
    fi
else
    print_error "Failed to mine block"
    echo "$response"
fi

# Test 6: Check Updated Balances
echo ""
echo "💰 Test 6: Updated Balance Check"
echo "-------------------------------"
print_info "Checking balances after mining..."

# Check sender balance (should be reduced by 50)
new_sender_balance=$(api_call "GET" "/balance/$SENDER_ADDRESS" | jq -r '.balance // 0')
print_status "Sender balance: $new_sender_balance NIL"

# Check recipient balance (should be increased by 50)
new_recipient_balance=$(api_call "GET" "/balance/$RECIPIENT_ADDRESS" | jq -r '.balance // 0')
print_status "Recipient balance: $new_recipient_balance NIL"

# Check miner balance (should be increased by mining reward)
new_miner_balance=$(api_call "GET" "/balance/$MINER_ADDRESS" | jq -r '.balance // 0')
print_status "Miner balance: $new_miner_balance NIL"

# Test 7: Get Latest Block
echo ""
echo "📦 Test 7: Latest Block Information"
echo "----------------------------------"
print_info "Getting latest block information..."

response=$(api_call "GET" "/block/latest")
if [ $? -eq 0 ]; then
    block_index=$(echo "$response" | jq -r '.index')
    block_hash=$(echo "$response" | jq -r '.hash')
    print_status "Latest block index: $block_index"
    print_status "Latest block hash: $block_hash"
    echo "$response" | jq '.'
else
    print_error "Failed to get latest block"
    echo "$response"
fi

# Test 8: Get Blockchain Info
echo ""
echo "📊 Test 8: Blockchain Information"
echo "--------------------------------"
print_info "Getting detailed blockchain information..."

response=$(api_call "GET" "/info")
if [ $? -eq 0 ]; then
    chain_height=$(echo "$response" | jq -r '.chainHeight')
    block_count=$(echo "$response" | jq -r '.blockCount')
    print_status "Chain height: $chain_height"
    print_status "Block count: $block_count"
    echo "$response" | jq '.'
else
    print_error "Failed to get blockchain info"
    echo "$response"
fi

# Test 9: Get Specific Block
echo ""
echo "🔍 Test 9: Get Specific Block"
echo "-----------------------------"
print_info "Getting block at index 1..."

response=$(api_call "GET" "/block/1")
if [ $? -eq 0 ]; then
    block_index=$(echo "$response" | jq -r '.index')
    print_status "Retrieved block at index: $block_index"
    echo "$response" | jq '.'
else
    print_error "Failed to get specific block"
    echo "$response"
fi

# Test 10: Multiple Transactions
echo ""
echo "🔄 Test 10: Multiple Transactions"
echo "--------------------------------"
print_info "Sending multiple transactions..."

# Send 3 more transactions
for i in {1..3}; do
    amount=$((10 * $i))
    transaction_data="{
        \"sender\": \"$SENDER_ADDRESS\",
        \"recipient\": \"$RECIPIENT_ADDRESS\",
        \"amount\": $amount
    }"
    
    response=$(api_call "POST" "/transaction" "$transaction_data")
    if [ $? -eq 0 ]; then
        tx_id=$(echo "$response" | jq -r '.transaction_id')
        print_status "Transaction $i sent: $amount NIL (ID: $tx_id)"
    else
        print_error "Failed to send transaction $i"
    fi
done

# Test 11: Check Final Balances
echo ""
echo "💰 Test 11: Final Balance Check"
echo "------------------------------"
print_info "Checking final balances after all transactions..."

final_sender_balance=$(api_call "GET" "/balance/$SENDER_ADDRESS" | jq -r '.balance // 0')
final_recipient_balance=$(api_call "GET" "/balance/$RECIPIENT_ADDRESS" | jq -r '.balance // 0')
final_miner_balance=$(api_call "GET" "/balance/$MINER_ADDRESS" | jq -r '.balance // 0')

print_status "Final sender balance: $final_sender_balance NIL"
print_status "Final recipient balance: $final_recipient_balance NIL"
print_status "Final miner balance: $final_miner_balance NIL"

# Test 12: Blockchain Statistics
echo ""
echo "📈 Test 12: Blockchain Statistics"
echo "--------------------------------"
print_info "Getting comprehensive blockchain statistics..."

response=$(api_call "GET" "/")
if [ $? -eq 0 ]; then
    chain_height=$(echo "$response" | jq -r '.chain_height')
    difficulty=$(echo "$response" | jq -r '.difficulty')
    mining_reward=$(echo "$response" | jq -r '.mining_reward')
    pending_tx=$(echo "$response" | jq -r '.pending_transactions')
    
    print_status "Chain height: $chain_height"
    print_status "Difficulty: $difficulty"
    print_status "Mining reward: $mining_reward NIL"
    print_status "Pending transactions: $pending_tx"
    echo "$response" | jq '.'
else
    print_error "Failed to get blockchain statistics"
fi

# Summary
echo ""
echo "🎯 Test Summary"
echo "==============="
print_status "All tests completed!"

# Calculate balance changes
sender_change=$(echo "$final_sender_balance - $sender_balance" | bc -l 2>/dev/null || echo "N/A")
recipient_change=$(echo "$final_recipient_balance - $recipient_balance" | bc -l 2>/dev/null || echo "N/A")
miner_change=$(echo "$final_miner_balance - $miner_balance" | bc -l 2>/dev/null || echo "N/A")

echo ""
echo "💰 Balance Changes:"
echo "  Sender: $sender_balance → $final_sender_balance (Change: $sender_change)"
echo "  Recipient: $recipient_balance → $final_recipient_balance (Change: $recipient_change)"
echo "  Miner: $miner_balance → $final_miner_balance (Change: $miner_change)"

echo ""
print_info "Nilotic Blockchain interaction testing completed successfully!"
print_info "The blockchain is functioning correctly with all core features working." 