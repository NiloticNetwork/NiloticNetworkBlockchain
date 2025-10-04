#ifndef EMERGENCY_CONSENSUS_MODE_H
#define EMERGENCY_CONSENSUS_MODE_H

#include "consensus_harmony.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>

// Forward declarations
class ConsensusHarmonyManager;
class Blockchain;

// Emergency mode types
enum class EmergencyType {
    CONSENSUS_CONFLICT,
    MECHANISM_FAILURE,
    NETWORK_ATTACK,
    DATA_CORRUPTION,
    CRITICAL_ERROR,
    MANUAL_ACTIVATION
};

// Emergency mode severity levels
enum class EmergencySeverity {
    LOW,        // Minor issues, automatic recovery possible
    MEDIUM,     // Significant issues, may require intervention
    HIGH,       // Critical issues, immediate action required
    CRITICAL    // System-threatening issues, emergency protocols activated
};

// Emergency event structure
struct EmergencyEvent {
    EmergencyType type;
    EmergencySeverity severity;
    std::string description;
    std::string source;
    uint64_t timestamp;
    std::map<std::string, std::string> metadata;
    
    EmergencyEvent(EmergencyType t, EmergencySeverity s, const std::string& desc, const std::string& src = "")
        : type(t), severity(s), description(desc), source(src),
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// Recovery strategy
struct RecoveryStrategy {
    std::string name;
    std::function<bool()> recoveryFunction;
    uint32_t maxAttempts;
    uint32_t currentAttempts;
    std::chrono::seconds retryDelay;
    bool isActive;
    
    RecoveryStrategy(const std::string& n, std::function<bool()> func, uint32_t maxAttempts = 3)
        : name(n), recoveryFunction(func), maxAttempts(maxAttempts), 
          currentAttempts(0), retryDelay(std::chrono::seconds(30)), isActive(false) {}
};

// Emergency consensus mode configuration
struct EmergencyConfig {
    // Activation thresholds
    uint32_t maxConsensusConflicts = 5;
    uint32_t maxMechanismFailures = 3;
    std::chrono::seconds conflictTimeWindow = std::chrono::seconds(300); // 5 minutes
    
    // Recovery settings
    uint32_t maxRecoveryAttempts = 5;
    std::chrono::seconds recoveryRetryDelay = std::chrono::seconds(60);
    std::chrono::seconds emergencyModeTimeout = std::chrono::seconds(3600); // 1 hour
    
    // Backup mechanisms
    std::vector<ConsensusType> backupMechanisms = {
        ConsensusType::PROOF_OF_WORK,
        ConsensusType::PROOF_OF_STAKE
    };
    
    // Network protection
    bool enableNetworkProtection = true;
    bool enableTransactionHalt = true;
    bool enableBlockProductionHalt = false;
    
    // Data consistency
    bool enableDataVerification = true;
    bool enableStateRollback = true;
    uint32_t maxRollbackBlocks = 10;
};

/**
 * Emergency Consensus Mode Manager
 * Handles critical failure scenarios and implements recovery procedures
 */
class EmergencyConsensusMode {
private:
    // Core components
    ConsensusHarmonyManager* harmonyManager;
    Blockchain* blockchain;
    
    // Emergency state
    std::atomic<bool> emergencyActive;
    std::atomic<bool> recoveryInProgress;
    std::atomic<EmergencySeverity> currentSeverity;
    
    // Configuration
    EmergencyConfig config;
    
    // Event tracking
    std::queue<EmergencyEvent> emergencyEvents;
    std::chrono::steady_clock::time_point emergencyStartTime;
    std::chrono::steady_clock::time_point lastEventTime;
    
    // Recovery management
    std::vector<RecoveryStrategy> recoveryStrategies;
    std::thread recoveryThread;
    std::atomic<bool> shouldStopRecovery;
    std::condition_variable recoveryCV;
    std::mutex recoveryMutex;
    
    // Thread safety
    mutable std::mutex emergencyMutex;
    
