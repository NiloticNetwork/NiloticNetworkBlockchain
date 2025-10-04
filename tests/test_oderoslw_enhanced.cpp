#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>
#include "../include/core/oderoslw.h"
#include "../include/core/oderoslw_manager.h"
#include "../include/core/logger.h"

void testBasicTokenCreation() {
    std::cout << "Testing basic token creation..." << std::endl;
    
    OderoSLW token("OSLW123456789", 100.0, "user123");
    
    assert(token.getTokenId() == "OSLW123456789");
    assert(token.getAmount() == 100.0);
    assert(token.getCreator() == "user123");
    assert(token.getState() == TokenState::CREATED);
    assert(token.getSecurityLevel() == SecurityLevel::BASIC);
    
    std::cout << "✓ Basic token creation test passed" << std::endl;
}

void testEnhancedTokenCreation() {
    std::cout << "Testing enhanced token creation..." << std::endl;
    
    OderoSLW token("OSLW987654321", 500.0, "enterprise_user", 
                   SecurityLevel::ENHANCED, "public_key_123");
    
    assert(token.getTokenId() == "OSLW987654321");
    assert(token.getAmount() == 500.0);
    assert(token.getCreator() == "enterprise_user");
    assert(token.getState() == TokenState::CREATED);
    assert(token.getSecurityLevel() == SecurityLevel::ENHANCED);
    assert(token.getPublicKey() == "public_key_123");
    
    std::cout << "✓ Enhanced token creation test passed" << std::endl;
}

void testTokenValidation() {
    std::cout << "Testing token validation..." << std::endl;
    
    OderoSLW token("OSLW111222333", 250.0, "validator_user");
    
    // Basic validation should pass
    bool isValid = token.verify();
    assert(isValid);
    
    // Check validation attempts are tracked
    assert(token.getValidationAttempts() > 0);
    
    std::cout << "✓ Token validation test passed" << std::endl;
}

void testTokenStateManagement() {
    std::cout << "Testing token state management..." << std::endl;
    
    OderoSLW token("OSLW444555666", 300.0, "state_user");
    
    // Test activation
    assert(token.getState() == TokenState::CREATED);
    bool activated = token.activate("activation_key");
    assert(activated);
    assert(token.getState() == TokenState::ACTIVE);
    
    // Test freezing
    bool frozen = token.freeze("admin", "Security review");
    assert(frozen);
    assert(token.getState() == TokenState::FROZEN);
    
    // Test unfreezing
    bool unfrozen = token.unfreeze("admin", "Review completed");
    assert(unfrozen);
    assert(token.getState() == TokenState::ACTIVE);
    
    // Test redemption
    bool redeemed = token.redeem("redeemer_user", "signature123");
    assert(redeemed);
    assert(token.getState() == TokenState::REDEEMED);
    
    std::cout << "✓ Token state management test passed" << std::endl;
}

void testTokenMetadata() {
    std::cout << "Testing token metadata..." << std::endl;
    
    OderoSLW token("OSLW777888999", 150.0, "metadata_user");
    
    // Add metadata
    token.addMetadata("category", "premium");
    token.addMetadata("region", "US");
    
    // Retrieve metadata
    assert(token.getMetadata("category") == "premium");
    assert(token.getMetadata("region") == "US");
    assert(token.getMetadata("nonexistent") == "");
    
    std::cout << "✓ Token metadata test passed" << std::endl;
}

void testTokenSerialization() {
    std::cout << "Testing token serialization..." << std::endl;
    
    OderoSLW originalToken("OSLW000111222", 400.0, "serial_user", SecurityLevel::ENHANCED);
    originalToken.addMetadata("test", "value");
    originalToken.activate("key");
    
    // Serialize to JSON
    std::string jsonStr = originalToken.toSecureJson();
    assert(!jsonStr.empty());
    
    // Deserialize from JSON
    OderoSLW deserializedToken = OderoSLW::fromJson(jsonStr);
    
    // Verify data integrity
    assert(deserializedToken.getTokenId() == originalToken.getTokenId());
    assert(deserializedToken.getAmount() == originalToken.getAmount());
    assert(deserializedToken.getCreator() == originalToken.getCreator());
    assert(deserializedToken.getState() == originalToken.getState());
    assert(deserializedToken.getSecurityLevel() == originalToken.getSecurityLevel());
    assert(deserializedToken.getMetadata("test") == "value");
    
    std::cout << "✓ Token serialization test passed" << std::endl;
}

void testTokenBatchOperations() {
    std::cout << "Testing token batch operations..." << std::endl;
    
    std::vector<std::pair<std::string, double>> tokenData = {
        {"token1", 100.0},
        {"token2", 200.0},
        {"token3", 300.0}
    };
    
    std::vector<OderoSLW> batchTokens = OderoSLW::createTokenBatch(tokenData, "batch_user", SecurityLevel::BASIC);
    
    assert(batchTokens.size() == 3);
    
    // Verify each token
    for (size_t i = 0; i < batchTokens.size(); ++i) {
        assert(batchTokens[i].getCreator() == "batch_user");
        assert(batchTokens[i].getAmount() == tokenData[i].second);
        assert(batchTokens[i].getSecurityLevel() == SecurityLevel::BASIC);
        assert(!batchTokens[i].getTokenId().empty());
    }
    
    // Test batch verification
    std::string merkleRoot = OderoSLW::generateBatchMerkleRoot(batchTokens);
    assert(!merkleRoot.empty());
    
    bool batchValid = OderoSLW::verifyTokenBatch(batchTokens, merkleRoot);
    assert(batchValid);
    
    std::cout << "✓ Token batch operations test passed" << std::endl;
}

