#include "../../include/core/consensus_harmony_manager.h"
#include "../../include/core/blockchain.h"
#include "../../include/core/logger.h"
#include "../../include/core/block.h"
#include "../../include/core/transaction.h"
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
        
        // Log security initialization
        if (securityAuditor) {
            securityAuditor->logSystemEvent("Consensus Harmony Manager initialized", AuditSeverity::INFO);
        }
        
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
    if (!initialized.load()) {
        return;
    }
    
    Logger::info("Shutting down Consensus Harmony Manager");
    
    // Stop management thread - do this without holding the lock
    shouldStop = true;
    running = false;
    managerCV.notify_all();
    
    if (managementThread.joinable()) {
        managementThread.join();
    }
    
    // Now acquire lock for final cleanup
    std::lock_guard<std::mutex> lock(managerMutex);
    
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
        // First perform security validation
        SecurityValidationResult securityResult = validateBlockSecurity(block);
        if (!securityResult.isSecure) {
            Logger::warning("Block failed security validation");
            
            // Log security violation
            if (securityAuditor) {
                for (const auto& threat : securityResult.threats) {
                    securityAuditor->logSecurityViolation(threat, "block_validation");
                }
            }
            
            return false;
        }
        
        // Create consensus request for block validation
        ConsensusRequest request(RequestType::BLOCK_VALIDATION, block.serialize());
        request.metadata["source"] = "consensus_harmony_manager";
        
        // Validate the consensus request security
        SecurityValidationResult requestSecurity = validateSecurity(request);
        if (!requestSecurity.isSecure) {
            Logger::warning("Consensus request failed security validation");
            return false;
        }
        
        // Process the request through all applicable consensus mechanisms
        ConsensusResult result = processConsensusRequest(request);
        
        // Log consensus validation
        if (securityAuditor) {
            securityAuditor->logConsensusValidation(request, result);
        }
        
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
        
        // Log the exception as a security event
        if (securityAuditor) {
            nlohmann::json details;
            details["exception"] = e.what();
            securityAuditor->logSecurityViolation("block_validation_exception", "consensus_harmony_manager", details);
        }
        
        return false;
    }
}

bool ConsensusHarmonyManager::validateTransaction(const Transaction& transaction) {
    if (!initialized.load()) {
        Logger::error("ConsensusHarmonyManager not initialized");
        return false;
    }
    
    try {
        // First perform security validation
        SecurityValidationResult securityResult = validateTransactionSecurity(transaction);
        if (!securityResult.isSecure) {
            Logger::warning("Transaction failed security validation");
            
            // Log security violation
            if (securityAuditor) {
                for (const auto& threat : securityResult.threats) {
                    securityAuditor->logSecurityViolation(threat, "transaction_validation");
                }
            }
            
            return false;
        }
        
        // Create consensus request for transaction validation
        ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, transaction.serialize());
        request.metadata["source"] = "consensus_harmony_manager";
        
        // Validate the consensus request security
        SecurityValidationResult requestSecurity = validateSecurity(request);
        if (!requestSecurity.isSecure) {
            Logger::warning("Consensus request failed security validation");
            return false;
        }
        
        // Process the request through all applicable consensus mechanisms
        ConsensusResult result = processConsensusRequest(request);
        
        // Log consensus validation
        if (securityAuditor) {
            securityAuditor->logConsensusValidation(request, result);
        }
        
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
        
        // Log the exception as a security event
        if (securityAuditor) {
            nlohmann::json details;
            details["exception"] = e.what();
            securityAuditor->logSecurityViolation("transaction_validation_exception", "consensus_harmony_manager", details);
        }
        
        return false;
    }
}

