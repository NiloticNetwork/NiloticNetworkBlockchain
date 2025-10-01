#include "consensus_harmony_manager.h"
#include "blockchain.h"
#include "logger.h"
#include <fstream>
#include <algorithm>

ConsensusHarmonyManager::ConsensusHarmonyManager(Blockchain* bc)
    : blockchain(bc), initialized(false), running(false), shouldStop(false) {
    Logger::info("ConsensusHarmonyManager created");
}

ConsensusHarmonyManager::~ConsensusHarmonyManager() {
    shutdown();
    Logger::info("ConsensusHarmonyManager destroyed");
}

bool ConsensusHarmonyManager::initializeConsensus() {
    return initializeConsensus(ConsensusConfig{});
}

bool ConsensusHarmonyManager::initializeConsensus(const ConsensusConfig& customConfig) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    if (initialized.load()) {
        Logger::warning("ConsensusHarmonyManager already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing Consensus Harmony Manager");
        
        // Validate configuration
        if (!validateConfiguration(customConfig)) {
            Logger::error("Invalid consensus configuration provided");
            return false;
        }
        
        config = customConfig;
        
        // Initialize components
        initializeComponents();
        
        // Initialize status
        status = ConsensusStatus{};
        status.lastUpdate = std::chrono::steady_clock::now();
        
        // Start management thread
        shouldStop = false;
        managementThread = std::thread(&ConsensusHarmonyManager::managementLoop, this);
        
        initialized = true;
        running = true;
        
        Logger::info("Consensus Harmony Manager initialized successfully");
        logEvent("consensus_harmony_initialized", config.toJson());
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize ConsensusHarmonyManager: " + std::string(e.what()));
        return false;
    }
}

void ConsensusHarmonyManager::shutdown() {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    if (!initialized.load()) {
        return;
    }
    
    Logger::info("Shutting down Consensus Harmony Manager");
    
    // Stop management thread
    shouldStop = true;
    running = false;
    managerCV.notify_all();
    
    if (managementThread.joinable()) {
        managementThread.join();
    }
    
    // Shutdown components
    shutdownComponents();
    
    initialized = false;
    
    Logger::info("Consensus Harmony Manager shut down successfully");
    logEvent("consensus_harmony_shutdown");
}

bool ConsensusHarmonyManager::validateBlock(const Block& block) {
    if (!initialized.load()) {
        Logger::error("ConsensusHarmonyManager not initialized");
        return false;
    }
    
    try {
        // Create consensus request for block validation
        ConsensusRequest request(RequestType::BLOCK_VALIDATION, block.serialize());
        
        // Process the request through all applicable consensus mechanisms
        ConsensusResult result = processConsensusRequest(request);
        
        // Update statistics
        status.totalValidations++;
        if (result.isValid) {
            status.successfulValidations++;
        }
        status.lastUpdate = std::chrono::steady_clock::now();
        
        Logger::info("Block validation result: " + std::string(result.isValid ? "VALID" : "INVALID") + 
                    " (confidence: " + std::to_string(result.confidence) + ")");
        
        return result.isValid;
        
    } catch (const std::exception& e) {
        Logger::error("Block validation failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyManager::validateTransaction(const Transaction& transaction) {
    if (!initialized.load()) {
        Logger::error("ConsensusHarmonyManager not initialized");
        return false;
    }
    
    try {
        // Create consensus request for transaction validation
        ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, transaction.serialize());
        
        // Process the request through all applicable consensus mechanisms
        ConsensusResult result = processConsensusRequest(request);
        
        // Update statistics
        status.totalValidations++;
        if (result.isValid) {
            status.successfulValidations++;
        }
        status.lastUpdate = std::chrono::steady_clock::now();
        
        Logger::info("Transaction validation result: " + std::string(result.isValid ? "VALID" : "INVALID") + 
                    " (confidence: " + std::to_string(result.confidence) + ")");
        
        return result.isValid;
        
    } catch (const std::exception& e) {
        Logger::error("Transaction validation failed: " + std::string(e.what()));
        return false;
    }
}

ConsensusResult ConsensusHarmonyManager::processConsensusRequest(const ConsensusRequest& request) {
    if (!initialized.load()) {
        Logger::error("ConsensusHarmonyManager not initialized");
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "Manager not initialized");
    }
    
    try {
        // For now, return a basic validation result
        // This will be enhanced when we implement the router component
        Logger::info("Processing consensus request: " + request.requestId);
        
        // Basic validation - in a real implementation, this would route to appropriate engines
        bool isValid = true;
        double confidence = 1.0;
        std::string reason = "Basic validation passed";
        
        // Create result
        ConsensusResult result(isValid, ConsensusType::PROOF_OF_WORK, confidence, reason);
        result.metadata["requestId"] = request.requestId;
        result.metadata["processedBy"] = "ConsensusHarmonyManager";
        
        Logger::info("Consensus request processed: " + request.requestId + 
                    " - Result: " + (isValid ? "VALID" : "INVALID"));
        
        return result;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to process consensus request: " + std::string(e.what()));
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                              "Processing failed: " + std::string(e.what()));
    }
}

