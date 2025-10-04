#include "oderoslw_manager.h"
#include "blockchain.h"
#include "transaction.h"
#include "utils.h"
#include "logger.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <set>

// Constructor
OderoSLWManager::OderoSLWManager(Blockchain* blockchainPtr) 
    : blockchain(blockchainPtr), maxRiskThreshold(0.8), maxValidationAttempts(5),
      enableFraudDetection(true), requireDeviceFingerprint(false), 
      requireLocationVerification(false) {
    
    lastCleanup = std::chrono::steady_clock::now();
    Logger::info("OderoSLW Token Manager initialized");
}

// Destructor
OderoSLWManager::~OderoSLWManager() {
    Logger::info("OderoSLW Token Manager shutting down");
}

// Validate token creation request
bool OderoSLWManager::validateTokenCreationRequest(const std::string& creator, double amount, SecurityLevel level) {
    // Check if address is blacklisted
    if (isAddressBlacklisted(creator)) {
        Logger::error("Token creation denied: Blacklisted address - " + creator);
        return false;
    }
    
    // Validate amount
    if (!OderoSLW::validateAmount(amount)) {
        Logger::error("Token creation denied: Invalid amount - " + std::to_string(amount));
        return false;
    }
    
    // Validate creator address
    if (!OderoSLW::validateAddress(creator)) {
        Logger::error("Token creation denied: Invalid creator address - " + creator);
        return false;
    }
    
    // Check rate limiting (simplified)
    auto userTokenCount = getUserTokens(creator).size();
    if (userTokenCount > 100) { // Max 100 tokens per user
        Logger::error("Token creation denied: Rate limit exceeded for " + creator);
        return false;
    }
    
    return true;
}

// Check fraud indicators
bool OderoSLWManager::checkFraudIndicators(const OderoSLW& token) {
    if (!enableFraudDetection) {
        return false; // No fraud detected if detection is disabled
    }
    
    // Check risk score
    if (token.getRiskScore() > maxRiskThreshold) {
        Logger::warning("Fraud indicator: High risk score - " + std::to_string(token.getRiskScore()));
        return true;
    }
    
    // Check validation attempts
    if (token.getValidationAttempts() > maxValidationAttempts) {
        Logger::warning("Fraud indicator: Excessive validation attempts - " + std::to_string(token.getValidationAttempts()));
        return true;
    }
    
    // Check for missing security features
    if (requireDeviceFingerprint && token.getMetadata("deviceFingerprint").empty()) {
        Logger::warning("Fraud indicator: Missing device fingerprint");
        return true;
    }
    
    if (requireLocationVerification && token.getMetadata("locationHash").empty()) {
        Logger::warning("Fraud indicator: Missing location verification");
        return true;
    }
    
    return false;
}

// Create token
std::string OderoSLWManager::createToken(const std::string& creator, double amount, 
                                        SecurityLevel level, const std::string& publicKey) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    // Validate creation request
    if (!validateTokenCreationRequest(creator, amount, level)) {
        return "";
    }
    
    // Generate unique token ID
    std::string tokenId = "OSLW" + Utils::calculateSHA256(creator + std::to_string(amount) + 
                                                          std::to_string(time(nullptr))).substr(0, 16);
    
    // Create token
    OderoSLW token(tokenId, amount, creator, level, publicKey);
    
    // Store token
    tokens[tokenId] = token;
    userTokens[creator].push_back(tokenId);
    tokenOwners[tokenId] = creator;
    
    // Update operation count
    operationCounts["CREATE"]++;
    
    // Integrate with blockchain if available
    if (blockchain) {
        integrateWithBlockchain(tokenId);
    }
    
    Logger::info("Token created successfully: " + tokenId + " for " + creator);
    return tokenId;
}

