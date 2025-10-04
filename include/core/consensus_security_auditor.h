#ifndef CONSENSUS_SECURITY_AUDITOR_H
#define CONSENSUS_SECURITY_AUDITOR_H

#include "consensus_harmony.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <mutex>
#include <chrono>

// Forward declarations
class Block;
class Transaction;

// Audit event types
enum class AuditEventType {
    CONSENSUS_VALIDATION,
    SECURITY_VIOLATION,
    ATTACK_DETECTED,
    CRYPTO_VALIDATION_FAILED,
    PARAMETER_CHANGED,
    EMERGENCY_MODE_ACTIVATED,
    SYSTEM_STARTUP,
    SYSTEM_SHUTDOWN
};

// Audit event severity levels
enum class AuditSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// Audit event structure
struct AuditEvent {
    uint64_t timestamp;
    AuditEventType type;
    AuditSeverity severity;
    std::string source;
    std::string description;
    nlohmann::json metadata;
    std::string hash; // Hash of the event for integrity
    
    AuditEvent() : timestamp(0), type(AuditEventType::CONSENSUS_VALIDATION), 
                  severity(AuditSeverity::INFO) {}
};

// Audit configuration
struct AuditConfig {
    bool enableFileLogging;
    bool enableDatabaseLogging;
    bool enableRemoteLogging;
    std::string logFilePath;
    std::string databasePath;
    std::string remoteEndpoint;
    uint64_t maxLogFileSize; // bytes
    uint32_t maxLogFiles;
    bool enableEncryption;
    std::string encryptionKey;
    
    AuditConfig() : enableFileLogging(true), enableDatabaseLogging(false), 
                   enableRemoteLogging(false), maxLogFileSize(100 * 1024 * 1024), // 100MB
                   maxLogFiles(10), enableEncryption(false) {}
};

/**
 * Security audit logger for consensus operations
 * Provides comprehensive logging and monitoring of security-related events
 */
class ConsensusSecurityAuditor {
private:
    AuditConfig config;
    std::ofstream logFile;
    mutable std::mutex auditorMutex;
    
    // Statistics
    std::atomic<uint64_t> totalEvents;
    std::atomic<uint64_t> securityViolations;
    std::atomic<uint64_t> criticalEvents;
    std::map<AuditEventType, uint64_t> eventTypeCounts;
    
    // Event buffer for batch processing
    std::vector<AuditEvent> eventBuffer;
    uint32_t bufferSize;
    bool batchMode;

public:
    explicit ConsensusSecurityAuditor(const AuditConfig& auditConfig = AuditConfig());
    ~ConsensusSecurityAuditor();
    
    // Initialization and lifecycle
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // Core audit logging methods
    void logConsensusValidation(const ConsensusRequest& request, const ConsensusResult& result);
    void logSecurityViolation(const std::string& violation, const std::string& source, 
                            const nlohmann::json& details = {});
    void logAttackDetection(const std::string& attackType, const std::string& source,
                          const nlohmann::json& details = {});
    void logCryptoValidationFailure(const std::string& reason, const std::string& source,
                                  const nlohmann::json& details = {});
    void logParameterChange(const std::string& parameter, const std::string& oldValue,
                          const std::string& newValue, const std::string& source);
    void logEmergencyModeActivation(const std::string& reason, const std::string& source);
    void logSystemEvent(const std::string& event, AuditSeverity severity, 
                       const nlohmann::json& details = {});
    
    // Generic audit logging
    void logEvent(AuditEventType type, AuditSeverity severity, const std::string& source,
                 const std::string& description, const nlohmann::json& metadata = {});
    
    // Audit query and analysis
    std::vector<AuditEvent> getEvents(uint64_t fromTimestamp = 0, uint64_t toTimestamp = 0) const;
    std::vector<AuditEvent> getEventsByType(AuditEventType type) const;
    std::vector<AuditEvent> getEventsBySeverity(AuditSeverity severity) const;
    std::vector<AuditEvent> getEventsBySource(const std::string& source) const;
    
    // Statistics and reporting
    nlohmann::json getAuditStatistics() const;
    nlohmann::json getSecurityReport(uint64_t fromTimestamp = 0, uint64_t toTimestamp = 0) const;
    nlohmann::json getAuditSummary() const;
    
    // Configuration management
    bool updateConfiguration(const AuditConfig& newConfig);
    AuditConfig getConfiguration() const { return config; }
    
    // Audit integrity
    bool verifyAuditIntegrity() const;
    std::string calculateEventHash(const AuditEvent& event) const;
    
    // Batch processing
    void enableBatchMode(uint32_t batchSize = 100);
    void disableBatchMode();
    void flushEventBuffer();

private:
    // Internal logging methods
    bool writeToFile(const AuditEvent& event);
    bool writeToDatabase(const AuditEvent& event);
    bool writeToRemote(const AuditEvent& event);
    
    // File management
    bool rotateLogFile();
    bool createLogFile();
    std::string generateLogFileName() const;
    
    // Encryption helpers
    std::string encryptData(const std::string& data) const;
    std::string decryptData(const std::string& encryptedData) const;
    
    // Helper methods
    std::string auditEventTypeToString(AuditEventType type) const;
    std::string auditSeverityToString(AuditSeverity severity) const;
    AuditEventType stringToAuditEventType(const std::string& typeStr) const;
    AuditSeverity stringToAuditSeverity(const std::string& severityStr) const;
    
    // Event processing
    void processEvent(const AuditEvent& event);
    void updateStatistics(const AuditEvent& event);
    
    // Validation
    bool validateEvent(const AuditEvent& event) const;
    bool validateConfiguration(const AuditConfig& config) const;
};

#endif // CONSENSUS_SECURITY_AUDITOR_H