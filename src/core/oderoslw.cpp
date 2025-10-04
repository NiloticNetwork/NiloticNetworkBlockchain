#include "oderoslw.h"
#include "utils.h"
#include "logger.h"
#include "json.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cmath>

// Default constructor
OderoSLW::OderoSLW() 
    : tokenId(""), amount(0.0), creator(""), creationTime(""), expirationTime(""),
      state(TokenState::CREATED), securityLevel(SecurityLevel::BASIC),
      digitalSignature(""), publicKey(""), merkleRoot(""), nonce(""), salt(""),
      validationAttempts(0), lastValidationTime(""), isMultiSig(false),
      deviceFingerprint(""), locationHash(""), riskScore(0.0) {
    
    // Generate security parameters
    nonce = generateNonce();
    salt = generateSalt();
}

// Constructor with parameters
OderoSLW::OderoSLW(const std::string& tokenIdIn, double amountIn, const std::string& creatorIn)
    : tokenId(tokenIdIn), amount(amountIn), creator(creatorIn), expirationTime(""),
      state(TokenState::CREATED), securityLevel(SecurityLevel::BASIC),
      digitalSignature(""), publicKey(""), merkleRoot(""), nonce(""), salt(""),
      validationAttempts(0), lastValidationTime(""), isMultiSig(false),
      deviceFingerprint(""), locationHash(""), riskScore(0.0) {
    
    // Set creation time to current timestamp in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    creationTime = ss.str();
    
    // Generate security parameters
    nonce = generateNonce();
    salt = generateSalt();
    
    // Add creation audit entry
    addAuditEntry("CREATE", creator, "Token created with amount: " + std::to_string(amount));
    
    Logger::info("OderoSLW token created: " + tokenId + " for " + std::to_string(amount));
}

// Enhanced constructor with security level
OderoSLW::OderoSLW(const std::string& tokenIdIn, double amountIn, const std::string& creatorIn, 
                   SecurityLevel level, const std::string& publicKeyIn)
    : tokenId(tokenIdIn), amount(amountIn), creator(creatorIn), expirationTime(""),
      state(TokenState::CREATED), securityLevel(level), digitalSignature(""), 
      publicKey(publicKeyIn), merkleRoot(""), nonce(""), salt(""),
      validationAttempts(0), lastValidationTime(""), isMultiSig(false),
      deviceFingerprint(""), locationHash(""), riskScore(0.0) {
    
    // Set creation time to current timestamp in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    creationTime = ss.str();
    
    // Generate security parameters
    nonce = generateNonce();
    salt = generateSalt();
    
    // Set multi-sig for enterprise level
    if (level == SecurityLevel::ENTERPRISE) {
        isMultiSig = true;
        requiredSigners.push_back(creator); // Creator is always required
    }
    
    // Add creation audit entry
    addAuditEntry("CREATE", creator, "Enhanced token created with security level: " + securityLevelToString());
    
    Logger::info("Enhanced OderoSLW token created: " + tokenId + " with security level: " + securityLevelToString());
}

// Generate cryptographic nonce
std::string OderoSLW::generateNonce() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// Generate salt for hashing
std::string OderoSLW::generateSalt() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

// Calculate token hash with all security parameters
std::string OderoSLW::calculateSecureHash() const {
    std::stringstream ss;
    ss << tokenId << ":" << amount << ":" << creator << ":" << creationTime;
    ss << ":" << expirationTime << ":" << static_cast<int>(state);
    ss << ":" << static_cast<int>(securityLevel) << ":" << nonce << ":" << salt;
    
    // Include metadata in hash
    for (const auto& pair : metadata) {
        ss << ":" << pair.first << "=" << pair.second;
    }
    
    return Utils::calculateSHA256(ss.str());
}