// Create token batch
std::vector<std::string> OderoSLWManager::createTokenBatch(const std::vector<std::pair<std::string, double>>& tokenData,
                                                          const std::string& creator, SecurityLevel level) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    std::vector<std::string> tokenIds;
    
    // Validate creation request
    if (!validateTokenCreationRequest(creator, 0.0, level)) {
        return tokenIds;
    }
    
    // Create tokens using static method
    std::vector<OderoSLW> batchTokens = OderoSLW::createTokenBatch(tokenData, creator, level);
    
    // Store tokens
    for (const auto& token : batchTokens) {
        std::string tokenId = token.getTokenId();
        tokens[tokenId] = token;
        userTokens[creator].push_back(tokenId);
        tokenOwners[tokenId] = creator;
        tokenIds.push_back(tokenId);
        
        // Integrate with blockchain if available
        if (blockchain) {
            integrateWithBlockchain(tokenId);
        }
    }
    
    // Update operation count
    operationCounts["CREATE_BATCH"]++;
    
    Logger::info("Token batch created: " + std::to_string(tokenIds.size()) + " tokens for " + creator);
    return tokenIds;
}

// Activate token
bool OderoSLWManager::activateToken(const std::string& tokenId, const std::string& activatorKey) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    auto it = tokens.find(tokenId);
    if (it == tokens.end()) {
        Logger::error("Token activation failed: Token not found - " + tokenId);
        return false;
    }
    
    if (it->second.activate(activatorKey)) {
        operationCounts["ACTIVATE"]++;
        
        // Create blockchain transaction
        if (blockchain) {
            createBlockchainTransaction(it->second, "ACTIVATE");
        }
        
        return true;
    }
    
    return false;
}

// Redeem token
bool OderoSLWManager::redeemToken(const std::string& tokenId, const std::string& redeemer, const std::string& signature) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    auto it = tokens.find(tokenId);
    if (it == tokens.end()) {
        Logger::error("Token redemption failed: Token not found - " + tokenId);
        return false;
    }
    
    // Check fraud indicators
    if (checkFraudIndicators(it->second)) {
        Logger::error("Token redemption blocked: Fraud indicators detected - " + tokenId);
        return false;
    }
    
    if (it->second.redeem(redeemer, signature)) {
        operationCounts["REDEEM"]++;
        
        // Update ownership
        tokenOwners[tokenId] = redeemer;
        
        // Create blockchain transaction
        if (blockchain) {
            createBlockchainTransaction(it->second, "REDEEM");
        }
        
        return true;
    }
    
    return false;
}

// Transfer token
bool OderoSLWManager::transferToken(const std::string& tokenId, const std::string& newOwner, const std::string& signature) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    auto it = tokens.find(tokenId);
    if (it == tokens.end()) {
        Logger::error("Token transfer failed: Token not found - " + tokenId);
        return false;
    }
    
    // Check if new owner is blacklisted
    if (isAddressBlacklisted(newOwner)) {
        Logger::error("Token transfer blocked: Blacklisted recipient - " + newOwner);
        return false;
    }
    
    std::string oldOwner = it->second.getCreator();
    
    if (it->second.transfer(newOwner, signature)) {
        operationCounts["TRANSFER"]++;
        
        // Update ownership tracking
        tokenOwners[tokenId] = newOwner;
        
        // Update user token lists
        auto& oldUserTokens = userTokens[oldOwner];
        oldUserTokens.erase(std::remove(oldUserTokens.begin(), oldUserTokens.end(), tokenId), oldUserTokens.end());
        userTokens[newOwner].push_back(tokenId);
        
        // Create blockchain transaction
        if (blockchain) {
            createBlockchainTransaction(it->second, "TRANSFER");
        }
        
        return true;
    }
    
    return false;
}

// Validate token
TokenValidationResult OderoSLWManager::validateToken(const std::string& tokenId) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    TokenValidationResult result;
    result.isValid = false;
    result.riskScore = 1.0;
    
    auto it = tokens.find(tokenId);
    if (it == tokens.end()) {
        result.errorMessage = "Token not found";
        return result;
    }
    
    // Check if token is blacklisted
    if (isBlacklisted(tokenId)) {
        result.errorMessage = "Token is blacklisted";
        result.warnings.push_back("Security: Token appears on blacklist");
        return result;
    }
    
    // Perform token validation
    result.isValid = it->second.verify();
    result.riskScore = it->second.getRiskScore();
    
    if (!result.isValid) {
        result.errorMessage = "Token validation failed";
    }
    
    // Add warnings based on risk score
    if (result.riskScore > 0.6) {
        result.warnings.push_back("High risk score: " + std::to_string(result.riskScore));
    }
    
    if (it->second.getValidationAttempts() > 3) {
        result.warnings.push_back("Multiple validation attempts detected");
    }
    
    operationCounts["VALIDATE"]++;
    return result;
}

