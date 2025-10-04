#include "core/consensus_balancer.h"
#include "core/consensus_router.h"

#include <algorithm>
#include <numeric>
#include <cmath>

ConsensusBalancer::ConsensusBalancer() 
    : initialized(false), running(false), emergencyMode(false), 
      router(nullptr), totalRebalances(0), emergencyActivations(0),
      shouldStop(false), lastBalanceCheck(std::chrono::steady_clock::now()) {
    Logger::info("ConsensusBalancer created");
}

ConsensusBalancer::ConsensusBalancer(const BalanceConfig& customConfig) 
    : config(customConfig), initialized(false), running(false), emergencyMode(false),
      router(nullptr), totalRebalances(0), emergencyActivations(0),
      shouldStop(false), lastBalanceCheck(std::chrono::steady_clock::now()) {
    Logger::info("ConsensusBalancer created with custom configuration");
}

ConsensusBalancer::~ConsensusBalancer() {
    shutdown();
    Logger::info("ConsensusBalancer destroyed");
}

bool ConsensusBalancer::initialize() {
    return initialize(nullptr);
}

bool ConsensusBalancer::initialize(ConsensusRouter* consensusRouter) {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    if (initialized.load()) {
        Logger::warning("ConsensusBalancer already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing ConsensusBalancer");
        
        // Validate configuration
        if (!validateConfig(config)) {
            Logger::error("Invalid balance configuration");
            return false;
        }
        
        router = consensusRouter;
        
        // Initialize metrics
        currentMetrics = BalanceMetrics{};
        currentMetrics.lastRebalanceTime = getCurrentTimestamp();
        
        // Initialize participation metrics for all consensus types
        for (const auto& [type, targetRate] : config.targetParticipationRates) {
            currentMetrics.participationMetrics[type] = ParticipationMetrics{};
            currentMetrics.participationRates[type] = 0.0;
            currentMetrics.dominanceRatios[type] = 0.0;
            historicalMetrics[type] = std::vector<ParticipationMetrics>();
        }
        
        // Reset statistics
        totalRebalances = 0;
        emergencyActivations = 0;
        emergencyMode.store(false);
        shouldStop.store(false);
        
        initialized.store(true);
        running.store(true);
        
        // Start balancing thread
        balancingThread = std::thread(&ConsensusBalancer::balancingLoop, this);
        
        Logger::info("ConsensusBalancer initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize ConsensusBalancer: " + std::string(e.what()));
        return false;
    }
}

void ConsensusBalancer::shutdown() {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    if (!initialized.load()) {
        return;
    }
    
    Logger::info("Shutting down ConsensusBalancer");
    
    // Stop balancing thread
    shouldStop.store(true);
    running.store(false);
    balancerCV.notify_all();
    
    if (balancingThread.joinable()) {
        balancingThread.join();
    }
    
    // Clear engines
    engines.clear();
    router = nullptr;
    
    // Clear historical data
    historicalMetrics.clear();
    
    initialized.store(false);
    Logger::info("ConsensusBalancer shut down successfully");
}

