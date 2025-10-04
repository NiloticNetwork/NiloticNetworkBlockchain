#include "consensus_security_validator.h"
#include "logger.h"
#include "block.h"
#include "transaction.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <set>

ConsensusSecurityValidator::ConsensusSecurityValidator()
    : enableCryptographicValidation(true)
    , enableAttackDetection(true)
    , enableAuditLogging(true)
    , maxSuspiciousActivityThreshold(100)
    , attackDetectionWindow(3600) // 1 hour
    , totalValidations(0)
    , securityViolations(0)
    , attacksDetected(0)
    , cryptoValidationFailures(0)
{
    initializeAttackPatterns();
}

bool ConsensusSecurityValidator::initialize() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    Logger::info("ConsensusSecurityValidator: Initializing security validator");
    
    // Initialize attack patterns
    initializeAttackPatterns();
    
    // Reset statistics
    totalValidations = 0;
    securityViolations = 0;
    attacksDetected = 0;
    cryptoValidationFailures = 0;
    
    // Clear activity counters
    suspiciousActivityCounters.clear();
    lastActivityTime.clear();
    
    Logger::info("ConsensusSecurityValidator: Security validator initialized successfully");
    return true;
}

void ConsensusSecurityValidator::shutdown() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    Logger::info("ConsensusSecurityValidator: Shutting down security validator");
    
    // Clear all data structures
    attackPatterns.clear();
    suspiciousActivityCounters.clear();
    lastActivityTime.clear();
    
    Logger::info("ConsensusSecurityValidator: Security validator shutdown complete");
}

SecurityValidationResult ConsensusSecurityValidator::validateConsensusRequest(const ConsensusRequest& request) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    totalValidations++;
    
    SecurityValidationResult result;
    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    try {
        // Basic validation
        if (request.data.empty()) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::MEDIUM;
            result.threats.push_back("Empty request data");
            securityViolations++;
            return result;
        }
        
        // Validate consensus integrity
        if (!validateConsensusIntegrity(request)) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::HIGH;
            result.threats.push_back("Consensus integrity validation failed");
            securityViolations++;
            return result;
        }
        
        // Validate request authenticity
        if (!validateRequestAuthenticity(request)) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::HIGH;
            result.threats.push_back("Request authenticity validation failed");
            securityViolations++;
            return result;
        }
        
        // Validate timestamp security
        if (!validateTimestampSecurity(request.timestamp)) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::MEDIUM;
            result.threats.push_back("Timestamp validation failed");
            result.warnings.push_back("Request timestamp appears suspicious");
        }
        
        // All validations passed
        result.isSecure = true;
        result.threatLevel = ThreatLevel::LOW;
        result.metadata["validation_time"] = std::to_string(result.timestamp);
        result.metadata["request_id"] = request.requestId;
        
        if (enableAuditLogging) {
            logSecurityEvent("CONSENSUS_REQUEST_VALIDATED", ThreatLevel::LOW, 
                           request.requestId, nlohmann::json{{"result", "SECURE"}});
        }
        
    } catch (const std::exception& e) {
        result.isSecure = false;
        result.threatLevel = ThreatLevel::CRITICAL;
        result.threats.push_back("Exception during validation: " + std::string(e.what()));
        securityViolations++;
        
        Logger::error("ConsensusSecurityValidator: Exception during consensus request validation: " + std::string(e.what()));
    }
    
    return result;
}