// Validate token structure and format
bool OderoSLW::validateTokenStructure() const {
    // Check basic structure
    if (tokenId.empty() || creator.empty() || creationTime.empty() || amount <= 0) {
        return false;
    }
    
    // Validate token ID format
    if (!validateTokenId(tokenId)) {
        return false;
    }
    
    // Validate amount
    if (!validateAmount(amount)) {
        return false;
    }
    
    // Validate creator address
    if (!validateAddress(creator)) {
        return false;
    }
    
    return true;
}

// Validate cryptographic components
bool OderoSLW::validateCryptography() const {
    // Check nonce and salt
    if (nonce.empty() || salt.empty()) {
        return false;
    }
    
    // For enhanced and enterprise security, require digital signature
    if (securityLevel != SecurityLevel::BASIC) {
        if (digitalSignature.empty() || publicKey.empty()) {
            return false;
        }
        
        // Verify signature
        std::string hash = calculateSecureHash();
        if (!Utils::verifySignature(hash, digitalSignature, publicKey)) {
            return false;
        }
    }
    
    // For enterprise level, check multi-sig requirements
    if (securityLevel == SecurityLevel::ENTERPRISE && isMultiSig) {
        if (!isMultiSigComplete()) {
            return false;
        }
    }
    
    return true;
}

// Check token expiration
bool OderoSLW::isExpired() const {
    if (expirationTime.empty()) {
        return false; // No expiration set
    }
    
    // Parse expiration time and compare with current time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // For simplicity, assume expiration time is in the same format as creation time
    // In a real implementation, you'd use proper date parsing
    return false; // Placeholder - implement proper date comparison
}

// Calculate fraud risk score
double OderoSLW::calculateRiskScore() const {
    double risk = 0.0;
    
    // High validation attempts indicate potential fraud
    if (validationAttempts > 5) {
        risk += 0.3;
    }
    
    // Missing device fingerprint increases risk
    if (deviceFingerprint.empty()) {
        risk += 0.2;
    }
    
    // Missing location hash increases risk
    if (locationHash.empty()) {
        risk += 0.1;
    }
    
    // Large amounts have higher risk
    if (amount > 10000.0) {
        risk += 0.2;
    }
    
    // Recent creation might indicate rushed fraud attempt
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    // Simplified check - in real implementation, parse creation time properly
    
    return std::min(risk, 1.0); // Cap at 1.0
}

// Generate a QR code representation as a string
std::string OderoSLW::generateQrCode() const {
    std::stringstream qr_data;
    qr_data << "ODEROSLW:" << tokenId << ":" << amount << ":" << creator << ":" << creationTime;
    return "QR Code data: " + qr_data.str();
}

// Enhanced QR code with security features
std::string OderoSLW::generateSecureQrCode() const {
    std::stringstream qr_data;
    qr_data << "ODEROSLW_SECURE:" << tokenId << ":" << amount << ":" << creator;
    qr_data << ":" << creationTime << ":" << nonce << ":" << calculateSecureHash().substr(0, 16);
    qr_data << ":" << static_cast<int>(securityLevel) << ":" << static_cast<int>(state);
    
    return "Secure QR Code data: " + qr_data.str();
}

// Verify the token validity with enhanced security
bool OderoSLW::verify() const {
    // Increment validation attempts (mutable operation for security tracking)
    const_cast<OderoSLW*>(this)->validationAttempts++;
    
    // Update last validation time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    const_cast<OderoSLW*>(this)->lastValidationTime = ss.str();
    
    // Check if token is in a valid state for verification
    if (state == TokenState::REVOKED || state == TokenState::EXPIRED) {
        Logger::warning("Token verification failed: Invalid state - " + stateToString());
        return false;
    }
    
    // Check expiration
    if (isExpired()) {
        const_cast<OderoSLW*>(this)->state = TokenState::EXPIRED;
        Logger::warning("Token verification failed: Token expired - " + tokenId);
        return false;
    }
    
    // Validate token structure
    if (!validateTokenStructure()) {
        Logger::error("Token verification failed: Invalid structure - " + tokenId);
        return false;
    }
    
    // Validate cryptographic components based on security level
    if (!validateCryptography()) {
        Logger::error("Token verification failed: Cryptographic validation failed - " + tokenId);
        return false;
    }
    
    // Calculate and update risk score
    const_cast<OderoSLW*>(this)->riskScore = calculateRiskScore();
    
    // Reject high-risk tokens
    if (riskScore > 0.8) {
        Logger::warning("Token verification failed: High risk score - " + std::to_string(riskScore));
        return false;
    }
    
    // Add audit entry for successful verification
    const_cast<OderoSLW*>(this)->addAuditEntry("VERIFY", "SYSTEM", "Token verified successfully");
    
    Logger::info("Token verified successfully: " + tokenId);
    return true;
}