bool ConsensusBalancer::registerEngine(ConsensusType type, ConsensusEngine* engine) {
    if (!engine || !validateEngine(engine)) {
        Logger::error("Cannot register invalid consensus engine");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    try {
        engines[type] = engine;
        
        // Initialize metrics for this engine if not already present
        if (currentMetrics.participationMetrics.find(type) == currentMetrics.participationMetrics.end()) {
            currentMetrics.participationMetrics[type] = ParticipationMetrics{};
            currentMetrics.participationRates[type] = 0.0;
            currentMetrics.dominanceRatios[type] = 0.0;
            historicalMetrics[type] = std::vector<ParticipationMetrics>();
        }
        
        Logger::info("Registered consensus engine for balancing: " + consensusTypeToString(type));
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to register consensus engine: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusBalancer::unregisterEngine(ConsensusType type) {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    auto it = engines.find(type);
    if (it == engines.end()) {
        Logger::warning("Attempted to unregister non-existent engine: " + consensusTypeToString(type));
        return false;
    }
    
    engines.erase(it);
    Logger::info("Unregistered consensus engine: " + consensusTypeToString(type));
    return true;
}

void ConsensusBalancer::balanceConsensusParticipation() {
    if (!initialized.load() || !running.load()) {
        Logger::warning("ConsensusBalancer not running, cannot balance participation");
        return;
    }
    
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    try {
        Logger::info("Starting consensus participation balancing");
        
        // Collect current metrics
        collectMetrics();
        
        // Analyze balance
        analyzeBalance();
        
        // Apply adjustments if needed
        if (!currentMetrics.isBalanced) {
            applyBalanceAdjustments();
            totalRebalances++;
            currentMetrics.lastRebalanceTime = getCurrentTimestamp();
        }
        
        // Update historical data
        updateHistoricalData();
        
        Logger::info("Consensus participation balancing completed");
        
    } catch (const std::exception& e) {
        Logger::error("Error during consensus balancing: " + std::string(e.what()));
    }
}

void ConsensusBalancer::adjustDifficulty(ConsensusType type, double adjustment) {
    if (!isValidAdjustment(adjustment)) {
        Logger::error("Invalid difficulty adjustment: " + std::to_string(adjustment));
        return;
    }
    
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::warning("Cannot adjust difficulty for unregistered engine: " + consensusTypeToString(type));
        return;
    }
    
    try {
        // Get current parameters
        auto currentParams = engineIt->second->getParameters();
        
        // Calculate new difficulty
        double currentDifficulty = currentParams.count("difficulty") > 0 ? 
            currentParams["difficulty"] : 1.0;
        double newDifficulty = currentDifficulty * (1.0 + adjustment);
        
        // Ensure difficulty stays within reasonable bounds
        newDifficulty = std::max(0.1, std::min(newDifficulty, 100.0));
        
        // Apply adjustment
        std::map<std::string, double> newParams = {{"difficulty", newDifficulty}};
        if (engineIt->second->adjustParameters(newParams)) {
            // Update metrics
            currentMetrics.participationMetrics[type].currentDifficulty = newDifficulty;
            
            Logger::info("Adjusted difficulty for " + consensusTypeToString(type) + 
                        " from " + std::to_string(currentDifficulty) + 
                        " to " + std::to_string(newDifficulty) + 
                        " (adjustment: " + std::to_string(adjustment * 100) + "%)");
        } else {
            Logger::error("Failed to apply difficulty adjustment for " + consensusTypeToString(type));
        }
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting difficulty: " + std::string(e.what()));
    }
}

void ConsensusBalancer::adjustRewards(ConsensusType type, double multiplier) {
    if (multiplier <= 0.0 || multiplier > 10.0) {
        Logger::error("Invalid reward multiplier: " + std::to_string(multiplier));
        return;
    }
    
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        Logger::warning("Cannot adjust rewards for unregistered engine: " + consensusTypeToString(type));
        return;
    }
    
    try {
        // Get current parameters
        auto currentParams = engineIt->second->getParameters();
        
        // Calculate new reward multiplier
        double currentMultiplier = currentParams.count("rewardMultiplier") > 0 ? 
            currentParams["rewardMultiplier"] : 1.0;
        double newMultiplier = currentMultiplier * multiplier;
        
        // Ensure multiplier stays within reasonable bounds
        newMultiplier = std::max(0.1, std::min(newMultiplier, 5.0));
        
        // Apply adjustment
        std::map<std::string, double> newParams = {{"rewardMultiplier", newMultiplier}};
        if (engineIt->second->adjustParameters(newParams)) {
            // Update metrics
            currentMetrics.participationMetrics[type].currentRewardMultiplier = newMultiplier;
            
            Logger::info("Adjusted reward multiplier for " + consensusTypeToString(type) + 
                        " from " + std::to_string(currentMultiplier) + 
                        " to " + std::to_string(newMultiplier) + 
                        " (multiplier: " + std::to_string(multiplier) + ")");
        } else {
            Logger::error("Failed to apply reward adjustment for " + consensusTypeToString(type));
        }
        
    } catch (const std::exception& e) {
        Logger::error("Error adjusting rewards: " + std::string(e.what()));
    }
}

bool ConsensusBalancer::performAutomaticRebalancing() {
    if (!initialized.load() || !running.load()) {
        return false;
    }
    
    try {
        Logger::info("Performing automatic rebalancing");
        
        balanceConsensusParticipation();
        
        // Check if emergency mode is needed
        checkEmergencyConditions();
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Error during automatic rebalancing: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusBalancer::rebalanceSpecificMechanism(ConsensusType type) {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    auto it = currentMetrics.participationMetrics.find(type);
    if (it == currentMetrics.participationMetrics.end()) {
        Logger::error("Cannot rebalance unknown mechanism: " + consensusTypeToString(type));
        return false;
    }
    
    try {
        Logger::info("Rebalancing specific mechanism: " + consensusTypeToString(type));
        
        // Update metrics for this mechanism
        updateParticipationMetrics(type);
        
        // Calculate required adjustments
        double difficultyAdjustment = calculateDifficultyAdjustment(type);
        double rewardAdjustment = calculateRewardAdjustment(type);
        
        // Apply adjustments
        if (std::abs(difficultyAdjustment) > 0.01) {
            adjustDifficulty(type, difficultyAdjustment);
        }
        
        if (std::abs(rewardAdjustment - 1.0) > 0.01) {
            adjustRewards(type, rewardAdjustment);
        }
        
        Logger::info("Specific mechanism rebalancing completed: " + consensusTypeToString(type));
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Error rebalancing specific mechanism: " + std::string(e.what()));
        return false;
    }
}

void ConsensusBalancer::optimizeNetworkPerformance() {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    try {
        Logger::info("Optimizing network performance");
        
        // Collect current metrics
        collectMetrics();
        
        // Identify underperforming mechanisms
        for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
            if (metrics.getEfficiencyScore() < config.minEfficiencyScore) {
                Logger::info("Optimizing underperforming mechanism: " + consensusTypeToString(type));
                
                // Reduce difficulty to improve performance
                double difficultyReduction = -0.1; // 10% reduction
                adjustDifficulty(type, difficultyReduction);
                
                // Increase rewards to incentivize participation
                double rewardIncrease = 1.1; // 10% increase
                adjustRewards(type, rewardIncrease);
            }
        }
        
        Logger::info("Network performance optimization completed");
        
    } catch (const std::exception& e) {
        Logger::error("Error optimizing network performance: " + std::string(e.what()));
    }
}

BalanceMetrics ConsensusBalancer::getBalanceMetrics() const {
    std::lock_guard<std::mutex> lock(balancerMutex);
    return currentMetrics;
}

ParticipationMetrics ConsensusBalancer::getParticipationMetrics(ConsensusType type) const {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    auto it = currentMetrics.participationMetrics.find(type);
    return (it != currentMetrics.participationMetrics.end()) ? it->second : ParticipationMetrics{};
}

std::vector<ParticipationMetrics> ConsensusBalancer::getHistoricalMetrics(ConsensusType type) const {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    auto it = historicalMetrics.find(type);
    return (it != historicalMetrics.end()) ? it->second : std::vector<ParticipationMetrics>{};
}

nlohmann::json ConsensusBalancer::getDetailedAnalysis() const {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    nlohmann::json analysis;
    
    // Overall metrics
    analysis["overallBalance"] = currentMetrics.overallBalance;
    analysis["networkEfficiency"] = currentMetrics.networkEfficiency;
    analysis["isBalanced"] = currentMetrics.isBalanced;
    analysis["emergencyMode"] = currentMetrics.emergencyMode;
    analysis["networkHealth"] = currentMetrics.getNetworkHealth();
    analysis["mostDominant"] = consensusTypeToString(currentMetrics.getMostDominant());
    
    // Participation metrics
    nlohmann::json participationData;
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        nlohmann::json mechanismData;
        mechanismData["totalValidations"] = metrics.totalValidations;
        mechanismData["successfulValidations"] = metrics.successfulValidations;
        mechanismData["participationRate"] = metrics.getParticipationRate();
        mechanismData["efficiencyScore"] = metrics.getEfficiencyScore();
        mechanismData["averageResponseTime"] = metrics.averageResponseTime;
        mechanismData["currentDifficulty"] = metrics.currentDifficulty;
        mechanismData["currentRewardMultiplier"] = metrics.currentRewardMultiplier;
        mechanismData["activeParticipants"] = metrics.activeParticipants;
        
        participationData[consensusTypeToString(type)] = mechanismData;
    }
    analysis["participationMetrics"] = participationData;
    
    // Dominance ratios
    nlohmann::json dominanceData;
    for (const auto& [type, ratio] : currentMetrics.dominanceRatios) {
        dominanceData[consensusTypeToString(type)] = ratio;
    }
    analysis["dominanceRatios"] = dominanceData;
    
    // Recommendations
    analysis["recommendations"] = currentMetrics.recommendations;
    
    // Imbalance analysis
    analysis["imbalances"] = analyzeImbalances();
    
    return analysis;
}

void ConsensusBalancer::setBalanceConfig(const BalanceConfig& newConfig) {
    if (!validateConfig(newConfig)) {
        Logger::error("Invalid balance configuration provided");
        return;
    }
    
    std::lock_guard<std::mutex> lock(balancerMutex);
    config = newConfig;
    Logger::info("Balance configuration updated");
}

BalanceConfig ConsensusBalancer::getBalanceConfig() const {
    std::lock_guard<std::mutex> lock(balancerMutex);
    return config;
}

bool ConsensusBalancer::updateTargetParticipation(ConsensusType type, double targetRate) {
    if (targetRate < 0.0 || targetRate > 1.0) {
        Logger::error("Invalid target participation rate: " + std::to_string(targetRate));
        return false;
    }
    
    std::lock_guard<std::mutex> lock(balancerMutex);
    config.targetParticipationRates[type] = targetRate;
    
    Logger::info("Updated target participation rate for " + consensusTypeToString(type) + 
                ": " + std::to_string(targetRate * 100) + "%");
    return true;
}

bool ConsensusBalancer::enterEmergencyMode() {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    if (emergencyMode.load()) {
        Logger::warning("Already in emergency mode");
        return true;
    }
    
    try {
        Logger::warning("Entering emergency mode");
        
        emergencyMode.store(true);
        currentMetrics.emergencyMode = true;
        emergencyActivations++;
        
        activateEmergencyProtocols();
        
        Logger::warning("Emergency mode activated");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to enter emergency mode: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusBalancer::exitEmergencyMode() {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    if (!emergencyMode.load()) {
        Logger::info("Not in emergency mode");
        return true;
    }
    
    try {
        Logger::info("Exiting emergency mode");
        
        deactivateEmergencyProtocols();
        
        emergencyMode.store(false);
        currentMetrics.emergencyMode = false;
        
        Logger::info("Emergency mode deactivated");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to exit emergency mode: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> ConsensusBalancer::analyzeImbalances() const {
    std::vector<std::string> imbalances;
    
    for (const auto& [type, rate] : currentMetrics.participationRates) {
        auto targetIt = config.targetParticipationRates.find(type);
        if (targetIt != config.targetParticipationRates.end()) {
            double target = targetIt->second;
            double deviation = std::abs(rate - target) / target;
            
            if (deviation > config.balanceThreshold) {
                std::string imbalance = consensusTypeToString(type) + 
                    " participation rate (" + std::to_string(rate * 100) + 
                    "%) deviates from target (" + std::to_string(target * 100) + 
                    "%) by " + std::to_string(deviation * 100) + "%";
                imbalances.push_back(imbalance);
            }
        }
    }
    
    // Check for dominance issues
    for (const auto& [type, ratio] : currentMetrics.dominanceRatios) {
        if (ratio > config.maxDominanceThreshold) {
            std::string dominance = consensusTypeToString(type) + 
                " is over-dominant with " + std::to_string(ratio * 100) + 
                "% of network activity";
            imbalances.push_back(dominance);
        }
    }
    
    return imbalances;
}

std::vector<std::string> ConsensusBalancer::generateRecommendations() const {
    std::vector<std::string> recommendations;
    
    // Analyze each mechanism
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        if (metrics.getEfficiencyScore() < config.minEfficiencyScore) {
            recommendations.push_back("Improve " + consensusTypeToString(type) + 
                " efficiency (current: " + std::to_string(metrics.getEfficiencyScore() * 100) + "%)");
        }
        
        if (metrics.averageResponseTime > config.targetResponseTime * 2) {
            recommendations.push_back("Reduce " + consensusTypeToString(type) + 
                " response time (current: " + std::to_string(metrics.averageResponseTime) + "s)");
        }
        
        auto targetIt = config.targetParticipationRates.find(type);
        if (targetIt != config.targetParticipationRates.end()) {
            double currentRate = currentMetrics.participationRates.at(type);
            double target = targetIt->second;
            
            if (currentRate < target * 0.8) {
                recommendations.push_back("Increase " + consensusTypeToString(type) + 
                    " participation (current: " + std::to_string(currentRate * 100) + 
                    "%, target: " + std::to_string(target * 100) + "%)");
            } else if (currentRate > target * 1.2) {
                recommendations.push_back("Reduce " + consensusTypeToString(type) + 
                    " dominance (current: " + std::to_string(currentRate * 100) + 
                    "%, target: " + std::to_string(target * 100) + "%)");
            }
        }
    }
    
    return recommendations;
}

bool ConsensusBalancer::detectNetworkThreats() const {
    // Check for 51% attacks or similar threats
    for (const auto& [type, ratio] : currentMetrics.dominanceRatios) {
        if (ratio > 0.51) {
            Logger::warning("Potential 51% attack detected in " + consensusTypeToString(type));
            return true;
        }
    }
    
    // Check for network partitioning
    double totalParticipation = 0.0;
    for (const auto& [type, rate] : currentMetrics.participationRates) {
        totalParticipation += rate;
    }
    
    if (totalParticipation < 0.5) {
        Logger::warning("Low network participation detected: " + std::to_string(totalParticipation * 100) + "%");
        return true;
    }
    
    return false;
}

nlohmann::json ConsensusBalancer::getStatistics() const {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    nlohmann::json stats;
    stats["totalRebalances"] = totalRebalances;
    stats["emergencyActivations"] = emergencyActivations;
    stats["isInitialized"] = initialized.load();
    stats["isRunning"] = running.load();
    stats["isInEmergencyMode"] = emergencyMode.load();
    stats["registeredEngines"] = engines.size();
    
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - lastBalanceCheck).count();
    stats["uptimeSeconds"] = uptime;
    
    return stats;
}

void ConsensusBalancer::resetStatistics() {
    std::lock_guard<std::mutex> lock(balancerMutex);
    
    totalRebalances = 0;
    emergencyActivations = 0;
    lastBalanceCheck = std::chrono::steady_clock::now();
    
    Logger::info("ConsensusBalancer statistics reset");
}

// Private methods implementation

void ConsensusBalancer::balancingLoop() {
    Logger::info("ConsensusBalancer background thread started");
    
    while (running.load() && !shouldStop.load()) {
        try {
            std::unique_lock<std::mutex> lock(balancerMutex);
            
            // Wait for rebalancing interval or stop signal
            balancerCV.wait_for(lock, std::chrono::seconds(config.rebalancingInterval), 
                [this] { return shouldStop.load(); });
            
            if (shouldStop.load()) {
                break;
            }
            
            lock.unlock();
            
            // Perform periodic balancing
            performAutomaticRebalancing();
            
        } catch (const std::exception& e) {
            Logger::error("Error in balancing loop: " + std::string(e.what()));
            
            // Sleep briefly before retrying
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    
    Logger::info("ConsensusBalancer background thread stopped");
}

void ConsensusBalancer::collectMetrics() {
    // Update metrics for all registered engines
    for (const auto& [type, engine] : engines) {
        updateParticipationMetrics(type);
    }
    
    // Calculate derived metrics
    calculateParticipationRates();
    calculateDominanceRatios();
    
    // Update overall metrics
    currentMetrics.networkEfficiency = currentMetrics.getNetworkHealth();
    currentMetrics.overallBalance = 1.0 - calculateImbalanceScore();
}

void ConsensusBalancer::analyzeBalance() {
    // Check if network is balanced
    currentMetrics.isBalanced = isNetworkBalanced();
    
    // Generate recommendations
    currentMetrics.recommendations = generateRecommendations();
    
    // Check for emergency conditions
    checkEmergencyConditions();
}

void ConsensusBalancer::applyBalanceAdjustments() {
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        // Calculate required adjustments
        double difficultyAdjustment = calculateDifficultyAdjustment(type);
        double rewardAdjustment = calculateRewardAdjustment(type);
        
        // Apply gradual adjustments
        if (std::abs(difficultyAdjustment) > 0.01) {
            applyGradualAdjustment(type, difficultyAdjustment);
        }
        
        if (std::abs(rewardAdjustment - 1.0) > 0.01) {
            adjustRewards(type, rewardAdjustment);
        }
    }
}

void ConsensusBalancer::updateParticipationMetrics(ConsensusType type) {
    auto engineIt = engines.find(type);
    if (engineIt == engines.end()) {
        return;
    }
    
    try {
        // Get engine metrics
        nlohmann::json engineMetrics = engineIt->second->getMetrics();
        
        // Update participation metrics
        ParticipationMetrics& metrics = currentMetrics.participationMetrics[type];
        
        if (engineMetrics.contains("totalValidations")) {
            metrics.totalValidations = engineMetrics["totalValidations"];
        }
        
        if (engineMetrics.contains("successfulValidations")) {
            metrics.successfulValidations = engineMetrics["successfulValidations"];
        }
        
        if (engineMetrics.contains("failedValidations")) {
            metrics.failedValidations = engineMetrics["failedValidations"];
        }
        
        if (engineMetrics.contains("averageResponseTime")) {
            metrics.averageResponseTime = engineMetrics["averageResponseTime"];
        }
        
        if (engineMetrics.contains("activeParticipants")) {
            metrics.activeParticipants = engineMetrics["activeParticipants"];
        }
        
        if (engineMetrics.contains("networkHashRate")) {
            metrics.networkHashRate = engineMetrics["networkHashRate"];
        }
        
        if (engineMetrics.contains("totalStake")) {
            metrics.totalStake = engineMetrics["totalStake"];
        }
        
        if (engineMetrics.contains("resourceContribution")) {
            metrics.resourceContribution = engineMetrics["resourceContribution"];
        }
        
        metrics.lastUpdateTime = getCurrentTimestamp();
        
    } catch (const std::exception& e) {
        Logger::error("Error updating participation metrics for " + 
                     consensusTypeToString(type) + ": " + std::string(e.what()));
    }
}

void ConsensusBalancer::calculateParticipationRates() {
    uint64_t totalValidations = 0;
    
    // Calculate total validations across all mechanisms
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        totalValidations += metrics.totalValidations;
    }
    
    // Calculate participation rates
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        if (totalValidations > 0) {
            currentMetrics.participationRates[type] = 
                static_cast<double>(metrics.totalValidations) / totalValidations;
        } else {
            currentMetrics.participationRates[type] = 0.0;
        }
    }
}

void ConsensusBalancer::calculateDominanceRatios() {
    // Find the most active mechanism
    uint64_t maxValidations = 0;
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        maxValidations = std::max(maxValidations, metrics.totalValidations);
    }
    
    // Calculate dominance ratios
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        if (maxValidations > 0) {
            currentMetrics.dominanceRatios[type] = 
                static_cast<double>(metrics.totalValidations) / maxValidations;
        } else {
            currentMetrics.dominanceRatios[type] = 0.0;
        }
    }
}

