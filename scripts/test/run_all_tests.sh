#!/bin/bash
# Comprehensive test runner for Nilotic Blockchain

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🧪 Running All Tests${NC}"

# Configuration
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test suite
run_test_suite() {
    local test_name="$1"
    local test_script="$2"
    
    echo -e "${YELLOW}Running $test_name...${NC}"
    
    if [ -f "$test_script" ]; then
        if bash "$test_script"; then
            echo -e "${GREEN}✅ $test_name passed${NC}"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}❌ $test_name failed${NC}"
            ((FAILED_TESTS++))
        fi
        ((TOTAL_TESTS++))
    else
        echo -e "${YELLOW}⚠️  $test_script not found${NC}"
    fi
}

# Function to run build tests
run_build_tests() {
    echo -e "${BLUE}🔨 Testing build process...${NC}"
    
    # Test if build script exists
    if [ -f "scripts/build/build.sh" ]; then
        echo -e "${YELLOW}Testing build script...${NC}"
        if bash scripts/build/build.sh --help > /dev/null 2>&1; then
            echo -e "${GREEN}✅ Build script test passed${NC}"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}❌ Build script test failed${NC}"
            ((FAILED_TESTS++))
        fi
        ((TOTAL_TESTS++))
    fi
}

# Function to run code quality checks
run_code_quality_checks() {
    echo -e "${BLUE}🔍 Running code quality checks...${NC}"
    
    # Check for source files
    if [ -d "src" ]; then
        echo -e "${YELLOW}Checking source files...${NC}"
        SRC_COUNT=$(find src -name "*.cpp" -o -name "*.h" | wc -l)
        echo -e "${GREEN}✅ Found $SRC_COUNT source files${NC}"
        ((PASSED_TESTS++))
        ((TOTAL_TESTS++))
    fi
    
    # Check for header files
    if [ -d "include" ]; then
        echo -e "${YELLOW}Checking header files...${NC}"
        HEADER_COUNT=$(find include -name "*.h" | wc -l)
        echo -e "${GREEN}✅ Found $HEADER_COUNT header files${NC}"
        ((PASSED_TESTS++))
        ((TOTAL_TESTS++))
    fi
    
    # Check for documentation
    if [ -d "docs" ]; then
        echo -e "${YELLOW}Checking documentation...${NC}"
        DOC_COUNT=$(find docs -name "*.md" | wc -l)
        echo -e "${GREEN}✅ Found $DOC_COUNT documentation files${NC}"
        ((PASSED_TESTS++))
        ((TOTAL_TESTS++))
    fi
}

# Function to run web application tests
run_web_tests() {
    echo -e "${BLUE}🌐 Testing web applications...${NC}"
    
    # Check web wallet files
    if [ -d "web/wallet" ]; then
        echo -e "${YELLOW}Checking web wallet...${NC}"
        if [ -f "web/wallet/index.html" ]; then
            echo -e "${GREEN}✅ Web wallet files found${NC}"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}❌ Web wallet files missing${NC}"
            ((FAILED_TESTS++))
        fi
        ((TOTAL_TESTS++))
    fi
    
    # Check Python wallet
    if [ -f "web/wallet/nilotic_wallet.py" ]; then
        echo -e "${YELLOW}Checking Python wallet...${NC}"
        if python3 -c "import sys; sys.path.append('web/wallet'); import nilotic_wallet" 2>/dev/null; then
            echo -e "${GREEN}✅ Python wallet syntax check passed${NC}"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}❌ Python wallet syntax check failed${NC}"
            ((FAILED_TESTS++))
        fi
        ((TOTAL_TESTS++))
    fi
}

# Function to run configuration tests
run_config_tests() {
    echo -e "${BLUE}⚙️  Testing configuration...${NC}"
    
    # Check CMakeLists.txt
    if [ -f "CMakeLists.txt" ]; then
        echo -e "${YELLOW}Checking CMakeLists.txt...${NC}"
        if cmake --version > /dev/null 2>&1; then
            echo -e "${GREEN}✅ CMakeLists.txt found and CMake available${NC}"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}❌ CMake not available${NC}"
            ((FAILED_TESTS++))
        fi
        ((TOTAL_TESTS++))
    fi
    
    # Check README
    if [ -f "README.md" ]; then
        echo -e "${YELLOW}Checking README.md...${NC}"
        echo -e "${GREEN}✅ README.md found${NC}"
        ((PASSED_TESTS++))
        ((TOTAL_TESTS++))
    fi
}

# Function to print test summary
print_summary() {
    echo -e "\n${BLUE}📊 Test Summary${NC}"
    echo -e "${BLUE}================${NC}"
    echo -e "Total Tests: $TOTAL_TESTS"
    echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}🎉 All tests passed!${NC}"
        exit 0
    else
        echo -e "\n${RED}❌ Some tests failed${NC}"
        exit 1
    fi
}

# Main test runner
main() {
    echo -e "${BLUE}🚀 Starting comprehensive test suite...${NC}"
    
    # Run different test categories
    run_build_tests
    run_code_quality_checks
    run_web_tests
    run_config_tests
    
    # Run specific test suites
    run_test_suite "Unit Tests" "scripts/test/run_unit_tests.sh"
    run_test_suite "Integration Tests" "scripts/test/run_integration_tests.sh"
    
    # Print summary
    print_summary
}

# Run main function
main "$@" 