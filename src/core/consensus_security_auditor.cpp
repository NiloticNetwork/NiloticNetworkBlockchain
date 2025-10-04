#include "consensus_security_auditor.h"
#include "logger.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

ConsensusSecurityAuditor::ConsensusSecurityAuditor(const AuditConfig& auditConfig)
    : config(auditConfig), totalEvents(0), securityViolations(0), criticalEvents(0),
      bufferSize(100), batchMode(false) {
    Logger::info("ConsensusSecurityAuditor: Constructor called");
}

ConsensusSecurityAuditor::~ConsensusSecurityAuditor() {
    Logger::info("ConsensusSecurityAuditor: Destructor called");
    shutdown();
}

bool ConsensusSecurityAuditor::initialize() {
    std::lock_guard<std::mutex> lock(auditorMutex);
    Logger::info("ConsensusSecurityAuditor: Initializing security auditor");
    
    if (config.enableFileLogging) {
        if (!createLogFile()) {
            Logger::error("ConsensusSecurityAuditor: Failed to create log file");
            return false;
        }
    }
    
    // Initialize event type counters
    eventTypeCounts[AuditEventType::CONSENSUS_VALIDATION] = 0;
    eventTypeCounts[AuditEventType::SECURITY_VIOLATION] = 0;
    eventTypeCounts[AuditEventType::ATTACK_DETECTED] = 0;
    eventTypeCounts[AuditEventType::CRYPTO_VALIDATION_FAILED] = 0;
    eventTypeCounts[AuditEventType::PARAMETER_CHANGED] = 0;
    eventTypeCounts[AuditEventType::EMERGENCY_MODE_ACTIVATED] = 0;
    eventTypeCounts[AuditEventType::SYSTEM_STARTUP] = 0;
    eventTypeCounts[AuditEventType::SYSTEM_SHUTDOWN] = 0;
    
    logSystemEvent("Security auditor initialized", AuditSeverity::INFO);
    return true;
}

void ConsensusSecurityAuditor::shutdown() {
    std::lock_guard<std::mutex> lock(auditorMutex);
    Logger::info("ConsensusSecurityAuditor: Shutting down security auditor");
    
    if (batchMode) {
        flushEventBuffer();
    }
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    logSystemEvent("Security auditor shutdown", AuditSeverity::INFO);
}

bool ConsensusSecurityAuditor::isInitialized() const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    return config.enableFileLogging ? logFile.is_open() : true;
}

void ConsensusSecurityAuditor::logConsensusValidation(const ConsensusRequest& request, const ConsensusResult& result) {
    nlohmann::json metadata;
    metadata["request_id"] = request.requestId;
    metadata["request_type"] = static_cast<int>(request.type);
    metadata["result_valid"] = result.isValid;
    metadata["confidence"] = result.confidence;
    metadata["mechanism"] = static_cast<int>(result.mechanism);
    
    std::string description = "Consensus validation: " + request.requestId + 
                             " - " + (result.isValid ? "VALID" : "INVALID");
    
    logEvent(AuditEventType::CONSENSUS_VALIDATION, 
             result.isValid ? AuditSeverity::INFO : AuditSeverity::WARNING,
             "ConsensusEngine", description, metadata);
}

void ConsensusSecurityAuditor::logSecurityViolation(const std::string& violation, const std::string& source, 
                                                   const nlohmann::json& details) {
    std::string description = "Security violation detected: " + violation;
    logEvent(AuditEventType::SECURITY_VIOLATION, AuditSeverity::ERROR, source, description, details);
    securityViolations++;
}

void ConsensusSecurityAuditor::logAttackDetection(const std::string& attackType, const std::string& source,
                                                 const nlohmann::json& details) {
    std::string description = "Attack detected: " + attackType;
    logEvent(AuditEventType::ATTACK_DETECTED, AuditSeverity::CRITICAL, source, description, details);
}

void ConsensusSecurityAuditor::logCryptoValidationFailure(const std::string& reason, const std::string& source,
                                                         const nlohmann::json& details) {
    std::string description = "Cryptographic validation failed: " + reason;
    logEvent(AuditEventType::CRYPTO_VALIDATION_FAILED, AuditSeverity::ERROR, source, description, details);
}

void ConsensusSecurityAuditor::logParameterChange(const std::string& parameter, const std::string& oldValue,
                                                 const std::string& newValue, const std::string& source) {
    nlohmann::json metadata;
    metadata["parameter"] = parameter;
    metadata["old_value"] = oldValue;
    metadata["new_value"] = newValue;
    
    std::string description = "Parameter changed: " + parameter + " from " + oldValue + " to " + newValue;
    logEvent(AuditEventType::PARAMETER_CHANGED, AuditSeverity::INFO, source, description, metadata);
}