void ConsensusBalancer::updateHistoricalData() {
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        auto& history = historicalMetrics[type];
        
        // Add current metrics to history
        history.push_back(metrics);
        
        // Limit history size
        if (history.size() > maxHistorySize) {
            history.erase(history.begin());
        }
    }
}

bool ConsensusBalancer::isNetworkBalanced() const {
    double imbalanceScore = calculateImbalanceScore();
    return imbalanceScore < config.balanceThreshold;
}

bool ConsensusBalancer::isMechanismOverDominant(ConsensusType type) const {
    auto it = currentMetrics.dominanceRatios.find(type);
    if (it == currentMetrics.dominanceRatios.end()) {
        return false;
    }
    
    return it->second > config.maxDominanceThreshold;
}

bool ConsensusBalancer::isMechanismUnderPerforming(ConsensusType type) const {
    auto it = currentMetrics.participationMetrics.find(type);
    if (it == currentMetrics.participationMetrics.end()) {
        return false;
    }
    
    return it->second.getEfficiencyScore() < config.minEfficiencyScore;
}

double ConsensusBalancer::calculateImbalanceScore() const {
    double totalDeviation = 0.0;
    size_t count = 0;
    
    for (const auto& [type, rate] : currentMetrics.participationRates) {
        auto targetIt = config.targetParticipationRates.find(type);
        if (targetIt != config.targetParticipationRates.end()) {
            double target = targetIt->second;
            double deviation = std::abs(rate - target) / std::max(target, 0.01);
            totalDeviation += deviation;
            count++;
        }
    }
    
    return count > 0 ? totalDeviation / count : 0.0;
}

