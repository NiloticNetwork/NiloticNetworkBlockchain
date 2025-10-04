#ifndef CONSENSUS_BALANCER_H
#define CONSENSUS_BALANCER_H

#include "consensus_harmony.h"
#include "logger.h"
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>

// Forward declarations
class ConsensusRouter;
class ConsensusEngine;

/**
 * Participation metrics for a consensus mechanism
 */
struct ParticipationMetrics {
    uint64_t totalValidations = 0;
    uint64_t successfulValidations = 0;
    uint64_t failedValidations = 0;
    double averageResponseTime = 0.0;
    double currentDifficulty = 1.0;
    double currentRewardMultiplier = 1.0;
    uint64_t activeParticipants = 0;
    double networkHashRate = 0.0;  // For PoW
    double totalStake = 0.0;       // For PoS
    double resourceContribution = 0.0; // For PoRC
    uint64_t lastUpdateTime = 0;
    
    // Calculate participation rate
    double getParticipationRate() const {
        return totalValidations > 0 ? 
            static_cast<double>(successfulValidations) / totalValidations : 0.0;
    }
    
    // Calculate efficiency score
    double getEfficiencyScore() const {
        double participationRate = getParticipationRate();
        double responseScore = averageResponseTime > 0 ? 1.0 / averageResponseTime : 1.0;
        return (participationRate * 0.7) + (responseScore * 0.3);
    }
};

/**
 * Balance configuration parameters
 */
struct BalanceConfig {
    // Target participation rates for each mechanism
    std::map<ConsensusType, double> targetParticipationRates = {
        {ConsensusType::PROOF_OF_WORK, 0.3},
        {ConsensusType::PROOF_OF_STAKE, 0.3},
        {ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.25},
        {ConsensusType::VOTING_CONSENSUS, 0.1},
        {ConsensusType::SMART_CONTRACT_VALIDATION, 0.05}
    };
    
    // Adjustment parameters
    double maxDifficultyAdjustment = 0.25;    // Max 25% adjustment per cycle
    double maxRewardAdjustment = 0.20;        // Max 20% reward adjustment per cycle
    double balanceThreshold = 0.15;           // 15% deviation triggers rebalancing
    uint64_t rebalancingInterval = 3600;      // 1 hour in seconds
    uint64_t metricsWindow = 86400;           // 24 hours metrics window
    
    // Emergency thresholds
    double emergencyImbalanceThreshold = 0.5; // 50% imbalance triggers emergency mode
    double minParticipationThreshold = 0.05;  // 5% minimum participation
    double maxDominanceThreshold = 0.7;       // 70% maximum dominance
    
    // Performance targets
    double targetResponseTime = 1.0;          // 1 second target response time
    double minEfficiencyScore = 0.6;          // 60% minimum efficiency
};

/**
 * Balance metrics and analysis
 */
struct BalanceMetrics {
    std::map<ConsensusType, ParticipationMetrics> participationMetrics;
    std::map<ConsensusType, double> participationRates;
    std::map<ConsensusType, double> dominanceRatios;
    double overallBalance = 1.0;
    double networkEfficiency = 1.0;
    bool isBalanced = true;
    bool emergencyMode = false;
    uint64_t lastRebalanceTime = 0;
    std::vector<std::string> recommendations;
    
    // Calculate overall network health
    double getNetworkHealth() const {
        double totalHealth = 0.0;
        size_t count = 0;
        
        for (const auto& [type, metrics] : participationMetrics) {
            totalHealth += metrics.getEfficiencyScore();
            count++;
        }
        
        return count > 0 ? totalHealth / count : 0.0;
    }
    
    // Get most dominant mechanism
    ConsensusType getMostDominant() const {
        ConsensusType dominant = ConsensusType::PROOF_OF_WORK;
        double maxRate = 0.0;
        
        for (const auto& [type, rate] : participationRates) {
            if (rate > maxRate) {
                maxRate = rate;
                dominant = type;
            }
        }
        
        return dominant;
    }
};

/**
 * Consensus Balancer - Maintains balance between different consensus mechanisms
 * Implements participation monitoring, automatic difficulty and reward adjustment,
 * and balancing metrics collection and analysis
 */
