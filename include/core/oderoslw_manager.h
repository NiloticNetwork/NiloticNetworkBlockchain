#ifndef ODEROSLW_MANAGER_H
#define ODEROSLW_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <set>
#include <chrono>
#include "oderoslw.h"
#include "json.hpp"

// Forward declarations
class Blockchain;
class Transaction;

// Token validation result
struct TokenValidationResult {
    bool isValid;
    std::string errorMessage;
    double riskScore;
    std::vector<std::string> warnings;
};

// Token statistics
struct TokenStatistics {
    size_t totalTokens;
    size_t activeTokens;
    size_t redeemedTokens;
    size_t expiredTokens;
    size_t frozenTokens;
    size_t revokedTokens;
    double totalValue;
    double averageRiskScore;
};

class OderoSLWManager {
private:
    std::unordered_map<std::string, OderoSLW> tokens;
    std::map<std::string, std::vector<std::string>> userTokens; // user -> token IDs
    std::map<std::string, std::string> tokenOwners; // token ID -> owner
    std::mutex tokensMutex;
    
    // Security settings
    double maxRiskThreshold;
    int maxValidationAttempts;
    bool enableFraudDetection;
    bool requireDeviceFingerprint;
    bool requireLocationVerification;
    
    // Blockchain integration
    Blockchain* blockchain;
    
    // Token blacklist for security
    std::set<std::string> blacklistedTokens;
    std::set<std::string> blacklistedAddresses;
    
    // Performance monitoring
    std::map<std::string, int> operationCounts;
    std::chrono::steady_clock::time_point lastCleanup;
    
    // Internal validation methods
    bool validateTokenCreationRequest(const std::string& creator, double amount, SecurityLevel level);
    bool checkFraudIndicators(const OderoSLW& token);
    void updateTokenStatistics();
    void performPeriodicCleanup();

public:
    // Constructor
    OderoSLWManager(Blockchain* blockchainPtr = nullptr);
    
    // Destructor
    ~OderoSLWManager();
    
    // Token creation and management
    std::string createToken(const std::string& creator, double amount, 
                           SecurityLevel level = SecurityLevel::BASIC,
                           const std::string& publicKey = "");
    
    std::vector<std::string> createTokenBatch(const std::vector<std::pair<std::string, double>>& tokenData,
                                             const std::string& creator, SecurityLevel level);
    
    bool activateToken(const std::string& tokenId, const std::string& activatorKey);
    bool redeemToken(const std::string& tokenId, const std::string& redeemer, const std::string& signature);
    bool transferToken(const std::string& tokenId, const std::string& newOwner, const std::string& signature);
    
    // Token state management
    bool freezeToken(const std::string& tokenId, const std::string& authority, const std::string& reason);
    bool unfreezeToken(const std::string& tokenId, const std::string& authority, const std::string& reason);
    bool revokeToken(const std::string& tokenId, const std::string& authority, const std::string& reason);
    
    // Token validation and verification
    TokenValidationResult validateToken(const std::string& tokenId);
    TokenValidationResult validateTokenWithContext(const std::string& tokenId, 
                                                  const std::string& deviceFingerprint,
                                                  const std::string& locationHash);
    
    bool verifyTokenBatch(const std::vector<std::string>& tokenIds, const std::string& merkleRoot);
    
    // Token queries
    OderoSLW* getToken(const std::string& tokenId);
    std::vector<std::string> getUserTokens(const std::string& userAddress);
    std::vector<OderoSLW> getTokensByState(TokenState state);
    std::vector<OderoSLW> getTokensBySecurityLevel(SecurityLevel level);
    
    // Security and fraud detection
    void addToBlacklist(const std::string& tokenId);
    void removeFromBlacklist(const std::string& tokenId);
    bool isBlacklisted(const std::string& tokenId);
    
    void blacklistAddress(const std::string& address);
    void removeAddressFromBlacklist(const std::string& address);
    bool isAddressBlacklisted(const std::string& address);
    
    std::vector<std::string> detectSuspiciousTokens();
    void performSecurityAudit();
    
    // Configuration
    void setMaxRiskThreshold(double threshold) { maxRiskThreshold = threshold; }
    void setMaxValidationAttempts(int attempts) { maxValidationAttempts = attempts; }
    void setEnableFraudDetection(bool enable) { enableFraudDetection = enable; }
    void setRequireDeviceFingerprint(bool require) { requireDeviceFingerprint = require; }
    void setRequireLocationVerification(bool require) { requireLocationVerification = require; }
    
    // Statistics and monitoring
    TokenStatistics getStatistics();
    nlohmann::json getDetailedStatistics();
    std::map<std::string, int> getOperationCounts() { return operationCounts; }
    
    // Blockchain integration
    void setBlockchain(Blockchain* blockchainPtr) { blockchain = blockchainPtr; }
    bool integrateWithBlockchain(const std::string& tokenId);
    bool createBlockchainTransaction(const OderoSLW& token, const std::string& action);
    
    // Persistence
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);
    
    // Cleanup and maintenance
    void cleanupExpiredTokens();
    void archiveOldTokens(int daysOld);
    void optimizeStorage();
    
    // API helpers
    nlohmann::json tokenToJson(const std::string& tokenId, bool includeSecurityDetails = false);
    std::string generateTokenReport(const std::string& tokenId);
    
    // Batch operations
    std::vector<TokenValidationResult> validateTokenBatch(const std::vector<std::string>& tokenIds);
    bool processTokenBatch(const std::vector<std::string>& tokenIds, const std::string& action);
    
    // Event logging
    void logTokenEvent(const std::string& tokenId, const std::string& event, const std::string& details);
    std::vector<std::string> getTokenEvents(const std::string& tokenId);
    
    // Health check
    bool performHealthCheck();
    nlohmann::json getHealthStatus();
};

#endif // ODEROSLW_MANAGER_H