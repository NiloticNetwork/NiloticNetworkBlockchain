#ifndef PARAMETER_ADJUSTER_H
#define PARAMETER_ADJUSTER_H

#include "consensus_harmony.h"
#include "consensus_balancer.h"
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
 * Network condition metrics for parameter adjustment
 */
struct NetworkConditions {
    double networkLoad = 0.0;              // Current network load (0.0 - 1.0)
    double averageBlockTime = 600.0;       // Average block time in seconds
    double transactionThroughput = 0.0;    // Transactions per second
    uint64_t pendingTransactions = 0;      // Number of pending transactions
    double networkLatency = 0.0;           // Average network latency in ms
    uint64_t activeNodes = 0;              // Number of active network nodes
    double memoryUsage = 0.0;              // Memory usage percentage
    double cpuUsage = 0.0;                 // CPU usage percentage
    double averageResponseTime = 0.0;      // Average response time in seconds
    uint64_t timestamp = 0;                // When metrics were collected
    
    // Calculate overall network health score
    double getHealthScore() const {
        double loadScore = 1.0 - networkLoad;
        double throughputScore = std::min(transactionThroughput / 100.0, 1.0);
        double latencyScore = networkLatency > 0 ? std::min(1000.0 / networkLatency, 1.0) : 1.0;
        double resourceScore = (1.0 - memoryUsage) * (1.0 - cpuUsage);
        
        return (loadScore * 0.3) + (throughputScore * 0.3) + 
               (latencyScore * 0.2) + (resourceScore * 0.2);
    }
};

/**
 * Security threat detection metrics
 */
struct SecurityThreats {
    bool potentialFiftyOneAttack = false;   // 51% attack detected
    bool unusualHashRateSpike = false;      // Unusual hash rate changes
    bool suspiciousVotingPattern = false;   // Suspicious voting patterns
    bool networkPartitioning = false;       // Network partition detected
    bool ddosAttack = false;                // DDoS attack detected
    double threatLevel = 0.0;               // Overall threat level (0.0 - 1.0)
    std::vector<std::string> activeThreats; // List of active threats
    uint64_t lastThreatDetection = 0;       // Timestamp of last threat
    
    // Check if any critical threats are active
    bool hasCriticalThreats() const {
        return potentialFiftyOneAttack || networkPartitioning || ddosAttack;
    }
    
    // Get threat severity level
    std::string getThreatSeverity() const {
        if (threatLevel >= 0.8) return "CRITICAL";
        if (threatLevel >= 0.6) return "HIGH";
        if (threatLevel >= 0.4) return "MEDIUM";
        if (threatLevel >= 0.2) return "LOW";
        return "NONE";
    }
};

/**
 * Parameter adjustment strategy
 */
enum class AdjustmentStrategy {
    CONSERVATIVE,    // Small, gradual adjustments
    MODERATE,        // Balanced adjustments
    AGGRESSIVE,      // Large, rapid adjustments
    EMERGENCY        // Emergency response adjustments
};

/**
 * Parameter adjustment configuration
 */
struct ParameterAdjustmentConfig {
    // Monitoring intervals
    uint64_t monitoringInterval = 300;      // 5 minutes
    uint64_t adjustmentInterval = 1800;     // 30 minutes
    uint64_t emergencyCheckInterval = 60;   // 1 minute
    
    // Adjustment limits
    double maxDifficultyChange = 0.25;      // Max 25% difficulty change
    double maxRewardChange = 0.20;          // Max 20% reward change
    double maxStakeChange = 0.15;           // Max 15% stake requirement change
    double maxResourceChange = 0.30;        // Max 30% resource requirement change
    
    // Thresholds for adjustment triggers
    double loadThresholdHigh = 0.8;         // High load threshold
    double loadThresholdLow = 0.3;          // Low load threshold
    double latencyThresholdHigh = 5000.0;   // High latency threshold (ms)
    double throughputThresholdLow = 10.0;   // Low throughput threshold (TPS)
    
    // Security response thresholds
    double threatResponseThreshold = 0.4;   // Threat level requiring response
    double emergencyThreshold = 0.7;        // Emergency response threshold
    
    // Decentralization maintenance
    double maxMechanismDominance = 0.6;     // Max dominance for any mechanism
    double minMechanismParticipation = 0.05; // Min participation for any mechanism
    
    // Strategy selection
    AdjustmentStrategy defaultStrategy = AdjustmentStrategy::MODERATE;
    bool enableEmergencyMode = true;
    bool enableAutomaticAdjustment = true;
};

/**
 * Parameter adjustment record for audit trail
 */
struct AdjustmentRecord {
    uint64_t timestamp;
    ConsensusType mechanism;
    std::string parameter;
    double oldValue;
    double newValue;
    std::string reason;
    AdjustmentStrategy strategy;
    NetworkConditions conditions;
    SecurityThreats threats;
    
    AdjustmentRecord() : timestamp(0), mechanism(ConsensusType::PROOF_OF_WORK),
                        oldValue(0.0), newValue(0.0), strategy(AdjustmentStrategy::MODERATE) {}
};

/**
 * Parameter Adjuster - Automatically adjusts consensus parameters based on
 * network conditions, load, and security threats to maintain optimal performance
 * and decentralization
 */
class ParameterAdjuster {
private:
    // Configuration and state
    ParameterAdjustmentConfig config;
    std::atomic<bool> initialized;
    std::atomic<bool> running;
    std::atomic<bool> emergencyMode;
    
    // Thread safety
    mutable std::mutex adjusterMutex;
    std::condition_variable adjusterCV;
    
    // Background processing
    std::thread monitoringThread;
    std::thread adjustmentThread;
    std::atomic<bool> shouldStop;
    
