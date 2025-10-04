#include "../../include/core/consensus_config_manager.h"
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <limits>
#include <ctime>
#include <iomanip>
#include <set>

ConsensusConfigManager::ConsensusConfigManager(const std::string& configPath, 
                                             const std::string& backupDir)
    : configFilePath(configPath), 
      maxHistorySize(100), maxChangeLogSize(1000),
      backupDirectory(backupDir),
      logger(std::make_shared<Logger>()) {
    // Initialize parameter registry and safety bounds for validation methods
    initializeParameterRegistry();
    initializeSafetyBounds();
}

ConsensusConfigManager::~ConsensusConfigManager() {
    shutdown();
}

bool ConsensusConfigManager::initialize() {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Initialize parameter registry and safety bounds
        initializeParameterRegistry();
        initializeSafetyBounds();
        
        // Ensure directories exist
        auto parentPath = std::filesystem::path(configFilePath).parent_path();
        if (!parentPath.empty() && !ensureDirectoryExists(parentPath.string())) {
            logger->error("Failed to create configuration directory");
            return false;
        }
        
        if (!ensureDirectoryExists(backupDirectory)) {
            logger->error("Failed to create backup directory");
            return false;
        }
        
        // Load existing configuration or create default
        if (std::filesystem::exists(configFilePath)) {
            if (!loadConfiguration()) {
                logger->warning("Failed to load existing configuration, using defaults");
                currentConfig = getDefaultConfiguration();
            }
        } else {
            currentConfig = getDefaultConfiguration();
            if (!saveConfiguration()) {
                logger->warning("Failed to save default configuration");
            }
        }
        
        // Validate current configuration
        auto validationResult = validateConfiguration(currentConfig);
        if (!validationResult.isValid) {
            logger->error("Current configuration is invalid:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        // Log warnings if any
        for (const auto& warning : validationResult.warnings) {
            logger->warning(warning);
        }
        
        // Create initial backup
        createBackup("initialization");
        
        logger->info("ConsensusConfigManager initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to initialize ConsensusConfigManager: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::initialize(const ConsensusConfig& initialConfig) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Initialize parameter registry and safety bounds
        initializeParameterRegistry();
        initializeSafetyBounds();
        
        // Validate initial configuration
        auto validationResult = validateConfiguration(initialConfig);
        if (!validationResult.isValid) {
            logger->error("Initial configuration is invalid:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        // Set current configuration
        currentConfig = initialConfig;
        
        // Ensure directories exist and save configuration
        auto parentPath = std::filesystem::path(configFilePath).parent_path();
        if ((!parentPath.empty() && !ensureDirectoryExists(parentPath.string())) ||
            !ensureDirectoryExists(backupDirectory)) {
            logger->error("Failed to create required directories");
            return false;
        }
        
        if (!saveConfiguration()) {
            logger->warning("Failed to save initial configuration");
        }
        
        // Create initial backup
        createBackup("initialization");
        
        logger->info("ConsensusConfigManager initialized with custom configuration");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to initialize ConsensusConfigManager: " + std::string(e.what()));
        return false;
    }
}

void ConsensusConfigManager::shutdown() {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Save current configuration
        saveConfiguration();
        
        // Create shutdown backup
        createBackup("shutdown");
        
        // Clear callbacks
        changeCallbacks.clear();
        
        logger->info("ConsensusConfigManager shutdown completed");
        
    } catch (const std::exception& e) {
        logger->error("Error during ConsensusConfigManager shutdown: " + std::string(e.what()));
    }
}

ConsensusConfig ConsensusConfigManager::getConfiguration() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return currentConfig;
}

ConsensusConfig ConsensusConfigManager::getConfiguration(ConsensusType mechanism) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    // Return a filtered configuration containing only parameters for the specified mechanism
    ConsensusConfig filtered = currentConfig;
    
    // This is a simplified approach - in practice, you might want more sophisticated filtering
    switch (mechanism) {
        case ConsensusType::PROOF_OF_WORK:
            // Keep only PoW parameters, reset others to defaults
            filtered.minStakeAmount = 0;
            filtered.stakingPeriod = 0;
            filtered.minResourceContribution = 0;
            filtered.acceptedResourceTypes.clear();
            filtered.supermajorityThreshold = 0;
            filtered.votingPeriod = 0;
            break;
        case ConsensusType::PROOF_OF_STAKE:
            // Keep only PoS parameters
            filtered.powDifficulty = 0;
            filtered.powTargetBlockTime = 0;
            filtered.minResourceContribution = 0;
            filtered.acceptedResourceTypes.clear();
            filtered.supermajorityThreshold = 0;
            filtered.votingPeriod = 0;
            break;
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
            // Keep only PoRC parameters
            filtered.powDifficulty = 0;
            filtered.powTargetBlockTime = 0;
            filtered.minStakeAmount = 0;
            filtered.stakingPeriod = 0;
            filtered.supermajorityThreshold = 0;
            filtered.votingPeriod = 0;
            break;
        case ConsensusType::VOTING_CONSENSUS:
            // Keep only voting parameters
            filtered.powDifficulty = 0;
            filtered.powTargetBlockTime = 0;
            filtered.minStakeAmount = 0;
            filtered.stakingPeriod = 0;
            filtered.minResourceContribution = 0;
            filtered.acceptedResourceTypes.clear();
            break;
        default:
            break;
    }
    
    return filtered;
}

bool ConsensusConfigManager::setConfiguration(const ConsensusConfig& config, const std::string& source) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Validate new configuration
        auto validationResult = validateConfiguration(config);
        if (!validationResult.isValid) {
            logger->error("Configuration validation failed:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        // Create backup before changing
        createBackup("before_set_configuration");
        
        // Track changes
        auto differences = compareConfigurations(currentConfig, config);
        
        // Update configuration
        ConsensusConfig oldConfig = currentConfig;
        currentConfig = config;
        
        // Log changes and notify callbacks
        for (const auto& diff : differences) {
            ConfigChangeEvent event(diff, "old_value", "new_value", 
                                  ConsensusType::PROOF_OF_WORK, source);
            changeLog.push_back(event);
            notifyConfigChange(event);
            logConfigChange(event);
        }
        
        // Trim change log if necessary
        if (changeLog.size() > maxChangeLogSize) {
            changeLog.erase(changeLog.begin(), 
                          changeLog.begin() + (changeLog.size() - maxChangeLogSize));
        }
        
        // Save configuration
        if (!saveConfiguration()) {
            logger->warning("Failed to save updated configuration");
        }
        
        logger->info("Configuration updated successfully from source: " + source);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to set configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::updateConfiguration(const ConsensusConfig& updates, const std::string& source) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Merge configurations
        ConsensusConfig mergedConfig = mergeConfigurations(currentConfig, updates);
        
        // Validate merged configuration
        auto validationResult = validateConfiguration(mergedConfig);
        if (!validationResult.isValid) {
            logger->error("Updated configuration validation failed:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        // Apply the merged configuration
        return setConfiguration(mergedConfig, source);
        
    } catch (const std::exception& e) {
        logger->error("Failed to update configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::setParameter(const std::string& parameter, double value, 
                                        ConsensusType mechanism, const std::string& source) {
    return setParameterInternal(parameter, value, mechanism, source);
}

bool ConsensusConfigManager::setParameter(const std::string& parameter, uint64_t value,
                                        ConsensusType mechanism, const std::string& source) {
    return setParameterInternal(parameter, value, mechanism, source);
}

bool ConsensusConfigManager::setParameter(const std::string& parameter, const std::string& value,
                                        ConsensusType mechanism, const std::string& source) {
    return setParameterInternal(parameter, value, mechanism, source);
}

bool ConsensusConfigManager::setParameter(const std::string& parameter, bool value,
                                        ConsensusType mechanism, const std::string& source) {
    return setParameterInternal(parameter, value, mechanism, source);
}

template<typename T>
bool ConsensusConfigManager::setParameterInternal(const std::string& parameter, T value,
                                                 ConsensusType mechanism, const std::string& source) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Validate parameter bounds if applicable
        if constexpr (std::is_arithmetic_v<T>) {
            auto validationResult = validateParameter(parameter, static_cast<double>(value), mechanism);
            if (!validationResult.isValid) {
                logger->error("Parameter validation failed for " + parameter + ":");
                for (const auto& error : validationResult.errors) {
                    logger->error("  - " + error);
                }
                return false;
            }
        }
        
        // Create backup before changing
        createBackup("before_parameter_change");
        
        // Get old value for change tracking
        std::string oldValue = "unknown";
        std::string newValue;
        if constexpr (std::is_same_v<T, std::string>) {
            newValue = value;
        } else if constexpr (std::is_same_v<T, bool>) {
            newValue = value ? "true" : "false";
        } else {
            newValue = std::to_string(value);
        }
        
        // Update the parameter based on mechanism and parameter name
        bool updated = false;
        
        if (mechanism == ConsensusType::PROOF_OF_WORK) {
            if (parameter == "difficulty") {
                oldValue = std::to_string(currentConfig.powDifficulty);
                if constexpr (std::is_integral_v<T>) {
                    currentConfig.powDifficulty = static_cast<uint64_t>(value);
                    updated = true;
                }
            } else if (parameter == "targetBlockTime") {
                oldValue = std::to_string(currentConfig.powTargetBlockTime);
                if constexpr (std::is_integral_v<T>) {
                    currentConfig.powTargetBlockTime = static_cast<uint64_t>(value);
                    updated = true;
                }
            }
        } else if (mechanism == ConsensusType::PROOF_OF_STAKE) {
            if (parameter == "minStakeAmount") {
                oldValue = std::to_string(currentConfig.minStakeAmount);
                if constexpr (std::is_arithmetic_v<T>) {
                    currentConfig.minStakeAmount = static_cast<double>(value);
                    updated = true;
                }
            } else if (parameter == "stakingPeriod") {
                oldValue = std::to_string(currentConfig.stakingPeriod);
                if constexpr (std::is_integral_v<T>) {
                    currentConfig.stakingPeriod = static_cast<uint64_t>(value);
                    updated = true;
                }
            }
        } else if (mechanism == ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION) {
            if (parameter == "minResourceContribution") {
                oldValue = std::to_string(currentConfig.minResourceContribution);
                if constexpr (std::is_arithmetic_v<T>) {
                    currentConfig.minResourceContribution = static_cast<double>(value);
                    updated = true;
                }
            }
        } else if (mechanism == ConsensusType::VOTING_CONSENSUS) {
            if (parameter == "supermajorityThreshold") {
                oldValue = std::to_string(currentConfig.supermajorityThreshold);
                if constexpr (std::is_arithmetic_v<T>) {
                    currentConfig.supermajorityThreshold = static_cast<double>(value);
                    updated = true;
                }
            } else if (parameter == "votingPeriod") {
                oldValue = std::to_string(currentConfig.votingPeriod);
                if constexpr (std::is_integral_v<T>) {
                    currentConfig.votingPeriod = static_cast<uint64_t>(value);
                    updated = true;
                }
            }
        }
        
        // Handle general balancing parameters
        if (!updated) {
            if (parameter == "maxDominanceRatio") {
                oldValue = std::to_string(currentConfig.maxDominanceRatio);
                if constexpr (std::is_arithmetic_v<T>) {
                    currentConfig.maxDominanceRatio = static_cast<double>(value);
                    updated = true;
                }
            } else if (parameter == "rebalancingInterval") {
                oldValue = std::to_string(currentConfig.rebalancingInterval);
                if constexpr (std::is_integral_v<T>) {
                    currentConfig.rebalancingInterval = static_cast<uint64_t>(value);
                    updated = true;
                }
            }
        }
        
        if (!updated) {
            logger->error("Unknown parameter: " + parameter + " for mechanism");
            return false;
        }
        
        // Log change and notify callbacks
        ConfigChangeEvent event(parameter, oldValue, newValue, mechanism, source);
        changeLog.push_back(event);
        notifyConfigChange(event);
        logConfigChange(event);
        
        // Trim change log if necessary
        if (changeLog.size() > maxChangeLogSize) {
            changeLog.erase(changeLog.begin(), 
                          changeLog.begin() + (changeLog.size() - maxChangeLogSize));
        }
        
        // Save configuration
        if (!saveConfiguration()) {
            logger->warning("Failed to save configuration after parameter update");
        }
        
        logger->info("Parameter " + parameter + " updated successfully");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to set parameter " + parameter + ": " + std::string(e.what()));
        return false;
    }
}

ConfigValidationResult ConsensusConfigManager::validateConfiguration(const ConsensusConfig& config) const {
    ConfigValidationResult result;
    validateConfigurationInternal(config, result);
    return result;
}

ConfigValidationResult ConsensusConfigManager::validateParameter(const std::string& parameter, 
                                                               double value, ConsensusType mechanism) const {
    ConfigValidationResult result;
    
    try {
        // Check if parameter exists in registry
        auto it = parameterRegistry.find(parameter);
        if (it == parameterRegistry.end()) {
            result.addError("Unknown parameter: " + parameter);
            return result;
        }
        
        const ConfigParameter& param = it->second;
        
        // Check bounds
        if (!param.bounds.isValid(value)) {
            result.addError("Parameter " + parameter + " value " + std::to_string(value) + 
                          " is outside valid range [" + std::to_string(param.bounds.minValue) + 
                          ", " + std::to_string(param.bounds.maxValue) + "]");
        }
        
        // Check mechanism-specific safety bounds
        auto mechanismBounds = safetyBounds.find(mechanism);
        if (mechanismBounds != safetyBounds.end()) {
            auto paramBounds = mechanismBounds->second.find(parameter);
            if (paramBounds != mechanismBounds->second.end()) {
                if (!paramBounds->second.isValid(value)) {
                    result.addError("Parameter " + parameter + " value " + std::to_string(value) + 
                                  " violates safety bounds for mechanism");
                }
            }
        }
        
        // Add mechanism-specific validation
        if (mechanism == ConsensusType::PROOF_OF_WORK) {
            if (parameter == "difficulty" && value < 1) {
                result.addError("PoW difficulty must be at least 1");
            }
            if (parameter == "targetBlockTime" && value < 10) {
                result.addWarning("PoW target block time less than 10 seconds may cause instability");
            }
        } else if (mechanism == ConsensusType::PROOF_OF_STAKE) {
            if (parameter == "minStakeAmount" && value <= 0) {
                result.addError("PoS minimum stake amount must be positive");
            }
            if (parameter == "stakingPeriod" && value < 3600) {
                result.addWarning("PoS staking period less than 1 hour may reduce security");
            }
        } else if (mechanism == ConsensusType::VOTING_CONSENSUS) {
            if (parameter == "supermajorityThreshold" && (value <= 0.5 || value > 1.0)) {
                result.addError("Supermajority threshold must be between 0.5 and 1.0");
            }
        }
        
    } catch (const std::exception& e) {
        result.addError("Validation error: " + std::string(e.what()));
    }
    
    return result;
}

bool ConsensusConfigManager::isParameterValid(const std::string& parameter, double value,
                                            ConsensusType mechanism) const {
    auto result = validateParameter(parameter, value, mechanism);
    return result.isValid;
}

bool ConsensusConfigManager::setSafetyBounds(const std::string& parameter, ConsensusType mechanism,
                                           const ConfigBounds& bounds) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        safetyBounds[mechanism][parameter] = bounds;
        logger->info("Safety bounds set for parameter " + parameter);
        return true;
    } catch (const std::exception& e) {
        logger->error("Failed to set safety bounds: " + std::string(e.what()));
        return false;
    }
}

ConfigBounds ConsensusConfigManager::getSafetyBounds(const std::string& parameter, 
                                                    ConsensusType mechanism) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    auto mechanismBounds = safetyBounds.find(mechanism);
    if (mechanismBounds != safetyBounds.end()) {
        auto paramBounds = mechanismBounds->second.find(parameter);
        if (paramBounds != mechanismBounds->second.end()) {
            return paramBounds->second;
        }
    }
    
    return ConfigBounds(); // Return default bounds
}