    // Statistics
    uint32_t totalEmergencyActivations;
    uint32_t successfulRecoveries;
    uint32_t failedRecoveries;
    std::chrono::seconds totalEmergencyTime;

public:
    explicit EmergencyConsensusMode(ConsensusHarmonyManager* manager, Blockchain* bc = nullptr);
    ~EmergencyConsensusMode();
    
    // Initialization and lifecycle
    bool initialize();
    bool initialize(const EmergencyConfig& customConfig);
    void shutdown();
    
    // Emergency mode control
    bool activateEmergencyMode(EmergencyType type, EmergencySeverity severity, 
                              const std::string& description, const std::string& source = "");
    bool deactivateEmergencyMode();
    bool isEmergencyActive() const { return emergencyActive.load(); }
    EmergencySeverity getCurrentSeverity() const { return currentSeverity.load(); }
    
    // Event handling
    void reportEmergencyEvent(const EmergencyEvent& event);
    bool shouldActivateEmergency(const EmergencyEvent& event);
    std::vector<EmergencyEvent> getRecentEvents(uint32_t count = 10) const;
    
    // Recovery management
    bool startRecovery();
    bool isRecoveryInProgress() const { return recoveryInProgress.load(); }
    void addRecoveryStrategy(const RecoveryStrategy& strategy);
    bool executeRecoveryStrategy(const std::string& strategyName);
    
    // Backup mechanism management
    bool activateBackupMechanisms();
    bool deactivateBackupMechanisms();
    std::vector<ConsensusType> getActiveBackupMechanisms() const;
    
    // Network protection
    bool enableNetworkProtection();
    bool disableNetworkProtection();
    bool haltTransactionProcessing();
    bool resumeTransactionProcessing();
    bool haltBlockProduction();
    bool resumeBlockProduction();
    
    // Data consistency verification
    bool verifyDataConsistency();
    bool performStateRollback(uint32_t blocks);
    bool validateBlockchainIntegrity();
    
    // Attack protection
    bool detectConsensusAttack();
    bool implementAttackCountermeasures();
    bool isolateMaliciousNodes();
    
    // Configuration and status
    bool updateConfiguration(const EmergencyConfig& newConfig);
    EmergencyConfig getConfiguration() const;
    nlohmann::json getStatus() const;
    nlohmann::json getEmergencyStatus() const;
    nlohmann::json getEmergencyMetrics() const;
    
    // Automated recovery
    bool enableAutomatedRecovery();
    bool disableAutomatedRecovery();
    bool isAutomatedRecoveryEnabled() const;

private:
    // Internal recovery methods
    void recoveryLoop();
    void setupDefaultRecoveryStrategies();
    bool attemptAutomaticRecovery();
    bool executeRecoveryPlan();
    bool validateRecoverySuccess();
    
    // Emergency detection
    bool analyzeEmergencyTriggers();
    bool checkConsensusConflicts();
    bool checkMechanismFailures();
    bool checkNetworkAttacks();
    
    // Backup mechanism helpers
    bool initializeBackupMechanism(ConsensusType type);
    bool shutdownBackupMechanism(ConsensusType type);
    bool validateBackupMechanism(ConsensusType type);
    
    // Data consistency helpers
    bool createDataSnapshot();
    bool restoreFromSnapshot();
    bool verifyBlockIntegrity(uint64_t blockHeight);
    bool verifyTransactionIntegrity();
    
    // Network protection helpers
    bool implementNetworkIsolation();
    bool removeNetworkIsolation();
    bool adjustNetworkParameters();
    bool resetNetworkParameters();
    
    // Logging and monitoring
    void logEmergencyEvent(const std::string& event, const nlohmann::json& data = {}) const;
    void updateEmergencyMetrics();
    void notifyAdministrators(const EmergencyEvent& event);
    
    // Configuration validation
    bool validateEmergencyConfig(const EmergencyConfig& config) const;
    
    // Helper methods
    std::string emergencyTypeToString(EmergencyType type) const;
    std::string emergencySeverityToString(EmergencySeverity severity) const;
    EmergencyType stringToEmergencyType(const std::string& typeStr) const;
    EmergencySeverity stringToEmergencySeverity(const std::string& severityStr) const;
};

#endif // EMERGENCY_CONSENSUS_MODE_H