    // Component references
    ConsensusRouter* router;
    ConsensusBalancer* balancer;
    std::map<ConsensusType, ConsensusEngine*> engines;
    
    // Current state
    NetworkConditions currentConditions;
    SecurityThreats currentThreats;
    std::vector<AdjustmentRecord> adjustmentHistory;
    uint64_t maxHistorySize = 1000;
    
    // Statistics
    uint64_t totalAdjustments = 0;
    uint64_t emergencyAdjustments = 0;
    std::chrono::steady_clock::time_point lastAdjustment;

public:
    ParameterAdjuster();
    explicit ParameterAdjuster(const ParameterAdjustmentConfig& customConfig);
    ~ParameterAdjuster();
    
    // Lifecycle management
    bool initialize();
    bool initialize(ConsensusRouter* consensusRouter, ConsensusBalancer* consensusBalancer);
    void shutdown();
    bool isInitialized() const { return initialized.load(); }
    bool isRunning() const { return running.load(); }
    
    // Component registration
    bool registerEngine(ConsensusType type, ConsensusEngine* engine);
    bool unregisterEngine(ConsensusType type);
    void setRouter(ConsensusRouter* consensusRouter) { router = consensusRouter; }
    void setBalancer(ConsensusBalancer* consensusBalancer) { balancer = consensusBalancer; }
    
    // Core functionality
    void monitorNetworkConditions();
    void detectSecurityThreats();
    bool adjustParametersBasedOnLoad();
    bool adjustParametersBasedOnThreats();
    bool maintainDecentralization();
    
    // Manual adjustment methods
    bool adjustDifficulty(ConsensusType type, double adjustment, const std::string& reason = "");
    bool adjustRewards(ConsensusType type, double multiplier, const std::string& reason = "");
    bool adjustStakeRequirements(ConsensusType type, double adjustment, const std::string& reason = "");
    bool adjustResourceRequirements(ConsensusType type, double adjustment, const std::string& reason = "");
    
    // Automatic adjustment control
    bool performAutomaticAdjustments();
    void enableAutomaticAdjustment() { config.enableAutomaticAdjustment = true; }
    void disableAutomaticAdjustment() { config.enableAutomaticAdjustment = false; }
    bool isAutomaticAdjustmentEnabled() const { return config.enableAutomaticAdjustment; }
    
    // Emergency mode
    bool enterEmergencyMode();
    bool exitEmergencyMode();
    bool isInEmergencyMode() const { return emergencyMode.load(); }
    
    // Monitoring and metrics
    NetworkConditions getNetworkConditions() const;
    SecurityThreats getSecurityThreats() const;
    std::vector<AdjustmentRecord> getAdjustmentHistory() const;
    nlohmann::json getDetailedStatus() const;
    
    // Configuration management
    void setConfig(const ParameterAdjustmentConfig& newConfig);
    ParameterAdjustmentConfig getConfig() const;
    bool updateAdjustmentLimits(double maxDifficulty, double maxReward, double maxStake, double maxResource);
    
    // Analysis and recommendations
    std::vector<std::string> analyzeNetworkConditions() const;
    std::vector<std::string> generateAdjustmentRecommendations() const;
    nlohmann::json getPerformanceAnalysis() const;
    
    // Statistics
    nlohmann::json getStatistics() const;
    void resetStatistics();

private:
    // Background processing loops
    void monitoringLoop();
    void adjustmentLoop();
    
    // Network condition monitoring
    void collectNetworkMetrics();
    void analyzeNetworkLoad();
    void checkNetworkHealth();
    void updateNetworkConditions();
    
    // Security threat detection
    void scanForThreats();
    void detectHashRateAnomalies();
    void detectVotingAnomalies();
    void detectNetworkPartitioning();
    void detectDDoSAttacks();
    void updateThreatLevel();
    
    // Parameter adjustment algorithms
    double calculateOptimalDifficulty(ConsensusType type) const;
    double calculateOptimalReward(ConsensusType type) const;
    double calculateOptimalStake(ConsensusType type) const;
    double calculateOptimalResource(ConsensusType type) const;
    
    // Adjustment strategy selection
    AdjustmentStrategy selectAdjustmentStrategy() const;
    double getAdjustmentMultiplier(AdjustmentStrategy strategy) const;
    
    // Load-based adjustments
    bool adjustForHighLoad();
    bool adjustForLowLoad();
    bool adjustForLatencyIssues();
    bool adjustForThroughputIssues();
    
    // Security-based adjustments
    bool respondToFiftyOneAttack();
    bool respondToHashRateSpike();
    bool respondToVotingAnomaly();
    bool respondToNetworkPartition();
    bool respondToDDoSAttack();
    
    // Decentralization maintenance
    bool rebalanceMechanismDominance();
    bool boostUnderperformingMechanisms();
    bool preventMechanismMonopoly();
    
    // Adjustment execution
    bool executeAdjustment(ConsensusType type, const std::string& parameter, 
                          double newValue, const std::string& reason, 
                          AdjustmentStrategy strategy);
    void recordAdjustment(const AdjustmentRecord& record);
    bool validateAdjustment(ConsensusType type, const std::string& parameter, double newValue) const;
    
    // Helper methods
    std::string consensusTypeToString(ConsensusType type) const;
    std::string adjustmentStrategyToString(AdjustmentStrategy strategy) const;
    double getCurrentTimestamp() const;
    bool isValidAdjustmentValue(double value) const;
    void logAdjustmentEvent(const std::string& event, const nlohmann::json& data = {}) const;
    
    // Validation
    bool validateConfig(const ParameterAdjustmentConfig& config) const;
    bool validateEngine(ConsensusEngine* engine) const;
};

#endif // PARAMETER_ADJUSTER_H