// Verify token with specific public key
bool OderoSLW::verifyWithKey(const std::string& publicKey) const {
    if (publicKey.empty()) {
        return false;
    }
    
    // Verify signature with provided key
    std::string hash = calculateSecureHash();
    return Utils::verifySignature(hash, digitalSignature, publicKey);
}

// Verify token batch using merkle tree
bool OderoSLW::verifyTokenBatch(const std::vector<OderoSLW>& tokens, const std::string& merkleRoot) {
    if (tokens.empty() || merkleRoot.empty()) {
        return false;
    }
    
    // Calculate merkle root from tokens
    std::string calculatedRoot = generateBatchMerkleRoot(tokens);
    
    // Compare with provided root
    bool isValid = (calculatedRoot == merkleRoot);
    
    if (isValid) {
        Logger::info("Token batch verification successful: " + std::to_string(tokens.size()) + " tokens");
    } else {
        Logger::error("Token batch verification failed: Merkle root mismatch");
    }
    
    return isValid;
}

// Get token metadata as JSON
std::string OderoSLW::getMetadata() const {
    nlohmann::json meta;
    meta["tokenType"] = "OderoSLW";
    meta["version"] = "2.0";
    meta["tokenId"] = tokenId;
    meta["creator"] = creator;
    meta["creationTime"] = creationTime;
    meta["amount"] = amount;
    meta["state"] = stateToString();
    meta["securityLevel"] = securityLevelToString();
    
    return meta.dump(4);
}

// Get enhanced metadata with security info
std::string OderoSLW::getEnhancedMetadata() const {
    nlohmann::json meta;
    meta["tokenType"] = "OderoSLW";
    meta["version"] = "2.0";
    meta["tokenId"] = tokenId;
    meta["creator"] = creator;
    meta["creationTime"] = creationTime;
    meta["expirationTime"] = expirationTime;
    meta["amount"] = amount;
    meta["state"] = stateToString();
    meta["securityLevel"] = securityLevelToString();
    meta["riskScore"] = riskScore;
    meta["validationAttempts"] = validationAttempts;
    meta["lastValidationTime"] = lastValidationTime;
    meta["isMultiSig"] = isMultiSig;
    meta["hasDeviceFingerprint"] = !deviceFingerprint.empty();
    meta["hasLocationHash"] = !locationHash.empty();
    
    // Add custom metadata
    if (!metadata.empty()) {
        meta["customMetadata"] = metadata;
    }
    
    // Add audit trail summary
    meta["auditTrailCount"] = auditTrail.size();
    
    return meta.dump(4);
}

// Export token to JSON
std::string OderoSLW::toJson() const {
    nlohmann::json j;
    j["tokenId"] = tokenId;
    j["amount"] = amount;
    j["creator"] = creator;
    j["creationTime"] = creationTime;
    j["expirationTime"] = expirationTime;
    j["state"] = static_cast<int>(state);
    j["securityLevel"] = static_cast<int>(securityLevel);
    
    return j.dump(4);
}

