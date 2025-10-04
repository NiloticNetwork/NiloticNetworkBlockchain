#!/bin/bash

# Consensus Security Penetration Testing Script
# This script performs comprehensive security testing of the consensus harmony system

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TEST_DIR}/build"
LOG_DIR="${TEST_DIR}/logs/security_tests"
REPORT_FILE="${LOG_DIR}/penetration_test_report_$(date +%Y%m%d_%H%M%S).json"

# Create directories
mkdir -p "${LOG_DIR}"
mkdir -p "${BUILD_DIR}"

echo -e "${BLUE}=== Nilotic Blockchain Consensus Security Penetration Testing ===${NC}"
echo "Test Directory: ${TEST_DIR}"
echo "Log Directory: ${LOG_DIR}"
echo "Report File: ${REPORT_FILE}"
echo ""

# Function to log messages
log_message() {
    local level=$1
    local message=$2
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    case $level in
        "INFO")
            echo -e "${GREEN}[INFO]${NC} ${timestamp} - ${message}"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} ${timestamp} - ${message}"
            ;;
        "ERROR")
            echo -e "${RED}[ERROR]${NC} ${timestamp} - ${message}"
            ;;
        "TEST")
            echo -e "${BLUE}[TEST]${NC} ${timestamp} - ${message}"
            ;;
    esac
}

# Function to run a test and capture results
run_test() {
    local test_name=$1
    local test_command=$2
    local expected_result=$3
    
    log_message "TEST" "Running ${test_name}..."
    
    local start_time=$(date +%s)
    local result=0
    local output=""
    
    # Run the test and capture output
    if output=$(eval "$test_command" 2>&1); then
        result=0
    else
        result=$?
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Determine if test passed based on expected result
    local status="UNKNOWN"
    if [ "$expected_result" = "PASS" ] && [ $result -eq 0 ]; then
        status="PASS"
        log_message "INFO" "${test_name} - PASSED (${duration}s)"
    elif [ "$expected_result" = "FAIL" ] && [ $result -ne 0 ]; then
        status="PASS"
        log_message "INFO" "${test_name} - PASSED (Expected failure detected) (${duration}s)"
    elif [ "$expected_result" = "PASS" ] && [ $result -ne 0 ]; then
        status="FAIL"
        log_message "ERROR" "${test_name} - FAILED (${duration}s)"
    elif [ "$expected_result" = "FAIL" ] && [ $result -eq 0 ]; then
        status="FAIL"
        log_message "ERROR" "${test_name} - FAILED (Expected failure not detected) (${duration}s)"
    fi
    
    # Store test result in JSON format
    cat >> "${REPORT_FILE}" << EOF
{
  "test_name": "${test_name}",
  "status": "${status}",
  "duration": ${duration},
  "expected_result": "${expected_result}",
  "actual_result": ${result},
  "output": $(echo "$output" | jq -R -s .),
  "timestamp": "$(date -Iseconds)"
},
EOF
    
    return $result
}

# Initialize report file
cat > "${REPORT_FILE}" << EOF
{
  "penetration_test_report": {
    "start_time": "$(date -Iseconds)",
    "test_environment": {
      "hostname": "$(hostname)",
      "os": "$(uname -s)",
      "architecture": "$(uname -m)",
      "test_directory": "${TEST_DIR}"
    },
    "tests": [
EOF

log_message "INFO" "Starting penetration testing suite..."

# Test 1: Build the security test executable
log_message "TEST" "Building security test executable..."
cd "${TEST_DIR}/tests"
if make -f Makefile_consensus_security clean && make -f Makefile_consensus_security all; then
    log_message "INFO" "Security test executable built successfully"
else
    log_message "ERROR" "Failed to build security test executable"
    exit 1
fi

# Test 2: Basic security validator functionality
run_test "Basic Security Validator" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.ValidatorInitialization'" \
         "PASS"

# Test 3: Cryptographic validation tests
run_test "Cryptographic Validation" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.CryptographicSignatureValidation'" \
         "PASS"

# Test 4: Invalid cryptographic signature detection
run_test "Invalid Crypto Detection" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.InvalidCryptographicSignature'" \
         "PASS"

# Test 5: Timestamp manipulation detection
run_test "Timestamp Manipulation Detection" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.TimestampManipulationDetection'" \
         "PASS"

# Test 6: Sybil attack detection
run_test "Sybil Attack Detection" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.SybilAttackDetection'" \
         "PASS"

# Test 7: 51% attack detection
run_test "51% Attack Detection" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.FiftyOnePercentAttackDetection'" \
         "PASS"

# Test 8: Request flooding attack
run_test "Request Flooding Attack" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_MassiveRequestFlood'" \
         "PASS"

# Test 9: Timestamp manipulation penetration test
run_test "Timestamp Manipulation Penetration" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_TimestampManipulation'" \
         "PASS"

# Test 10: Cryptographic attacks penetration test
run_test "Cryptographic Attacks Penetration" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_CryptographicAttacks'" \
         "PASS"

# Test 11: Data integrity attacks
run_test "Data Integrity Attacks" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_DataIntegrityAttacks'" \
         "PASS"

# Test 12: Sybil attack variations
run_test "Sybil Attack Variations" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_SybilAttackVariations'" \
         "PASS"

# Test 13: Consensus manipulation
run_test "Consensus Manipulation" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_ConsensusManipulation'" \
         "PASS"

# Test 14: Security auditor functionality
run_test "Security Auditor" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.AuditorInitialization'" \
         "PASS"

# Test 15: Integrated security validation and auditing
run_test "Integrated Security System" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.IntegratedSecurityValidationAndAuditing'" \
         "PASS"

# Test 16: Security metrics accuracy
run_test "Security Metrics Accuracy" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.SecurityMetricsAccuracy'" \
         "PASS"

# Test 17: Memory leak testing (if valgrind is available)
if command -v valgrind &> /dev/null; then
    log_message "TEST" "Running memory leak tests..."
    run_test "Memory Leak Test" \
             "valgrind --leak-check=full --error-exitcode=1 ./test_consensus_security --gtest_filter='ConsensusSecurityTest.ValidatorInitialization' > /dev/null 2>&1" \
             "PASS"
else
    log_message "WARN" "Valgrind not available, skipping memory leak tests"
fi

# Test 18: Performance stress test
log_message "TEST" "Running performance stress test..."
run_test "Performance Stress Test" \
         "timeout 30s ./test_consensus_security --gtest_filter='ConsensusSecurityTest.PenetrationTest_MassiveRequestFlood'" \
         "PASS"

# Test 19: Concurrent access test
log_message "TEST" "Running concurrent access test..."
run_test "Concurrent Access Test" \
         "for i in {1..5}; do ./test_consensus_security --gtest_filter='ConsensusSecurityTest.ValidatorInitialization' & done; wait" \
         "PASS"

# Test 20: Configuration validation
log_message "TEST" "Testing configuration validation..."
run_test "Configuration Validation" \
         "./test_consensus_security --gtest_filter='ConsensusSecurityTest.AuditorInitialization'" \
         "PASS"

# Finalize report
cat >> "${REPORT_FILE}" << EOF
    ],
    "end_time": "$(date -Iseconds)",
    "summary": {
      "total_tests": $(grep -c '"test_name"' "${REPORT_FILE}"),
      "passed_tests": $(grep -c '"status": "PASS"' "${REPORT_FILE}"),
      "failed_tests": $(grep -c '"status": "FAIL"' "${REPORT_FILE}")
    }
  }
}
EOF

