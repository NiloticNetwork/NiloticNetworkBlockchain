#ifndef CONSENSUS_SECURITY_VALIDATOR_H
#define CONSENSUS_SECURITY_VALIDATOR_H

#include "consensus_harmony.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

// Forward declarations
class Block;
class Transaction;

// Security threat levels
enum class ThreatLevel {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// Security validation result
struct SecurityValidationResult {
    bool isSecure;
    ThreatLevel threatLevel;
    std::vector<std::string> threats;
    std::vector<std::string> warnings;
    std::map<std::string, std::string> metadata;
    uint64_t timestamp;
    
    SecurityValidationResult() : isSecure(true), threatLevel(ThreatLevel::LOW), timestamp(0) {}
};

// Attack detection patterns
struct AttackPattern {
    std::string name;
    std::string description;
    ThreatLevel severity;
    std::vector<std::string> indicators;
    uint64_t detectionCount;
    uint64_t lastDetection;
    
    AttackPattern() : severity(ThreatLevel::LOW), detectionCount(0), lastDetection(0) {}
};

// Cryptographic validation context
struct CryptoValidationContext {
    std::string algorithm;
    std::string publicKey;
    std::string signature;
    std::string data;
    bool requireStrongCrypto;
    
    CryptoValidationContext() : requireStrongCrypto(true) {}
};

/**
 * Comprehensive security validator for consensus mechanisms
 * Provides cryptographic validation, attack detection, and security monitoring
 */
class ConsensusSecurityValidator {
private:
    // Attack detection patterns
    std::vector<AttackPattern> attackPatterns;
    std::map<std::string, uint64_t> suspiciousActivityCounters;
    std::map<std::string, std::chrono::steady_clock::time_point> lastActivityTime;
    
    // Security configuration
    bool enableCryptographicValidation;
    bool enableAttackDetection;
    bool enableAuditLogging;
    uint64_t maxSuspiciousActivityThreshold;
    uint64_t attackDetectionWindow; // seconds
    
    // Thread safety
    mutable std::mutex validatorMutex;
    
    // Statistics
    std::atomic<uint64_t> totalValidations;
    std::atomic<uint64_t> securityViolations;
    std::atomic<uint64_t> attacksDetected;
    std::atomic<uint64_t> cryptoValidationFailures;

public:
    ConsensusSecurityValidator();
    ~ConsensusSecurityValidator() = default;
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Core security validation
    SecurityValidationResult validateConsensusRequest(const ConsensusRequest& request);
    SecurityValidationResult validateBlock(const Block& block);
    SecurityValidationResult validateTransaction(const Transaction& transaction);
    
    // Cryptographic validation
    bool validateCryptographicSignature(const CryptoValidationContext& context);
    bool validateHashChain(const std::vector<std::string>& hashes);
    bool validateMerkleRoot(const std::vector<std::string>& transactions, const std::string& merkleRoot);
    
    // Attack detection
    bool detectConsensusAttack(const ConsensusRequest& request, const std::string& source);
    bool detect51PercentAttack(const std::vector<ConsensusResult>& results);
    bool detectDoubleSpendingAttack(const Transaction& transaction);
    bool detectTimestampManipulation(uint64_t timestamp);
    bool detectSybilAttack(const std::string& source, const std::vector<std::string>& identities);
    
    // Security monitoring
    void recordSecurityEvent(const std::string& event, ThreatLevel level, 
                           const std::string& source, const nlohmann::json& details = {});
    void updateAttackPattern(const std::string& patternName, bool detected);
    std::vector<AttackPattern> getActiveThreats() const;
    
    // Configuration
    void setCryptographicValidation(bool enable) { enableCryptographicValidation = enable; }
    void setAttackDetection(bool enable) { enableAttackDetection = enable; }
    void setAuditLogging(bool enable) { enableAuditLogging = enable; }
    void setMaxSuspiciousActivityThreshold(uint64_t threshold) { maxSuspiciousActivityThreshold = threshold; }
    void setAttackDetectionWindow(uint64_t seconds) { attackDetectionWindow = seconds; }
    
    // Statistics
    nlohmann::json getSecurityMetrics() const;
    nlohmann::json getSecurityStatus() const;
    void resetStatistics();

private:
    // Internal validation methods
    bool validateConsensusIntegrity(const ConsensusRequest& request);
    bool validateRequestAuthenticity(const ConsensusRequest& request);
    bool validateTimestampSecurity(uint64_t timestamp);
    bool validateDataIntegrity(const std::string& data, const std::string& expectedHash);
    
    // Attack detection helpers
    void initializeAttackPatterns();
    bool checkSuspiciousActivity(const std::string& source);
    void updateSuspiciousActivityCounter(const std::string& source);
    bool isWithinDetectionWindow(uint64_t timestamp) const;
    
    // Cryptographic helpers
    bool isStrongCryptographicAlgorithm(const std::string& algorithm) const;
    bool validateSignatureStrength(const std::string& signature) const;
    std::string calculateMerkleRoot(const std::vector<std::string>& transactions) const;
    
    // Logging helpers
    void logSecurityEvent(const std::string& event, ThreatLevel level, 
                         const std::string& source, const nlohmann::json& details) const;
    std::string threatLevelToString(ThreatLevel level) const;
};

#endif // CONSENSUS_SECURITY_VALIDATOR_H