void ConsensusSecurityAuditor::logEmergencyModeActivation(const std::string& reason, const std::string& source) {
    nlohmann::json metadata;
    metadata["reason"] = reason;
    
    std::string description = "Emergency mode activated: " + reason;
    logEvent(AuditEventType::EMERGENCY_MODE_ACTIVATED, AuditSeverity::CRITICAL, source, description, metadata);
}

void ConsensusSecurityAuditor::logSystemEvent(const std::string& event, AuditSeverity severity, 
                                             const nlohmann::json& details) {
    logEvent(AuditEventType::SYSTEM_STARTUP, severity, "System", event, details);
}

void ConsensusSecurityAuditor::logEvent(AuditEventType type, AuditSeverity severity, const std::string& source,
                                       const std::string& description, const nlohmann::json& metadata) {
    AuditEvent event;
    event.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    event.type = type;
    event.severity = severity;
    event.source = source;
    event.description = description;
    event.metadata = metadata;
    event.hash = calculateEventHash(event);
    
    processEvent(event);
}

std::vector<AuditEvent> ConsensusSecurityAuditor::getEvents(uint64_t fromTimestamp, uint64_t toTimestamp) const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    // Stub implementation - return empty vector
    // In a full implementation, this would query the stored events
    Logger::info("ConsensusSecurityAuditor: getEvents called (stub implementation)");
    return std::vector<AuditEvent>();
}

std::vector<AuditEvent> ConsensusSecurityAuditor::getEventsByType(AuditEventType type) const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    // Stub implementation - return empty vector
    Logger::info("ConsensusSecurityAuditor: getEventsByType called (stub implementation)");
    return std::vector<AuditEvent>();
}

std::vector<AuditEvent> ConsensusSecurityAuditor::getEventsBySeverity(AuditSeverity severity) const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    // Stub implementation - return empty vector
    Logger::info("ConsensusSecurityAuditor: getEventsBySeverity called (stub implementation)");
    return std::vector<AuditEvent>();
}

std::vector<AuditEvent> ConsensusSecurityAuditor::getEventsBySource(const std::string& source) const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    // Stub implementation - return empty vector
    Logger::info("ConsensusSecurityAuditor: getEventsBySource called (stub implementation)");
    return std::vector<AuditEvent>();
}

nlohmann::json ConsensusSecurityAuditor::getAuditStatistics() const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    nlohmann::json stats;
    
    stats["total_events"] = totalEvents.load();
    stats["security_violations"] = securityViolations.load();
    stats["critical_events"] = criticalEvents.load();
    
    nlohmann::json eventCounts;
    for (const auto& pair : eventTypeCounts) {
        eventCounts[auditEventTypeToString(pair.first)] = pair.second;
    }
    stats["event_type_counts"] = eventCounts;
    
    return stats;
}

nlohmann::json ConsensusSecurityAuditor::getSecurityReport(uint64_t fromTimestamp, uint64_t toTimestamp) const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    nlohmann::json report;
    
    report["report_type"] = "security_audit";
    report["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    report["from_timestamp"] = fromTimestamp;
    report["to_timestamp"] = toTimestamp;
    
    // Include basic statistics
    report["statistics"] = getAuditStatistics();
    
    // Stub implementation - in full version would include detailed analysis
    report["security_violations"] = nlohmann::json::array();
    report["attack_attempts"] = nlohmann::json::array();
    report["recommendations"] = nlohmann::json::array();
    
    return report;
}

nlohmann::json ConsensusSecurityAuditor::getAuditSummary() const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    nlohmann::json summary;
    
    summary["total_events"] = totalEvents.load();
    summary["security_violations"] = securityViolations.load();
    summary["critical_events"] = criticalEvents.load();
    summary["audit_status"] = "active";
    summary["configuration"] = {
        {"file_logging", config.enableFileLogging},
        {"database_logging", config.enableDatabaseLogging},
        {"remote_logging", config.enableRemoteLogging},
        {"encryption_enabled", config.enableEncryption}
    };
    
    return summary;
}

bool ConsensusSecurityAuditor::updateConfiguration(const AuditConfig& newConfig) {
    std::lock_guard<std::mutex> lock(auditorMutex);
    
    if (!validateConfiguration(newConfig)) {
        Logger::error("ConsensusSecurityAuditor: Invalid configuration provided");
        return false;
    }
    
    config = newConfig;
    Logger::info("ConsensusSecurityAuditor: Configuration updated");
    return true;
}