SecurityValidationResult ConsensusSecurityValidator::validateBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    totalValidations++;
    
    SecurityValidationResult result;
    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    try {
        // Validate block hash integrity
        std::string calculatedHash = block.calculateHash();
        if (calculatedHash != block.getHash()) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::CRITICAL;
            result.threats.push_back("Block hash mismatch - potential tampering detected");
            securityViolations++;
            return result;
        }
        
        // Validate merkle root
        std::vector<std::string> txHashes;
        for (const auto& tx : block.getTransactions()) {
            txHashes.push_back(tx.getHash());
        }
        
        if (!validateMerkleRoot(txHashes, block.getMerkleRoot())) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::HIGH;
            result.threats.push_back("Merkle root validation failed");
            securityViolations++;
            return result;
        }
        
        // Validate timestamp
        if (!validateTimestampSecurity(static_cast<uint64_t>(block.getTimestamp()))) {
            result.threatLevel = ThreatLevel::MEDIUM;
            result.warnings.push_back("Block timestamp appears suspicious");
        }
        
        // Validate all transactions in the block
        for (const auto& tx : block.getTransactions()) {
            SecurityValidationResult txResult = validateTransaction(tx);
            if (!txResult.isSecure) {
                result.isSecure = false;
                result.threatLevel = std::max(result.threatLevel, txResult.threatLevel);
                result.threats.insert(result.threats.end(), txResult.threats.begin(), txResult.threats.end());
                securityViolations++;
            }
        }
        
        // If we reach here and no issues found, block is secure
        if (result.isSecure) {
            result.threatLevel = ThreatLevel::LOW;
            result.metadata["block_index"] = std::to_string(block.getIndex());
            result.metadata["transaction_count"] = std::to_string(block.getTransactions().size());
        }
        
        if (enableAuditLogging) {
            logSecurityEvent("BLOCK_VALIDATED", result.threatLevel, 
                           std::to_string(block.getIndex()), 
                           nlohmann::json{{"secure", result.isSecure}});
        }
        
    } catch (const std::exception& e) {
        result.isSecure = false;
        result.threatLevel = ThreatLevel::CRITICAL;
        result.threats.push_back("Exception during block validation: " + std::string(e.what()));
        securityViolations++;
        
        Logger::error("ConsensusSecurityValidator: Exception during block validation: " + std::string(e.what()));
    }
    
    return result;
}

SecurityValidationResult ConsensusSecurityValidator::validateTransaction(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    totalValidations++;
    
    SecurityValidationResult result;
    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    try {
        // Basic transaction validation
        if (!transaction.isValid()) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::HIGH;
            result.threats.push_back("Transaction failed basic validation");
            securityViolations++;
            return result;
        }
        
        // Validate transaction hash
        std::string calculatedHash = transaction.calculateHash();
        if (calculatedHash != transaction.getHash()) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::CRITICAL;
            result.threats.push_back("Transaction hash mismatch - potential tampering");
            securityViolations++;
            return result;
        }
        
        // Check for double spending patterns
        if (detectDoubleSpendingAttack(transaction)) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::CRITICAL;
            result.threats.push_back("Potential double spending attack detected");
            attacksDetected++;
            return result;
        }
        
        // Validate timestamp
        if (!validateTimestampSecurity(static_cast<uint64_t>(transaction.getTimestamp()))) {
            result.threatLevel = ThreatLevel::MEDIUM;
            result.warnings.push_back("Transaction timestamp appears suspicious");
        }
        
        // Validate amount (check for overflow/underflow)
        if (transaction.getAmount() < 0 || transaction.getAmount() > 1e18) {
            result.isSecure = false;
            result.threatLevel = ThreatLevel::HIGH;
            result.threats.push_back("Invalid transaction amount");
            securityViolations++;
            return result;
        }
        
        // All validations passed
        result.isSecure = true;
        result.threatLevel = ThreatLevel::LOW;
        result.metadata["tx_hash"] = transaction.getHash();
        result.metadata["sender"] = transaction.getSender();
        result.metadata["amount"] = std::to_string(transaction.getAmount());
        
        if (enableAuditLogging) {
            logSecurityEvent("TRANSACTION_VALIDATED", ThreatLevel::LOW, 
                           transaction.getSender(), 
                           nlohmann::json{{"tx_hash", transaction.getHash()}});
        }
        
    } catch (const std::exception& e) {
        result.isSecure = false;
        result.threatLevel = ThreatLevel::CRITICAL;
        result.threats.push_back("Exception during transaction validation: " + std::string(e.what()));
        securityViolations++;
        
        Logger::error("ConsensusSecurityValidator: Exception during transaction validation: " + std::string(e.what()));
    }
    
    return result;
}