bool ConsensusConfigManager::removeSafetyBounds(const std::string& parameter, ConsensusType mechanism) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        auto mechanismBounds = safetyBounds.find(mechanism);
        if (mechanismBounds != safetyBounds.end()) {
            mechanismBounds->second.erase(parameter);
            logger->info("Safety bounds removed for parameter " + parameter);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        logger->error("Failed to remove safety bounds: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::registerParameter(const ConfigParameter& parameter) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        parameterRegistry[parameter.name] = parameter;
        logger->info("Parameter registered: " + parameter.name);
        return true;
    } catch (const std::exception& e) {
        logger->error("Failed to register parameter: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::unregisterParameter(const std::string& parameterName) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        parameterRegistry.erase(parameterName);
        logger->info("Parameter unregistered: " + parameterName);
        return true;
    } catch (const std::exception& e) {
        logger->error("Failed to unregister parameter: " + std::string(e.what()));
        return false;
    }
}

std::vector<ConfigParameter> ConsensusConfigManager::getRegisteredParameters() const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::vector<ConfigParameter> parameters;
    for (const auto& [name, param] : parameterRegistry) {
        parameters.push_back(param);
    }
    return parameters;
}

std::vector<ConfigParameter> ConsensusConfigManager::getRegisteredParameters(ConsensusType mechanism) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::vector<ConfigParameter> parameters;
    std::string mechanismCategory;
    
    switch (mechanism) {
        case ConsensusType::PROOF_OF_WORK:
            mechanismCategory = "pow";
            break;
        case ConsensusType::PROOF_OF_STAKE:
            mechanismCategory = "pos";
            break;
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
            mechanismCategory = "porc";
            break;
        case ConsensusType::VOTING_CONSENSUS:
            mechanismCategory = "voting";
            break;
        case ConsensusType::SMART_CONTRACT_VALIDATION:
            mechanismCategory = "smart_contract";
            break;
    }
    
    for (const auto& [name, param] : parameterRegistry) {
        if (param.category == mechanismCategory || param.category == "general") {
            parameters.push_back(param);
        }
    }
    return parameters;
}