bool ConsensusSecurityAuditor::verifyAuditIntegrity() const {
    std::lock_guard<std::mutex> lock(auditorMutex);
    // Stub implementation - always return true
    // In full implementation, would verify event hashes and integrity
    Logger::info("ConsensusSecurityAuditor: verifyAuditIntegrity called (stub implementation)");
    return true;
}

std::string ConsensusSecurityAuditor::calculateEventHash(const AuditEvent& event) const {
    // Simple hash calculation for stub implementation
    std::stringstream ss;
    ss << event.timestamp << static_cast<int>(event.type) << static_cast<int>(event.severity) 
       << event.source << event.description;
    
    // In a full implementation, would use proper cryptographic hash
    std::hash<std::string> hasher;
    return std::to_string(hasher(ss.str()));
}

void ConsensusSecurityAuditor::enableBatchMode(uint32_t batchSize) {
    std::lock_guard<std::mutex> lock(auditorMutex);
    batchMode = true;
    bufferSize = batchSize;
    eventBuffer.reserve(batchSize);
    Logger::info("ConsensusSecurityAuditor: Batch mode enabled with size " + std::to_string(batchSize));
}

void ConsensusSecurityAuditor::disableBatchMode() {
    std::lock_guard<std::mutex> lock(auditorMutex);
    if (batchMode) {
        flushEventBuffer();
        batchMode = false;
        Logger::info("ConsensusSecurityAuditor: Batch mode disabled");
    }
}

void ConsensusSecurityAuditor::flushEventBuffer() {
    // Process all events in buffer
    for (const auto& event : eventBuffer) {
        writeToFile(event);
        if (config.enableDatabaseLogging) {
            writeToDatabase(event);
        }
        if (config.enableRemoteLogging) {
            writeToRemote(event);
        }
    }
    eventBuffer.clear();
    Logger::info("ConsensusSecurityAuditor: Event buffer flushed");
}

// Private helper methods

bool ConsensusSecurityAuditor::writeToFile(const AuditEvent& event) {
    if (!config.enableFileLogging || !logFile.is_open()) {
        return false;
    }
    
    nlohmann::json eventJson;
    eventJson["timestamp"] = event.timestamp;
    eventJson["type"] = auditEventTypeToString(event.type);
    eventJson["severity"] = auditSeverityToString(event.severity);
    eventJson["source"] = event.source;
    eventJson["description"] = event.description;
    eventJson["metadata"] = event.metadata;
    eventJson["hash"] = event.hash;
    
    logFile << eventJson.dump() << std::endl;
    logFile.flush();
    
    return true;
}

bool ConsensusSecurityAuditor::writeToDatabase(const AuditEvent& event) {
    // Stub implementation - always return true
    // In full implementation, would write to database
    Logger::debug("ConsensusSecurityAuditor: writeToDatabase called (stub implementation)");
    return true;
}

bool ConsensusSecurityAuditor::writeToRemote(const AuditEvent& event) {
    // Stub implementation - always return true
    // In full implementation, would send to remote endpoint
    Logger::debug("ConsensusSecurityAuditor: writeToRemote called (stub implementation)");
    return true;
}

bool ConsensusSecurityAuditor::rotateLogFile() {
    if (!logFile.is_open()) {
        return false;
    }
    
    logFile.close();
    
    // Simple rotation - rename current file with timestamp
    std::string timestamp = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    std::string rotatedName = config.logFilePath + "." + timestamp;
    
    try {
        std::filesystem::rename(config.logFilePath, rotatedName);
    } catch (const std::exception& e) {
        Logger::error("ConsensusSecurityAuditor: Failed to rotate log file: " + std::string(e.what()));
        return false;
    }
    
    return createLogFile();
}

bool ConsensusSecurityAuditor::createLogFile() {
    if (config.logFilePath.empty()) {
        config.logFilePath = "audit.log";
    }
    
    logFile.open(config.logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        Logger::error("ConsensusSecurityAuditor: Failed to open log file: " + config.logFilePath);
        return false;
    }
    
    return true;
}

std::string ConsensusSecurityAuditor::generateLogFileName() const {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_c);
    
    std::stringstream ss;
    ss << "audit_" << std::put_time(now_tm, "%Y%m%d_%H%M%S") << ".log";
    return ss.str();
}

std::string ConsensusSecurityAuditor::encryptData(const std::string& data) const {
    // Stub implementation - return data as-is
    // In full implementation, would use proper encryption
    return data;
}

