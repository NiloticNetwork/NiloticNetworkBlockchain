#!/bin/bash
# Unit test runner for Nilotic Blockchain

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🧪 Running Unit Tests${NC}"

# Check if tests directory exists
if [ ! -d "tests/unit" ]; then
    echo -e "${YELLOW}⚠️  No unit tests found${NC}"
    echo "Create tests in tests/unit/ directory"
    exit 0
fi

# Run unit tests
echo -e "${YELLOW}Running unit tests...${NC}"

# Find and run all test executables
find tests/unit -name "*_test" -type f -executable | while read test_file; do
    echo -e "${BLUE}Running: $test_file${NC}"
    if $test_file; then
        echo -e "${GREEN}✅ $test_file passed${NC}"
    else
        echo -e "${RED}❌ $test_file failed${NC}"
        exit 1
    fi
done

echo -e "${GREEN}🎉 All unit tests passed!${NC}" 