bool ConsensusConfigManager::saveConfiguration() const {
    return saveConfiguration(configFilePath);
}

bool ConsensusConfigManager::saveConfiguration(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        nlohmann::json configJson = currentConfig.toJson();
        
        // Add metadata
        configJson["metadata"]["version"] = "1.0";
        configJson["metadata"]["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        configJson["metadata"]["source"] = "ConsensusConfigManager";
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            logger->error("Failed to open configuration file for writing: " + filePath);
            return false;
        }
        
        file << configJson.dump(4);
        file.close();
        
        logger->info("Configuration saved to: " + filePath);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to save configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::loadConfiguration() {
    return loadConfiguration(configFilePath);
}

bool ConsensusConfigManager::loadConfiguration(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        if (!std::filesystem::exists(filePath)) {
            logger->error("Configuration file does not exist: " + filePath);
            return false;
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            logger->error("Failed to open configuration file: " + filePath);
            return false;
        }
        
        nlohmann::json configJson;
        file >> configJson;
        file.close();
        
        // Extract configuration
        ConsensusConfig loadedConfig;
        loadedConfig.fromJson(configJson);
        
        // Validate loaded configuration
        auto validationResult = validateConfiguration(loadedConfig);
        if (!validationResult.isValid) {
            logger->error("Loaded configuration is invalid:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        currentConfig = loadedConfig;
        logger->info("Configuration loaded from: " + filePath);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to load configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::createBackup(const std::string& reason) {
    try {
        ConfigBackup backup(currentConfig, reason, "1.0");
        configHistory.push_back(backup);
        
        // Trim history if necessary
        if (configHistory.size() > maxHistorySize) {
            configHistory.erase(configHistory.begin(), 
                              configHistory.begin() + (configHistory.size() - maxHistorySize));
        }
        
        // Save backup to file
        std::string backupFileName = generateBackupFileName();
        std::string backupPath = backupDirectory + "/" + backupFileName;
        
        nlohmann::json backupJson = backup.config.toJson();
        backupJson["backup_metadata"]["timestamp"] = backup.timestamp;
        backupJson["backup_metadata"]["reason"] = backup.reason;
        backupJson["backup_metadata"]["version"] = backup.version;
        
        std::ofstream file(backupPath);
        if (file.is_open()) {
            file << backupJson.dump(4);
            file.close();
            logger->info("Configuration backup created: " + backupPath);
        } else {
            logger->warning("Failed to save backup file: " + backupPath);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to create backup: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::restoreFromBackup(size_t backupIndex) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        if (backupIndex >= configHistory.size()) {
            logger->error("Invalid backup index: " + std::to_string(backupIndex));
            return false;
        }
        
        const ConfigBackup& backup = configHistory[backupIndex];
        
        // Validate backup configuration
        auto validationResult = validateConfiguration(backup.config);
        if (!validationResult.isValid) {
            logger->error("Backup configuration is invalid:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        // Create backup of current config before restoring
        createBackup("before_restore");
        
        currentConfig = backup.config;
        
        // Save restored configuration
        if (!saveConfiguration()) {
            logger->warning("Failed to save restored configuration");
        }
        
        logger->info("Configuration restored from backup (index: " + std::to_string(backupIndex) + ")");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to restore from backup: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::restoreFromBackup(uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Find backup with matching timestamp
        for (size_t i = 0; i < configHistory.size(); ++i) {
            if (configHistory[i].timestamp == timestamp) {
                return restoreFromBackup(i);
            }
        }
        
        logger->error("No backup found with timestamp: " + std::to_string(timestamp));
        return false;
        
    } catch (const std::exception& e) {
        logger->error("Failed to restore from backup: " + std::string(e.what()));
        return false;
    }
}

std::vector<ConfigBackup> ConsensusConfigManager::getBackupHistory() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return configHistory;
}

bool ConsensusConfigManager::cleanupOldBackups(uint64_t olderThanTimestamp) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        // Remove from memory
        configHistory.erase(
            std::remove_if(configHistory.begin(), configHistory.end(),
                          [olderThanTimestamp](const ConfigBackup& backup) {
                              return backup.timestamp < olderThanTimestamp;
                          }),
            configHistory.end());
        
        // Remove backup files
        if (std::filesystem::exists(backupDirectory)) {
            for (const auto& entry : std::filesystem::directory_iterator(backupDirectory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    auto fileTime = std::filesystem::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        fileTime - std::filesystem::file_time_type::clock::now() + 
                        std::chrono::system_clock::now());
                    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        sctp.time_since_epoch()).count();
                    
                    if (timestamp < olderThanTimestamp) {
                        std::filesystem::remove(entry.path());
                        logger->info("Removed old backup file: " + entry.path().string());
                    }
                }
            }
        }
        
        logger->info("Old backups cleaned up");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to cleanup old backups: " + std::string(e.what()));
        return false;
    }
}

std::vector<ConfigChangeEvent> ConsensusConfigManager::getChangeLog() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return changeLog;
}

std::vector<ConfigChangeEvent> ConsensusConfigManager::getChangeLog(ConsensusType mechanism) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::vector<ConfigChangeEvent> filtered;
    for (const auto& event : changeLog) {
        if (event.mechanism == mechanism) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

std::vector<ConfigChangeEvent> ConsensusConfigManager::getChangeLog(uint64_t fromTimestamp, 
                                                                   uint64_t toTimestamp) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::vector<ConfigChangeEvent> filtered;
    for (const auto& event : changeLog) {
        if (event.timestamp >= fromTimestamp && event.timestamp <= toTimestamp) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

void ConsensusConfigManager::clearChangeLog() {
    std::lock_guard<std::mutex> lock(configMutex);
    changeLog.clear();
    logger->info("Change log cleared");
}

void ConsensusConfigManager::registerChangeCallback(std::function<void(const ConfigChangeEvent&)> callback) {
    std::lock_guard<std::mutex> lock(configMutex);
    changeCallbacks.push_back(callback);
}

void ConsensusConfigManager::unregisterAllChangeCallbacks() {
    std::lock_guard<std::mutex> lock(configMutex);
    changeCallbacks.clear();
}

nlohmann::json ConsensusConfigManager::exportConfiguration() const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    nlohmann::json exportJson = currentConfig.toJson();
    exportJson["export_metadata"]["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    exportJson["export_metadata"]["version"] = "1.0";
    exportJson["export_metadata"]["source"] = "ConsensusConfigManager";
    
    return exportJson;
}

nlohmann::json ConsensusConfigManager::exportConfiguration(ConsensusType mechanism) const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    ConsensusConfig filtered = getConfiguration(mechanism);
    nlohmann::json exportJson = filtered.toJson();
    exportJson["export_metadata"]["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    exportJson["export_metadata"]["version"] = "1.0";
    exportJson["export_metadata"]["mechanism"] = static_cast<int>(mechanism);
    exportJson["export_metadata"]["source"] = "ConsensusConfigManager";
    
    return exportJson;
}

bool ConsensusConfigManager::importConfiguration(const nlohmann::json& configJson, const std::string& source) {
    try {
        ConsensusConfig importedConfig;
        importedConfig.fromJson(configJson);
        
        return setConfiguration(importedConfig, source);
        
    } catch (const std::exception& e) {
        logger->error("Failed to import configuration: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> ConsensusConfigManager::compareConfigurations(const ConsensusConfig& config1,
                                                                      const ConsensusConfig& config2) const {
    std::vector<std::string> differences;
    
    if (config1.powDifficulty != config2.powDifficulty) {
        differences.push_back("powDifficulty: " + std::to_string(config1.powDifficulty) + 
                            " -> " + std::to_string(config2.powDifficulty));
    }
    
    if (config1.powTargetBlockTime != config2.powTargetBlockTime) {
        differences.push_back("powTargetBlockTime: " + std::to_string(config1.powTargetBlockTime) + 
                            " -> " + std::to_string(config2.powTargetBlockTime));
    }
    
    if (config1.minStakeAmount != config2.minStakeAmount) {
        differences.push_back("minStakeAmount: " + std::to_string(config1.minStakeAmount) + 
                            " -> " + std::to_string(config2.minStakeAmount));
    }
    
    if (config1.stakingPeriod != config2.stakingPeriod) {
        differences.push_back("stakingPeriod: " + std::to_string(config1.stakingPeriod) + 
                            " -> " + std::to_string(config2.stakingPeriod));
    }
    
    if (config1.minResourceContribution != config2.minResourceContribution) {
        differences.push_back("minResourceContribution: " + std::to_string(config1.minResourceContribution) + 
                            " -> " + std::to_string(config2.minResourceContribution));
    }
    
    if (config1.supermajorityThreshold != config2.supermajorityThreshold) {
        differences.push_back("supermajorityThreshold: " + std::to_string(config1.supermajorityThreshold) + 
                            " -> " + std::to_string(config2.supermajorityThreshold));
    }
    
    if (config1.votingPeriod != config2.votingPeriod) {
        differences.push_back("votingPeriod: " + std::to_string(config1.votingPeriod) + 
                            " -> " + std::to_string(config2.votingPeriod));
    }
    
    if (config1.maxDominanceRatio != config2.maxDominanceRatio) {
        differences.push_back("maxDominanceRatio: " + std::to_string(config1.maxDominanceRatio) + 
                            " -> " + std::to_string(config2.maxDominanceRatio));
    }
    
    if (config1.rebalancingInterval != config2.rebalancingInterval) {
        differences.push_back("rebalancingInterval: " + std::to_string(config1.rebalancingInterval) + 
                            " -> " + std::to_string(config2.rebalancingInterval));
    }
    
    return differences;
}

nlohmann::json ConsensusConfigManager::getStatus() const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    nlohmann::json status;
    status["initialized"] = true;
    status["config_file"] = configFilePath;
    status["backup_directory"] = backupDirectory;
    status["backup_count"] = configHistory.size();
    status["change_log_size"] = changeLog.size();
    status["registered_parameters"] = parameterRegistry.size();
    status["safety_bounds_count"] = safetyBounds.size();
    status["callback_count"] = changeCallbacks.size();
    
    return status;
}

nlohmann::json ConsensusConfigManager::getDiagnostics() const {
    std::lock_guard<std::mutex> lock(configMutex);
    
    nlohmann::json diagnostics;
    
    // Configuration validation
    auto validationResult = validateConfiguration(currentConfig);
    diagnostics["configuration_valid"] = validationResult.isValid;
    diagnostics["validation_errors"] = validationResult.errors;
    diagnostics["validation_warnings"] = validationResult.warnings;
    
    // File system checks
    diagnostics["config_file_exists"] = std::filesystem::exists(configFilePath);
    diagnostics["backup_directory_exists"] = std::filesystem::exists(backupDirectory);
    
    // Recent changes
    if (!changeLog.empty()) {
        diagnostics["last_change_timestamp"] = changeLog.back().timestamp;
        diagnostics["recent_changes"] = std::min(static_cast<size_t>(10), changeLog.size());
    }
    
    return diagnostics;
}

bool ConsensusConfigManager::performSelfTest() {
    try {
        logger->info("Performing self-test...");
        
        // Test configuration validation
        auto testConfig = getDefaultConfiguration();
        auto validationResult = validateConfiguration(testConfig);
        if (!validationResult.isValid) {
            logger->error("Self-test failed: Default configuration is invalid");
            return false;
        }
        
        // Test backup creation
        if (!createBackup("self_test")) {
            logger->error("Self-test failed: Cannot create backup");
            return false;
        }
        
        // Test file operations
        std::string testPath = backupDirectory + "/self_test.json";
        if (!saveConfiguration(testPath)) {
            logger->error("Self-test failed: Cannot save configuration");
            return false;
        }
        
        // Cleanup test file
        std::filesystem::remove(testPath);
        
        logger->info("Self-test completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Self-test failed with exception: " + std::string(e.what()));
        return false;
    }
}

// Static methods for default configurations
ConsensusConfig ConsensusConfigManager::getDefaultConfiguration() {
    ConsensusConfig config;
    // Default values are already set in the struct definition
    return config;
}

ConsensusConfig ConsensusConfigManager::getTestConfiguration() {
    ConsensusConfig config;
    
    // Test-friendly values
    config.powDifficulty = 1;
    config.powTargetBlockTime = 10;
    config.minStakeAmount = 10.0;
    config.stakingPeriod = 60;
    config.minResourceContribution = 1.0;
    config.supermajorityThreshold = 0.6;
    config.votingPeriod = 300;
    config.maxDominanceRatio = 0.8;
    config.rebalancingInterval = 60;
    
    return config;
}

ConsensusConfig ConsensusConfigManager::getProductionConfiguration() {
    ConsensusConfig config;
    
    // Production-ready values
    config.powDifficulty = 6;
    config.powTargetBlockTime = 600;
    config.minStakeAmount = 10000.0;
    config.stakingPeriod = 604800; // 7 days
    config.minResourceContribution = 1000.0;
    config.supermajorityThreshold = 0.67;
    config.votingPeriod = 1209600; // 14 days
    config.maxDominanceRatio = 0.5;
    config.rebalancingInterval = 3600; // 1 hour
    
    return config;
}

// Private helper methods implementation

void ConsensusConfigManager::initializeParameterRegistry() {
    // Note: This method is called from within a locked context, so we directly modify the registry
    // PoW parameters
    parameterRegistry["powDifficulty"] = ConfigParameter("powDifficulty", "uint64", 
                                    ConfigBounds(1, 20, true), "pow",
                                    "Proof of Work mining difficulty", true, false);
    parameterRegistry["powTargetBlockTime"] = ConfigParameter("powTargetBlockTime", "uint64",
                                    ConfigBounds(10, 3600, true), "pow",
                                    "Target time between blocks in seconds", true, false);
    
    // PoS parameters
    parameterRegistry["minStakeAmount"] = ConfigParameter("minStakeAmount", "double",
                                    ConfigBounds(1.0, 1000000.0, true), "pos",
                                    "Minimum stake amount required for validation", true, false);
    parameterRegistry["stakingPeriod"] = ConfigParameter("stakingPeriod", "uint64",
                                    ConfigBounds(3600, 31536000, true), "pos",
                                    "Staking period in seconds", true, false);
    
    // PoRC parameters
    parameterRegistry["minResourceContribution"] = ConfigParameter("minResourceContribution", "double",
                                    ConfigBounds(1.0, 100000.0, true), "porc",
                                    "Minimum resource contribution required", true, false);
    
    // Voting parameters
    parameterRegistry["supermajorityThreshold"] = ConfigParameter("supermajorityThreshold", "double",
                                    ConfigBounds(0.51, 1.0, true), "voting",
                                    "Supermajority threshold for governance decisions", true, false);
    parameterRegistry["votingPeriod"] = ConfigParameter("votingPeriod", "uint64",
                                    ConfigBounds(3600, 2592000, true), "voting",
                                    "Voting period in seconds", true, false);
    
    // General balancing parameters
    parameterRegistry["maxDominanceRatio"] = ConfigParameter("maxDominanceRatio", "double",
                                    ConfigBounds(0.1, 0.9, true), "general",
                                    "Maximum dominance ratio for any consensus mechanism", true, false);
    parameterRegistry["rebalancingInterval"] = ConfigParameter("rebalancingInterval", "uint64",
                                    ConfigBounds(60, 86400, true), "general",
                                    "Interval between rebalancing operations in seconds", true, false);
}

void ConsensusConfigManager::initializeSafetyBounds() {
    // Note: This method is called from within a locked context, so we directly modify the bounds
    // PoW safety bounds
    safetyBounds[ConsensusType::PROOF_OF_WORK]["powDifficulty"] = 
        ConfigBounds(1, 15, true, "Safe difficulty range");
    safetyBounds[ConsensusType::PROOF_OF_WORK]["powTargetBlockTime"] = 
        ConfigBounds(30, 1800, true, "Safe block time range");
    
    // PoS safety bounds
    safetyBounds[ConsensusType::PROOF_OF_STAKE]["minStakeAmount"] = 
        ConfigBounds(10.0, 100000.0, true, "Safe stake amount range");
    safetyBounds[ConsensusType::PROOF_OF_STAKE]["stakingPeriod"] = 
        ConfigBounds(3600, 2592000, true, "Safe staking period range");
    
    // PoRC safety bounds
    safetyBounds[ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION]["minResourceContribution"] = 
        ConfigBounds(10.0, 10000.0, true, "Safe resource contribution range");
    
    // Voting safety bounds
    safetyBounds[ConsensusType::VOTING_CONSENSUS]["supermajorityThreshold"] = 
        ConfigBounds(0.55, 0.95, true, "Safe supermajority threshold range");
    safetyBounds[ConsensusType::VOTING_CONSENSUS]["votingPeriod"] = 
        ConfigBounds(86400, 1209600, true, "Safe voting period range");
}

void ConsensusConfigManager::notifyConfigChange(const ConfigChangeEvent& event) {
    for (const auto& callback : changeCallbacks) {
        try {
            callback(event);
        } catch (const std::exception& e) {
            logger->error("Error in change callback: " + std::string(e.what()));
        }
    }
}

bool ConsensusConfigManager::validateConfigurationInternal(const ConsensusConfig& config,
                                                         ConfigValidationResult& result) const {
    // Validate PoW parameters
    if (config.powDifficulty < 1 || config.powDifficulty > 20) {
        result.addError("PoW difficulty must be between 1 and 20");
    }
    
    if (config.powTargetBlockTime < 10 || config.powTargetBlockTime > 3600) {
        result.addError("PoW target block time must be between 10 and 3600 seconds");
    }
    
    // Validate PoS parameters
    if (config.minStakeAmount <= 0) {
        result.addError("Minimum stake amount must be positive");
    }
    
    if (config.stakingPeriod < 3600) {
        result.addWarning("Staking period less than 1 hour may reduce security");
    }
    
    // Validate PoRC parameters
    if (config.minResourceContribution <= 0) {
        result.addError("Minimum resource contribution must be positive");
    }
    
    if (config.acceptedResourceTypes.empty()) {
        result.addWarning("No accepted resource types specified");
    }
    
    // Validate voting parameters
    if (config.supermajorityThreshold <= 0.5 || config.supermajorityThreshold > 1.0) {
        result.addError("Supermajority threshold must be between 0.5 and 1.0");
    }
    
    if (config.votingPeriod < 3600) {
        result.addWarning("Voting period less than 1 hour may not allow sufficient participation");
    }
    
    // Validate balancing parameters
    if (config.maxDominanceRatio <= 0 || config.maxDominanceRatio >= 1.0) {
        result.addError("Max dominance ratio must be between 0 and 1");
    }
    
    if (config.rebalancingInterval < 60) {
        result.addWarning("Rebalancing interval less than 1 minute may cause instability");
    }
    
    // Validate consensus priority
    if (config.consensusPriority.empty()) {
        result.addError("Consensus priority mapping cannot be empty");
    }
    
    // Check for duplicate priorities
    std::set<int> priorities;
    for (const auto& [type, priority] : config.consensusPriority) {
        if (priorities.count(priority)) {
            result.addError("Duplicate consensus priority: " + std::to_string(priority));
        }
        priorities.insert(priority);
    }
    
    return result.isValid;
}

std::string ConsensusConfigManager::generateBackupFileName() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "consensus_config_backup_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".json";
    return ss.str();
}

bool ConsensusConfigManager::ensureDirectoryExists(const std::string& directory) const {
    try {
        if (!std::filesystem::exists(directory)) {
            return std::filesystem::create_directories(directory);
        }
        return true;
    } catch (const std::exception& e) {
        logger->error("Failed to create directory " + directory + ": " + std::string(e.what()));
        return false;
    }
}

void ConsensusConfigManager::logConfigChange(const ConfigChangeEvent& event) const {
    std::stringstream ss;
    ss << "Configuration change - Parameter: " << event.parameter
       << ", Old: " << event.oldValue
       << ", New: " << event.newValue
       << ", Source: " << event.source;
    logger->info(ss.str());
}

ConsensusConfig ConsensusConfigManager::mergeConfigurations(const ConsensusConfig& base,
                                                          const ConsensusConfig& updates) const {
    ConsensusConfig merged = base;
    
    // This is a simplified merge - in practice, you might want more sophisticated merging logic
    // For now, we'll just overwrite non-zero/non-empty values from updates
    
    if (updates.powDifficulty != 0) {
        merged.powDifficulty = updates.powDifficulty;
    }
    
    if (updates.powTargetBlockTime != 0) {
        merged.powTargetBlockTime = updates.powTargetBlockTime;
    }
    
    if (updates.minStakeAmount != 0.0) {
        merged.minStakeAmount = updates.minStakeAmount;
    }
    
    if (updates.stakingPeriod != 0) {
        merged.stakingPeriod = updates.stakingPeriod;
    }
    
    if (updates.minResourceContribution != 0.0) {
        merged.minResourceContribution = updates.minResourceContribution;
    }
    
    if (!updates.acceptedResourceTypes.empty()) {
        merged.acceptedResourceTypes = updates.acceptedResourceTypes;
    }
    
    if (updates.supermajorityThreshold != 0.0) {
        merged.supermajorityThreshold = updates.supermajorityThreshold;
    }
    
    if (updates.votingPeriod != 0) {
        merged.votingPeriod = updates.votingPeriod;
    }
    
    if (updates.maxDominanceRatio != 0.0) {
        merged.maxDominanceRatio = updates.maxDominanceRatio;
    }
    
    if (updates.rebalancingInterval != 0) {
        merged.rebalancingInterval = updates.rebalancingInterval;
    }
    
    if (!updates.consensusPriority.empty()) {
        merged.consensusPriority = updates.consensusPriority;
    }
    
    return merged;
}

// Governance integration methods (Requirements 3.2, 3.5)
bool ConsensusConfigManager::applyGovernanceDecision(const std::string& proposalId, 
                                                   const nlohmann::json& parameters, 
                                                   const std::string& source) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        logger->info("Applying governance decision: " + proposalId);
        
        // Validate governance parameters
        if (!validateGovernanceParameters(parameters)) {
            logger->error("Governance parameters validation failed for proposal: " + proposalId);
            return false;
        }
        
        // Create backup before applying changes
        createBackup("before_governance_" + proposalId);
        
        // Create updated configuration
        ConsensusConfig updatedConfig = currentConfig;
        
        // Apply parameter changes from governance decision
        for (const auto& [param, value] : parameters.items()) {
            if (param == "powDifficulty" && value.is_number_unsigned()) {
                updatedConfig.powDifficulty = value.get<uint64_t>();
            } else if (param == "powTargetBlockTime" && value.is_number_unsigned()) {
                updatedConfig.powTargetBlockTime = value.get<uint64_t>();
            } else if (param == "minStakeAmount" && value.is_number()) {
                updatedConfig.minStakeAmount = value.get<double>();
            } else if (param == "stakingPeriod" && value.is_number_unsigned()) {
                updatedConfig.stakingPeriod = value.get<uint64_t>();
            } else if (param == "minResourceContribution" && value.is_number()) {
                updatedConfig.minResourceContribution = value.get<double>();
            } else if (param == "supermajorityThreshold" && value.is_number()) {
                updatedConfig.supermajorityThreshold = value.get<double>();
            } else if (param == "votingPeriod" && value.is_number_unsigned()) {
                updatedConfig.votingPeriod = value.get<uint64_t>();
            } else if (param == "maxDominanceRatio" && value.is_number()) {
                updatedConfig.maxDominanceRatio = value.get<double>();
            } else if (param == "rebalancingInterval" && value.is_number_unsigned()) {
                updatedConfig.rebalancingInterval = value.get<uint64_t>();
            } else if (param == "acceptedResourceTypes" && value.is_array()) {
                updatedConfig.acceptedResourceTypes = value.get<std::vector<std::string>>();
            } else {
                logger->warning("Unknown governance parameter: " + param);
            }
        }
        
        // Ensure backward compatibility
        if (!ensureBackwardCompatibility(updatedConfig)) {
            logger->error("Governance decision would break backward compatibility");
            auto issues = getCompatibilityIssues(updatedConfig);
            for (const auto& issue : issues) {
                logger->error("Compatibility issue: " + issue);
            }
            return false;
        }
        
        // Validate the updated configuration
        auto validationResult = validateConfiguration(updatedConfig);
        if (!validationResult.isValid) {
            logger->error("Governance decision results in invalid configuration:");
            for (const auto& error : validationResult.errors) {
                logger->error("  - " + error);
            }
            return false;
        }
        
        // Apply the configuration
        ConsensusConfig oldConfig = currentConfig;
        currentConfig = updatedConfig;
        
        // Track changes
        for (const auto& [param, value] : parameters.items()) {
            ConfigChangeEvent event(param, "governance_old", "governance_new", 
                                  ConsensusType::VOTING_CONSENSUS, source + "_" + proposalId);
            changeLog.push_back(event);
            notifyConfigChange(event);
            logConfigChange(event);
        }
        
        // Trim change log if necessary
        if (changeLog.size() > maxChangeLogSize) {
            changeLog.erase(changeLog.begin(), 
                          changeLog.begin() + (changeLog.size() - maxChangeLogSize));
        }
        
        // Save configuration
        if (!saveConfiguration()) {
            logger->warning("Failed to save configuration after governance decision");
        }
        
        logger->info("Governance decision applied successfully: " + proposalId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Failed to apply governance decision: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::validateGovernanceParameters(const nlohmann::json& parameters) const {
    try {
        // Check that all parameters are recognized and have valid types
        for (const auto& [param, value] : parameters.items()) {
            // Check if parameter exists in registry
            auto it = parameterRegistry.find(param);
            if (it == parameterRegistry.end()) {
                logger->error("Unknown governance parameter: " + param);
                return false;
            }
            
            const ConfigParameter& paramInfo = it->second;
            
            // Validate parameter type and bounds
            if (paramInfo.type == "double" && value.is_number()) {
                double val = value.get<double>();
                if (!paramInfo.bounds.isValid(val)) {
                    logger->error("Governance parameter " + param + " value " + 
                                std::to_string(val) + " is outside valid bounds");
                    return false;
                }
            } else if (paramInfo.type == "uint64" && value.is_number_unsigned()) {
                uint64_t val = value.get<uint64_t>();
                if (!paramInfo.bounds.isValid(static_cast<double>(val))) {
                    logger->error("Governance parameter " + param + " value " + 
                                std::to_string(val) + " is outside valid bounds");
                    return false;
                }
            } else if (paramInfo.type == "string" && !value.is_string()) {
                logger->error("Governance parameter " + param + " must be a string");
                return false;
            } else if (paramInfo.type == "array" && !value.is_array()) {
                logger->error("Governance parameter " + param + " must be an array");
                return false;
            } else if (paramInfo.type == "bool" && !value.is_boolean()) {
                logger->error("Governance parameter " + param + " must be a boolean");
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Error validating governance parameters: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusConfigManager::ensureBackwardCompatibility(const ConsensusConfig& newConfig) const {
    auto issues = getCompatibilityIssues(newConfig);
    return issues.empty();
}

std::vector<std::string> ConsensusConfigManager::getCompatibilityIssues(const ConsensusConfig& newConfig) const {
    std::vector<std::string> issues;
    
    try {
        // Check for breaking changes that would affect existing functionality
        
        // PoW compatibility checks
        if (newConfig.powDifficulty > currentConfig.powDifficulty * 2) {
            issues.push_back("PoW difficulty increase > 100% may cause mining disruption");
        }
        
        if (newConfig.powTargetBlockTime < currentConfig.powTargetBlockTime * 0.5) {
            issues.push_back("PoW target block time reduction > 50% may cause network instability");
        }
        
        // PoS compatibility checks
        if (newConfig.minStakeAmount > currentConfig.minStakeAmount * 5) {
            issues.push_back("Minimum stake amount increase > 400% may exclude existing validators");
        }
        
        if (newConfig.stakingPeriod > currentConfig.stakingPeriod * 2) {
            issues.push_back("Staking period increase > 100% may affect validator participation");
        }
        
        // Voting compatibility checks
        if (newConfig.supermajorityThreshold > currentConfig.supermajorityThreshold + 0.1) {
            issues.push_back("Supermajority threshold increase > 10% may make governance decisions harder");
        }
        
        if (newConfig.votingPeriod < currentConfig.votingPeriod * 0.5) {
            issues.push_back("Voting period reduction > 50% may not allow sufficient participation");
        }
        
        // Resource contribution compatibility checks
        if (newConfig.minResourceContribution > currentConfig.minResourceContribution * 3) {
            issues.push_back("Resource contribution requirement increase > 200% may exclude contributors");
        }
        
        // Check for removed resource types
        for (const auto& currentType : currentConfig.acceptedResourceTypes) {
            if (std::find(newConfig.acceptedResourceTypes.begin(), 
                         newConfig.acceptedResourceTypes.end(), currentType) == 
                newConfig.acceptedResourceTypes.end()) {
                issues.push_back("Removing resource type '" + currentType + "' may affect existing contributors");
            }
        }
        
        // Balancing compatibility checks
        if (newConfig.maxDominanceRatio < currentConfig.maxDominanceRatio - 0.2) {
            issues.push_back("Max dominance ratio reduction > 20% may cause consensus mechanism conflicts");
        }
        
        if (newConfig.rebalancingInterval < currentConfig.rebalancingInterval * 0.1) {
            issues.push_back("Rebalancing interval reduction > 90% may cause excessive rebalancing");
        }
        
    } catch (const std::exception& e) {
        issues.push_back("Error checking compatibility: " + std::string(e.what()));
    }
    
    return issues;
}

// Explicit template instantiations for common types
template bool ConsensusConfigManager::setParameterInternal<double>(const std::string&, double, ConsensusType, const std::string&);
template bool ConsensusConfigManager::setParameterInternal<uint64_t>(const std::string&, uint64_t, ConsensusType, const std::string&);
template bool ConsensusConfigManager::setParameterInternal<std::string>(const std::string&, std::string, ConsensusType, const std::string&);
template bool ConsensusConfigManager::setParameterInternal<bool>(const std::string&, bool, ConsensusType, const std::string&);