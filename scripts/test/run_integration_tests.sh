#!/bin/bash
# Integration test runner for Nilotic Blockchain

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🧪 Running Integration Tests${NC}"

# Configuration
BLOCKCHAIN_URL="http://localhost:5500"
TEST_TIMEOUT=30

# Function to start blockchain server
start_blockchain() {
    echo -e "${YELLOW}Starting blockchain server...${NC}"
    ./build/nilotic_blockchain --port 5500 --debug &
    BLOCKCHAIN_PID=$!
    
    # Wait for server to start
    sleep 3
    
    # Test if server is running
    if curl -s "$BLOCKCHAIN_URL/" > /dev/null; then
        echo -e "${GREEN}✅ Blockchain server started${NC}"
    else
        echo -e "${RED}❌ Failed to start blockchain server${NC}"
        kill $BLOCKCHAIN_PID 2>/dev/null
        exit 1
    fi
}

# Function to stop blockchain server
stop_blockchain() {
    echo -e "${YELLOW}Stopping blockchain server...${NC}"
    kill $BLOCKCHAIN_PID 2>/dev/null
    wait $BLOCKCHAIN_PID 2>/dev/null || true
    echo -e "${GREEN}✅ Blockchain server stopped${NC}"
}

# Function to test API endpoints
test_api_endpoints() {
    echo -e "${BLUE}Testing API endpoints...${NC}"
    
    # Test basic info
    echo -e "${YELLOW}Testing GET /${NC}"
    if curl -s "$BLOCKCHAIN_URL/" | grep -q "status"; then
        echo -e "${GREEN}✅ GET / passed${NC}"
    else
        echo -e "${RED}❌ GET / failed${NC}"
        return 1
    fi
    
    # Test balance endpoint
    echo -e "${YELLOW}Testing GET /balance${NC}"
    if curl -s "$BLOCKCHAIN_URL/balance?address=test_wallet" | grep -q "balance"; then
        echo -e "${GREEN}✅ GET /balance passed${NC}"
    else
        echo -e "${RED}❌ GET /balance failed${NC}"
        return 1
    fi
    
    # Test mining endpoint
    echo -e "${YELLOW}Testing POST /mine${NC}"
    if curl -s -X POST -H "Content-Type: application/json" \
        -d '{"miner_address":"test_wallet"}' \
        "$BLOCKCHAIN_URL/mine" | grep -q "success"; then
        echo -e "${GREEN}✅ POST /mine passed${NC}"
    else
        echo -e "${RED}❌ POST /mine failed${NC}"
        return 1
    fi
    
    # Test chain endpoint
    echo -e "${YELLOW}Testing GET /chain${NC}"
    if curl -s "$BLOCKCHAIN_URL/chain" | grep -q "chain_height"; then
        echo -e "${GREEN}✅ GET /chain passed${NC}"
    else
        echo -e "${RED}❌ GET /chain failed${NC}"
        return 1
    fi
}

# Function to test wallet functionality
test_wallet_functionality() {
    echo -e "${BLUE}Testing wallet functionality...${NC}"
    
    # Test Python wallet
    if [ -f "web/wallet/nilotic_wallet.py" ]; then
        echo -e "${YELLOW}Testing Python wallet...${NC}"
        if python3 web/wallet/nilotic_wallet.py --url "$BLOCKCHAIN_URL" info > /dev/null; then
            echo -e "${GREEN}✅ Python wallet test passed${NC}"
        else
            echo -e "${RED}❌ Python wallet test failed${NC}"
            return 1
        fi
    fi
}

# Function to test web wallet
test_web_wallet() {
    echo -e "${BLUE}Testing web wallet...${NC}"
    
    # Start a simple HTTP server for testing
    cd web/wallet
    python3 -m http.server 8000 &
    HTTP_PID=$!
    cd ../..
    
    sleep 2
    
    # Test if web wallet files exist
    if [ -f "web/wallet/index.html" ]; then
        echo -e "${GREEN}✅ Web wallet files found${NC}"
    else
        echo -e "${RED}❌ Web wallet files not found${NC}"
        kill $HTTP_PID 2>/dev/null
        return 1
    fi
    
    # Stop HTTP server
    kill $HTTP_PID 2>/dev/null
    wait $HTTP_PID 2>/dev/null || true
}

# Main test function
main() {
    echo -e "${BLUE}🚀 Starting integration tests...${NC}"
    
    # Start blockchain server
    start_blockchain
    
    # Set trap to stop server on exit
    trap stop_blockchain EXIT
    
    # Run tests
    test_api_endpoints
    test_wallet_functionality
    test_web_wallet
    
    echo -e "${GREEN}🎉 All integration tests passed!${NC}"
}

# Run main function
main "$@" 