ConsensusResult ConsensusHarmonyManager::processConsensusRequest(const ConsensusRequest& request) {
    if (!initialized.load()) {
        Logger::error("ConsensusHarmonyManager not initialized");
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "Manager not initialized");
    }
    
    try {
        Logger::info("Processing consensus request: " + request.requestId);
        
        // Route the request through the consensus router with performance optimization
        ConsensusResult result;
        if (router && performanceOptimizer) {
            // Get applicable engines from router
            auto engines = router->getApplicableEngines(request);
            
            // Use performance optimizer for validation
            result = performanceOptimizer->optimizedValidation(request, engines);
        } else if (router) {
            // Fallback to regular router validation
            result = router->routeValidation(request);
        } else {
            // Fallback to basic validation if router is not available
            result = ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 1.0, "Basic validation passed");
        }
        
        // Add metadata
        result.metadata["requestId"] = request.requestId;
        result.metadata["processedBy"] = "ConsensusHarmonyManager";
        
        Logger::info("Consensus request processed: " + request.requestId + 
                    " - Result: " + (result.isValid ? "VALID" : "INVALID"));
        
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
        
        // Register the engine with the router if available
        if (router) {
            if (!router->registerEngine(std::move(engine))) {
                Logger::error("Failed to register engine with router: " + name);
                return false;
            }
        } else {
            Logger::warning("Router not available, engine registration deferred: " + name);
        }
        
        // Update status
        status.mechanismStatus[type] = true;
        status.mechanismHealth[type] = 1.0;
        
        logEvent("consensus_engine_registered", {{"type", name}});
        
        Logger::info("Consensus engine registered successfully: " + name);
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
    // Initialize router, balancer, monitor, and security components
    Logger::info("Initializing consensus harmony components");
    
    try {
        // Initialize security components first
        securityValidator = std::make_unique<ConsensusSecurityValidator>();
        if (!securityValidator->initialize()) {
            throw std::runtime_error("Failed to initialize ConsensusSecurityValidator");
        }
        
        securityAuditor = std::make_unique<ConsensusSecurityAuditor>();
        if (!securityAuditor->initialize()) {
            throw std::runtime_error("Failed to initialize ConsensusSecurityAuditor");
        }
        
        // Initialize router
        router = std::make_unique<ConsensusRouter>();
        if (!router->initialize()) {
            throw std::runtime_error("Failed to initialize ConsensusRouter");
        }
        
        // Initialize balancer
        balancer = std::make_unique<ConsensusBalancer>();
        if (!balancer->initialize()) {
            throw std::runtime_error("Failed to initialize ConsensusBalancer");
        }
        
        // Initialize monitor
        monitor = std::make_unique<ConsensusMonitor>();
        if (!monitor->initialize()) {
            throw std::runtime_error("Failed to initialize ConsensusMonitor");
        }
        
        // Initialize emergency mode
        emergencyMode = std::make_unique<EmergencyConsensusMode>(this, blockchain);
        if (!emergencyMode->initialize()) {
            throw std::runtime_error("Failed to initialize EmergencyConsensusMode");
        }
        
        // Initialize performance optimizer
        performanceOptimizer = std::make_unique<ConsensusPerformanceOptimizer>();
        if (!performanceOptimizer->initialize()) {
            throw std::runtime_error("Failed to initialize ConsensusPerformanceOptimizer");
        }
        
        Logger::info("All consensus harmony components initialized successfully");
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize consensus harmony components: " + std::string(e.what()));
        throw;
    }
}

void ConsensusHarmonyManager::shutdownComponents() {
    // Shutdown all components
    Logger::info("Shutting down consensus harmony components");
    
    try {
        // Log shutdown event before shutting down auditor
        if (securityAuditor) {
            securityAuditor->logSystemEvent("Consensus Harmony Manager shutting down", AuditSeverity::INFO);
        }
        
        if (emergencyMode) {
            emergencyMode->shutdown();
            emergencyMode.reset();
        }
        
        if (performanceOptimizer) {
            performanceOptimizer->shutdown();
            performanceOptimizer.reset();
        }
        
        if (monitor) {
            monitor->shutdown();
            monitor.reset();
        }
        
        if (balancer) {
            balancer->shutdown();
            balancer.reset();
        }
        
        if (router) {
            router->shutdown();
            router.reset();
        }
        
        // Shutdown security components last
        if (securityValidator) {
            securityValidator->shutdown();
            securityValidator.reset();
        }
        
        if (securityAuditor) {
            securityAuditor->shutdown();
            securityAuditor.reset();
        }
        
        Logger::info("All consensus harmony components shut down successfully");
        
    } catch (const std::exception& e) {
        Logger::error("Error during component shutdown: " + std::string(e.what()));
    }
}

