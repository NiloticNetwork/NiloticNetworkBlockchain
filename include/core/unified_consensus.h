#ifndef UNIFIED_CONSENSUS_H
#define UNIFIED_CONSENSUS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <queue>
#include <functional>
#include <random>
#include "blockchain.h"
#include "mining.h"
#include "porc.h"
#include "wallet.h"
#include "json.hpp"

// Consensus method types
enum class ConsensusMethod {
    PROOF_OF_WORK = 0,
    PROOF_OF_STAKE = 1,
    PROOF_OF_RESOURCE_CONTRIBUTION = 2
};

// Consensus participant status
enum class ParticipantStatus {
    IDLE = 0,
    ACTIVE = 1,
    VALIDATING = 2,
    REWARDED = 3,
    DISQUALIFIED = 4
};

// Consensus participant information
struct ConsensusParticipant {
    std::string address;
    ConsensusMethod method;
    ParticipantStatus status;
    double stake;                    // For PoS
    uint64_t bandwidth;              // For PoRC
    uint64_t hashRate;               // For PoW
    double reputationScore;
    uint64_t lastActivity;
    uint64_t totalRewards;
    uint64_t successfulValidations;
    uint64_t failedValidations;
    
    ConsensusParticipant() : method(ConsensusMethod::PROOF_OF_WORK), 
                           status(ParticipantStatus::IDLE), stake(0.0), 
                           bandwidth(0), hashRate(0), reputationScore(0.0),
                           lastActivity(0), totalRewards(0), 
                           successfulValidations(0), failedValidations(0) {}
    
    nlohmann::json toJson() const;
    static ConsensusParticipant fromJson(const nlohmann::json& json);
};

// Transaction validation task
struct ValidationTask {
    std::string taskId;
    std::string transactionId;
    std::vector<std::string> assignedParticipants;
    std::map<std::string, ConsensusMethod> participantMethods;
    uint64_t timestamp;
    uint64_t blockHeight;
    bool isCompleted;
    std::string winningParticipant;
    ConsensusMethod winningMethod;
    double reward;
    
    ValidationTask() : timestamp(0), blockHeight(0), isCompleted(false), reward(0.0) {}
    
    nlohmann::json toJson() const;
    static ValidationTask fromJson(const nlohmann::json& json);
};

// Consensus round information
struct ConsensusRound {
    uint64_t roundId;
    uint64_t blockHeight;
    std::vector<ValidationTask> tasks;
    std::map<ConsensusMethod, std::vector<std::string>> participants;
    std::map<ConsensusMethod, double> methodRewards;
    uint64_t startTime;
    uint64_t endTime;
    bool isCompleted;
    
    ConsensusRound() : roundId(0), blockHeight(0), startTime(0), 
                      endTime(0), isCompleted(false) {}
    
    nlohmann::json toJson() const;
};

// Consensus statistics
struct ConsensusStats {
    uint64_t totalRounds;
    uint64_t totalValidations;
    std::map<ConsensusMethod, uint64_t> methodWins;
    std::map<ConsensusMethod, double> methodRewards;
    double totalRewardsDistributed;
    uint64_t averageRoundTime;
    uint64_t activeParticipants;
    
    ConsensusStats() : totalRounds(0), totalValidations(0), 
                      totalRewardsDistributed(0.0), averageRoundTime(0), 
                      activeParticipants(0) {}
    
    nlohmann::json toJson() const;
};

// Unified Consensus System Configuration
struct UnifiedConsensusConfig {
    // General settings
    uint64_t roundDuration = 30;              // Seconds per consensus round
    uint64_t maxParticipantsPerRound = 100;   // Maximum participants per round
    uint64_t minParticipantsPerRound = 10;    // Minimum participants per round
    double totalRewardPerRound = 100.0;       // Total reward per round
    uint64_t maxConcurrentRounds = 5;         // Maximum concurrent rounds
    
