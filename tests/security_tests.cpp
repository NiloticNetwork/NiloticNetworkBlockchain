#include <gtest/gtest.h>
#include "../include/core/utils.h"
#include "../include/core/rate_limiter.h"
#include "../include/core/security_middleware.h"
#include "../include/core/transaction.h"
#include <thread>
#include <chrono>

class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup
    }
};

// Test cryptographic signing and verification
TEST_F(SecurityTest, CryptographicSigningTest) {
    std::string publicKey;
    std::string privateKey = Utils::generateKeyPair(publicKey);
    
    ASSERT_FALSE(privateKey.empty());
    ASSERT_FALSE(publicKey.empty());
    
    std::string testData = "test transaction data";
    std::string signature = Utils::signData(testData, privateKey);
    
    ASSERT_FALSE(signature.empty());
    ASSERT_TRUE(Utils::verifySignature(testData, signature, publicKey));
    
    // Test with wrong data
    ASSERT_FALSE(Utils::verifySignature("wrong data", signature, publicKey));
}

// Test input validation
TEST_F(SecurityTest, InputValidationTest) {
    // Valid inputs
    ASSERT_TRUE(Utils::validateInput("valid_address_123", 256));
    ASSERT_TRUE(Utils::validateInput("user@example.com", 256));
    
    // Invalid inputs
    ASSERT_FALSE(Utils::validateInput("", 256)); // Empty
    ASSERT_FALSE(Utils::validateInput(std::string(1000, 'a'), 256)); // Too long
    ASSERT_FALSE(Utils::validateInput("test\0null", 256)); // Null byte
    ASSERT_FALSE(Utils::validateInput("test\x01control", 256)); // Control character
}

// Test input sanitization
TEST_F(SecurityTest, InputSanitizationTest) {
    std::string malicious = "<script>alert('xss')</script>";
    std::string sanitized = Utils::sanitizeInput(malicious);
    
    ASSERT_EQ(sanitized, "scriptalert");
    
    std::string sqlInjection = "'; DROP TABLE users; --";
    std::string sanitizedSql = Utils::sanitizeInput(sqlInjection);
    
    ASSERT_EQ(sanitizedSql, "");
}

// Test rate limiting
TEST_F(SecurityTest, RateLimitingTest) {
    RateLimiter limiter(5, 100, std::chrono::minutes(1)); // 5 per minute
    
    std::string testIP = "192.168.1.100";
    
    // First 5 requests should be allowed
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(limiter.isAllowed(testIP));
    }
    
    // 6th request should be blocked
    ASSERT_FALSE(limiter.isAllowed(testIP));
    
    // Different IP should still be allowed
    ASSERT_TRUE(limiter.isAllowed("192.168.1.101"));
}

// Test security middleware
TEST_F(SecurityTest, SecurityMiddlewareTest) {
    SecurityMiddleware middleware(false); // Don't enforce HTTPS for testing
    
    // Test suspicious pattern detection
    std::string maliciousRequest = "GET /?id=1' OR '1'='1 HTTP/1.1\r\n\r\n";
    ASSERT_FALSE(middleware.validateRequest(maliciousRequest, "192.168.1.100"));
    
    // Test valid request
    std::string validRequest = "GET /balance?address=test_wallet HTTP/1.1\r\nContent-Type: application/json\r\n\r\n";
    ASSERT_TRUE(middleware.validateRequest(validRequest, "192.168.1.100"));
}

// Test transaction security
TEST_F(SecurityTest, TransactionSecurityTest) {
    Transaction tx("sender", "recipient", 100.0);
    
    // Test that demo keys are rejected
    ASSERT_FALSE(tx.signTransaction("demo-key"));
    ASSERT_FALSE(tx.signTransaction(""));
    
    // Test with proper key
    std::string publicKey;
    std::string privateKey = Utils::generateKeyPair(publicKey);
    ASSERT_TRUE(tx.signTransaction(privateKey));
    
    // Test signature verification
    ASSERT_TRUE(tx.verifySignature(publicKey));
    
    // Test with wrong public key
    std::string wrongPublicKey;
    std::string wrongPrivateKey = Utils::generateKeyPair(wrongPublicKey);
    ASSERT_FALSE(tx.verifySignature(wrongPublicKey));
}

// Test password strength validation
TEST_F(SecurityTest, PasswordStrengthTest) {
    // This would test the password validation if we had it in C++
    // For now, we test basic validation principles
    
    std::vector<std::string> weakPasswords = {
        "123456",
        "password",
        "qwerty",
        "abc123",
        "password123"
    };
    
    std::vector<std::string> strongPasswords = {
        "MyStr0ng!P@ssw0rd",
        "C0mpl3x&S3cur3!",
        "Th1s!sV3ryS3cur3"
    };
    
    // In a real implementation, you'd test actual password validation
    for (const auto& weak : weakPasswords) {
        ASSERT_LT(weak.length(), 12); // Weak passwords are typically short
    }
    
    for (const auto& strong : strongPasswords) {
        ASSERT_GE(strong.length(), 12); // Strong passwords should be long enough
        // Test for mixed case, numbers, and special characters
        bool hasUpper = std::any_of(strong.begin(), strong.end(), ::isupper);
        bool hasLower = std::any_of(strong.begin(), strong.end(), ::islower);
        bool hasDigit = std::any_of(strong.begin(), strong.end(), ::isdigit);
        bool hasSpecial = std::any_of(strong.begin(), strong.end(), [](char c) {
            return !std::isalnum(c);
        });
        
        ASSERT_TRUE(hasUpper && hasLower && hasDigit && hasSpecial);
    }
}

// Test CORS headers
TEST_F(SecurityTest, CorsHeadersTest) {
    SecurityMiddleware middleware(false);
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"test\": true}";
    std::string corsResponse = middleware.addCorsHeaders(response, "http://localhost:3000");
    
    ASSERT_NE(corsResponse.find("Access-Control-Allow-Origin"), std::string::npos);
    ASSERT_NE(corsResponse.find("Access-Control-Allow-Methods"), std::string::npos);
}

// Test security headers
TEST_F(SecurityTest, SecurityHeadersTest) {
    SecurityMiddleware middleware(false);
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"test\": true}";
    std::string secureResponse = middleware.addSecurityHeaders(response);
    
    ASSERT_NE(secureResponse.find("X-Content-Type-Options: nosniff"), std::string::npos);
    ASSERT_NE(secureResponse.find("X-Frame-Options: DENY"), std::string::npos);
    ASSERT_NE(secureResponse.find("X-XSS-Protection: 1; mode=block"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}