// Validate token with context
TokenValidationResult OderoSLWManager::validateTokenWithContext(const std::string& tokenId, 
                                                               const std::string& deviceFingerprint,
                                                               const std::string& locationHash) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    auto it = tokens.find(tokenId);
    if (it == tokens.end()) {
        TokenValidationResult result;
        result.isValid = false;
        result.errorMessage = "Token not found";
        result.riskScore = 1.0;
        return result;
    }
    
    // Set context information
    if (!deviceFingerprint.empty()) {
        const_cast<OderoSLW&>(it->second).setDeviceFingerprint(deviceFingerprint);
    }
    
    if (!locationHash.empty()) {
        const_cast<OderoSLW&>(it->second).setLocationHash(locationHash);
    }
    
    // Perform validation
    return validateToken(tokenId);
}

// Get token
OderoSLW* OderoSLWManager::getToken(const std::string& tokenId) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    auto it = tokens.find(tokenId);
    if (it != tokens.end()) {
        return &it->second;
    }
    
    return nullptr;
}

// Get user tokens
std::vector<std::string> OderoSLWManager::getUserTokens(const std::string& userAddress) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    auto it = userTokens.find(userAddress);
    if (it != userTokens.end()) {
        return it->second;
    }
    
    return std::vector<std::string>();
}

// Get tokens by state
std::vector<OderoSLW> OderoSLWManager::getTokensByState(TokenState state) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    std::vector<OderoSLW> result;
    
    for (const auto& pair : tokens) {
        if (pair.second.getState() == state) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

// Blacklist management
void OderoSLWManager::addToBlacklist(const std::string& tokenId) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    blacklistedTokens.insert(tokenId);
    Logger::warning("Token added to blacklist: " + tokenId);
}

bool OderoSLWManager::isBlacklisted(const std::string& tokenId) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    return blacklistedTokens.find(tokenId) != blacklistedTokens.end();
}

void OderoSLWManager::blacklistAddress(const std::string& address) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    blacklistedAddresses.insert(address);
    Logger::warning("Address added to blacklist: " + address);
}

bool OderoSLWManager::isAddressBlacklisted(const std::string& address) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    return blacklistedAddresses.find(address) != blacklistedAddresses.end();
}

// Get statistics
TokenStatistics OderoSLWManager::getStatistics() {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    TokenStatistics stats = {};
    stats.totalTokens = tokens.size();
    
    double totalRisk = 0.0;
    
    for (const auto& pair : tokens) {
        const OderoSLW& token = pair.second;
        
        switch (token.getState()) {
            case TokenState::ACTIVE:
                stats.activeTokens++;
                break;
            case TokenState::REDEEMED:
                stats.redeemedTokens++;
                break;
            case TokenState::EXPIRED:
                stats.expiredTokens++;
                break;
            case TokenState::FROZEN:
                stats.frozenTokens++;
                break;
            case TokenState::REVOKED:
                stats.revokedTokens++;
                break;
            default:
                break;
        }
        
        stats.totalValue += token.getAmount();
        totalRisk += token.getRiskScore();
    }
    
    if (stats.totalTokens > 0) {
        stats.averageRiskScore = totalRisk / stats.totalTokens;
    }
    
    return stats;
}

// Integrate with blockchain
bool OderoSLWManager::integrateWithBlockchain(const std::string& tokenId) {
    if (!blockchain) {
        return false;
    }
    
    auto it = tokens.find(tokenId);
    if (it == tokens.end()) {
        return false;
    }
    
    return createBlockchainTransaction(it->second, "CREATE");
}