bool ConsensusHarmonyManager::enterEmergencyMode() {
    return enterEmergencyMode(EmergencyType::MANUAL_ACTIVATION, EmergencySeverity::HIGH, 
                             "Manual emergency mode activation", "ConsensusHarmonyManager");
}

bool ConsensusHarmonyManager::enterEmergencyMode(EmergencyType type, EmergencySeverity severity, 
                                                const std::string& description, const std::string& source) {
    if (!emergencyMode) {
        Logger::error("Emergency mode not initialized");
        return false;
    }
    
    Logger::warning("Entering emergency consensus mode");
    return emergencyMode->activateEmergencyMode(type, severity, description, source);
}

bool ConsensusHarmonyManager::exitEmergencyMode() {
    if (!emergencyMode) {
        Logger::error("Emergency mode not initialized");
        return false;
    }
    
    Logger::info("Exiting emergency consensus mode");
    return emergencyMode->deactivateEmergencyMode();
}

bool ConsensusHarmonyManager::isInEmergencyMode() const {
    if (!emergencyMode) {
        return false;
    }
    
    return emergencyMode->isEmergencyActive();
}

void ConsensusHarmonyManager::logEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logEntry;
    logEntry["event"] = event;
    logEntry["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    logEntry["data"] = data;
    
    Logger::info("ConsensusHarmony: " + logEntry.dump());
}

// Security method implementations
SecurityValidationResult ConsensusHarmonyManager::validateSecurity(const ConsensusRequest& request) {
    if (!securityValidator) {
        SecurityValidationResult result;
        result.isSecure = false;
        result.threatLevel = ThreatLevel::HIGH;
        result.threats.push_back("Security validator not initialized");
        return result;
    }
    
    return securityValidator->validateConsensusRequest(request);
}

SecurityValidationResult ConsensusHarmonyManager::validateBlockSecurity(const Block& block) {
    if (!securityValidator) {
        SecurityValidationResult result;
        result.isSecure = false;
        result.threatLevel = ThreatLevel::HIGH;
        result.threats.push_back("Security validator not initialized");
        return result;
    }
    
    return securityValidator->validateBlock(block);
}

SecurityValidationResult ConsensusHarmonyManager::validateTransactionSecurity(const Transaction& transaction) {
    if (!securityValidator) {
        SecurityValidationResult result;
        result.isSecure = false;
        result.threatLevel = ThreatLevel::HIGH;
        result.threats.push_back("Security validator not initialized");
        return result;
    }
    
    return securityValidator->validateTransaction(transaction);
}

void ConsensusHarmonyManager::logSecurityEvent(const std::string& event, const std::string& source, 
                                             const nlohmann::json& details) {
    if (securityAuditor) {
        securityAuditor->logSystemEvent(event, AuditSeverity::INFO, details);
    }
    
    // Also log to regular logger
    Logger::info("Security Event: " + event + " from " + source);
}

nlohmann::json ConsensusHarmonyManager::getSecurityMetrics() const {
    nlohmann::json metrics;
    
    if (securityValidator) {
        metrics["validator"] = securityValidator->getSecurityMetrics();
    }
    
    if (securityAuditor) {
        metrics["auditor"] = securityAuditor->getAuditStatistics();
    }
    
    return metrics;
}