    // Method-specific settings
    double powWeight = 0.4;                   // PoW weight in reward distribution
    double posWeight = 0.3;                   // PoS weight in reward distribution
    double porcWeight = 0.3;                  // PoRC weight in reward distribution
    
    // PoW settings
    uint64_t powDifficulty = 4;               // PoW difficulty
    uint64_t powMaxAttempts = 1000000;        // Maximum PoW attempts
    
    // PoS settings
    double minStake = 10.0;                   // Minimum stake for PoS
    double maxStake = 10000.0;                // Maximum stake for PoS
    uint64_t stakeLockTime = 3600;            // Stake lock time in seconds
    
    // PoRC settings
    uint64_t minBandwidth = 10;               // Minimum bandwidth for PoRC
    uint64_t maxBandwidth = 1000;             // Maximum bandwidth for PoRC
    uint64_t bandwidthMeasurementTime = 60;   // Bandwidth measurement time
    
    // Reputation system
    double reputationDecayRate = 0.95;        // Reputation decay per round
    double minReputationScore = 0.1;          // Minimum reputation to participate
    double maxReputationScore = 10.0;         // Maximum reputation score
    
    // Anti-sybil measures
    uint64_t minActivityTime = 300;           // Minimum activity time in seconds
    uint64_t maxFailuresBeforeBan = 5;        // Maximum failures before ban
    uint64_t banDuration = 3600;              // Ban duration in seconds
};

// Main Unified Consensus System Class
class UnifiedConsensusSystem {
private:
    // Core components
    Blockchain& blockchain;
    MiningEngine& miningEngine;
    PoRCSystem& porcSystem;
    
    // Configuration
    UnifiedConsensusConfig config;
    
    // State management
    std::atomic<bool> isRunning;
    std::atomic<uint64_t> currentRoundId;
    std::map<uint64_t, ConsensusRound> activeRounds;
    std::map<std::string, ConsensusParticipant> participants;
    std::queue<ValidationTask> pendingTasks;
    
    // Threads
    std::thread consensusLoopThread;
    std::thread participantManagementThread;
    std::thread rewardDistributionThread;
    
    // Mutexes for thread safety
    mutable std::mutex roundsMutex;
    mutable std::mutex participantsMutex;
    mutable std::mutex tasksMutex;
    mutable std::mutex statsMutex;
    
    // Statistics
    ConsensusStats stats;
    
    // Random number generation
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<double> dis;
    
    // Consensus loop management
    void consensusLoop();
    void participantManagementLoop();
    void rewardDistributionLoop();
    
    // Round management
    ConsensusRound createNewRound(uint64_t blockHeight);
    void startRound(ConsensusRound& round);
    void endRound(ConsensusRound& round);
    bool isRoundComplete(const ConsensusRound& round) const;
    
    // Task management
    ValidationTask createValidationTask(const std::string& transactionId, uint64_t blockHeight);
    void assignTaskToParticipants(ValidationTask& task, const ConsensusRound& round);
    bool validateTask(const ValidationTask& task, const std::string& participantAddress);
    
    // Participant management
    bool registerParticipant(const std::string& address, ConsensusMethod method);
    bool unregisterParticipant(const std::string& address);
    void updateParticipantStatus(const std::string& address, ParticipantStatus status);
    void updateParticipantReputation(const std::string& address, bool success);
    
    // Method-specific validation
    bool validateWithPoW(const std::string& participantAddress, const ValidationTask& task);
    bool validateWithPoS(const std::string& participantAddress, const ValidationTask& task);
    bool validateWithPoRC(const std::string& participantAddress, const ValidationTask& task);
    
    // Reward calculation
    double calculateReward(const std::string& participantAddress, ConsensusMethod method);
    void distributeRewards(const ConsensusRound& round);
    double getMethodWeight(ConsensusMethod method) const;
    