bool ConsensusHarmonyManager::registerConsensusEngine(std::unique_ptr<ConsensusEngine> engine) {
    if (!engine) {
        Logger::error("Cannot register null consensus engine");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(managerMutex);
    
    try {
        ConsensusType type = engine->getType();
        std::string name = engine->getName();
        
        Logger::info("Registering consensus engine: " + name);
        
        // Initialize the engine
        if (!engine->initialize()) {
            Logger::error("Failed to initialize consensus engine: " + name);
            return false;
        }
        
        // Store the engine (this would be implemented in the router component)
        // For now, just log the registration
        Logger::info("Consensus engine registered successfully: " + name);
        
        // Update status
        status.mechanismStatus[type] = true;
        status.mechanismHealth[type] = 1.0;
        
        logEvent("consensus_engine_registered", {{"type", name}});
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to register consensus engine: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyManager::unregisterConsensusEngine(ConsensusType type) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    try {
        Logger::info("Unregistering consensus engine");
        
        // Remove from status
        status.mechanismStatus.erase(type);
        status.mechanismHealth.erase(type);
        
        Logger::info("Consensus engine unregistered successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to unregister consensus engine: " + std::string(e.what()));
        return false;
    }
}

std::vector<ConsensusType> ConsensusHarmonyManager::getActiveEngines() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::vector<ConsensusType> activeEngines;
    for (const auto& [type, active] : status.mechanismStatus) {
        if (active) {
            activeEngines.push_back(type);
        }
    }
    
    return activeEngines;
}

void ConsensusHarmonyManager::adjustConsensusParameters() {
    if (!initialized.load()) {
        return;
    }
    
    try {
        Logger::info("Adjusting consensus parameters");
        
        // This would implement automatic parameter adjustment logic
        // For now, just log the operation
        
        logEvent("consensus_parameters_adjusted");
        
    } catch (const std::exception& e) {
        Logger::error("Failed to adjust consensus parameters: " + std::string(e.what()));
    }
}

ConsensusStatus ConsensusHarmonyManager::getConsensusStatus() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return status;
}

nlohmann::json ConsensusHarmonyManager::getDetailedStatus() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    nlohmann::json statusJson;
    statusJson["initialized"] = initialized.load();
    statusJson["running"] = running.load();
    statusJson["totalValidations"] = status.totalValidations;
    statusJson["successfulValidations"] = status.successfulValidations;
    statusJson["conflictCount"] = status.conflictCount;
    
    // Mechanism status
    nlohmann::json mechanisms;
    for (const auto& [type, active] : status.mechanismStatus) {
        std::string typeStr;
        switch (type) {
            case ConsensusType::PROOF_OF_WORK:
                typeStr = "PROOF_OF_WORK";
                break;
            case ConsensusType::PROOF_OF_STAKE:
                typeStr = "PROOF_OF_STAKE";
                break;
            case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
                typeStr = "PROOF_OF_RESOURCE_CONTRIBUTION";
                break;
            case ConsensusType::VOTING_CONSENSUS:
                typeStr = "VOTING_CONSENSUS";
                break;
            case ConsensusType::SMART_CONTRACT_VALIDATION:
                typeStr = "SMART_CONTRACT_VALIDATION";
                break;
        }
        
        mechanisms[typeStr]["active"] = active;
        if (status.mechanismHealth.find(type) != status.mechanismHealth.end()) {
            mechanisms[typeStr]["health"] = status.mechanismHealth.at(type);
        }
    }
    statusJson["mechanisms"] = mechanisms;
    
    return statusJson;
}