nlohmann::json ConsensusHarmonyManager::getSecurityReport() const {
    nlohmann::json report;
    
    if (securityValidator) {
        report["security_status"] = securityValidator->getSecurityStatus();
        report["active_threats"] = nlohmann::json::array();
        
        auto threats = securityValidator->getActiveThreats();
        for (const auto& threat : threats) {
            nlohmann::json threatInfo;
            threatInfo["name"] = threat.name;
            threatInfo["description"] = threat.description;
            threatInfo["detection_count"] = threat.detectionCount;
            threatInfo["last_detection"] = threat.lastDetection;
            report["active_threats"].push_back(threatInfo);
        }
    }
    
    if (securityAuditor) {
        report["audit_report"] = securityAuditor->getSecurityReport();
    }
    
    return report;
}

bool ConsensusHarmonyManager::enableSecurityFeature(const std::string& feature, bool enable) {
    if (!securityValidator) {
        Logger::error("Security validator not initialized");
        return false;
    }
    
    try {
        if (feature == "cryptographic_validation") {
            securityValidator->setCryptographicValidation(enable);
        } else if (feature == "attack_detection") {
            securityValidator->setAttackDetection(enable);
        } else if (feature == "audit_logging") {
            securityValidator->setAuditLogging(enable);
        } else {
            Logger::warning("Unknown security feature: " + feature);
            return false;
        }
        
        // Log the configuration change
        if (securityAuditor) {
            securityAuditor->logParameterChange(feature, enable ? "false" : "true", 
                                              enable ? "true" : "false", "consensus_harmony_manager");
        }
        
        Logger::info("Security feature " + feature + " " + (enable ? "enabled" : "disabled"));
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to configure security feature: " + std::string(e.what()));
        return false;
    }
}

