#!/bin/bash

# Consensus Integration Test Runner
# Runs all consensus integration tests in sequence
# Requirements: 1.1, 1.2, 1.3, 1.4, 1.5

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TESTS_DIR="tests"
REPORT_DIR="test_reports"
LOG_FILE="$REPORT_DIR/test_execution.log"

# Create reports directory
mkdir -p "$REPORT_DIR"

# Initialize log file
echo "Consensus Integration Test Execution Log" > "$LOG_FILE"
echo "Started at: $(date)" >> "$LOG_FILE"
echo "=========================================" >> "$LOG_FILE"

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
    echo "$message" >> "$LOG_FILE"
}

# Function to run a test
run_test() {
    local test_name=$1
    local makefile=$2
    local description=$3
    
    print_status "$BLUE" "\n=== Running $test_name ==="
    print_status "$YELLOW" "Description: $description"
    
    echo "Starting $test_name at $(date)" >> "$LOG_FILE"
    
    # Build the test
    print_status "$YELLOW" "Building $test_name..."
    if make -f "$makefile" clean > /dev/null 2>&1 && make -f "$makefile" all >> "$LOG_FILE" 2>&1; then
        print_status "$GREEN" "✓ Build successful"
    else
        print_status "$RED" "✗ Build failed"
        echo "Build failed for $test_name" >> "$LOG_FILE"
        return 1
    fi
    
    # Run the test
    print_status "$YELLOW" "Executing $test_name..."
    local start_time=$(date +%s)
    
    if make -f "$makefile" test >> "$LOG_FILE" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_status "$GREEN" "✓ $test_name PASSED (${duration}s)"
        echo "$test_name PASSED in ${duration}s" >> "$LOG_FILE"
        return 0
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_status "$RED" "✗ $test_name FAILED (${duration}s)"
        echo "$test_name FAILED in ${duration}s" >> "$LOG_FILE"
        return 1
    fi
}

# Function to check prerequisites
check_prerequisites() {
    print_status "$BLUE" "=== Checking Prerequisites ==="
    
    # Check if we're in the right directory
    if [ ! -d "src/core" ] || [ ! -d "include/core" ]; then
        print_status "$RED" "✗ Please run this script from the project root directory"
        exit 1
    fi
    
    # Check for required tools
    local tools=("g++" "make")
    for tool in "${tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            print_status "$GREEN" "✓ $tool found"
        else
            print_status "$RED" "✗ $tool not found. Please install it."
            exit 1
        fi
    done
    
    # Check for required libraries
    if [ ! -d "lib/nlohmann_json" ]; then
        print_status "$RED" "✗ nlohmann_json library not found"
        exit 1
    fi
    
    print_status "$GREEN" "✓ All prerequisites satisfied"
}

# Function to generate summary report
generate_summary() {
    local total_tests=$1
    local passed_tests=$2
    local failed_tests=$3
    local total_time=$4
    
    local summary_file="$REPORT_DIR/test_summary.txt"
    
    cat > "$summary_file" << EOF
Consensus Integration Test Suite Summary
=======================================
Execution Date: $(date)
Total Tests: $total_tests
Passed: $passed_tests
Failed: $failed_tests
Success Rate: $(( (passed_tests * 100) / total_tests ))%
Total Execution Time: ${total_time}s

Test Details:
EOF
    
    # Append log details to summary
    echo "" >> "$summary_file"
    echo "Detailed Log:" >> "$summary_file"
    echo "=============" >> "$summary_file"
    cat "$LOG_FILE" >> "$summary_file"
    
    print_status "$BLUE" "\n=== Test Suite Summary ==="
    print_status "$YELLOW" "Total Tests: $total_tests"
    print_status "$GREEN" "Passed: $passed_tests"
    print_status "$RED" "Failed: $failed_tests"
    print_status "$YELLOW" "Success Rate: $(( (passed_tests * 100) / total_tests ))%"
    print_status "$YELLOW" "Total Time: ${total_time}s"
    print_status "$BLUE" "Summary saved to: $summary_file"
}

# Main execution
main() {
    local suite_start_time=$(date +%s)
    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    
    print_status "$BLUE" "Consensus Integration Test Suite"
    print_status "$BLUE" "Requirements: 1.1, 1.2, 1.3, 1.4, 1.5"
    print_status "$BLUE" "================================="
    
    # Check prerequisites
    check_prerequisites
    
    # Define tests to run
    declare -a tests=(
        "Comprehensive Integration Tests|Makefile_comprehensive_consensus_integration|Tests all consensus mechanisms working together in harmony"
        "End-to-End Workflow Tests|Makefile_consensus_end_to_end|Tests complete consensus workflows from start to finish"
        "Stress Tests|Makefile_consensus_stress|Tests consensus system under extreme load conditions"
        "Automated Test Suite|Makefile_automated_consensus_suite|Comprehensive automated testing for continuous integration"
    )
    
    # Run each test
    for test_info in "${tests[@]}"; do
        IFS='|' read -r test_name makefile description <<< "$test_info"
        
        total_tests=$((total_tests + 1))
        
        if run_test "$test_name" "$TESTS_DIR/$makefile" "$description"; then
            passed_tests=$((passed_tests + 1))
        else
            failed_tests=$((failed_tests + 1))
            
            # Ask user if they want to continue after a failure
            print_status "$YELLOW" "Test failed. Continue with remaining tests? (y/n)"
            read -r continue_choice
            if [[ "$continue_choice" != "y" && "$continue_choice" != "Y" ]]; then
                print_status "$RED" "Test execution stopped by user"
                break
            fi
        fi
    done
    
    local suite_end_time=$(date +%s)
    local total_time=$((suite_end_time - suite_start_time))
    
    # Generate summary
    generate_summary "$total_tests" "$passed_tests" "$failed_tests" "$total_time"
    
    # Final status
    if [ "$failed_tests" -eq 0 ]; then
        print_status "$GREEN" "\n🎉 All consensus integration tests passed!"
        echo "All tests passed at $(date)" >> "$LOG_FILE"
        exit 0
    else
        print_status "$RED" "\n❌ $failed_tests out of $total_tests tests failed"
        echo "$failed_tests tests failed at $(date)" >> "$LOG_FILE"
        exit 1
    fi
}

# Handle script interruption
cleanup() {
    print_status "$YELLOW" "\nTest execution interrupted"
    echo "Test execution interrupted at $(date)" >> "$LOG_FILE"
    exit 130
}

trap cleanup INT TERM

# Run main function
main "$@"