double ConsensusBalancer::calculateDifficultyAdjustment(ConsensusType type) const {
    auto metricsIt = currentMetrics.participationMetrics.find(type);
    auto targetIt = config.targetParticipationRates.find(type);
    
    if (metricsIt == currentMetrics.participationMetrics.end() || 
        targetIt == config.targetParticipationRates.end()) {
        return 0.0;
    }
    
    double currentRate = currentMetrics.participationRates.at(type);
    double targetRate = targetIt->second;
    double deviation = (currentRate - targetRate) / targetRate;
    
    // If participation is too high, increase difficulty
    // If participation is too low, decrease difficulty
    double adjustment = deviation * config.maxDifficultyAdjustment;
    
    // Clamp adjustment
    return std::max(-config.maxDifficultyAdjustment, 
                   std::min(adjustment, config.maxDifficultyAdjustment));
}

double ConsensusBalancer::calculateRewardAdjustment(ConsensusType type) const {
    auto metricsIt = currentMetrics.participationMetrics.find(type);
    auto targetIt = config.targetParticipationRates.find(type);
    
    if (metricsIt == currentMetrics.participationMetrics.end() || 
        targetIt == config.targetParticipationRates.end()) {
        return 1.0;
    }
    
    double currentRate = currentMetrics.participationRates.at(type);
    double targetRate = targetIt->second;
    double deviation = (targetRate - currentRate) / targetRate;
    
    // If participation is too low, increase rewards
    // If participation is too high, decrease rewards
    double adjustment = 1.0 + (deviation * config.maxRewardAdjustment);
    
    // Clamp adjustment
    return std::max(1.0 - config.maxRewardAdjustment, 
                   std::min(adjustment, 1.0 + config.maxRewardAdjustment));
}