// Performance optimization method implementations
bool ConsensusHarmonyManager::enablePerformanceOptimization(bool enable) {
    if (!performanceOptimizer) {
        Logger::error("Performance optimizer not initialized");
        return false;
    }
    
    try {
        OptimizationConfig config = performanceOptimizer->getConfiguration();
        config.enableResultCaching = enable;
        config.enableComputationMemoization = enable;
        config.enableParallelValidation = enable;
        config.enableMemoryOptimization = enable;
        
        if (!performanceOptimizer->updateConfiguration(config)) {
            Logger::error("Failed to update optimization configuration");
            return false;
        }
        
        Logger::info("Performance optimization " + std::string(enable ? "enabled" : "disabled"));
        
        // Log the configuration change
        if (securityAuditor) {
            securityAuditor->logParameterChange("performance_optimization", 
                                              enable ? "false" : "true", 
                                              enable ? "true" : "false", 
                                              "consensus_harmony_manager");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to configure performance optimization: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyManager::updateOptimizationConfiguration(const OptimizationConfig& config) {
    if (!performanceOptimizer) {
        Logger::error("Performance optimizer not initialized");
        return false;
    }
    
    try {
        if (!performanceOptimizer->updateConfiguration(config)) {
            Logger::error("Failed to update optimization configuration");
            return false;
        }
        
        Logger::info("Optimization configuration updated");
        
        // Log the configuration change
        if (securityAuditor) {
            nlohmann::json configJson;
            configJson["maxCacheSize"] = config.maxCacheSize;
            configJson["enableResultCaching"] = config.enableResultCaching;
            configJson["enableParallelValidation"] = config.enableParallelValidation;
            configJson["maxWorkerThreads"] = config.maxWorkerThreads;
            
            securityAuditor->logParameterChange("optimization_configuration", 
                                              "previous_config", configJson.dump(), 
                                              "consensus_harmony_manager");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to update optimization configuration: " + std::string(e.what()));
        return false;
    }
}

OptimizationConfig ConsensusHarmonyManager::getOptimizationConfiguration() const {
    if (!performanceOptimizer) {
        return OptimizationConfig{};
    }
    
    return performanceOptimizer->getConfiguration();
}

nlohmann::json ConsensusHarmonyManager::getPerformanceReport() const {
    nlohmann::json report;
    
    if (performanceOptimizer) {
        report["optimizer"] = performanceOptimizer->getOptimizationReport();
    } else {
        report["optimizer"] = "not_initialized";
    }
    
    // Add consensus harmony manager metrics
    report["harmony_manager"] = {
        {"initialized", initialized.load()},
        {"running", running.load()},
        {"total_validations", status.totalValidations},
        {"successful_validations", status.successfulValidations},
        {"conflict_count", status.conflictCount}
    };
    
    // Add router metrics if available
    if (router) {
        report["router"] = router->getStatistics();
    }
    
    return report;
}

nlohmann::json ConsensusHarmonyManager::runPerformanceBenchmark(const std::vector<ConsensusRequest>& testRequests) {
    if (!performanceOptimizer) {
        Logger::error("Performance optimizer not initialized");
        return nlohmann::json{{"error", "Performance optimizer not initialized"}};
    }
    
    if (!router) {
        Logger::error("Consensus router not initialized");
        return nlohmann::json{{"error", "Consensus router not initialized"}};
    }
    
    try {
        Logger::info("Running performance benchmark with " + std::to_string(testRequests.size()) + " requests");
        
        // Get all registered engines
        auto engineTypes = router->getRegisteredEngines();
        std::vector<ConsensusEngine*> engines;
        
        for (auto type : engineTypes) {
            ConsensusEngine* engine = router->getEngine(type);
            if (engine && engine->isHealthy()) {
                engines.push_back(engine);
            }
        }
        
        if (engines.empty()) {
            Logger::warning("No healthy engines available for benchmark");
            return nlohmann::json{{"error", "No healthy engines available"}};
        }
        
        // Run benchmark
        auto benchmarkResult = performanceOptimizer->runBenchmark(testRequests, engines);
        
        Logger::info("Performance benchmark completed");
        
        return benchmarkResult;
        
    } catch (const std::exception& e) {
        Logger::error("Performance benchmark failed: " + std::string(e.what()));
        return nlohmann::json{{"error", std::string(e.what())}};
    }
}

// Missing methods implementation
bool ConsensusHarmonyManager::setConsensusParameter(ConsensusType type, const std::string& parameter, double value) {
    if (!performanceOptimizer) {
        Logger::error("Performance optimizer not initialized");
        return false;
    }
    
    try {
        Logger::info("Setting consensus parameter: " + parameter + " = " + std::to_string(value) + " for type " + std::to_string(static_cast<int>(type)));
        
        // Update configuration based on parameter
        if (parameter == "difficulty" && type == ConsensusType::PROOF_OF_WORK) {
            config.powDifficulty = static_cast<uint32_t>(value);
        } else if (parameter == "min_stake" && type == ConsensusType::PROOF_OF_STAKE) {
            config.minStakeAmount = value;
        } else if (parameter == "supermajority_threshold") {
            config.supermajorityThreshold = value;
        } else if (parameter == "max_dominance_ratio") {
            config.maxDominanceRatio = value;
        } else {
            Logger::warning("Unknown parameter: " + parameter);
            return false;
        }
        
        // Log the parameter change
        if (securityAuditor) {
            securityAuditor->logParameterChange(parameter, "previous_value", std::to_string(value), "consensus_harmony_manager");
        }
        
        logEvent("consensus_parameter_updated", {{"parameter", parameter}, {"value", value}, {"type", static_cast<int>(type)}});
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to set consensus parameter: " + std::string(e.what()));
        return false;
    }
}

std::map<std::string, double> ConsensusHarmonyManager::getConsensusParameters(ConsensusType type) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::map<std::string, double> parameters;
    
    switch (type) {
        case ConsensusType::PROOF_OF_WORK:
            parameters["difficulty"] = static_cast<double>(config.powDifficulty);
            break;
        case ConsensusType::PROOF_OF_STAKE:
            parameters["min_stake"] = config.minStakeAmount;
            break;
        case ConsensusType::VOTING_CONSENSUS:
            parameters["supermajority_threshold"] = config.supermajorityThreshold;
            break;
        default:
            parameters["max_dominance_ratio"] = config.maxDominanceRatio;
            break;
    }
    
    return parameters;
}