bool ConsensusSecurityValidator::validateCryptographicSignature(const CryptoValidationContext& context) {
    if (!enableCryptographicValidation) {
        return true; // Skip validation if disabled
    }
    
    try {
        // Basic validation
        if (context.signature.empty() || context.publicKey.empty() || context.data.empty()) {
            cryptoValidationFailures++;
            return false;
        }
        
        // Check algorithm strength
        if (!isStrongCryptographicAlgorithm(context.algorithm)) {
            cryptoValidationFailures++;
            Logger::warning("ConsensusSecurityValidator: Weak cryptographic algorithm detected: " + context.algorithm);
            return false;
        }
        
        // Validate signature strength
        if (!validateSignatureStrength(context.signature)) {
            cryptoValidationFailures++;
            return false;
        }
        
        // For stub implementation, we'll accept valid-looking signatures
        // In production, this would use actual cryptographic verification
        bool isValid = context.signature.length() > 64 && context.publicKey.length() > 100;
        
        if (!isValid) {
            cryptoValidationFailures++;
        }
        
        return isValid;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during cryptographic validation: " + std::string(e.what()));
        cryptoValidationFailures++;
        return false;
    }
}

bool ConsensusSecurityValidator::validateHashChain(const std::vector<std::string>& hashes) {
    if (hashes.empty()) {
        return true; // Empty chain is valid
    }
    
    try {
        // Check each hash format and length
        for (const auto& hash : hashes) {
            if (hash.length() != 64) { // SHA256 length
                return false;
            }
            
            // Check if hash contains only valid hex characters
            for (char c : hash) {
                if (!std::isxdigit(c)) {
                    return false;
                }
            }
        }
        
        // For stub implementation, basic format validation is sufficient
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during hash chain validation: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::validateMerkleRoot(const std::vector<std::string>& transactions, const std::string& merkleRoot) {
    try {
        if (transactions.empty()) {
            return merkleRoot == "0" || merkleRoot.empty();
        }
        
        // Calculate expected merkle root
        std::string calculatedRoot = calculateMerkleRoot(transactions);
        return calculatedRoot == merkleRoot;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during merkle root validation: " + std::string(e.what()));
        return false;
    }
}bool
 ConsensusSecurityValidator::detectConsensusAttack(const ConsensusRequest& request, const std::string& source) {
    if (!enableAttackDetection) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    try {
        // Check for suspicious activity patterns
        if (checkSuspiciousActivity(source)) {
            attacksDetected++;
            recordSecurityEvent("SUSPICIOUS_CONSENSUS_ACTIVITY", ThreatLevel::HIGH, source, 
                              nlohmann::json{{"request_id", request.requestId}});
            return true;
        }
        
        // Check for rapid-fire requests (potential DoS)
        auto now = std::chrono::steady_clock::now();
        auto it = lastActivityTime.find(source);
        if (it != lastActivityTime.end()) {
            auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
            if (timeDiff < 1) { // Less than 1 second between requests
                updateSuspiciousActivityCounter(source);
                if (suspiciousActivityCounters[source] > 10) { // More than 10 rapid requests
                    attacksDetected++;
                    recordSecurityEvent("CONSENSUS_DOS_ATTACK", ThreatLevel::CRITICAL, source);
                    return true;
                }
            }
        }
        lastActivityTime[source] = now;
        
        return false;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during consensus attack detection: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::detect51PercentAttack(const std::vector<ConsensusResult>& results) {
    if (!enableAttackDetection || results.empty()) {
        return false;
    }
    
    try {
        // Count results by mechanism
        std::map<ConsensusType, int> mechanismCounts;
        for (const auto& result : results) {
            mechanismCounts[result.mechanism]++;
        }
        
        // Check if any single mechanism dominates more than 51%
        for (const auto& pair : mechanismCounts) {
            double percentage = static_cast<double>(pair.second) / results.size();
            if (percentage > 0.51) {
                attacksDetected++;
                recordSecurityEvent("POTENTIAL_51_PERCENT_ATTACK", ThreatLevel::CRITICAL, "SYSTEM", 
                                  nlohmann::json{{"dominant_mechanism", static_cast<int>(pair.first)}, 
                                               {"percentage", percentage}});
                return true;
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during 51% attack detection: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::detectDoubleSpendingAttack(const Transaction& transaction) {
    if (!enableAttackDetection) {
        return false;
    }
    
    try {
        // For stub implementation, we'll do basic checks
        // In production, this would check against UTXO set and pending transactions
        
        // Check for suspicious patterns in transaction
        if (transaction.getAmount() <= 0) {
            return true; // Invalid amount
        }
        
        // Check for duplicate hash (simplified check)
        std::string txHash = transaction.getHash();
        static std::set<std::string> seenHashes;
        
        if (seenHashes.find(txHash) != seenHashes.end()) {
            attacksDetected++;
            recordSecurityEvent("DOUBLE_SPENDING_ATTEMPT", ThreatLevel::CRITICAL, 
                              transaction.getSender(), 
                              nlohmann::json{{"tx_hash", txHash}});
            return true;
        }
        
        seenHashes.insert(txHash);
        
        // Limit the size of seen hashes to prevent memory issues
        if (seenHashes.size() > 10000) {
            seenHashes.clear();
        }
        
        return false;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during double spending detection: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::detectTimestampManipulation(uint64_t timestamp) {
    if (!enableAttackDetection) {
        return false;
    }
    
    try {
        auto currentTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Check if timestamp is too far in the future (more than 2 hours)
        if (timestamp > currentTime + 7200) {
            attacksDetected++;
            recordSecurityEvent("TIMESTAMP_MANIPULATION", ThreatLevel::HIGH, "SYSTEM", 
                              nlohmann::json{{"timestamp", timestamp}, {"current_time", currentTime}});
            return true;
        }
        
        // Check if timestamp is too far in the past (more than 24 hours)
        if (timestamp < currentTime - 86400) {
            attacksDetected++;
            recordSecurityEvent("TIMESTAMP_MANIPULATION", ThreatLevel::MEDIUM, "SYSTEM", 
                              nlohmann::json{{"timestamp", timestamp}, {"current_time", currentTime}});
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during timestamp manipulation detection: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::detectSybilAttack(const std::string& source, const std::vector<std::string>& identities) {
    if (!enableAttackDetection || identities.empty()) {
        return false;
    }
    
    try {
        // Check for too many identities from single source
        if (identities.size() > 100) { // Threshold for suspicious identity count
            attacksDetected++;
            recordSecurityEvent("SYBIL_ATTACK", ThreatLevel::HIGH, source, 
                              nlohmann::json{{"identity_count", identities.size()}});
            return true;
        }
        
        // Check for similar identity patterns
        std::set<std::string> uniqueIdentities(identities.begin(), identities.end());
        if (uniqueIdentities.size() < identities.size() * 0.8) { // Less than 80% unique
            attacksDetected++;
            recordSecurityEvent("SYBIL_ATTACK", ThreatLevel::MEDIUM, source, 
                              nlohmann::json{{"duplicate_identities", identities.size() - uniqueIdentities.size()}});
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during Sybil attack detection: " + std::string(e.what()));
        return false;
    }
}

void ConsensusSecurityValidator::recordSecurityEvent(const std::string& event, ThreatLevel level, 
                                                   const std::string& source, const nlohmann::json& details) {
    if (!enableAuditLogging) {
        return;
    }
    
    try {
        logSecurityEvent(event, level, source, details);
        
        // Update statistics based on threat level
        if (level >= ThreatLevel::MEDIUM) {
            securityViolations++;
        }
        
        if (level >= ThreatLevel::HIGH) {
            attacksDetected++;
        }
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during security event recording: " + std::string(e.what()));
    }
}

void ConsensusSecurityValidator::updateAttackPattern(const std::string& patternName, bool detected) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    try {
        for (auto& pattern : attackPatterns) {
            if (pattern.name == patternName) {
                if (detected) {
                    pattern.detectionCount++;
                    pattern.lastDetection = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                }
                break;
            }
        }
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during attack pattern update: " + std::string(e.what()));
    }
}

std::vector<AttackPattern> ConsensusSecurityValidator::getActiveThreats() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    std::vector<AttackPattern> activeThreats;
    
    try {
        auto currentTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        for (const auto& pattern : attackPatterns) {
            // Consider a threat active if detected within the last hour
            if (pattern.detectionCount > 0 && (currentTime - pattern.lastDetection) < 3600) {
                activeThreats.push_back(pattern);
            }
        }
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during active threats retrieval: " + std::string(e.what()));
    }
    
    return activeThreats;
}

nlohmann::json ConsensusSecurityValidator::getSecurityMetrics() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    nlohmann::json metrics;
    
    try {
        metrics["total_validations"] = totalValidations.load();
        metrics["security_violations"] = securityViolations.load();
        metrics["attacks_detected"] = attacksDetected.load();
        metrics["crypto_validation_failures"] = cryptoValidationFailures.load();
        
        // Calculate success rate
        uint64_t total = totalValidations.load();
        if (total > 0) {
            metrics["success_rate"] = 1.0 - (static_cast<double>(securityViolations.load()) / total);
        } else {
            metrics["success_rate"] = 1.0;
        }
        
        // Attack pattern statistics
        nlohmann::json patterns = nlohmann::json::array();
        for (const auto& pattern : attackPatterns) {
            nlohmann::json patternJson;
            patternJson["name"] = pattern.name;
            patternJson["detection_count"] = pattern.detectionCount;
            patternJson["last_detection"] = pattern.lastDetection;
            patternJson["severity"] = static_cast<int>(pattern.severity);
            patterns.push_back(patternJson);
        }
        metrics["attack_patterns"] = patterns;
        
        // Configuration status
        metrics["cryptographic_validation_enabled"] = enableCryptographicValidation;
        metrics["attack_detection_enabled"] = enableAttackDetection;
        metrics["audit_logging_enabled"] = enableAuditLogging;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during metrics generation: " + std::string(e.what()));
        metrics["error"] = "Failed to generate metrics";
    }
    
    return metrics;
}

nlohmann::json ConsensusSecurityValidator::getSecurityStatus() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    nlohmann::json status;
    
    try {
        // Overall security status
        uint64_t violations = securityViolations.load();
        uint64_t attacks = attacksDetected.load();
        
        if (attacks > 0) {
            status["status"] = "CRITICAL";
            status["level"] = static_cast<int>(ThreatLevel::CRITICAL);
        } else if (violations > 10) {
            status["status"] = "HIGH_RISK";
            status["level"] = static_cast<int>(ThreatLevel::HIGH);
        } else if (violations > 0) {
            status["status"] = "MEDIUM_RISK";
            status["level"] = static_cast<int>(ThreatLevel::MEDIUM);
        } else {
            status["status"] = "SECURE";
            status["level"] = static_cast<int>(ThreatLevel::LOW);
        }
        
        // Active threats
        std::vector<AttackPattern> activeThreats = getActiveThreats();
        status["active_threats"] = activeThreats.size();
        
        // System health
        status["validator_healthy"] = true;
        status["last_update"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Configuration
        status["configuration"] = nlohmann::json{
            {"cryptographic_validation", enableCryptographicValidation},
            {"attack_detection", enableAttackDetection},
            {"audit_logging", enableAuditLogging},
            {"max_suspicious_threshold", maxSuspiciousActivityThreshold},
            {"attack_detection_window", attackDetectionWindow}
        };
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during status generation: " + std::string(e.what()));
        status["status"] = "ERROR";
        status["error"] = "Failed to generate status";
    }
    
    return status;
}

void ConsensusSecurityValidator::resetStatistics() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    totalValidations = 0;
    securityViolations = 0;
    attacksDetected = 0;
    cryptoValidationFailures = 0;
    
    // Reset attack patterns
    for (auto& pattern : attackPatterns) {
        pattern.detectionCount = 0;
        pattern.lastDetection = 0;
    }
    
    // Clear activity counters
    suspiciousActivityCounters.clear();
    lastActivityTime.clear();
    
    Logger::info("ConsensusSecurityValidator: Statistics reset");
}

// Private helper methods

bool ConsensusSecurityValidator::validateConsensusIntegrity(const ConsensusRequest& request) {
    try {
        // Basic integrity checks
        if (request.requestId.empty()) {
            return false;
        }
        
        if (request.requiredMechanisms.empty()) {
            return false; // At least one mechanism should be required
        }
        
        // Validate request type
        if (request.type < RequestType::BLOCK_VALIDATION || 
            request.type > RequestType::SMART_CONTRACT_EXECUTION) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during consensus integrity validation: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::validateRequestAuthenticity(const ConsensusRequest& request) {
    try {
        // For stub implementation, basic checks
        // In production, this would verify digital signatures
        
        // Check if request has reasonable timestamp
        auto currentTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (request.timestamp > currentTime + 300) { // 5 minutes in future
            return false;
        }
        
        if (request.timestamp < currentTime - 3600) { // 1 hour in past
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during request authenticity validation: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusSecurityValidator::validateTimestampSecurity(uint64_t timestamp) {
    return !detectTimestampManipulation(timestamp);
}

bool ConsensusSecurityValidator::validateDataIntegrity(const std::string& data, const std::string& expectedHash) {
    try {
        if (data.empty() || expectedHash.empty()) {
            return false;
        }
        
        // For stub implementation, basic length and format checks
        if (expectedHash.length() != 64) { // SHA256 length
            return false;
        }
        
        // Check if hash contains only valid hex characters
        for (char c : expectedHash) {
            if (!std::isxdigit(c)) {
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during data integrity validation: " + std::string(e.what()));
        return false;
    }
}

void ConsensusSecurityValidator::initializeAttackPatterns() {
    attackPatterns.clear();
    
    // Define common attack patterns
    AttackPattern pattern1;
    pattern1.name = "51_PERCENT_ATTACK";
    pattern1.description = "Single consensus mechanism dominates more than 51% of decisions";
    pattern1.severity = ThreatLevel::CRITICAL;
    pattern1.indicators = {"high_mechanism_dominance", "consensus_centralization"};
    attackPatterns.push_back(pattern1);
    
    AttackPattern pattern2;
    pattern2.name = "DOUBLE_SPENDING";
    pattern2.description = "Attempt to spend the same funds multiple times";
    pattern2.severity = ThreatLevel::CRITICAL;
    pattern2.indicators = {"duplicate_transactions", "conflicting_spends"};
    attackPatterns.push_back(pattern2);
    
    AttackPattern pattern3;
    pattern3.name = "TIMESTAMP_MANIPULATION";
    pattern3.description = "Manipulation of block or transaction timestamps";
    pattern3.severity = ThreatLevel::HIGH;
    pattern3.indicators = {"future_timestamps", "inconsistent_timing"};
    attackPatterns.push_back(pattern3);
    
    AttackPattern pattern4;
    pattern4.name = "SYBIL_ATTACK";
    pattern4.description = "Creation of multiple fake identities to gain influence";
    pattern4.severity = ThreatLevel::HIGH;
    pattern4.indicators = {"multiple_identities", "coordinated_behavior"};
    attackPatterns.push_back(pattern4);
    
    AttackPattern pattern5;
    pattern5.name = "DOS_ATTACK";
    pattern5.description = "Denial of service through resource exhaustion";
    pattern5.severity = ThreatLevel::MEDIUM;
    pattern5.indicators = {"rapid_requests", "resource_exhaustion"};
    attackPatterns.push_back(pattern5);
}

bool ConsensusSecurityValidator::checkSuspiciousActivity(const std::string& source) {
    auto it = suspiciousActivityCounters.find(source);
    if (it != suspiciousActivityCounters.end()) {
        return it->second > maxSuspiciousActivityThreshold;
    }
    return false;
}

void ConsensusSecurityValidator::updateSuspiciousActivityCounter(const std::string& source) {
    suspiciousActivityCounters[source]++;
}

bool ConsensusSecurityValidator::isWithinDetectionWindow(uint64_t timestamp) const {
    auto currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return (currentTime - timestamp) <= attackDetectionWindow;
}

bool ConsensusSecurityValidator::isStrongCryptographicAlgorithm(const std::string& algorithm) const {
    // List of acceptable strong algorithms
    std::vector<std::string> strongAlgorithms = {
        "ECDSA", "RSA-2048", "RSA-4096", "Ed25519", "secp256k1"
    };
    
    return std::find(strongAlgorithms.begin(), strongAlgorithms.end(), algorithm) != strongAlgorithms.end();
}

bool ConsensusSecurityValidator::validateSignatureStrength(const std::string& signature) const {
    // Basic signature strength validation
    if (signature.length() < 64) { // Minimum length for strong signature
        return false;
    }
    
    // Check for obvious weak patterns
    if (signature.find("00000000") != std::string::npos) {
        return false; // Too many zeros
    }
    
    return true;
}

std::string ConsensusSecurityValidator::calculateMerkleRoot(const std::vector<std::string>& transactions) const {
    if (transactions.empty()) {
        return "0";
    }
    
    std::vector<std::string> hashes = transactions;
    
    while (hashes.size() > 1) {
        if (hashes.size() % 2 != 0) {
            hashes.push_back(hashes.back());
        }
        
        std::vector<std::string> newHashes;
        for (size_t i = 0; i < hashes.size(); i += 2) {
            // For stub implementation, simple concatenation and hash
            std::string combined = hashes[i] + hashes[i + 1];
            // In production, this would use proper SHA256 hashing
            std::hash<std::string> hasher;
            std::stringstream ss;
            ss << std::hex << hasher(combined);
            newHashes.push_back(ss.str());
        }
        
        hashes = newHashes;
    }
    
    return hashes[0];
}

void ConsensusSecurityValidator::logSecurityEvent(const std::string& event, ThreatLevel level, 
                                                const std::string& source, const nlohmann::json& details) const {
    try {
        std::stringstream logMessage;
        logMessage << "SECURITY_EVENT: " << event 
                  << " | Level: " << threatLevelToString(level)
                  << " | Source: " << source;
        
        if (!details.empty()) {
            logMessage << " | Details: " << details.dump();
        }
        
        // Log based on threat level
        switch (level) {
            case ThreatLevel::LOW:
                Logger::info(logMessage.str());
                break;
            case ThreatLevel::MEDIUM:
                Logger::warning(logMessage.str());
                break;
            case ThreatLevel::HIGH:
                Logger::error(logMessage.str());
                break;
            case ThreatLevel::CRITICAL:
                Logger::critical(logMessage.str());
                break;
        }
        
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityValidator: Exception during security event logging: " + std::string(e.what()));
    }
}

std::string ConsensusSecurityValidator::threatLevelToString(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::LOW:      return "LOW";
        case ThreatLevel::MEDIUM:   return "MEDIUM";
        case ThreatLevel::HIGH:     return "HIGH";
        case ThreatLevel::CRITICAL: return "CRITICAL";
        default:                    return "UNKNOWN";
    }
}