// Export with security details
std::string OderoSLW::toSecureJson() const {
    nlohmann::json j;
    j["tokenId"] = tokenId;
    j["amount"] = amount;
    j["creator"] = creator;
    j["creationTime"] = creationTime;
    j["expirationTime"] = expirationTime;
    j["state"] = static_cast<int>(state);
    j["securityLevel"] = static_cast<int>(securityLevel);
    j["digitalSignature"] = digitalSignature;
    j["publicKey"] = publicKey;
    j["merkleRoot"] = merkleRoot;
    j["nonce"] = nonce;
    j["salt"] = salt;
    j["validationAttempts"] = validationAttempts;
    j["lastValidationTime"] = lastValidationTime;
    j["isMultiSig"] = isMultiSig;
    j["requiredSigners"] = requiredSigners;
    j["signatures"] = signatures;
    j["deviceFingerprint"] = deviceFingerprint;
    j["locationHash"] = locationHash;
    j["riskScore"] = riskScore;
    j["metadata"] = metadata;
    
    // Add audit trail
    nlohmann::json auditJson = nlohmann::json::array();
    for (const auto& entry : auditTrail) {
        nlohmann::json entryJson;
        entryJson["transactionId"] = entry.transactionId;
        entryJson["action"] = entry.action;
        entryJson["actor"] = entry.actor;
        entryJson["timestamp"] = entry.timestamp;
        entryJson["details"] = entry.details;
        entryJson["signature"] = entry.signature;
        auditJson.push_back(entryJson);
    }
    j["auditTrail"] = auditJson;
    
    return j.dump(4);
}

// Import token from JSON
OderoSLW OderoSLW::fromJson(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    
    OderoSLW token;
    token.tokenId = j["tokenId"].get<std::string>();
    token.amount = j["amount"].get<double>();
    token.creator = j["creator"].get<std::string>();
    token.creationTime = j["creationTime"].get<std::string>();
    
    // Load enhanced fields if present
    if (j.contains("expirationTime")) {
        token.expirationTime = j["expirationTime"].get<std::string>();
    }
    
    if (j.contains("state")) {
        token.state = static_cast<TokenState>(j["state"].get<int>());
    }
    
    if (j.contains("securityLevel")) {
        token.securityLevel = static_cast<SecurityLevel>(j["securityLevel"].get<int>());
    }
    
    if (j.contains("digitalSignature")) {
        token.digitalSignature = j["digitalSignature"].get<std::string>();
    }
    
    if (j.contains("publicKey")) {
        token.publicKey = j["publicKey"].get<std::string>();
    }
    
    if (j.contains("merkleRoot")) {
        token.merkleRoot = j["merkleRoot"].get<std::string>();
    }
    
    if (j.contains("nonce")) {
        token.nonce = j["nonce"].get<std::string>();
    }
    
    if (j.contains("salt")) {
        token.salt = j["salt"].get<std::string>();
    }
    
    if (j.contains("validationAttempts")) {
        token.validationAttempts = j["validationAttempts"].get<int>();
    }
    
    if (j.contains("lastValidationTime")) {
        token.lastValidationTime = j["lastValidationTime"].get<std::string>();
    }
    
    if (j.contains("isMultiSig")) {
        token.isMultiSig = j["isMultiSig"].get<bool>();
    }
    
    if (j.contains("requiredSigners")) {
        token.requiredSigners = j["requiredSigners"].get<std::vector<std::string>>();
    }
    
    if (j.contains("signatures")) {
        token.signatures = j["signatures"].get<std::vector<std::string>>();
    }
    
    if (j.contains("deviceFingerprint")) {
        token.deviceFingerprint = j["deviceFingerprint"].get<std::string>();
    }
    
    if (j.contains("locationHash")) {
        token.locationHash = j["locationHash"].get<std::string>();
    }
    
    if (j.contains("riskScore")) {
        token.riskScore = j["riskScore"].get<double>();
    }
    
    if (j.contains("metadata")) {
        token.metadata = j["metadata"].get<std::map<std::string, std::string>>();
    }
    
    // Load audit trail
    if (j.contains("auditTrail")) {
        for (const auto& entryJson : j["auditTrail"]) {
            TokenTransaction entry;
            entry.transactionId = entryJson["transactionId"].get<std::string>();
            entry.action = entryJson["action"].get<std::string>();
            entry.actor = entryJson["actor"].get<std::string>();
            entry.timestamp = entryJson["timestamp"].get<std::string>();
            entry.details = entryJson["details"].get<std::string>();
            entry.signature = entryJson["signature"].get<std::string>();
            token.auditTrail.push_back(entry);
        }
    }
    
    return token;
}

