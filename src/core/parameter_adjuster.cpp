#include "core/parameter_adjuster.h"
#include "core/consensus_router.h"
#include <algorithm>
#include <numeric>
#include <cmath>

ParameterAdjuster::ParameterAdjuster() 
    : initialized(false), running(false), emergencyMode(false),
      router(nullptr), balancer(nullptr), totalAdjustments(0), 
      shouldStop(false), emergencyAdjustments(0),
      lastAdjustment(std::chrono::steady_clock::now()) {
    Logger::info("ParameterAdjuster created");
}

ParameterAdjuster::ParameterAdjuster(const ParameterAdjustmentConfig& customConfig) 
    : config(customConfig), initialized(false), running(false), emergencyMode(false),
      router(nullptr), balancer(nullptr), totalAdjustments(0), 
      shouldStop(false), emergencyAdjustments(0),
      lastAdjustment(std::chrono::steady_clock::now()) {
    Logger::info("ParameterAdjuster created with custom configuration");
}

ParameterAdjuster::~ParameterAdjuster() {
    shutdown();
    Logger::info("ParameterAdjuster destroyed");
}

bool ParameterAdjuster::initialize() {
    return initialize(nullptr, nullptr);
}

bool ParameterAdjuster::initialize(ConsensusRouter* consensusRouter, ConsensusBalancer* consensusBalancer) {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    if (initialized.load()) {
        Logger::warning("ParameterAdjuster already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing ParameterAdjuster");
        
        // Validate configuration
        if (!validateConfig(config)) {
            Logger::error("Invalid parameter adjustment configuration");
            return false;
        }
        
        router = consensusRouter;
        balancer = consensusBalancer;
        
        // Initialize state
        currentConditions = NetworkConditions{};
        currentThreats = SecurityThreats{};
        adjustmentHistory.clear();
        
        // Reset statistics
        totalAdjustments = 0;
        emergencyAdjustments = 0;
        emergencyMode.store(false);
        shouldStop.store(false);
        
        initialized.store(true);
        running.store(true);
        
        // Start background threads
        monitoringThread = std::thread(&ParameterAdjuster::monitoringLoop, this);
        adjustmentThread = std::thread(&ParameterAdjuster::adjustmentLoop, this);
        
        Logger::info("ParameterAdjuster initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize ParameterAdjuster: " + std::string(e.what()));
        return false;
    }
}

void ParameterAdjuster::shutdown() {
    {
        std::lock_guard<std::mutex> lock(adjusterMutex);
        
        if (!initialized.load()) {
            return;
        }
        
        Logger::info("Shutting down ParameterAdjuster");
        
        // Stop background threads
        shouldStop.store(true);
        running.store(false);
    }
    
    // Notify threads outside the lock
    adjusterCV.notify_all();
    
    // Join threads outside the lock to avoid deadlock
    if (monitoringThread.joinable()) {
        monitoringThread.join();
    }
    
    if (adjustmentThread.joinable()) {
        adjustmentThread.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(adjusterMutex);
        
        // Clear state
        engines.clear();
        router = nullptr;
        balancer = nullptr;
        adjustmentHistory.clear();
        
        initialized.store(false);
    }
    
    Logger::info("ParameterAdjuster shut down successfully");
}