void testTokenManager() {
    std::cout << "Testing token manager..." << std::endl;
    
    OderoSLWManager manager;
    
    // Test token creation
    std::string tokenId = manager.createToken("manager_user", 500.0, SecurityLevel::ENHANCED);
    assert(!tokenId.empty());
    
    // Test token retrieval
    OderoSLW* token = manager.getToken(tokenId);
    assert(token != nullptr);
    assert(token->getAmount() == 500.0);
    
    // Test token validation
    TokenValidationResult result = manager.validateToken(tokenId);
    assert(result.isValid);
    
    // Test user tokens
    std::vector<std::string> userTokens = manager.getUserTokens("manager_user");
    assert(userTokens.size() == 1);
    assert(userTokens[0] == tokenId);
    
    // Test token activation
    bool activated = manager.activateToken(tokenId, "activation_key");
    assert(activated);
    
    // Test statistics
    TokenStatistics stats = manager.getStatistics();
    assert(stats.totalTokens >= 1);
    assert(stats.activeTokens >= 1);
    
    std::cout << "✓ Token manager test passed" << std::endl;
}

void testSecurityFeatures() {
    std::cout << "Testing security features..." << std::endl;
    
    OderoSLWManager manager;
    
    // Test blacklisting
    std::string tokenId = manager.createToken("security_user", 100.0);
    assert(!tokenId.empty());
    
    // Blacklist the token
    manager.addToBlacklist(tokenId);
    assert(manager.isBlacklisted(tokenId));
    
    // Validation should fail for blacklisted token
    TokenValidationResult result = manager.validateToken(tokenId);
    assert(!result.isValid);
    assert(result.errorMessage.find("blacklisted") != std::string::npos);
    
    // Test address blacklisting
    manager.blacklistAddress("malicious_user");
    assert(manager.isAddressBlacklisted("malicious_user"));
    
    // Token creation should fail for blacklisted address
    std::string blockedTokenId = manager.createToken("malicious_user", 200.0);
    assert(blockedTokenId.empty());
    
    std::cout << "✓ Security features test passed" << std::endl;
}

void testQRCodeGeneration() {
    std::cout << "Testing QR code generation..." << std::endl;
    
    OderoSLW token("OSLWQR123456", 75.0, "qr_user");
    
    // Test basic QR code
    std::string qrCode = token.generateQrCode();
    assert(!qrCode.empty());
    assert(qrCode.find("ODEROSLW") != std::string::npos);
    assert(qrCode.find("OSLWQR123456") != std::string::npos);
    
    // Test secure QR code
    std::string secureQrCode = token.generateSecureQrCode();
    assert(!secureQrCode.empty());
    assert(secureQrCode.find("ODEROSLW_SECURE") != std::string::npos);
    
    std::cout << "✓ QR code generation test passed" << std::endl;
}

void testAuditTrail() {
    std::cout << "Testing audit trail..." << std::endl;
    
    OderoSLW token("OSLWAUDIT123", 200.0, "audit_user");
    
    // Token should have creation audit entry
    std::vector<TokenTransaction> auditTrail = token.getAuditTrail();
    assert(!auditTrail.empty());
    
    // First entry should be creation
    assert(auditTrail[0].action == "CREATE");
    assert(auditTrail[0].actor == "audit_user");
    
    // Perform operations and check audit trail
    token.activate("key");
    token.freeze("admin", "test");
    
    auditTrail = token.getAuditTrail();
    assert(auditTrail.size() >= 3); // CREATE, ACTIVATE, FREEZE
    
    std::cout << "✓ Audit trail test passed" << std::endl;
}

void testPerformanceAndStress() {
    std::cout << "Testing performance and stress..." << std::endl;
    
    OderoSLWManager manager;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create many tokens
    const int numTokens = 1000;
    std::vector<std::string> tokenIds;
    
    for (int i = 0; i < numTokens; ++i) {
        std::string tokenId = manager.createToken("perf_user_" + std::to_string(i), 
                                                 100.0 + i, SecurityLevel::BASIC);
        if (!tokenId.empty()) {
            tokenIds.push_back(tokenId);
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Created " << tokenIds.size() << " tokens in " << duration.count() << "ms" << std::endl;
    
    // Validate all tokens
    startTime = std::chrono::high_resolution_clock::now();
    
    int validTokens = 0;
    for (const std::string& tokenId : tokenIds) {
        TokenValidationResult result = manager.validateToken(tokenId);
        if (result.isValid) {
            validTokens++;
        }
    }
    
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Validated " << validTokens << " tokens in " << duration.count() << "ms" << std::endl;
    
    assert(validTokens == tokenIds.size());
    
    std::cout << "✓ Performance and stress test passed" << std::endl;
}

int main() {
    std::cout << "=== Enhanced OderoSLW Token System Tests ===" << std::endl;
    
    try {
        testBasicTokenCreation();
        testEnhancedTokenCreation();
        testTokenValidation();
        testTokenStateManagement();
        testTokenMetadata();
        testTokenSerialization();
        testTokenBatchOperations();
        testTokenManager();
        testSecurityFeatures();
        testQRCodeGeneration();
        testAuditTrail();
        testPerformanceAndStress();
        
        std::cout << std::endl << "🎉 All tests passed successfully!" << std::endl;
        std::cout << "Enhanced OderoSLW Token System is working correctly." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
    
    return 0;
}