// Sign the token with private key
bool OderoSLW::signToken(const std::string& privateKey) {
    if (privateKey.empty()) {
        Logger::error("Cannot sign token: Empty private key");
        return false;
    }
    
    std::string hash = calculateSecureHash();
    digitalSignature = Utils::signData(hash, privateKey);
    
    if (!digitalSignature.empty()) {
        addAuditEntry("SIGN", creator, "Token signed with private key");
        Logger::info("Token signed successfully: " + tokenId);
        return true;
    }
    
    Logger::error("Token signing failed: " + tokenId);
    return false;
}

// Add signature for multi-sig tokens
bool OderoSLW::addSignature(const std::string& signerAddress, const std::string& signature) {
    if (!isMultiSig) {
        Logger::error("Token is not configured for multi-signature");
        return false;
    }
    
    // Check if signer is required
    auto it = std::find(requiredSigners.begin(), requiredSigners.end(), signerAddress);
    if (it == requiredSigners.end()) {
        Logger::error("Signer not in required signers list: " + signerAddress);
        return false;
    }
    
    // Add signature
    signatures.push_back(signature);
    
    addAuditEntry("ADD_SIGNATURE", signerAddress, "Multi-signature added");
    Logger::info("Signature added for token: " + tokenId + " by " + signerAddress);
    
    return true;
}

// Check if multi-sig requirements are met
bool OderoSLW::isMultiSigComplete() const {
    if (!isMultiSig) {
        return true; // Not a multi-sig token
    }
    
    return signatures.size() >= requiredSigners.size();
}

// Activate the token
bool OderoSLW::activate(const std::string& activatorKey) {
    if (state != TokenState::CREATED) {
        Logger::error("Token cannot be activated from current state: " + stateToString());
        return false;
    }
    
    // Verify activator has permission (simplified check)
    if (activatorKey.empty()) {
        Logger::error("Invalid activator key");
        return false;
    }
    
    state = TokenState::ACTIVE;
    addAuditEntry("ACTIVATE", "SYSTEM", "Token activated");
    Logger::info("Token activated: " + tokenId);
    
    return true;
}

// Redeem the token
bool OderoSLW::redeem(const std::string& redeemer, const std::string& signature) {
    if (state != TokenState::ACTIVE) {
        Logger::error("Token cannot be redeemed from current state: " + stateToString());
        return false;
    }
    
    if (redeemer.empty() || signature.empty()) {
        Logger::error("Invalid redeemer or signature");
        return false;
    }
    
    // Verify signature (simplified)
    // In real implementation, verify that redeemer has permission to redeem
    
    state = TokenState::REDEEMED;
    addAuditEntry("REDEEM", redeemer, "Token redeemed");
    Logger::info("Token redeemed: " + tokenId + " by " + redeemer);
    
    return true;
}

// Transfer token ownership
bool OderoSLW::transfer(const std::string& newOwner, const std::string& signature) {
    if (state != TokenState::ACTIVE) {
        Logger::error("Token cannot be transferred from current state: " + stateToString());
        return false;
    }
    
    if (newOwner.empty() || signature.empty()) {
        Logger::error("Invalid new owner or signature");
        return false;
    }
    
    // Update creator to new owner
    std::string oldOwner = creator;
    creator = newOwner;
    
    addAuditEntry("TRANSFER", newOwner, "Token transferred from " + oldOwner);
    Logger::info("Token transferred: " + tokenId + " from " + oldOwner + " to " + newOwner);
    
    return true;
}

