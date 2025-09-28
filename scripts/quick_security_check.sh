#!/bin/bash
# Quick security validation script

echo "🔒 Quick Security Check"
echo "======================"

PASSED=0
FAILED=0

# Check 1: No demo keys
echo "1. Checking for demo keys..."
if ! grep -r "demo-key" src/ include/ 2>/dev/null; then
    echo "✅ No demo keys found"
    ((PASSED++))
else
    echo "❌ Demo keys still present"
    ((FAILED++))
fi

# Check 2: Cryptographic functions
echo "2. Checking cryptographic implementation..."
if grep -q "signData" include/core/utils.h && grep -q "verifySignature" include/core/utils.h; then
    echo "✅ Cryptographic functions implemented"
    ((PASSED++))
else
    echo "❌ Cryptographic functions missing"
    ((FAILED++))
fi

# Check 3: Rate limiting
echo "3. Checking rate limiting..."
if test -f include/core/rate_limiter.h && grep -q "rateLimiter.isAllowed" src/core/main.cpp; then
    echo "✅ Rate limiting implemented"
    ((PASSED++))
else
    echo "❌ Rate limiting missing"
    ((FAILED++))
fi

# Check 4: Security middleware
echo "4. Checking security middleware..."
if test -f include/core/security_middleware.h && grep -q "securityMiddleware" src/core/main.cpp; then
    echo "✅ Security middleware implemented"
    ((PASSED++))
else
    echo "❌ Security middleware missing"
    ((FAILED++))
fi

# Check 5: Input validation
echo "5. Checking input validation..."
if grep -q "validateInput" include/core/utils.h && grep -q "sanitizeInput" include/core/utils.h; then
    echo "✅ Input validation implemented"
    ((PASSED++))
else
    echo "❌ Input validation missing"
    ((FAILED++))
fi

echo ""
echo "Summary:"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo "🎉 All critical security fixes are in place!"
    exit 0
else
    echo "❌ Some security fixes are missing"
    exit 1
fi