void ConsensusBalancer::applyGradualAdjustment(ConsensusType type, double targetAdjustment) {
    // Apply adjustment gradually to avoid shock
    double gradualAdjustment = targetAdjustment * 0.5; // Apply 50% of target adjustment
    adjustDifficulty(type, gradualAdjustment);
}

void ConsensusBalancer::checkEmergencyConditions() {
    bool shouldEnterEmergency = false;
    
    // Check for extreme imbalances
    double imbalanceScore = calculateImbalanceScore();
    if (imbalanceScore > config.emergencyImbalanceThreshold) {
        Logger::warning("Extreme imbalance detected: " + std::to_string(imbalanceScore));
        shouldEnterEmergency = true;
    }
    
    // Check for network threats
    if (detectNetworkThreats()) {
        shouldEnterEmergency = true;
    }
    
    // Check for mechanism failures
    for (const auto& [type, rate] : currentMetrics.participationRates) {
        if (rate < config.minParticipationThreshold) {
            Logger::warning("Mechanism below minimum participation: " + consensusTypeToString(type));
            shouldEnterEmergency = true;
        }
    }
    
    if (shouldEnterEmergency && !emergencyMode.load()) {
        enterEmergencyMode();
    } else if (!shouldEnterEmergency && emergencyMode.load()) {
        exitEmergencyMode();
    }
}