// Freeze token
bool OderoSLW::freeze(const std::string& authority, const std::string& reason) {
    if (state == TokenState::REDEEMED || state == TokenState::REVOKED) {
        Logger::error("Cannot freeze token in current state: " + stateToString());
        return false;
    }
    
    state = TokenState::FROZEN;
    addAuditEntry("FREEZE", authority, "Token frozen: " + reason);
    Logger::info("Token frozen: " + tokenId + " by " + authority);
    
    return true;
}

// Unfreeze token
bool OderoSLW::unfreeze(const std::string& authority, const std::string& reason) {
    if (state != TokenState::FROZEN) {
        Logger::error("Token is not frozen");
        return false;
    }
    
    state = TokenState::ACTIVE;
    addAuditEntry("UNFREEZE", authority, "Token unfrozen: " + reason);
    Logger::info("Token unfrozen: " + tokenId + " by " + authority);
    
    return true;
}

// Revoke token
bool OderoSLW::revoke(const std::string& authority, const std::string& reason) {
    if (state == TokenState::REDEEMED) {
        Logger::error("Cannot revoke redeemed token");
        return false;
    }
    
    state = TokenState::REVOKED;
    addAuditEntry("REVOKE", authority, "Token revoked: " + reason);
    Logger::info("Token revoked: " + tokenId + " by " + authority);
    
    return true;
}

// Set expiration time
void OderoSLW::setExpirationTime(const std::string& expiration) {
    expirationTime = expiration;
    addAuditEntry("SET_EXPIRATION", "SYSTEM", "Expiration time set: " + expiration);
}

// Set expiration duration
void OderoSLW::setExpirationDuration(int hours) {
    auto now = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::hours(hours);
    auto time_t_expiry = std::chrono::system_clock::to_time_t(expiry);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_expiry), "%Y-%m-%dT%H:%M:%SZ");
    expirationTime = ss.str();
    
    addAuditEntry("SET_EXPIRATION", "SYSTEM", "Expiration set to " + std::to_string(hours) + " hours");
}

// Add metadata
void OderoSLW::addMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
    addAuditEntry("ADD_METADATA", "SYSTEM", "Metadata added: " + key + "=" + value);
}

// Get metadata value
std::string OderoSLW::getMetadata(const std::string& key) const {
    auto it = metadata.find(key);
    return (it != metadata.end()) ? it->second : "";
}

// Set device fingerprint
void OderoSLW::setDeviceFingerprint(const std::string& fingerprint) {
    deviceFingerprint = fingerprint;
    addAuditEntry("SET_DEVICE", "SYSTEM", "Device fingerprint set");
}

// Set location hash
void OderoSLW::setLocationHash(const std::string& location) {
    locationHash = location;
    addAuditEntry("SET_LOCATION", "SYSTEM", "Location hash set");
}

// Get risk score
double OderoSLW::getRiskScore() const {
    return riskScore;
}

// Add audit entry
void OderoSLW::addAuditEntry(const std::string& action, const std::string& actor, const std::string& details) {
    TokenTransaction entry;
    entry.transactionId = Utils::calculateSHA256(action + actor + details + std::to_string(time(nullptr))).substr(0, 16);
    entry.action = action;
    entry.actor = actor;
    entry.details = details;
    
    // Set timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    entry.timestamp = ss.str();
    
    // Sign the audit entry (simplified)
    entry.signature = Utils::calculateSHA256(entry.transactionId + entry.action + entry.actor);
    
    auditTrail.push_back(entry);
}

// Get audit trail
std::vector<TokenTransaction> OderoSLW::getAuditTrail() const {
    return auditTrail;
}

