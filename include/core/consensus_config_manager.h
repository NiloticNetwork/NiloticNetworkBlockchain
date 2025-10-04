#ifndef CONSENSUS_CONFIG_MANAGER_H
#define CONSENSUS_CONFIG_MANAGER_H

#include "consensus_harmony.h"
#include "logger.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>
#include <fstream>

// Configuration validation result
struct ConfigValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    ConfigValidationResult() : isValid(true) {}
    
    void addError(const std::string& error) {
        errors.push_back(error);
        isValid = false;
    }
    
    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    bool hasErrors() const { return !errors.empty(); }
    bool hasWarnings() const { return !warnings.empty(); }
};

// Configuration change event
struct ConfigChangeEvent {
    std::string parameter;
    std::string oldValue;
    std::string newValue;
    ConsensusType mechanism;
    uint64_t timestamp;
    std::string source;
    
    ConfigChangeEvent(const std::string& param, const std::string& oldVal, 
                     const std::string& newVal, ConsensusType mech, 
                     const std::string& src = "system")
        : parameter(param), oldValue(oldVal), newValue(newVal), 
          mechanism(mech), 
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()),
          source(src) {}
};

// Configuration bounds for safety enforcement
struct ConfigBounds {
    double minValue;
    double maxValue;
    bool isRequired;
    std::string description;
    
    ConfigBounds(double min = 0.0, double max = std::numeric_limits<double>::max(), 
                bool required = false, const std::string& desc = "")
        : minValue(min), maxValue(max), isRequired(required), description(desc) {}
    
    bool isValid(double value) const {
        return value >= minValue && value <= maxValue;
    }
};

// Configuration parameter metadata
struct ConfigParameter {
    std::string name;
    std::string type;  // "double", "uint64", "string", "bool", "array"
    ConfigBounds bounds;
    std::string category;
    std::string description;
    bool isDynamic;  // Can be changed at runtime
    bool requiresRestart;  // Requires system restart to take effect
    
    ConfigParameter() : isDynamic(true), requiresRestart(false) {}
    
    ConfigParameter(const std::string& n, const std::string& t, 
                   const ConfigBounds& b = ConfigBounds(),
                   const std::string& cat = "", const std::string& desc = "",
                   bool dynamic = true, bool restart = false)
        : name(n), type(t), bounds(b), category(cat), description(desc),
          isDynamic(dynamic), requiresRestart(restart) {}
};

// Configuration backup for rollback functionality
struct ConfigBackup {
    ConsensusConfig config;
    uint64_t timestamp;
    std::string reason;
    std::string version;
    
    ConfigBackup(const ConsensusConfig& cfg, const std::string& r = "", 
                const std::string& v = "")
        : config(cfg), 
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()),
          reason(r), version(v) {}
};

/**
 * Comprehensive configuration management system for consensus mechanisms
 * Handles dynamic updates, validation, persistence, and safety enforcement
 */
class ConsensusConfigManager {
private:
    // Current configuration
    ConsensusConfig currentConfig;
    
    // Configuration metadata and bounds
    std::map<std::string, ConfigParameter> parameterRegistry;
    std::map<ConsensusType, std::map<std::string, ConfigBounds>> safetyBounds;
    
    // Configuration history and backups
    std::vector<ConfigBackup> configHistory;
    size_t maxHistorySize;
    
    // Change tracking
    std::vector<ConfigChangeEvent> changeLog;
    size_t maxChangeLogSize;
    
    // File paths
    std::string configFilePath;
    std::string backupDirectory;
    
    // Thread safety
    mutable std::mutex configMutex;
    
    // Change notification callbacks
    std::vector<std::function<void(const ConfigChangeEvent&)>> changeCallbacks;
    
    // Logger
    std::shared_ptr<Logger> logger;

public:
    explicit ConsensusConfigManager(const std::string& configPath = "config/consensus.json",
                                   const std::string& backupDir = "config/backups");
    ~ConsensusConfigManager();
    
    // Initialization
    bool initialize();
    bool initialize(const ConsensusConfig& initialConfig);
    void shutdown();
    
    // Configuration access
    ConsensusConfig getConfiguration() const;
    ConsensusConfig getConfiguration(ConsensusType mechanism) const;
    bool setConfiguration(const ConsensusConfig& config, const std::string& source = "api");
    bool updateConfiguration(const ConsensusConfig& updates, const std::string& source = "api");
    
    // Parameter management
    bool setParameter(const std::string& parameter, double value, 
                     ConsensusType mechanism = ConsensusType::PROOF_OF_WORK,
                     const std::string& source = "api");
    bool setParameter(const std::string& parameter, uint64_t value,
                     ConsensusType mechanism = ConsensusType::PROOF_OF_WORK,
                     const std::string& source = "api");
    bool setParameter(const std::string& parameter, const std::string& value,
                     ConsensusType mechanism = ConsensusType::PROOF_OF_WORK,
                     const std::string& source = "api");
    bool setParameter(const std::string& parameter, bool value,
                     ConsensusType mechanism = ConsensusType::PROOF_OF_WORK,
                     const std::string& source = "api");
    
    template<typename T>
    T getParameter(const std::string& parameter, ConsensusType mechanism = ConsensusType::PROOF_OF_WORK) const;
    