void ConsensusBalancer::handleEmergencyImbalance() {
    Logger::warning("Handling emergency imbalance");
    
    // Apply aggressive rebalancing
    for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
        if (isMechanismOverDominant(type)) {
            // Increase difficulty and reduce rewards for dominant mechanisms
            adjustDifficulty(type, config.maxDifficultyAdjustment);
            adjustRewards(type, 1.0 - config.maxRewardAdjustment);
        } else if (isMechanismUnderPerforming(type)) {
            // Decrease difficulty and increase rewards for underperforming mechanisms
            adjustDifficulty(type, -config.maxDifficultyAdjustment);
            adjustRewards(type, 1.0 + config.maxRewardAdjustment);
        }
    }
}

void ConsensusBalancer::activateEmergencyProtocols() {
    Logger::warning("Activating emergency protocols");
    
    // Handle emergency imbalance
    handleEmergencyImbalance();
    
    // Increase monitoring frequency
    // This would be implemented by reducing the rebalancing interval temporarily
    
    // Log emergency event
    nlohmann::json emergencyData;
    emergencyData["timestamp"] = getCurrentTimestamp();
    emergencyData["imbalanceScore"] = calculateImbalanceScore();
    emergencyData["networkHealth"] = currentMetrics.getNetworkHealth();
    logBalanceEvent("EMERGENCY_MODE_ACTIVATED", emergencyData);
}