std::string ConsensusSecurityAuditor::decryptData(const std::string& encryptedData) const {
    // Stub implementation - return data as-is
    // In full implementation, would use proper decryption
    return encryptedData;
}

std::string ConsensusSecurityAuditor::auditEventTypeToString(AuditEventType type) const {
    switch (type) {
        case AuditEventType::CONSENSUS_VALIDATION: return "CONSENSUS_VALIDATION";
        case AuditEventType::SECURITY_VIOLATION: return "SECURITY_VIOLATION";
        case AuditEventType::ATTACK_DETECTED: return "ATTACK_DETECTED";
        case AuditEventType::CRYPTO_VALIDATION_FAILED: return "CRYPTO_VALIDATION_FAILED";
        case AuditEventType::PARAMETER_CHANGED: return "PARAMETER_CHANGED";
        case AuditEventType::EMERGENCY_MODE_ACTIVATED: return "EMERGENCY_MODE_ACTIVATED";
        case AuditEventType::SYSTEM_STARTUP: return "SYSTEM_STARTUP";
        case AuditEventType::SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        default: return "UNKNOWN";
    }
}

std::string ConsensusSecurityAuditor::auditSeverityToString(AuditSeverity severity) const {
    switch (severity) {
        case AuditSeverity::INFO: return "INFO";
        case AuditSeverity::WARNING: return "WARNING";
        case AuditSeverity::ERROR: return "ERROR";
        case AuditSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

AuditEventType ConsensusSecurityAuditor::stringToAuditEventType(const std::string& typeStr) const {
    if (typeStr == "CONSENSUS_VALIDATION") return AuditEventType::CONSENSUS_VALIDATION;
    if (typeStr == "SECURITY_VIOLATION") return AuditEventType::SECURITY_VIOLATION;
    if (typeStr == "ATTACK_DETECTED") return AuditEventType::ATTACK_DETECTED;
    if (typeStr == "CRYPTO_VALIDATION_FAILED") return AuditEventType::CRYPTO_VALIDATION_FAILED;
    if (typeStr == "PARAMETER_CHANGED") return AuditEventType::PARAMETER_CHANGED;
    if (typeStr == "EMERGENCY_MODE_ACTIVATED") return AuditEventType::EMERGENCY_MODE_ACTIVATED;
    if (typeStr == "SYSTEM_STARTUP") return AuditEventType::SYSTEM_STARTUP;
    if (typeStr == "SYSTEM_SHUTDOWN") return AuditEventType::SYSTEM_SHUTDOWN;
    return AuditEventType::CONSENSUS_VALIDATION; // default
}

AuditSeverity ConsensusSecurityAuditor::stringToAuditSeverity(const std::string& severityStr) const {
    if (severityStr == "INFO") return AuditSeverity::INFO;
    if (severityStr == "WARNING") return AuditSeverity::WARNING;
    if (severityStr == "ERROR") return AuditSeverity::ERROR;
    if (severityStr == "CRITICAL") return AuditSeverity::CRITICAL;
    return AuditSeverity::INFO; // default
}

void ConsensusSecurityAuditor::processEvent(const AuditEvent& event) {
    updateStatistics(event);
    
    if (batchMode) {
        eventBuffer.push_back(event);
        if (eventBuffer.size() >= bufferSize) {
            flushEventBuffer();
        }
    } else {
        writeToFile(event);
        if (config.enableDatabaseLogging) {
            writeToDatabase(event);
        }
        if (config.enableRemoteLogging) {
            writeToRemote(event);
        }
    }
}

void ConsensusSecurityAuditor::updateStatistics(const AuditEvent& event) {
    totalEvents++;
    
    if (event.severity == AuditSeverity::CRITICAL) {
        criticalEvents++;
    }
    
    if (event.type == AuditEventType::SECURITY_VIOLATION) {
        securityViolations++;
    }
    
    eventTypeCounts[event.type]++;
}

bool ConsensusSecurityAuditor::validateEvent(const AuditEvent& event) const {
    // Basic validation
    if (event.source.empty() || event.description.empty()) {
        return false;
    }
    
    if (event.timestamp == 0) {
        return false;
    }
    
    return true;
}

bool ConsensusSecurityAuditor::validateConfiguration(const AuditConfig& config) const {
    // Basic configuration validation
    if (config.enableFileLogging && config.logFilePath.empty()) {
        return false;
    }
    
    if (config.maxLogFileSize == 0 || config.maxLogFiles == 0) {
        return false;
    }
    
    return true;
}