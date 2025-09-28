#!/bin/bash
# Security audit script for Nilotic Blockchain

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🔒 Nilotic Blockchain Security Audit${NC}"
echo -e "${BLUE}====================================${NC}"

AUDIT_PASSED=0
AUDIT_FAILED=0
AUDIT_WARNINGS=0

# Function to check for security issues
check_security_issue() {
    local description="$1"
    local command="$2"
    local expected_result="$3"
    
    echo -e "${YELLOW}Checking: $description${NC}"
    
    if eval "$command"; then
        if [ "$expected_result" = "pass" ]; then
            echo -e "${GREEN}✅ PASS: $description${NC}"
            ((AUDIT_PASSED++))
        else
            echo -e "${RED}❌ FAIL: $description${NC}"
            ((AUDIT_FAILED++))
        fi
    else
        if [ "$expected_result" = "fail" ]; then
            echo -e "${GREEN}✅ PASS: $description${NC}"
            ((AUDIT_PASSED++))
        else
            echo -e "${RED}❌ FAIL: $description${NC}"
            ((AUDIT_FAILED++))
        fi
    fi
}

# Function to check for warnings
check_security_warning() {
    local description="$1"
    local command="$2"
    
    echo -e "${YELLOW}Warning Check: $description${NC}"
    
    if eval "$command"; then
        echo -e "${YELLOW}⚠️  WARNING: $description${NC}"
        ((AUDIT_WARNINGS++))
    else
        echo -e "${GREEN}✅ OK: $description${NC}"
    fi
}

echo -e "\n${BLUE}1. Checking for hardcoded demo keys...${NC}"
check_security_issue "No demo-key usage in source code" \
    "! grep -r 'demo-key' src/ include/" "pass"

echo -e "\n${BLUE}2. Checking cryptographic implementation...${NC}"
check_security_issue "Proper signature verification implemented" \
    "grep -q 'Utils::verifySignature' include/core/transaction.h" "pass"

check_security_issue "Proper key generation implemented" \
    "grep -q 'Utils::generateKeyPair' include/core/utils.h" "pass"

echo -e "\n${BLUE}3. Checking input validation...${NC}"
check_security_issue "Input validation functions exist" \
    "grep -q 'validateInput' include/core/utils.h" "pass"

check_security_issue "Input sanitization functions exist" \
    "grep -q 'sanitizeInput' include/core/utils.h" "pass"

echo -e "\n${BLUE}4. Checking rate limiting...${NC}"
check_security_issue "Rate limiter implementation exists" \
    "test -f include/core/rate_limiter.h" "pass"

check_security_issue "Rate limiting integrated in main" \
    "grep -q 'rateLimiter.isAllowed' src/core/main.cpp" "pass"

echo -e "\n${BLUE}5. Checking security middleware...${NC}"
check_security_issue "Security middleware exists" \
    "test -f include/core/security_middleware.h" "pass"

check_security_issue "Security headers implementation" \
    "grep -q 'addSecurityHeaders' include/core/security_middleware.h" "pass"

echo -e "\n${BLUE}6. Checking smart contract security...${NC}"
check_security_issue "No eval() usage in smart contracts" \
    "! grep -r 'eval(' src/ include/" "pass"

check_security_issue "No file system access in contracts" \
    "! grep -r 'fopen\\|fwrite\\|system(' src/core/smart_contract_vm.cpp" "pass"

echo -e "\n${BLUE}7. Checking logging implementation...${NC}"
check_security_issue "Security event logging exists" \
    "grep -q 'logSecurityEvent' include/core/logger.h" "pass"

check_security_issue "Authentication logging exists" \
    "grep -q 'logAuthentication' include/core/logger.h" "pass"

echo -e "\n${BLUE}8. Checking configuration security...${NC}"
check_security_issue "Security configuration file exists" \
    "test -f config/security.json" "pass"

echo -e "\n${BLUE}9. Checking for dangerous patterns...${NC}"
check_security_warning "No TODO/FIXME security comments" \
    "grep -r 'TODO.*security\\|FIXME.*security' src/ include/"

check_security_warning "No hardcoded passwords" \
    "grep -r 'password.*=.*\"' src/ include/"

check_security_warning "No hardcoded secrets" \
    "grep -r 'secret.*=.*\"\\|key.*=.*\"' src/ include/"

echo -e "\n${BLUE}10. Checking build security...${NC}"
check_security_issue "Security components in CMakeLists" \
    "grep -q 'rate_limiter.cpp\\|security_middleware.cpp' CMakeLists.txt" "pass"

echo -e "\n${BLUE}11. Checking test coverage...${NC}"
check_security_issue "Security tests exist" \
    "test -f tests/security_tests.cpp" "pass"

echo -e "\n${BLUE}12. Checking documentation...${NC}"
check_security_issue "Security audit report exists" \
    "test -f SECURITY_AUDIT_REPORT.md" "pass"

# Summary
echo -e "\n${BLUE}📊 Security Audit Summary${NC}"
echo -e "${BLUE}=========================${NC}"
echo -e "Checks Passed: ${GREEN}$AUDIT_PASSED${NC}"
echo -e "Checks Failed: ${RED}$AUDIT_FAILED${NC}"
echo -e "Warnings: ${YELLOW}$AUDIT_WARNINGS${NC}"

if [ $AUDIT_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 Security audit completed successfully!${NC}"
    if [ $AUDIT_WARNINGS -gt 0 ]; then
        echo -e "${YELLOW}⚠️  Please review the warnings above${NC}"
    fi
    exit 0
else
    echo -e "\n${RED}❌ Security audit failed with $AUDIT_FAILED issues${NC}"
    echo -e "${RED}Please fix the failed checks before deployment${NC}"
    exit 1
fi