// Create blockchain transaction
bool OderoSLWManager::createBlockchainTransaction(const OderoSLW& token, const std::string& action) {
    if (!blockchain) {
        return false;
    }
    
    // Create a transaction representing the token operation
    Transaction tx(token.getCreator(), "ODEROSLW_" + action, token.getAmount(), true);
    
    // Add token metadata to transaction (in a real implementation, this would be more sophisticated)
    // For now, we'll just add it to the blockchain's pending transactions
    
    return blockchain->addTransaction(tx);
}

// Save to file
bool OderoSLWManager::saveToFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    try {
        nlohmann::json j;
        
        // Save tokens
        nlohmann::json tokensJson;
        for (const auto& pair : tokens) {
            tokensJson[pair.first] = nlohmann::json::parse(pair.second.toSecureJson());
        }
        j["tokens"] = tokensJson;
        
        // Save user tokens mapping
        j["userTokens"] = userTokens;
        j["tokenOwners"] = tokenOwners;
        
        // Save blacklists
        j["blacklistedTokens"] = std::vector<std::string>(blacklistedTokens.begin(), blacklistedTokens.end());
        j["blacklistedAddresses"] = std::vector<std::string>(blacklistedAddresses.begin(), blacklistedAddresses.end());
        
        // Save configuration
        j["config"]["maxRiskThreshold"] = maxRiskThreshold;
        j["config"]["maxValidationAttempts"] = maxValidationAttempts;
        j["config"]["enableFraudDetection"] = enableFraudDetection;
        
        // Save statistics
        j["operationCounts"] = operationCounts;
        
        std::ofstream file(filename);
        file << j.dump(4);
        file.close();
        
        Logger::info("Token manager data saved to: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to save token manager data: " + std::string(e.what()));
        return false;
    }
}

// Perform health check
bool OderoSLWManager::performHealthCheck() {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    bool healthy = true;
    
    // Check for corrupted tokens
    for (const auto& pair : tokens) {
        if (!pair.second.verify()) {
            Logger::warning("Health check: Corrupted token detected - " + pair.first);
            healthy = false;
        }
    }
    
    // Check memory usage (simplified)
    if (tokens.size() > 100000) {
        Logger::warning("Health check: High memory usage - " + std::to_string(tokens.size()) + " tokens");
        healthy = false;
    }
    
    // Perform periodic cleanup if needed
    auto now = std::chrono::steady_clock::now();
    auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::hours>(now - lastCleanup).count();
    
    if (timeSinceCleanup > 24) { // Cleanup every 24 hours
        performPeriodicCleanup();
        lastCleanup = now;
    }
    
    return healthy;
}

// Perform periodic cleanup
void OderoSLWManager::performPeriodicCleanup() {
    Logger::info("Performing periodic cleanup");
    
    // Clean up expired tokens
    cleanupExpiredTokens();
    
    // Archive old tokens (older than 30 days)
    archiveOldTokens(30);
    
    Logger::info("Periodic cleanup completed");
}

// Cleanup expired tokens
void OderoSLWManager::cleanupExpiredTokens() {
    std::lock_guard<std::mutex> lock(tokensMutex);
    
    std::vector<std::string> expiredTokens;
    
    for (auto& pair : tokens) {
        // Check if token is expired (simplified check)
        if (pair.second.getState() == TokenState::EXPIRED) {
            expiredTokens.push_back(pair.first);
        }
    }
    
    // Remove expired tokens
    for (const std::string& tokenId : expiredTokens) {
        tokens.erase(tokenId);
        
        // Clean up ownership tracking
        auto ownerIt = tokenOwners.find(tokenId);
        if (ownerIt != tokenOwners.end()) {
            std::string owner = ownerIt->second;
            tokenOwners.erase(ownerIt);
            
            // Remove from user tokens list
            auto& userTokenList = userTokens[owner];
            userTokenList.erase(std::remove(userTokenList.begin(), userTokenList.end(), tokenId), userTokenList.end());
        }
    }
    
    if (!expiredTokens.empty()) {
        Logger::info("Cleaned up " + std::to_string(expiredTokens.size()) + " expired tokens");
    }
}

// Archive old tokens
void OderoSLWManager::archiveOldTokens(int daysOld) {
    // Implementation would move old tokens to archive storage
    // For now, just log the operation
    Logger::info("Archiving tokens older than " + std::to_string(daysOld) + " days");
}