bool ParameterAdjuster::registerEngine(ConsensusType type, ConsensusEngine* engine) {
    if (!engine || !validateEngine(engine)) {
        Logger::error("Cannot register invalid consensus engine");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    try {
        engines[type] = engine;
        Logger::info("Registered consensus engine for parameter adjustment: " + consensusTypeToString(type));
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to register consensus engine: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::unregisterEngine(ConsensusType type) {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    auto it = engines.find(type);
    if (it == engines.end()) {
        Logger::warning("Attempted to unregister non-existent engine: " + consensusTypeToString(type));
        return false;
    }
    
    engines.erase(it);
    Logger::info("Unregistered consensus engine: " + consensusTypeToString(type));
    return true;
}

void ParameterAdjuster::monitorNetworkConditions() {
    if (!initialized.load() || !running.load()) {
        Logger::warning("ParameterAdjuster not running, cannot monitor network conditions");
        return;
    }
    
    try {
        Logger::debug("Monitoring network conditions");
        
        collectNetworkMetrics();
        analyzeNetworkLoad();
        checkNetworkHealth();
        updateNetworkConditions();
        
        Logger::debug("Network condition monitoring completed");
        
    } catch (const std::exception& e) {
        Logger::error("Error monitoring network conditions: " + std::string(e.what()));
    }
}

void ParameterAdjuster::detectSecurityThreats() {
    if (!initialized.load() || !running.load()) {
        Logger::warning("ParameterAdjuster not running, cannot detect security threats");
        return;
    }
    
    try {
        Logger::debug("Detecting security threats");
        
        scanForThreats();
        detectHashRateAnomalies();
        detectVotingAnomalies();
        detectNetworkPartitioning();
        detectDDoSAttacks();
        updateThreatLevel();
        
        Logger::debug("Security threat detection completed");
        
    } catch (const std::exception& e) {
        Logger::error("Error detecting security threats: " + std::string(e.what()));
    }
}

bool ParameterAdjuster::adjustParametersBasedOnLoad() {
    if (!config.enableAutomaticAdjustment) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    try {
        Logger::info("Adjusting parameters based on network load");
        
        bool adjustmentsMade = false;
        
        // High load adjustments
        if (currentConditions.networkLoad > config.loadThresholdHigh) {
            adjustmentsMade |= adjustForHighLoad();
        }
        
        // Low load adjustments
        if (currentConditions.networkLoad < config.loadThresholdLow) {
            adjustmentsMade |= adjustForLowLoad();
        }
        
        // Latency adjustments
        if (currentConditions.networkLatency > config.latencyThresholdHigh) {
            adjustmentsMade |= adjustForLatencyIssues();
        }
        
        // Throughput adjustments
        if (currentConditions.transactionThroughput < config.throughputThresholdLow) {
            adjustmentsMade |= adjustForThroughputIssues();
        }
        
        if (adjustmentsMade) {
            totalAdjustments++;
            Logger::info("Load-based parameter adjustments completed");
        }
        
        return adjustmentsMade;
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting parameters based on load: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::adjustParametersBasedOnThreats() {
    if (!config.enableAutomaticAdjustment) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    try {
        Logger::info("Adjusting parameters based on security threats");
        
        bool adjustmentsMade = false;
        
        // Only adjust if threat level is above threshold
        if (currentThreats.threatLevel < config.threatResponseThreshold) {
            return false;
        }
        
        // Respond to specific threats
        if (currentThreats.potentialFiftyOneAttack) {
            adjustmentsMade |= respondToFiftyOneAttack();
        }
        
        if (currentThreats.unusualHashRateSpike) {
            adjustmentsMade |= respondToHashRateSpike();
        }
        
        if (currentThreats.suspiciousVotingPattern) {
            adjustmentsMade |= respondToVotingAnomaly();
        }
        
        if (currentThreats.networkPartitioning) {
            adjustmentsMade |= respondToNetworkPartition();
        }
        
        if (currentThreats.ddosAttack) {
            adjustmentsMade |= respondToDDoSAttack();
        }
        
        if (adjustmentsMade) {
            totalAdjustments++;
            if (currentThreats.hasCriticalThreats()) {
                emergencyAdjustments++;
            }
            Logger::info("Threat-based parameter adjustments completed");
        }
        
        return adjustmentsMade;
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting parameters based on threats: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::maintainDecentralization() {
    if (!config.enableAutomaticAdjustment || !balancer) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    try {
        Logger::info("Maintaining decentralization through parameter adjustment");
        
        bool adjustmentsMade = false;
        
        // Get current balance metrics
        BalanceMetrics metrics = balancer->getBalanceMetrics();
        
        // Check for mechanism dominance
        for (const auto& [type, ratio] : metrics.dominanceRatios) {
            if (ratio > config.maxMechanismDominance) {
                Logger::warning("Mechanism dominance detected: " + consensusTypeToString(type) + 
                               " (" + std::to_string(ratio * 100) + "%)");
                adjustmentsMade |= rebalanceMechanismDominance();
                break;
            }
        }
        
        // Check for underperforming mechanisms
        for (const auto& [type, rate] : metrics.participationRates) {
            if (rate < config.minMechanismParticipation) {
                Logger::warning("Low mechanism participation: " + consensusTypeToString(type) + 
                               " (" + std::to_string(rate * 100) + "%)");
                adjustmentsMade |= boostUnderperformingMechanisms();
                break;
            }
        }
        
        // Prevent mechanism monopoly
        adjustmentsMade |= preventMechanismMonopoly();
        
        if (adjustmentsMade) {
            totalAdjustments++;
            Logger::info("Decentralization maintenance adjustments completed");
        }
        
        return adjustmentsMade;
        
    } catch (const std::exception& e) {
        Logger::error("Error maintaining decentralization: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::adjustDifficulty(ConsensusType type, double adjustment, const std::string& reason) {
    if (std::abs(adjustment) > config.maxDifficultyChange) {
        Logger::error("Difficulty adjustment exceeds maximum allowed change: " + std::to_string(adjustment));
        return false;
    }
    
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::error("Cannot adjust difficulty for unregistered engine: " + consensusTypeToString(type));
        return false;
    }
    
    try {
        // Get current parameters
        auto currentParams = engineIt->second->getParameters();
        double currentDifficulty = currentParams.count("difficulty") > 0 ? 
            currentParams["difficulty"] : 1.0;
        
        // Calculate new difficulty
        double newDifficulty = currentDifficulty * (1.0 + adjustment);
        newDifficulty = std::max(0.1, std::min(newDifficulty, 100.0));
        
        // Execute adjustment
        return executeAdjustment(type, "difficulty", newDifficulty, reason, selectAdjustmentStrategy());
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting difficulty: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::adjustRewards(ConsensusType type, double multiplier, const std::string& reason) {
    if (std::abs(multiplier - 1.0) > config.maxRewardChange) {
        Logger::error("Reward adjustment exceeds maximum allowed change: " + std::to_string(multiplier));
        return false;
    }
    
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::error("Cannot adjust rewards for unregistered engine: " + consensusTypeToString(type));
        return false;
    }
    
    try {
        // Get current parameters
        auto currentParams = engineIt->second->getParameters();
        double currentMultiplier = currentParams.count("rewardMultiplier") > 0 ? 
            currentParams["rewardMultiplier"] : 1.0;
        
        // Calculate new multiplier
        double newMultiplier = currentMultiplier * multiplier;
        newMultiplier = std::max(0.1, std::min(newMultiplier, 5.0));
        
        // Execute adjustment
        return executeAdjustment(type, "rewardMultiplier", newMultiplier, reason, selectAdjustmentStrategy());
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting rewards: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::adjustStakeRequirements(ConsensusType type, double adjustment, const std::string& reason) {
    if (std::abs(adjustment) > config.maxStakeChange) {
        Logger::error("Stake adjustment exceeds maximum allowed change: " + std::to_string(adjustment));
        return false;
    }
    
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::error("Cannot adjust stake requirements for unregistered engine: " + consensusTypeToString(type));
        return false;
    }
    
    try {
        // Get current parameters
        auto currentParams = engineIt->second->getParameters();
        double currentStake = currentParams.count("minStake") > 0 ? 
            currentParams["minStake"] : 1000.0;
        
        // Calculate new stake requirement
        double newStake = currentStake * (1.0 + adjustment);
        newStake = std::max(100.0, std::min(newStake, 100000.0));
        
        // Execute adjustment
        return executeAdjustment(type, "minStake", newStake, reason, selectAdjustmentStrategy());
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting stake requirements: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::adjustResourceRequirements(ConsensusType type, double adjustment, const std::string& reason) {
    if (std::abs(adjustment) > config.maxResourceChange) {
        Logger::error("Resource adjustment exceeds maximum allowed change: " + std::to_string(adjustment));
        return false;
    }
    
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::error("Cannot adjust resource requirements for unregistered engine: " + consensusTypeToString(type));
        return false;
    }
    
    try {
        // Get current parameters
        auto currentParams = engineIt->second->getParameters();
        double currentResource = currentParams.count("minResource") > 0 ? 
            currentParams["minResource"] : 100.0;
        
        // Calculate new resource requirement
        double newResource = currentResource * (1.0 + adjustment);
        newResource = std::max(10.0, std::min(newResource, 10000.0));
        
        // Execute adjustment
        return executeAdjustment(type, "minResource", newResource, reason, selectAdjustmentStrategy());
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting resource requirements: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::performAutomaticAdjustments() {
    if (!initialized.load() || !running.load() || !config.enableAutomaticAdjustment) {
        return false;
    }
    
    try {
        Logger::info("Performing automatic parameter adjustments");
        
        bool adjustmentsMade = false;
        
        // Monitor current conditions
        monitorNetworkConditions();
        detectSecurityThreats();
        
        // Perform adjustments based on conditions
        adjustmentsMade |= adjustParametersBasedOnLoad();
        adjustmentsMade |= adjustParametersBasedOnThreats();
        adjustmentsMade |= maintainDecentralization();
        
        if (adjustmentsMade) {
            Logger::info("Automatic parameter adjustments completed");
        }
        
        return adjustmentsMade;
        
    } catch (const std::exception& e) {
        Logger::error("Error performing automatic adjustments: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::enterEmergencyMode() {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    if (emergencyMode.load()) {
        Logger::warning("Already in emergency mode");
        return true;
    }
    
    try {
        Logger::warning("Entering emergency parameter adjustment mode");
        
        emergencyMode.store(true);
        
        // Switch to emergency adjustment strategy
        config.defaultStrategy = AdjustmentStrategy::EMERGENCY;
        
        // Reduce adjustment intervals for faster response
        config.monitoringInterval = 30;  // 30 seconds
        config.adjustmentInterval = 60;  // 1 minute
        
        // Increase adjustment limits for emergency response
        config.maxDifficultyChange = 0.5;  // 50% max change
        config.maxRewardChange = 0.4;      // 40% max change
        
        Logger::warning("Emergency parameter adjustment mode activated");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to enter emergency mode: " + std::string(e.what()));
        return false;
    }
}

bool ParameterAdjuster::exitEmergencyMode() {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    if (!emergencyMode.load()) {
        Logger::info("Not in emergency mode");
        return true;
    }
    
    try {
        Logger::info("Exiting emergency parameter adjustment mode");
        
        emergencyMode.store(false);
        
        // Restore normal adjustment strategy
        config.defaultStrategy = AdjustmentStrategy::MODERATE;
        
        // Restore normal intervals
        config.monitoringInterval = 300;   // 5 minutes
        config.adjustmentInterval = 1800;  // 30 minutes
        
        // Restore normal adjustment limits
        config.maxDifficultyChange = 0.25; // 25% max change
        config.maxRewardChange = 0.20;     // 20% max change
        
        Logger::info("Emergency parameter adjustment mode deactivated");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to exit emergency mode: " + std::string(e.what()));
        return false;
    }
}

NetworkConditions ParameterAdjuster::getNetworkConditions() const {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    return currentConditions;
}

SecurityThreats ParameterAdjuster::getSecurityThreats() const {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    return currentThreats;
}

std::vector<AdjustmentRecord> ParameterAdjuster::getAdjustmentHistory() const {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    return adjustmentHistory;
}

nlohmann::json ParameterAdjuster::getDetailedStatus() const {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    nlohmann::json status;
    
    // Basic status
    status["initialized"] = initialized.load();
    status["running"] = running.load();
    status["emergencyMode"] = emergencyMode.load();
    status["automaticAdjustmentEnabled"] = config.enableAutomaticAdjustment;
    
    // Network conditions
    nlohmann::json conditions;
    conditions["networkLoad"] = currentConditions.networkLoad;
    conditions["averageBlockTime"] = currentConditions.averageBlockTime;
    conditions["transactionThroughput"] = currentConditions.transactionThroughput;
    conditions["pendingTransactions"] = currentConditions.pendingTransactions;
    conditions["networkLatency"] = currentConditions.networkLatency;
    conditions["activeNodes"] = currentConditions.activeNodes;
    conditions["memoryUsage"] = currentConditions.memoryUsage;
    conditions["cpuUsage"] = currentConditions.cpuUsage;
    conditions["healthScore"] = currentConditions.getHealthScore();
    status["networkConditions"] = conditions;
    
    // Security threats
    nlohmann::json threats;
    threats["potentialFiftyOneAttack"] = currentThreats.potentialFiftyOneAttack;
    threats["unusualHashRateSpike"] = currentThreats.unusualHashRateSpike;
    threats["suspiciousVotingPattern"] = currentThreats.suspiciousVotingPattern;
    threats["networkPartitioning"] = currentThreats.networkPartitioning;
    threats["ddosAttack"] = currentThreats.ddosAttack;
    threats["threatLevel"] = currentThreats.threatLevel;
    threats["threatSeverity"] = currentThreats.getThreatSeverity();
    threats["activeThreats"] = currentThreats.activeThreats;
    threats["hasCriticalThreats"] = currentThreats.hasCriticalThreats();
    status["securityThreats"] = threats;
    
    // Statistics
    status["totalAdjustments"] = totalAdjustments;
    status["emergencyAdjustments"] = emergencyAdjustments;
    status["registeredEngines"] = engines.size();
    status["adjustmentHistorySize"] = adjustmentHistory.size();
    
    return status;
}

void ParameterAdjuster::setConfig(const ParameterAdjustmentConfig& newConfig) {
    if (!validateConfig(newConfig)) {
        Logger::error("Invalid parameter adjustment configuration provided");
        return;
    }
    
    std::lock_guard<std::mutex> lock(adjusterMutex);
    config = newConfig;
    Logger::info("Parameter adjustment configuration updated");
}

ParameterAdjustmentConfig ParameterAdjuster::getConfig() const {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    return config;
}

bool ParameterAdjuster::updateAdjustmentLimits(double maxDifficulty, double maxReward, double maxStake, double maxResource) {
    if (maxDifficulty <= 0 || maxReward <= 0 || maxStake <= 0 || maxResource <= 0 ||
        maxDifficulty > 1.0 || maxReward > 1.0 || maxStake > 1.0 || maxResource > 1.0) {
        Logger::error("Invalid adjustment limits provided");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    config.maxDifficultyChange = maxDifficulty;
    config.maxRewardChange = maxReward;
    config.maxStakeChange = maxStake;
    config.maxResourceChange = maxResource;
    
    Logger::info("Adjustment limits updated");
    return true;
}

std::vector<std::string> ParameterAdjuster::analyzeNetworkConditions() const {
    std::vector<std::string> analysis;
    
    // Network load analysis
    if (currentConditions.networkLoad > 0.8) {
        analysis.push_back("High network load detected (" + std::to_string(currentConditions.networkLoad * 100) + "%)");
    } else if (currentConditions.networkLoad < 0.3) {
        analysis.push_back("Low network load detected (" + std::to_string(currentConditions.networkLoad * 100) + "%)");
    }
    
    // Block time analysis
    if (currentConditions.averageBlockTime > 900) { // 15 minutes
        analysis.push_back("Slow block times detected (" + std::to_string(currentConditions.averageBlockTime) + "s)");
    } else if (currentConditions.averageBlockTime < 300) { // 5 minutes
        analysis.push_back("Fast block times detected (" + std::to_string(currentConditions.averageBlockTime) + "s)");
    }
    
    // Throughput analysis
    if (currentConditions.transactionThroughput < 10.0) {
        analysis.push_back("Low transaction throughput (" + std::to_string(currentConditions.transactionThroughput) + " TPS)");
    }
    
    // Latency analysis
    if (currentConditions.networkLatency > 5000.0) {
        analysis.push_back("High network latency (" + std::to_string(currentConditions.networkLatency) + "ms)");
    }
    
    // Resource usage analysis
    if (currentConditions.memoryUsage > 0.8) {
        analysis.push_back("High memory usage (" + std::to_string(currentConditions.memoryUsage * 100) + "%)");
    }
    
    if (currentConditions.cpuUsage > 0.8) {
        analysis.push_back("High CPU usage (" + std::to_string(currentConditions.cpuUsage * 100) + "%)");
    }
    
    // Overall health analysis
    double healthScore = currentConditions.getHealthScore();
    if (healthScore < 0.5) {
        analysis.push_back("Poor network health score (" + std::to_string(healthScore * 100) + "%)");
    }
    
    return analysis;
}

std::vector<std::string> ParameterAdjuster::generateAdjustmentRecommendations() const {
    std::vector<std::string> recommendations;
    
    // Load-based recommendations
    if (currentConditions.networkLoad > config.loadThresholdHigh) {
        recommendations.push_back("Reduce PoW difficulty to handle high network load");
        recommendations.push_back("Increase block size limits to improve throughput");
    }
    
    if (currentConditions.networkLoad < config.loadThresholdLow) {
        recommendations.push_back("Increase PoW difficulty to maintain security");
        recommendations.push_back("Reduce block rewards to prevent inflation");
    }
    
    // Latency-based recommendations
    if (currentConditions.networkLatency > config.latencyThresholdHigh) {
        recommendations.push_back("Optimize network topology to reduce latency");
        recommendations.push_back("Increase block time targets to accommodate latency");
    }
    
    // Throughput-based recommendations
    if (currentConditions.transactionThroughput < config.throughputThresholdLow) {
        recommendations.push_back("Reduce transaction validation complexity");
        recommendations.push_back("Implement transaction batching");
    }
    
    // Security-based recommendations
    if (currentThreats.threatLevel > config.threatResponseThreshold) {
        recommendations.push_back("Increase consensus mechanism diversity");
        recommendations.push_back("Strengthen validation requirements");
    }
    
    if (currentThreats.potentialFiftyOneAttack) {
        recommendations.push_back("Emergency: Activate additional consensus mechanisms");
        recommendations.push_back("Emergency: Increase stake requirements");
    }
    
    return recommendations;
}

nlohmann::json ParameterAdjuster::getPerformanceAnalysis() const {
    nlohmann::json analysis;
    
    // Network performance metrics
    analysis["networkHealth"] = currentConditions.getHealthScore();
    analysis["loadEfficiency"] = 1.0 - std::abs(currentConditions.networkLoad - 0.6); // Target 60% load
    analysis["latencyScore"] = currentConditions.networkLatency > 0 ? 
        std::min(1000.0 / currentConditions.networkLatency, 1.0) : 1.0;
    analysis["throughputScore"] = std::min(currentConditions.transactionThroughput / 100.0, 1.0);
    
    // Security analysis
    analysis["securityLevel"] = 1.0 - currentThreats.threatLevel;
    analysis["threatCount"] = currentThreats.activeThreats.size();
    analysis["criticalThreats"] = currentThreats.hasCriticalThreats();
    
    // Overall performance score
    double performanceScore = (analysis["networkHealth"].get<double>() * 0.3) +
                             (analysis["loadEfficiency"].get<double>() * 0.2) +
                             (analysis["latencyScore"].get<double>() * 0.2) +
                             (analysis["throughputScore"].get<double>() * 0.2) +
                             (analysis["securityLevel"].get<double>() * 0.1);
    analysis["overallPerformance"] = performanceScore;
    
    return analysis;
}

nlohmann::json ParameterAdjuster::getStatistics() const {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    nlohmann::json stats;
    stats["totalAdjustments"] = totalAdjustments;
    stats["emergencyAdjustments"] = emergencyAdjustments;
    stats["isInitialized"] = initialized.load();
    stats["isRunning"] = running.load();
    stats["isInEmergencyMode"] = emergencyMode.load();
    stats["registeredEngines"] = engines.size();
    stats["adjustmentHistorySize"] = adjustmentHistory.size();
    
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - lastAdjustment).count();
    stats["uptimeSeconds"] = uptime;
    
    return stats;
}

void ParameterAdjuster::resetStatistics() {
    std::lock_guard<std::mutex> lock(adjusterMutex);
    
    totalAdjustments = 0;
    emergencyAdjustments = 0;
    lastAdjustment = std::chrono::steady_clock::now();
    adjustmentHistory.clear();
    
    Logger::info("ParameterAdjuster statistics reset");
}

// Private methods implementation

void ParameterAdjuster::monitoringLoop() {
    Logger::info("ParameterAdjuster monitoring thread started");
    
    while (running.load() && !shouldStop.load()) {
        try {
            std::unique_lock<std::mutex> lock(adjusterMutex);
            
            // Wait for monitoring interval or stop signal
            adjusterCV.wait_for(lock, std::chrono::seconds(config.monitoringInterval), 
                [this] { return shouldStop.load(); });
            
            if (shouldStop.load()) {
                break;
            }
            
            lock.unlock();
            
            // Perform monitoring
            monitorNetworkConditions();
            detectSecurityThreats();
            
            // Check for emergency conditions
            if (currentThreats.hasCriticalThreats() && !emergencyMode.load()) {
                enterEmergencyMode();
            }
            
        } catch (const std::exception& e) {
            Logger::error("Error in monitoring loop: " + std::string(e.what()));
            
            // Sleep briefly before retrying
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    
    Logger::info("ParameterAdjuster monitoring thread stopped");
}

void ParameterAdjuster::adjustmentLoop() {
    Logger::info("ParameterAdjuster adjustment thread started");
    
    while (running.load() && !shouldStop.load()) {
        try {
            std::unique_lock<std::mutex> lock(adjusterMutex);
            
            // Wait for adjustment interval or stop signal
            adjusterCV.wait_for(lock, std::chrono::seconds(config.adjustmentInterval), 
                [this] { return shouldStop.load(); });
            
            if (shouldStop.load()) {
                break;
            }
            
            lock.unlock();
            
            // Perform automatic adjustments
            performAutomaticAdjustments();
            
        } catch (const std::exception& e) {
            Logger::error("Error in adjustment loop: " + std::string(e.what()));
            
            // Sleep briefly before retrying
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }
    
    Logger::info("ParameterAdjuster adjustment thread stopped");
}

void ParameterAdjuster::collectNetworkMetrics() {
    // Collect metrics from registered engines
    uint64_t totalValidations = 0;
    double totalResponseTime = 0.0;
    uint64_t totalPendingTx = 0;
    uint64_t totalActiveNodes = 0;
    
    for (const auto& [type, engine] : engines) {
        try {
            nlohmann::json metrics = engine->getMetrics();
            
            if (metrics.contains("totalValidations")) {
                totalValidations += metrics["totalValidations"].get<uint64_t>();
            }
            
            if (metrics.contains("averageResponseTime")) {
                totalResponseTime += metrics["averageResponseTime"].get<double>();
            }
            
            if (metrics.contains("pendingTransactions")) {
                totalPendingTx += metrics["pendingTransactions"].get<uint64_t>();
            }
            
            if (metrics.contains("activeNodes")) {
                totalActiveNodes += metrics["activeNodes"].get<uint64_t>();
            }
            
        } catch (const std::exception& e) {
            Logger::warning("Failed to collect metrics from " + consensusTypeToString(type) + 
                           ": " + std::string(e.what()));
        }
    }
    
    // Update network conditions
    currentConditions.pendingTransactions = totalPendingTx;
    currentConditions.activeNodes = totalActiveNodes;
    currentConditions.averageResponseTime = engines.size() > 0 ? totalResponseTime / engines.size() : 0.0;
    
    // Calculate derived metrics
    if (totalValidations > 0) {
        currentConditions.transactionThroughput = static_cast<double>(totalValidations) / 60.0; // Per minute to per second
    }
    
    currentConditions.timestamp = getCurrentTimestamp();
}

void ParameterAdjuster::analyzeNetworkLoad() {
    // Calculate network load based on various factors
    double loadFactors = 0.0;
    int factorCount = 0;
    
    // Pending transactions factor
    if (currentConditions.pendingTransactions > 0) {
        double txLoad = std::min(static_cast<double>(currentConditions.pendingTransactions) / 10000.0, 1.0);
        loadFactors += txLoad;
        factorCount++;
    }
    
    // Response time factor
    if (currentConditions.averageResponseTime > 0) {
        double responseLoad = std::min(currentConditions.averageResponseTime / 10.0, 1.0);
        loadFactors += responseLoad;
        factorCount++;
    }
    
    // Memory and CPU usage factors
    loadFactors += currentConditions.memoryUsage;
    loadFactors += currentConditions.cpuUsage;
    factorCount += 2;
    
    // Calculate overall network load
    currentConditions.networkLoad = factorCount > 0 ? loadFactors / factorCount : 0.0;
}

void ParameterAdjuster::checkNetworkHealth() {
    // Update network latency (simulated for now)
    currentConditions.networkLatency = 100.0 + (rand() % 1000); // 100-1100ms
    
    // Update resource usage (simulated for now)
    currentConditions.memoryUsage = 0.3 + (rand() % 50) / 100.0; // 30-80%
    currentConditions.cpuUsage = 0.2 + (rand() % 60) / 100.0;    // 20-80%
    
    // Calculate block time based on current conditions
    if (currentConditions.networkLoad > 0.8) {
        currentConditions.averageBlockTime = 900; // 15 minutes under high load
    } else if (currentConditions.networkLoad < 0.3) {
        currentConditions.averageBlockTime = 300; // 5 minutes under low load
    } else {
        currentConditions.averageBlockTime = 600; // 10 minutes normal
    }
}

void ParameterAdjuster::updateNetworkConditions() {
    // Network conditions are updated by other methods
    // This method can be used for final calculations or validations
    
    Logger::debug("Network conditions updated - Load: " + 
                 std::to_string(currentConditions.networkLoad * 100) + 
                 "%, Health: " + std::to_string(currentConditions.getHealthScore() * 100) + "%");
}

void ParameterAdjuster::scanForThreats() {
    // Reset threat indicators
    currentThreats.activeThreats.clear();
    
    // This is a simplified threat detection - in a real implementation,
    // this would analyze blockchain data, network patterns, etc.
    
    // Simulate threat detection based on network conditions
    if (currentConditions.networkLoad > 0.9) {
        currentThreats.ddosAttack = true;
        currentThreats.activeThreats.push_back("High network load - possible DDoS");
    }
    
    if (currentConditions.activeNodes < 10) {
        currentThreats.networkPartitioning = true;
        currentThreats.activeThreats.push_back("Low node count - possible network partition");
    }
}

void ParameterAdjuster::detectHashRateAnomalies() {
    // Get hash rate data from PoW engines
    for (const auto& [type, engine] : engines) {
        if (type == ConsensusType::PROOF_OF_WORK) {
            try {
                nlohmann::json metrics = engine->getMetrics();
                if (metrics.contains("networkHashRate")) {
                    double currentHashRate = metrics["networkHashRate"].get<double>();
                    
                    // Simple anomaly detection - in practice, this would use historical data
                    if (currentHashRate > 1000000) { // Arbitrary threshold
                        currentThreats.unusualHashRateSpike = true;
                        currentThreats.activeThreats.push_back("Unusual hash rate spike detected");
                    }
                }
            } catch (const std::exception& e) {
                Logger::warning("Failed to check hash rate anomalies: " + std::string(e.what()));
            }
        }
    }
}

void ParameterAdjuster::detectVotingAnomalies() {
    // Get voting data from voting engines
    for (const auto& [type, engine] : engines) {
        if (type == ConsensusType::VOTING_CONSENSUS) {
            try {
                nlohmann::json metrics = engine->getMetrics();
                if (metrics.contains("votingPatterns")) {
                    // Analyze voting patterns for anomalies
                    // This is simplified - real implementation would analyze vote distributions
                    currentThreats.suspiciousVotingPattern = false; // Placeholder
                }
            } catch (const std::exception& e) {
                Logger::warning("Failed to check voting anomalies: " + std::string(e.what()));
            }
        }
    }
}

void ParameterAdjuster::detectNetworkPartitioning() {
    // Already handled in scanForThreats() based on active node count
}

void ParameterAdjuster::detectDDoSAttacks() {
    // Already handled in scanForThreats() based on network load
}

void ParameterAdjuster::updateThreatLevel() {
    // Calculate overall threat level based on active threats
    double threatScore = 0.0;
    
    if (currentThreats.potentialFiftyOneAttack) threatScore += 0.8;
    if (currentThreats.unusualHashRateSpike) threatScore += 0.4;
    if (currentThreats.suspiciousVotingPattern) threatScore += 0.3;
    if (currentThreats.networkPartitioning) threatScore += 0.6;
    if (currentThreats.ddosAttack) threatScore += 0.5;
    
    currentThreats.threatLevel = std::min(threatScore, 1.0);
    currentThreats.lastThreatDetection = getCurrentTimestamp();
    
    Logger::debug("Threat level updated: " + currentThreats.getThreatSeverity() + 
                 " (" + std::to_string(currentThreats.threatLevel * 100) + "%)");
}

AdjustmentStrategy ParameterAdjuster::selectAdjustmentStrategy() const {
    if (emergencyMode.load() || currentThreats.hasCriticalThreats()) {
        return AdjustmentStrategy::EMERGENCY;
    }
    
    if (currentThreats.threatLevel > 0.6) {
        return AdjustmentStrategy::AGGRESSIVE;
    }
    
    if (currentConditions.getHealthScore() < 0.5) {
        return AdjustmentStrategy::AGGRESSIVE;
    }
    
    return config.defaultStrategy;
}

double ParameterAdjuster::getAdjustmentMultiplier(AdjustmentStrategy strategy) const {
    switch (strategy) {
        case AdjustmentStrategy::CONSERVATIVE:
            return 0.5;
        case AdjustmentStrategy::MODERATE:
            return 1.0;
        case AdjustmentStrategy::AGGRESSIVE:
            return 1.5;
        case AdjustmentStrategy::EMERGENCY:
            return 2.0;
        default:
            return 1.0;
    }
}

bool ParameterAdjuster::adjustForHighLoad() {
    Logger::info("Adjusting parameters for high network load");
    
    bool adjustmentsMade = false;
    
    // Reduce PoW difficulty to speed up block production
    adjustmentsMade |= adjustDifficulty(ConsensusType::PROOF_OF_WORK, -0.1, "High network load");
    
    // Increase PoS rewards to encourage more validators
    adjustmentsMade |= adjustRewards(ConsensusType::PROOF_OF_STAKE, 1.1, "High network load");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::adjustForLowLoad() {
    Logger::info("Adjusting parameters for low network load");
    
    bool adjustmentsMade = false;
    
    // Increase PoW difficulty to maintain security
    adjustmentsMade |= adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.05, "Low network load");
    
    // Reduce rewards to prevent inflation
    adjustmentsMade |= adjustRewards(ConsensusType::PROOF_OF_STAKE, 0.95, "Low network load");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::adjustForLatencyIssues() {
    Logger::info("Adjusting parameters for high network latency");
    
    bool adjustmentsMade = false;
    
    // Reduce difficulty to compensate for latency
    adjustmentsMade |= adjustDifficulty(ConsensusType::PROOF_OF_WORK, -0.05, "High network latency");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::adjustForThroughputIssues() {
    Logger::info("Adjusting parameters for low transaction throughput");
    
    bool adjustmentsMade = false;
    
    // Reduce resource requirements to allow more participation
    adjustmentsMade |= adjustResourceRequirements(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, -0.1, "Low throughput");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::respondToFiftyOneAttack() {
    Logger::warning("Responding to potential 51% attack");
    
    bool adjustmentsMade = false;
    
    // Increase stake requirements
    adjustmentsMade |= adjustStakeRequirements(ConsensusType::PROOF_OF_STAKE, 0.2, "51% attack response");
    
    // Increase PoW difficulty
    adjustmentsMade |= adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.15, "51% attack response");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::respondToHashRateSpike() {
    Logger::warning("Responding to unusual hash rate spike");
    
    bool adjustmentsMade = false;
    
    // Increase difficulty to maintain block time
    adjustmentsMade |= adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1, "Hash rate spike response");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::respondToVotingAnomaly() {
    Logger::warning("Responding to suspicious voting pattern");
    
    bool adjustmentsMade = false;
    
    // Increase voting requirements
    // This would be implemented based on the specific voting consensus engine
    
    return adjustmentsMade;
}

bool ParameterAdjuster::respondToNetworkPartition() {
    Logger::warning("Responding to network partitioning");
    
    bool adjustmentsMade = false;
    
    // Reduce requirements to allow more nodes to participate
    adjustmentsMade |= adjustResourceRequirements(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, -0.15, "Network partition response");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::respondToDDoSAttack() {
    Logger::warning("Responding to DDoS attack");
    
    bool adjustmentsMade = false;
    
    // Increase difficulty to slow down block production and reduce load
    adjustmentsMade |= adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1, "DDoS attack response");
    
    return adjustmentsMade;
}

bool ParameterAdjuster::rebalanceMechanismDominance() {
    if (!balancer) {
        return false;
    }
    
    Logger::info("Rebalancing mechanism dominance");
    
    bool adjustmentsMade = false;
    
    // Get balance metrics
    BalanceMetrics metrics = balancer->getBalanceMetrics();
    
    // Find most dominant mechanism
    ConsensusType dominant = metrics.getMostDominant();
    
    // Reduce rewards for dominant mechanism
    adjustmentsMade |= adjustRewards(dominant, 0.9, "Rebalancing dominance");
    
    // Increase rewards for less dominant mechanisms
    for (const auto& [type, ratio] : metrics.dominanceRatios) {
        if (type != dominant && ratio < 0.3) {
            adjustmentsMade |= adjustRewards(type, 1.1, "Boosting underperforming mechanism");
        }
    }
    
    return adjustmentsMade;
}

bool ParameterAdjuster::boostUnderperformingMechanisms() {
    if (!balancer) {
        return false;
    }
    
    Logger::info("Boosting underperforming mechanisms");
    
    bool adjustmentsMade = false;
    
    // Get balance metrics
    BalanceMetrics metrics = balancer->getBalanceMetrics();
    
    // Boost mechanisms with low participation
    for (const auto& [type, rate] : metrics.participationRates) {
        if (rate < config.minMechanismParticipation) {
            // Increase rewards
            adjustmentsMade |= adjustRewards(type, 1.15, "Boosting underperforming mechanism");
            
            // Reduce requirements
            if (type == ConsensusType::PROOF_OF_STAKE) {
                adjustmentsMade |= adjustStakeRequirements(type, -0.1, "Boosting underperforming mechanism");
            } else if (type == ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION) {
                adjustmentsMade |= adjustResourceRequirements(type, -0.1, "Boosting underperforming mechanism");
            }
        }
    }
    
    return adjustmentsMade;
}

bool ParameterAdjuster::preventMechanismMonopoly() {
    if (!balancer) {
        return false;
    }
    
    Logger::info("Preventing mechanism monopoly");
    
    bool adjustmentsMade = false;
    
    // Get balance metrics
    BalanceMetrics metrics = balancer->getBalanceMetrics();
    
    // Check for monopolistic behavior
    for (const auto& [type, ratio] : metrics.dominanceRatios) {
        if (ratio > 0.8) { // 80% dominance threshold
            Logger::warning("Monopolistic behavior detected in " + consensusTypeToString(type));
            
            // Drastically reduce rewards
            adjustmentsMade |= adjustRewards(type, 0.8, "Preventing monopoly");
            
            // Increase requirements
            if (type == ConsensusType::PROOF_OF_WORK) {
                adjustmentsMade |= adjustDifficulty(type, 0.2, "Preventing monopoly");
            } else if (type == ConsensusType::PROOF_OF_STAKE) {
                adjustmentsMade |= adjustStakeRequirements(type, 0.2, "Preventing monopoly");
            }
        }
    }
    
    return adjustmentsMade;
}

bool ParameterAdjuster::executeAdjustment(ConsensusType type, const std::string& parameter, 
                                         double newValue, const std::string& reason, 
                                         AdjustmentStrategy strategy) {
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::error("Cannot execute adjustment for unregistered engine: " + consensusTypeToString(type));
        return false;
    }
    
    if (!validateAdjustment(type, parameter, newValue)) {
        Logger::error("Invalid adjustment value: " + std::to_string(newValue));
        return false;
    }
    
    try {
        // Get current value
        auto currentParams = engineIt->second->getParameters();
        double oldValue = currentParams.count(parameter) > 0 ? currentParams[parameter] : 0.0;
        
        // Apply adjustment
        std::map<std::string, double> newParams = {{parameter, newValue}};
        if (engineIt->second->adjustParameters(newParams)) {
            // Record the adjustment
            AdjustmentRecord record;
            record.timestamp = getCurrentTimestamp();
            record.mechanism = type;
            record.parameter = parameter;
            record.oldValue = oldValue;
            record.newValue = newValue;
            record.reason = reason;
            record.strategy = strategy;
            record.conditions = currentConditions;
            record.threats = currentThreats;
            
            recordAdjustment(record);
            
            // Increment total adjustments counter
            totalAdjustments++;
            
            Logger::info("Parameter adjustment executed: " + consensusTypeToString(type) + 
                        "." + parameter + " changed from " + std::to_string(oldValue) + 
                        " to " + std::to_string(newValue) + " (" + reason + ")");
            
            return true;
        } else {
            Logger::error("Failed to apply parameter adjustment to engine");
            return false;
        }
        
    } catch (const std::exception& e) {
        Logger::error("Error executing adjustment: " + std::string(e.what()));
        return false;
    }
}

void ParameterAdjuster::recordAdjustment(const AdjustmentRecord& record) {
    adjustmentHistory.push_back(record);
    
    // Limit history size
    if (adjustmentHistory.size() > maxHistorySize) {
        adjustmentHistory.erase(adjustmentHistory.begin());
    }
    
    logAdjustmentEvent("parameter_adjusted", {
        {"mechanism", consensusTypeToString(record.mechanism)},
        {"parameter", record.parameter},
        {"oldValue", record.oldValue},
        {"newValue", record.newValue},
        {"reason", record.reason},
        {"strategy", adjustmentStrategyToString(record.strategy)}
    });
}

bool ParameterAdjuster::validateAdjustment(ConsensusType type, const std::string& parameter, double newValue) const {
    if (!isValidAdjustmentValue(newValue)) {
        return false;
    }
    
    // Parameter-specific validation
    if (parameter == "difficulty") {
        return newValue >= 0.1 && newValue <= 100.0;
    } else if (parameter == "rewardMultiplier") {
        return newValue >= 0.1 && newValue <= 5.0;
    } else if (parameter == "minStake") {
        return newValue >= 100.0 && newValue <= 100000.0;
    } else if (parameter == "minResource") {
        return newValue >= 10.0 && newValue <= 10000.0;
    }
    
    return true;
}

std::string ParameterAdjuster::consensusTypeToString(ConsensusType type) const {
    switch (type) {
        case ConsensusType::PROOF_OF_WORK:
            return "PoW";
        case ConsensusType::PROOF_OF_STAKE:
            return "PoS";
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
            return "PoRC";
        case ConsensusType::VOTING_CONSENSUS:
            return "Voting";
        case ConsensusType::SMART_CONTRACT_VALIDATION:
            return "SmartContract";
        default:
            return "Unknown";
    }
}

std::string ParameterAdjuster::adjustmentStrategyToString(AdjustmentStrategy strategy) const {
    switch (strategy) {
        case AdjustmentStrategy::CONSERVATIVE:
            return "Conservative";
        case AdjustmentStrategy::MODERATE:
            return "Moderate";
        case AdjustmentStrategy::AGGRESSIVE:
            return "Aggressive";
        case AdjustmentStrategy::EMERGENCY:
            return "Emergency";
        default:
            return "Unknown";
    }
}

double ParameterAdjuster::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool ParameterAdjuster::isValidAdjustmentValue(double value) const {
    return std::isfinite(value) && value > 0.0;
}

void ParameterAdjuster::logAdjustmentEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logData;
    logData["event"] = event;
    logData["timestamp"] = getCurrentTimestamp();
    logData["data"] = data;
    
    Logger::info("ParameterAdjuster: " + logData.dump());
}

bool ParameterAdjuster::validateConfig(const ParameterAdjustmentConfig& config) const {
    // Validate intervals
    if (config.monitoringInterval == 0 || config.adjustmentInterval == 0) {
        return false;
    }
    
    // Validate adjustment limits
    if (config.maxDifficultyChange <= 0 || config.maxDifficultyChange > 1.0 ||
        config.maxRewardChange <= 0 || config.maxRewardChange > 1.0 ||
        config.maxStakeChange <= 0 || config.maxStakeChange > 1.0 ||
        config.maxResourceChange <= 0 || config.maxResourceChange > 1.0) {
        return false;
    }
    
    // Validate thresholds
    if (config.loadThresholdHigh <= config.loadThresholdLow ||
        config.loadThresholdHigh > 1.0 || config.loadThresholdLow < 0.0) {
        return false;
    }
    
    return true;
}

bool ParameterAdjuster::validateEngine(ConsensusEngine* engine) const {
    if (!engine) {
        return false;
    }
    
    try {
        // Test basic engine functionality
        engine->getStatus();
        engine->getMetrics();
        engine->getParameters();
        return true;
    } catch (const std::exception& e) {
        Logger::warning("Engine validation failed: " + std::string(e.what()));
        return false;
    }
}