void ConsensusBalancer::deactivateEmergencyProtocols() {
    Logger::info("Deactivating emergency protocols");
    
    // Log emergency resolution
    nlohmann::json resolutionData;
    resolutionData["timestamp"] = getCurrentTimestamp();
    resolutionData["imbalanceScore"] = calculateImbalanceScore();
    resolutionData["networkHealth"] = currentMetrics.getNetworkHealth();
    logBalanceEvent("EMERGENCY_MODE_DEACTIVATED", resolutionData);
}

std::string ConsensusBalancer::consensusTypeToString(ConsensusType type) const {
    switch (type) {
        case ConsensusType::PROOF_OF_WORK:
            return "PROOF_OF_WORK";
        case ConsensusType::PROOF_OF_STAKE:
            return "PROOF_OF_STAKE";
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
            return "PROOF_OF_RESOURCE_CONTRIBUTION";
        case ConsensusType::VOTING_CONSENSUS:
            return "VOTING_CONSENSUS";
        case ConsensusType::SMART_CONTRACT_VALIDATION:
            return "SMART_CONTRACT_VALIDATION";
        default:
            return "UNKNOWN";
    }
}

double ConsensusBalancer::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool ConsensusBalancer::isValidAdjustment(double adjustment) const {
    return std::isfinite(adjustment) && 
           adjustment >= -1.0 && 
           adjustment <= 1.0;
}

