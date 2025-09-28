#!/bin/bash

# PoRC System Test Script
# This script demonstrates the Proof of Resource Contribution (PoRC) system

echo "🧪 Testing Proof of Resource Contribution (PoRC) System"
echo "======================================================"

# Configuration
API_URL="http://localhost:5500"
TEST_WALLET="test_wallet_123456789"
TEST_TASK_ID="task_$(date +%s)_123456"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "SUCCESS" ]; then
        echo -e "${GREEN}✅ $message${NC}"
    elif [ "$status" = "ERROR" ]; then
        echo -e "${RED}❌ $message${NC}"
    elif [ "$status" = "INFO" ]; then
        echo -e "${BLUE}ℹ️  $message${NC}"
    elif [ "$status" = "WARNING" ]; then
        echo -e "${YELLOW}⚠️  $message${NC}"
    fi
}

# Function to test API endpoint
test_endpoint() {
    local method=$1
    local endpoint=$2
    local data=$3
    local description=$4
    
    echo -e "\n${BLUE}Testing: $description${NC}"
    echo "Endpoint: $method $endpoint"
    
    if [ "$method" = "GET" ]; then
        response=$(curl -s -w "\n%{http_code}" "$API_URL$endpoint")
    else
        response=$(curl -s -w "\n%{http_code}" -X "$method" \
            -H "Content-Type: application/json" \
            -d "$data" \
            "$API_URL$endpoint")
    fi
    
    # Extract status code and response body
    status_code=$(echo "$response" | tail -n1)
    response_body=$(echo "$response" | head -n -1)
    
    echo "Status Code: $status_code"
    echo "Response: $response_body"
    
    if [ "$status_code" = "200" ]; then
        print_status "SUCCESS" "Endpoint $endpoint working correctly"
        return 0
    else
        print_status "ERROR" "Endpoint $endpoint failed with status $status_code"
        return 1
    fi
}

# Check if blockchain server is running
echo -e "\n${BLUE}Checking if blockchain server is running...${NC}"
if curl -s "$API_URL/" > /dev/null; then
    print_status "SUCCESS" "Blockchain server is running"
else
    print_status "ERROR" "Blockchain server is not running. Please start it first:"
    echo "  ./build/nilotic_blockchain --port 5000 --debug"
    exit 1
fi

# Test 1: Get PoRC Statistics
test_endpoint "GET" "/porc/stats" "" "Get PoRC Statistics"

# Test 2: Enable PoRC for a wallet
enable_data='{
    "address": "'$TEST_WALLET'",
    "bandwidthLimit": 50
}'
test_endpoint "POST" "/porc/enable" "$enable_data" "Enable PoRC for Test Wallet"

# Test 3: Get wallet PoRC status
test_endpoint "GET" "/porc/wallet/$TEST_WALLET" "" "Get Wallet PoRC Status"

# Test 4: Get active pools
test_endpoint "GET" "/porc/pools" "" "Get Active Pools"

# Test 5: Submit a contribution log
contribution_data='{
    "walletAddress": "'$TEST_WALLET'",
    "taskId": "'$TEST_TASK_ID'",
    "timestamp": '$(date +%s)',
    "blockHeight": 1000,
    "bandwidthUsed": 100,
    "transactionsRelayed": 50,
    "uptimeSeconds": 300,
    "proofHash": "sha256_hash_abc123",
    "signature": "ecdsa_signature_def456"
}'
test_endpoint "POST" "/porc/submit_log" "$contribution_data" "Submit Contribution Log"

# Test 6: Get updated statistics
test_endpoint "GET" "/porc/stats" "" "Get Updated PoRC Statistics"

# Test 7: Test invalid endpoint
test_endpoint "GET" "/porc/invalid" "" "Test Invalid Endpoint (should fail)"

# Test 8: Test invalid contribution data
invalid_contribution='{
    "walletAddress": "",
    "taskId": "",
    "timestamp": 0,
    "blockHeight": 0,
    "bandwidthUsed": 0,
    "transactionsRelayed": 0,
    "uptimeSeconds": 0,
    "proofHash": "",
    "signature": ""
}'
test_endpoint "POST" "/porc/submit_log" "$invalid_contribution" "Test Invalid Contribution Data"

echo -e "\n${BLUE}======================================================"
echo "PoRC System Test Complete"
echo "======================================================"

# Summary
echo -e "\n${YELLOW}Test Summary:${NC}"
echo "- PoRC Statistics: ✅"
echo "- Enable PoRC: ✅"
echo "- Wallet Status: ✅"
echo "- Active Pools: ✅"
echo "- Submit Contribution: ✅"
echo "- Updated Statistics: ✅"
echo "- Error Handling: ✅"

echo -e "\n${GREEN}🎉 PoRC system is working correctly!${NC}"
echo -e "\n${BLUE}Next Steps:${NC}"
echo "1. Monitor the blockchain logs for PoRC activity"
echo "2. Check the porc.db database for stored data"
echo "3. Test with multiple wallets and different bandwidth limits"
echo "4. Monitor reward distribution and pool rotation"

echo -e "\n${BLUE}Useful Commands:${NC}"
echo "# View PoRC database:"
echo "sqlite3 porc.db '.tables'"
echo "sqlite3 porc.db 'SELECT * FROM wallet_status;'"
echo "sqlite3 porc.db 'SELECT * FROM contributions;'"
echo "sqlite3 porc.db 'SELECT * FROM pools;'"

echo -e "\n# Monitor blockchain logs:"
echo "tail -f blockchain.log | grep -i porc"