# Remove trailing comma from JSON
sed -i '$ s/,$//' "${REPORT_FILE}"

# Generate summary
TOTAL_TESTS=$(grep -c '"test_name"' "${REPORT_FILE}")
PASSED_TESTS=$(grep -c '"status": "PASS"' "${REPORT_FILE}")
FAILED_TESTS=$(grep -c '"status": "FAIL"' "${REPORT_FILE}")

echo ""
echo -e "${BLUE}=== Penetration Testing Summary ===${NC}"
echo "Total Tests: ${TOTAL_TESTS}"
echo -e "Passed Tests: ${GREEN}${PASSED_TESTS}${NC}"
echo -e "Failed Tests: ${RED}${FAILED_TESTS}${NC}"
echo "Report saved to: ${REPORT_FILE}"

# Generate recommendations
echo ""
echo -e "${BLUE}=== Security Recommendations ===${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✓ All security tests passed successfully${NC}"
    echo "• Continue regular security monitoring"
    echo "• Consider implementing additional security measures for production"
    echo "• Schedule regular penetration testing"
else
    echo -e "${RED}⚠ ${FAILED_TESTS} security test(s) failed${NC}"
    echo "• Review failed tests and address security vulnerabilities"
    echo "• Implement additional security controls"
    echo "• Consider security audit by external experts"
fi

echo ""
echo "• Enable comprehensive audit logging in production"
echo "• Implement real-time security monitoring"
echo "• Set up automated security alerts"
echo "• Regular security updates and patches"
echo "• Network segmentation and access controls"
echo "• Backup and disaster recovery procedures"

# Create a simple HTML report
HTML_REPORT="${LOG_DIR}/penetration_test_report_$(date +%Y%m%d_%H%M%S).html"
cat > "${HTML_REPORT}" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Consensus Security Penetration Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .summary { background-color: #e8f5e8; padding: 15px; margin: 20px 0; border-radius: 5px; }
        .test-pass { color: green; }
        .test-fail { color: red; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Nilotic Blockchain Consensus Security Penetration Test Report</h1>
        <p>Generated on: $(date)</p>
        <p>Test Environment: $(hostname) ($(uname -s) $(uname -m))</p>
    </div>
    
    <div class="summary">
        <h2>Summary</h2>
        <p>Total Tests: ${TOTAL_TESTS}</p>
        <p class="test-pass">Passed Tests: ${PASSED_TESTS}</p>
        <p class="test-fail">Failed Tests: ${FAILED_TESTS}</p>
    </div>
    
    <h2>Test Results</h2>
    <table>
        <tr>
            <th>Test Name</th>
            <th>Status</th>
            <th>Duration (s)</th>
            <th>Expected</th>
        </tr>
EOF

# Parse JSON and add test results to HTML
if command -v jq &> /dev/null; then
    jq -r '.penetration_test_report.tests[] | "<tr><td>\(.test_name)</td><td class=\"test-\(.status | ascii_downcase)\">\(.status)</td><td>\(.duration)</td><td>\(.expected_result)</td></tr>"' "${REPORT_FILE}" >> "${HTML_REPORT}"
fi

cat >> "${HTML_REPORT}" << EOF
    </table>
    
    <h2>Recommendations</h2>
    <ul>
        <li>Enable comprehensive audit logging in production</li>
        <li>Implement real-time security monitoring</li>
        <li>Set up automated security alerts</li>
        <li>Regular security updates and patches</li>
        <li>Network segmentation and access controls</li>
        <li>Backup and disaster recovery procedures</li>
    </ul>
</body>
</html>
EOF

echo "HTML report saved to: ${HTML_REPORT}"

# Exit with appropriate code
if [ $FAILED_TESTS -eq 0 ]; then
    log_message "INFO" "All penetration tests completed successfully"
    exit 0
else
    log_message "ERROR" "Some penetration tests failed"
    exit 1
fi