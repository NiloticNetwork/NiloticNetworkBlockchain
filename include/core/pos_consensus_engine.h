#ifndef POS_CONSENSUS_ENGINE_H
#define POS_CONSENSUS_ENGINE_H

#include "consensus_harmony.h"
#include "block.h"
#include "transaction.h"
#include "logger.h"
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <algorithm>
#include <chrono>

// Validator information structure
struct ValidatorInfo {
    std::string address;
    double stakedAmount;
    uint64_t lastValidationTime;
    uint64_t validationCount;
    double reputationScore;
    bool isActive;
    
    ValidatorInfo() : stakedAmount(0.0), lastValidationTime(0), 
                     validationCount(0), reputationScore(1.0), isActive(true) {}
    
    ValidatorInfo(const std::string& addr, double stake) 
        : address(addr), stakedAmount(stake), lastValidationTime(0),
          validationCount(0), reputationScore(1.0), isActive(true) {}
};

// Stake-based validation coordination data
struct StakeCoordinationData {
    std::map<ConsensusType, double> mechanismWeights;
    double totalNetworkStake;
    double minimumStakeThreshold;
    uint64_t lastCoordinationUpdate;
    
    StakeCoordinationData() : totalNetworkStake(0.0), minimumStakeThreshold(1000.0),
                             lastCoordinationUpdate(0) {
        // Initialize default weights for coordination with other mechanisms
        mechanismWeights[ConsensusType::PROOF_OF_WORK] = 0.4;
        mechanismWeights[ConsensusType::PROOF_OF_STAKE] = 0.3;
        mechanismWeights[ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION] = 0.2;
        mechanismWeights[ConsensusType::VOTING_CONSENSUS] = 0.1;
    }
};

// PoS-specific configuration
struct PoSConfig {
    double minStakeAmount = 1000.0;
    uint64_t stakingPeriod = 86400;        // 24 hours in seconds
    double slashingPenalty = 0.1;          // 10% penalty for malicious behavior
    uint64_t validatorCooldown = 3600;     // 1 hour cooldown between validations
    double reputationDecayRate = 0.01;     // 1% reputation decay per day
    uint64_t maxValidatorsPerBlock = 5;    // Maximum validators for cross-mechanism coordination
    double coordinationThreshold = 0.67;   // Threshold for cross-mechanism agreement
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

class PoSConsensusEngine : public ConsensusEngine {
private:
    std::map<std::string, ValidatorInfo> validators;
    std::map<std::string, double> accountBalances;  // Reference to account balances
    StakeCoordinationData coordinationData;
    PoSConfig config;
    mutable std::mutex validatorMutex;
    mutable std::mutex coordinationMutex;
    std::mt19937 randomGenerator;
    
    // Metrics tracking
    uint64_t totalValidations;
    uint64_t successfulValidations;
    uint64_t coordinationEvents;
    std::chrono::steady_clock::time_point lastMetricsUpdate;
    
    // Cross-mechanism coordination
    std::map<ConsensusType, bool> mechanismAvailability;
    std::map<std::string, std::map<ConsensusType, bool>> validatorMechanismSupport;

public:
    PoSConsensusEngine();
    virtual ~PoSConsensusEngine() = default;
    
    // ConsensusEngine interface implementation
    bool validateBlock(const Block& block) override;
    bool validateTransaction(const Transaction& transaction) override;
    ConsensusResult processRequest(const ConsensusRequest& request) override;
    
    bool initialize() override;
    void shutdown() override;
    bool isHealthy() const override;
    
    ConsensusType getType() const override { return ConsensusType::PROOF_OF_STAKE; }
    std::string getName() const override { return "Proof of Stake Consensus Engine"; }
    nlohmann::json getStatus() const override;
    nlohmann::json getMetrics() const override;
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override;
    std::map<std::string, double> getParameters() const override;
    
    // PoS-specific methods
    bool stakeTokens(const std::string& address, double amount);
    bool unstakeTokens(const std::string& address, double amount);
    std::string selectValidator();
    std::vector<std::string> selectValidators(size_t count);
    bool validateBlockWithStake(const Block& block, const std::string& validatorAddress);
    
    // Cross-mechanism coordination methods
    bool coordinateWithMechanism(ConsensusType mechanism, const ConsensusRequest& request);
    std::vector<std::string> selectValidatorsForCoordination(const std::vector<ConsensusType>& mechanisms);
    bool validateCrossMechanismConsensus(const std::vector<ConsensusResult>& results);
    void updateMechanismWeights(const std::map<ConsensusType, double>& newWeights);
    
    // Validator management
    bool addValidator(const std::string& address, double initialStake);
    bool removeValidator(const std::string& address);
    bool slashValidator(const std::string& address, double penalty);
    void updateValidatorReputation(const std::string& address, bool successful);
    
    // Getters
    std::map<std::string, ValidatorInfo> getValidators() const;
    double getTotalStake() const;
    ValidatorInfo getValidatorInfo(const std::string& address) const;
    StakeCoordinationData getCoordinationData() const;
    
    // Balance management (integration with blockchain)
    void setAccountBalances(const std::map<std::string, double>& balances);
    void updateAccountBalance(const std::string& address, double balance);

private:
    // Internal helper methods
    double calculateValidatorWeight(const ValidatorInfo& validator) const;
    bool isValidatorEligible(const ValidatorInfo& validator) const;
    std::string selectValidatorWeighted();
    void updateCoordinationData();
    bool verifyStakeRequirements(const std::string& address, double requiredStake) const;
    void decayValidatorReputations();
    double calculateCoordinationScore(const std::vector<ConsensusResult>& results) const;
    bool checkMechanismCompatibility(ConsensusType mechanism) const;
};

#endif // POS_CONSENSUS_ENGINE_H