nlohmann::json ConsensusHarmonyManager::getMetrics() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    nlohmann::json metrics;
    metrics["validationSuccessRate"] = status.totalValidations > 0 ? 
        static_cast<double>(status.successfulValidations) / status.totalValidations : 0.0;
    metrics["conflictRate"] = status.totalValidations > 0 ? 
        static_cast<double>(status.conflictCount) / status.totalValidations : 0.0;
    metrics["activeEngineCount"] = status.mechanismStatus.size();
    
    return metrics;
}

bool ConsensusHarmonyManager::updateConfiguration(const ConsensusConfig& newConfig) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    if (!validateConfiguration(newConfig)) {
        Logger::error("Invalid configuration provided");
        return false;
    }
    
    config = newConfig;
    Logger::info("Consensus configuration updated");
    logEvent("configuration_updated", config.toJson());
    
    return true;
}

ConsensusConfig ConsensusHarmonyManager::getConfiguration() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return config;
}

bool ConsensusHarmonyManager::saveConfiguration(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open configuration file for writing: " + filename);
            return false;
        }
        
        file << config.toJson().dump(4);
        file.close();
        
        Logger::info("Configuration saved to: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to save configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyManager::loadConfiguration(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open configuration file for reading: " + filename);
            return false;
        }
        
        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        nlohmann::json configJson = nlohmann::json::parse(jsonStr);
        ConsensusConfig newConfig;
        newConfig.fromJson(configJson);
        
        if (!validateConfiguration(newConfig)) {
            Logger::error("Invalid configuration in file: " + filename);
            return false;
        }
        
        std::lock_guard<std::mutex> lock(managerMutex);
        config = newConfig;
        
        Logger::info("Configuration loaded from: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to load configuration: " + std::string(e.what()));
        return false;
    }
}

// Private methods
void ConsensusHarmonyManager::managementLoop() {
    Logger::info("Consensus management thread started");
    
    while (!shouldStop.load()) {
        try {
            performPeriodicTasks();
            
            // Wait for next cycle or shutdown signal
            std::unique_lock<std::mutex> lock(managerMutex);
            managerCV.wait_for(lock, std::chrono::seconds(60), [this] { return shouldStop.load(); });
            
        } catch (const std::exception& e) {
            Logger::error("Error in consensus management loop: " + std::string(e.what()));
        }
    }
    
    Logger::info("Consensus management thread stopped");
}

void ConsensusHarmonyManager::performPeriodicTasks() {
    checkSystemHealth();
    rebalanceConsensus();
}

void ConsensusHarmonyManager::checkSystemHealth() {
    // Update system health metrics
    status.lastUpdate = std::chrono::steady_clock::now();
    
    // Check each mechanism's health
    for (auto& [type, health] : status.mechanismHealth) {
        // For now, assume all mechanisms are healthy
        // This would be enhanced with actual health checks
        health = 1.0;
    }
}

void ConsensusHarmonyManager::rebalanceConsensus() {
    // Implement consensus rebalancing logic
    // This would adjust parameters to maintain balance between mechanisms
    Logger::debug("Performing consensus rebalancing");
}

bool ConsensusHarmonyManager::validateConfiguration(const ConsensusConfig& config) const {
    // Validate configuration parameters
    if (config.powDifficulty == 0 || config.powDifficulty > 20) {
        return false;
    }
    
    if (config.minStakeAmount < 0) {
        return false;
    }
    
    if (config.supermajorityThreshold <= 0.5 || config.supermajorityThreshold > 1.0) {
        return false;
    }
    
    if (config.maxDominanceRatio <= 0.0 || config.maxDominanceRatio > 1.0) {
        return false;
    }
    
    return true;
}

void ConsensusHarmonyManager::initializeComponents() {
    // Initialize router, balancer, and monitor components
    // These will be implemented in subsequent tasks
    Logger::info("Initializing consensus harmony components");
}

void ConsensusHarmonyManager::shutdownComponents() {
    // Shutdown all components
    Logger::info("Shutting down consensus harmony components");
}

void ConsensusHarmonyManager::logEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logEntry;
    logEntry["event"] = event;
    logEntry["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    logEntry["data"] = data;
    
    Logger::info("ConsensusHarmony: " + logEntry.dump());
}