class ConsensusBalancer {
private:
    // Configuration and state
    BalanceConfig config;
    BalanceMetrics currentMetrics;
    std::atomic<bool> initialized;
    std::atomic<bool> running;
    std::atomic<bool> emergencyMode;
    
    // Thread safety
    mutable std::mutex balancerMutex;
    std::condition_variable balancerCV;
    
    // Background processing
    std::thread balancingThread;
    std::atomic<bool> shouldStop;
    
    // Consensus engines reference
    std::map<ConsensusType, ConsensusEngine*> engines;
    ConsensusRouter* router;
    
    // Historical data
    std::map<ConsensusType, std::vector<ParticipationMetrics>> historicalMetrics;
    uint64_t maxHistorySize = 168; // 1 week of hourly data
    
    // Statistics
    uint64_t totalRebalances = 0;
    uint64_t emergencyActivations = 0;
    std::chrono::steady_clock::time_point lastBalanceCheck;

public:
    ConsensusBalancer();
    explicit ConsensusBalancer(const BalanceConfig& customConfig);
    ~ConsensusBalancer();
    
    // Lifecycle management
    bool initialize();
    bool initialize(ConsensusRouter* consensusRouter);
    void shutdown();
    bool isInitialized() const { return initialized.load(); }
    bool isRunning() const { return running.load(); }
    
    // Engine management
    bool registerEngine(ConsensusType type, ConsensusEngine* engine);
    bool unregisterEngine(ConsensusType type);
    void setRouter(ConsensusRouter* consensusRouter) { router = consensusRouter; }
    
    // Core balancing functionality
    void balanceConsensusParticipation();
    void adjustDifficulty(ConsensusType type, double adjustment);
    void adjustRewards(ConsensusType type, double multiplier);
    
    // Advanced balancing methods
    bool performAutomaticRebalancing();
    bool rebalanceSpecificMechanism(ConsensusType type);
    void optimizeNetworkPerformance();
    
    // Metrics and monitoring
    BalanceMetrics getBalanceMetrics() const;
    ParticipationMetrics getParticipationMetrics(ConsensusType type) const;
    std::vector<ParticipationMetrics> getHistoricalMetrics(ConsensusType type) const;
    nlohmann::json getDetailedAnalysis() const;
    
    // Configuration management
    void setBalanceConfig(const BalanceConfig& newConfig);
    BalanceConfig getBalanceConfig() const;
    bool updateTargetParticipation(ConsensusType type, double targetRate);
    
    // Emergency operations
    bool enterEmergencyMode();
    bool exitEmergencyMode();
    bool isInEmergencyMode() const { return emergencyMode.load(); }
    
    // Analysis and recommendations
    std::vector<std::string> analyzeImbalances() const;
    std::vector<std::string> generateRecommendations() const;
    bool detectNetworkThreats() const;
    
    // Statistics
    nlohmann::json getStatistics() const;
    void resetStatistics();

private:
    // Internal balancing logic
    void balancingLoop();
    void collectMetrics();
    void analyzeBalance();
    void applyBalanceAdjustments();
    
    // Metrics collection
    void updateParticipationMetrics(ConsensusType type);
    void calculateParticipationRates();
    void calculateDominanceRatios();
    void updateHistoricalData();
    
    // Balance analysis
    bool isNetworkBalanced() const;
    bool isMechanismOverDominant(ConsensusType type) const;
    bool isMechanismUnderPerforming(ConsensusType type) const;
    double calculateImbalanceScore() const;
    
    // Adjustment algorithms
    double calculateDifficultyAdjustment(ConsensusType type) const;
    double calculateRewardAdjustment(ConsensusType type) const;
    void applyGradualAdjustment(ConsensusType type, double targetAdjustment);
    
    // Emergency handling
    void checkEmergencyConditions();
    void handleEmergencyImbalance();
    void activateEmergencyProtocols();
    void deactivateEmergencyProtocols();
    
    // Helper methods
    std::string consensusTypeToString(ConsensusType type) const;
    double getCurrentTimestamp() const;
    bool isValidAdjustment(double adjustment) const;
    void logBalanceEvent(const std::string& event, const nlohmann::json& data = {}) const;
    
    // Validation
    bool validateConfig(const BalanceConfig& config) const;
    bool validateEngine(ConsensusEngine* engine) const;
};

#endif // CONSENSUS_BALANCER_H