void ConsensusBalancer::logBalanceEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logData = data;
    logData["event"] = event;
    logData["balancer"] = "ConsensusBalancer";
    
    Logger::info("Balance event: " + event + " - " + logData.dump());
}

bool ConsensusBalancer::validateConfig(const BalanceConfig& config) const {
    // Validate target participation rates
    double totalTarget = 0.0;
    for (const auto& [type, rate] : config.targetParticipationRates) {
        if (rate < 0.0 || rate > 1.0) {
            Logger::error("Invalid target participation rate for " + consensusTypeToString(type));
            return false;
        }
        totalTarget += rate;
    }
    
    if (totalTarget > 1.1) { // Allow some tolerance
        Logger::error("Total target participation rates exceed 100%");
        return false;
    }
    
    // Validate adjustment parameters
    if (config.maxDifficultyAdjustment <= 0.0 || config.maxDifficultyAdjustment > 1.0) {
        Logger::error("Invalid max difficulty adjustment");
        return false;
    }
    
    if (config.maxRewardAdjustment <= 0.0 || config.maxRewardAdjustment > 1.0) {
        Logger::error("Invalid max reward adjustment");
        return false;
    }
    
    // Validate thresholds
    if (config.balanceThreshold <= 0.0 || config.balanceThreshold > 1.0) {
        Logger::error("Invalid balance threshold");
        return false;
    }
    
    if (config.emergencyImbalanceThreshold <= config.balanceThreshold) {
        Logger::error("Emergency threshold must be greater than balance threshold");
        return false;
    }
    
    return true;
}

bool ConsensusBalancer::validateEngine(ConsensusEngine* engine) const {
    if (!engine) {
        return false;
    }
    
    try {
        // Check if engine is healthy
        if (!engine->isHealthy()) {
            Logger::warning("Engine is not healthy: " + engine->getName());
            return false;
        }
        
        // Check if engine supports required methods
        engine->getMetrics();
        engine->getParameters();
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Engine validation failed: " + std::string(e.what()));
        return false;
    }
}