    // Utility functions
    std::string generateTaskId();
    uint64_t getCurrentTimestamp() const;
    bool isParticipantEligible(const ConsensusParticipant& participant) const;
    double calculateReputationScore(const ConsensusParticipant& participant) const;

public:
    UnifiedConsensusSystem(Blockchain& blockchain, MiningEngine& miningEngine, PoRCSystem& porcSystem);
    ~UnifiedConsensusSystem();
    
    // System management
    bool start();
    void stop();
    bool isSystemRunning() const { return isRunning; }
    
    // Participant management
    bool joinAsPoW(const std::string& address);
    bool joinAsPoS(const std::string& address, double stake);
    bool joinAsPoRC(const std::string& address, uint64_t bandwidth);
    bool leave(const std::string& address);
    bool isParticipant(const std::string& address) const;
    ConsensusParticipant getParticipant(const std::string& address) const;
    
    // Task submission
    bool submitTransactionForValidation(const std::string& transactionId);
    bool submitValidationResult(const std::string& taskId, const std::string& participantAddress, bool success);
    
    // Statistics and monitoring
    ConsensusStats getStats() const;
    std::vector<ConsensusRound> getActiveRounds() const;
    std::vector<ConsensusParticipant> getActiveParticipants() const;
    std::map<ConsensusMethod, std::vector<std::string>> getParticipantsByMethod() const;
    
    // Configuration
    void updateConfig(const UnifiedConsensusConfig& newConfig);
    UnifiedConsensusConfig getConfig() const { return config; }
    
    // API endpoints
    nlohmann::json handleJoinRequest(const nlohmann::json& request);
    nlohmann::json handleLeaveRequest(const nlohmann::json& request);
    nlohmann::json handleSubmitTransactionRequest(const nlohmann::json& request);
    nlohmann::json handleSubmitResultRequest(const nlohmann::json& request);
    nlohmann::json handleStatsRequest(const nlohmann::json& request);
    nlohmann::json handleRoundsRequest(const nlohmann::json& request);
    nlohmann::json handleParticipantsRequest(const nlohmann::json& request);
    
    // Integration with blockchain
    void onBlockMined(uint64_t blockHeight);
    void onTransactionCreated(const std::string& transactionId);
    void onTransactionValidated(const std::string& transactionId, bool success);
    
    // Database operations
    bool saveParticipant(const ConsensusParticipant& participant);
    bool loadParticipant(const std::string& address, ConsensusParticipant& participant);
    bool saveRound(const ConsensusRound& round);
    bool loadRound(uint64_t roundId, ConsensusRound& round);
    bool saveTask(const ValidationTask& task);
    bool loadTask(const std::string& taskId, ValidationTask& task);
    
    // Validation and security
    bool validateParticipant(const ConsensusParticipant& participant);
    bool validateTask(const ValidationTask& task);
    bool validateRound(const ConsensusRound& round);
    bool isParticipantBanned(const std::string& address) const;
    void banParticipant(const std::string& address, uint64_t duration);
    void unbanParticipant(const std::string& address);
};

// Unified Consensus API Handler
class UnifiedConsensusAPI {
private:
    UnifiedConsensusSystem& consensusSystem;
    
public:
    UnifiedConsensusAPI(UnifiedConsensusSystem& system) : consensusSystem(system) {}
    
    // API endpoint handlers
    std::string handlePOST(const std::string& endpoint, const std::string& body);
    std::string handleGET(const std::string& endpoint);
    
    // Specific endpoint handlers
    std::string handleJoin(const nlohmann::json& request);
    std::string handleLeave(const nlohmann::json& request);
    std::string handleSubmitTransaction(const nlohmann::json& request);
    std::string handleSubmitResult(const nlohmann::json& request);
    std::string handleStats(const nlohmann::json& request);
    std::string handleRounds(const nlohmann::json& request);
    std::string handleParticipants(const nlohmann::json& request);
};

#endif // UNIFIED_CONSENSUS_H
