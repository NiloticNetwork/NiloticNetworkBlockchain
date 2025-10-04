#!/bin/bash

# Consensus API Test Runner
# This script runs all consensus API related tests

echo "🚀 Starting Consensus API Test Suite"
echo "====================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name=$1
    local makefile=$2
    local description=$3
    
    echo -e "\n${YELLOW}Running $test_name...${NC}"
    echo "Description: $description"
    echo "----------------------------------------"
    
    # Build the test
    if make -f $makefile clean && make -f $makefile; then
        echo -e "${GREEN}✓ Build successful${NC}"
        
        # Run the test
        if make -f $makefile test; then
            echo -e "${GREEN}✓ $test_name PASSED${NC}"
            ((TESTS_PASSED++))
        else
            echo -e "${RED}✗ $test_name FAILED${NC}"
            ((TESTS_FAILED++))
        fi
    else
        echo -e "${RED}✗ Build failed for $test_name${NC}"
        ((TESTS_FAILED++))
    fi
    
    # Clean up
    make -f $makefile clean > /dev/null 2>&1
}

# Check dependencies
echo "Checking dependencies..."

# Check for libcurl (needed for HTTP API tests)
if ! pkg-config --exists libcurl; then
    echo -e "${YELLOW}Warning: libcurl not found. HTTP API tests may fail.${NC}"
    echo "Install with: sudo apt-get install libcurl4-openssl-dev"
fi

# Check for required headers
if [ ! -f "../include/core/consensus_harmony_manager.h" ]; then
    echo -e "${RED}Error: Consensus harmony headers not found${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Dependencies check completed${NC}"

# Run integration test first (doesn't require external dependencies)
run_test "Consensus Integration Test" "Makefile_consensus_integration" \
    "Tests the complete consensus system integration and functionality"

# Run WebSocket test
run_test "Consensus WebSocket Test" "Makefile_consensus_websocket" \
    "Tests real-time consensus monitoring via WebSocket connections"

# Run HTTP API test (requires libcurl)
if pkg-config --exists libcurl; then
    run_test "Consensus HTTP API Test" "Makefile_consensus_api" \
        "Tests all consensus REST API endpoints with HTTP requests"
else
    echo -e "${YELLOW}Skipping HTTP API test (libcurl not available)${NC}"
fi

# Summary
echo ""
echo "====================================="
echo "🏁 Consensus API Test Suite Complete"
echo "====================================="
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All tests passed successfully!${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Some tests failed. Please check the output above.${NC}"
    exit 1
fi