// Batch operations
std::vector<OderoSLW> OderoSLW::createTokenBatch(const std::vector<std::pair<std::string, double>>& tokenData, 
                                                 const std::string& creator, SecurityLevel level) {
    std::vector<OderoSLW> tokens;
    
    for (const auto& data : tokenData) {
        std::string tokenId = "OSLW" + Utils::calculateSHA256(creator + std::to_string(data.second) + 
                                                              std::to_string(time(nullptr))).substr(0, 16);
        
        OderoSLW token(tokenId, data.second, creator, level);
        tokens.push_back(token);
    }
    
    // Generate batch merkle root
    std::string merkleRoot = generateBatchMerkleRoot(tokens);
    
    // Set merkle root for all tokens in batch
    for (auto& token : tokens) {
        token.merkleRoot = merkleRoot;
    }
    
    Logger::info("Token batch created: " + std::to_string(tokens.size()) + " tokens with merkle root: " + merkleRoot);
    
    return tokens;
}

std::string OderoSLW::generateBatchMerkleRoot(const std::vector<OderoSLW>& tokens) {
    if (tokens.empty()) {
        return "";
    }
    
    std::vector<std::string> hashes;
    for (const auto& token : tokens) {
        hashes.push_back(token.calculateSecureHash());
    }
    
    // Simple merkle tree implementation
    while (hashes.size() > 1) {
        std::vector<std::string> nextLevel;
        
        for (size_t i = 0; i < hashes.size(); i += 2) {
            std::string combined;
            if (i + 1 < hashes.size()) {
                combined = hashes[i] + hashes[i + 1];
            } else {
                combined = hashes[i] + hashes[i]; // Duplicate if odd number
            }
            nextLevel.push_back(Utils::calculateSHA256(combined));
        }
        
        hashes = nextLevel;
    }
    
    return hashes[0];
}

// Validation utilities
bool OderoSLW::validateTokenId(const std::string& tokenId) {
    if (tokenId.empty() || tokenId.length() < 4) {
        return false;
    }
    
    // Check if starts with OSLW
    if (tokenId.substr(0, 4) != "OSLW") {
        return false;
    }
    
    // Check for valid characters (alphanumeric)
    for (char c : tokenId) {
        if (!std::isalnum(c)) {
            return false;
        }
    }
    
    return true;
}

bool OderoSLW::validateAmount(double amount) {
    // Amount must be positive and reasonable
    return amount > 0.0 && amount <= 1000000.0; // Max 1M tokens
}

bool OderoSLW::validateAddress(const std::string& address) {
    if (address.empty() || address.length() < 3) {
        return false;
    }
    
    // Basic address validation - in real implementation, use proper address format
    return true;
}

// Convert state to string
std::string OderoSLW::stateToString() const {
    switch (state) {
        case TokenState::CREATED: return "CREATED";
        case TokenState::ACTIVE: return "ACTIVE";
        case TokenState::REDEEMED: return "REDEEMED";
        case TokenState::EXPIRED: return "EXPIRED";
        case TokenState::REVOKED: return "REVOKED";
        case TokenState::FROZEN: return "FROZEN";
        default: return "UNKNOWN";
    }
}

TokenState OderoSLW::stringToState(const std::string& stateStr) {
    if (stateStr == "CREATED") return TokenState::CREATED;
    if (stateStr == "ACTIVE") return TokenState::ACTIVE;
    if (stateStr == "REDEEMED") return TokenState::REDEEMED;
    if (stateStr == "EXPIRED") return TokenState::EXPIRED;
    if (stateStr == "REVOKED") return TokenState::REVOKED;
    if (stateStr == "FROZEN") return TokenState::FROZEN;
    return TokenState::CREATED; // Default
}

// Convert security level to string
std::string OderoSLW::securityLevelToString() const {
    switch (securityLevel) {
        case SecurityLevel::BASIC: return "BASIC";
        case SecurityLevel::ENHANCED: return "ENHANCED";
        case SecurityLevel::ENTERPRISE: return "ENTERPRISE";
        default: return "BASIC";
    }
}

SecurityLevel OderoSLW::stringToSecurityLevel(const std::string& levelStr) {
    if (levelStr == "BASIC") return SecurityLevel::BASIC;
    if (levelStr == "ENHANCED") return SecurityLevel::ENHANCED;
    if (levelStr == "ENTERPRISE") return SecurityLevel::ENTERPRISE;
    return SecurityLevel::BASIC; // Default
}