    // Validation
    ConfigValidationResult validateConfiguration(const ConsensusConfig& config) const;
    ConfigValidationResult validateParameter(const std::string& parameter, double value,
                                           ConsensusType mechanism) const;
    bool isParameterValid(const std::string& parameter, double value,
                         ConsensusType mechanism) const;
    
    // Safety bounds management
    bool setSafetyBounds(const std::string& parameter, ConsensusType mechanism,
                        const ConfigBounds& bounds);
    ConfigBounds getSafetyBounds(const std::string& parameter, ConsensusType mechanism) const;
    bool removeSafetyBounds(const std::string& parameter, ConsensusType mechanism);
    
    // Parameter registry
    bool registerParameter(const ConfigParameter& parameter);
    bool unregisterParameter(const std::string& parameterName);
    std::vector<ConfigParameter> getRegisteredParameters() const;
    std::vector<ConfigParameter> getRegisteredParameters(ConsensusType mechanism) const;
    
    // Persistence
    bool saveConfiguration() const;
    bool saveConfiguration(const std::string& filePath) const;
    bool loadConfiguration();
    bool loadConfiguration(const std::string& filePath);
    
    // Backup and restore
    bool createBackup(const std::string& reason = "manual");
    bool restoreFromBackup(size_t backupIndex);
    bool restoreFromBackup(uint64_t timestamp);
    std::vector<ConfigBackup> getBackupHistory() const;
    bool cleanupOldBackups(uint64_t olderThanTimestamp);
    
    // Change tracking
    std::vector<ConfigChangeEvent> getChangeLog() const;
    std::vector<ConfigChangeEvent> getChangeLog(ConsensusType mechanism) const;
    std::vector<ConfigChangeEvent> getChangeLog(uint64_t fromTimestamp, uint64_t toTimestamp) const;
    void clearChangeLog();
    
    // Change notifications
    void registerChangeCallback(std::function<void(const ConfigChangeEvent&)> callback);
    void unregisterAllChangeCallbacks();
    
    // Configuration export/import
    nlohmann::json exportConfiguration() const;
    nlohmann::json exportConfiguration(ConsensusType mechanism) const;
    bool importConfiguration(const nlohmann::json& configJson, const std::string& source = "import");
    
    // Governance integration (Requirements 3.2, 3.5)
    bool applyGovernanceDecision(const std::string& proposalId, const nlohmann::json& parameters, 
                                const std::string& source = "governance");
    bool validateGovernanceParameters(const nlohmann::json& parameters) const;
    bool ensureBackwardCompatibility(const ConsensusConfig& newConfig) const;
    std::vector<std::string> getCompatibilityIssues(const ConsensusConfig& newConfig) const;
    
    // Configuration comparison
    std::vector<std::string> compareConfigurations(const ConsensusConfig& config1,
                                                  const ConsensusConfig& config2) const;
    
    // Status and diagnostics
    nlohmann::json getStatus() const;
    nlohmann::json getDiagnostics() const;
    bool performSelfTest();
    
    // Default configurations
    static ConsensusConfig getDefaultConfiguration();
    static ConsensusConfig getTestConfiguration();
    static ConsensusConfig getProductionConfiguration();

private:
    // Internal helpers
    void initializeParameterRegistry();
    void initializeSafetyBounds();
    void notifyConfigChange(const ConfigChangeEvent& event);
    bool validateConfigurationInternal(const ConsensusConfig& config, 
                                     ConfigValidationResult& result) const;
    std::string generateBackupFileName() const;
    bool ensureDirectoryExists(const std::string& directory) const;
    void logConfigChange(const ConfigChangeEvent& event) const;
    
    // Parameter conversion helpers
    template<typename T>
    bool setParameterInternal(const std::string& parameter, T value,
                            ConsensusType mechanism, const std::string& source);
    
    // Configuration merging
    ConsensusConfig mergeConfigurations(const ConsensusConfig& base,
                                      const ConsensusConfig& updates) const;
};

// Template implementations
template<typename T>
T ConsensusConfigManager::getParameter(const std::string& parameter, ConsensusType mechanism) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    // This is a simplified implementation - in practice, you'd need proper type handling
    // For now, we'll handle the most common cases
    if constexpr (std::is_same_v<T, double>) {
        if (mechanism == ConsensusType::PROOF_OF_WORK) {
            if (parameter == "difficulty") return static_cast<T>(currentConfig.powDifficulty);
            if (parameter == "targetBlockTime") return static_cast<T>(currentConfig.powTargetBlockTime);
        } else if (mechanism == ConsensusType::PROOF_OF_STAKE) {
            if (parameter == "minStakeAmount") return static_cast<T>(currentConfig.minStakeAmount);
            if (parameter == "stakingPeriod") return static_cast<T>(currentConfig.stakingPeriod);
        } else if (mechanism == ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION) {
            if (parameter == "minResourceContribution") return static_cast<T>(currentConfig.minResourceContribution);
        } else if (mechanism == ConsensusType::VOTING_CONSENSUS) {
            if (parameter == "supermajorityThreshold") return static_cast<T>(currentConfig.supermajorityThreshold);
            if (parameter == "votingPeriod") return static_cast<T>(currentConfig.votingPeriod);
        }
        if (parameter == "maxDominanceRatio") return static_cast<T>(currentConfig.maxDominanceRatio);
        if (parameter == "rebalancingInterval") return static_cast<T>(currentConfig.rebalancingInterval);
    }
    
    return T{};  // Default value if parameter not found
}

#endif // CONSENSUS_CONFIG_MANAGER_H