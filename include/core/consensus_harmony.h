#ifndef CONSENSUS_HARMONY_H
#define CONSENSUS_HARMONY_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include "json.hpp"

// Forward declarations
class Block;
class Transaction;

// Consensus mechanism types
enum class ConsensusType {
    PROOF_OF_WORK,
    PROOF_OF_STAKE,
    PROOF_OF_RESOURCE_CONTRIBUTION,
    VOTING_CONSENSUS,
    SMART_CONTRACT_VALIDATION
};

// Request types for consensus validation
enum class RequestType {
    BLOCK_VALIDATION,
    TRANSACTION_VALIDATION,
    PARAMETER_ADJUSTMENT,
    GOVERNANCE_PROPOSAL,
    SMART_CONTRACT_EXECUTION
};

// Consensus request structure
struct ConsensusRequest {
    RequestType type;
    std::string data;                           // Serialized block/transaction data
    std::vector<ConsensusType> requiredMechanisms; // Required consensus mechanisms
    uint64_t timestamp;
    std::string requestId;
    std::map<std::string, std::string> metadata;
    
    ConsensusRequest() : type(RequestType::BLOCK_VALIDATION), timestamp(0) {}
    
    ConsensusRequest(RequestType t, const std::string& d, 
                    const std::vector<ConsensusType>& required = {})
        : type(t), data(d), requiredMechanisms(required), 
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {
        requestId = generateRequestId();
    }
    
private:
    std::string generateRequestId() const;
};

// Consensus result structure
struct ConsensusResult {
    bool isValid;
    ConsensusType mechanism;
    double confidence;                          // Confidence level (0.0 - 1.0)
    std::string reason;                        // Validation result reason
    std::map<std::string, std::string> metadata;
    uint64_t timestamp;
    
    ConsensusResult() : isValid(false), mechanism(ConsensusType::PROOF_OF_WORK), 
                       confidence(0.0), timestamp(0) {}
    
    ConsensusResult(bool valid, ConsensusType mech, double conf = 1.0, 
                   const std::string& r = "")
        : isValid(valid), mechanism(mech), confidence(conf), reason(r),
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// Consensus status information
struct ConsensusStatus {
    std::map<ConsensusType, bool> mechanismStatus;
    std::map<ConsensusType, double> mechanismHealth;
    uint64_t totalValidations;
    uint64_t successfulValidations;
    uint64_t conflictCount;
    std::chrono::steady_clock::time_point lastUpdate;
    
    ConsensusStatus() : totalValidations(0), successfulValidations(0), 
                       conflictCount(0), lastUpdate(std::chrono::steady_clock::now()) {}
};

// Base interface for all consensus engines
class ConsensusEngine {
public:
    virtual ~ConsensusEngine() = default;
    
    // Core validation methods
    virtual bool validateBlock(const Block& block) = 0;
    virtual bool validateTransaction(const Transaction& transaction) = 0;
    virtual ConsensusResult processRequest(const ConsensusRequest& request) = 0;
    
    // Engine management
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isHealthy() const = 0;
    
    // Configuration and status
    virtual ConsensusType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual nlohmann::json getStatus() const = 0;
    virtual nlohmann::json getMetrics() const = 0;
    
    // Parameter adjustment
    virtual bool adjustParameters(const std::map<std::string, double>& parameters) = 0;
    virtual std::map<std::string, double> getParameters() const = 0;
};

// Consensus configuration
struct ConsensusConfig {
    // PoW Configuration
    uint64_t powDifficulty = 4;
    uint64_t powTargetBlockTime = 600;
    
    // PoS Configuration
    double minStakeAmount = 1000.0;
    uint64_t stakingPeriod = 86400; // 24 hours
    
    // PoRC Configuration
    double minResourceContribution = 100.0;
    std::vector<std::string> acceptedResourceTypes = {"COMPUTE", "STORAGE", "BANDWIDTH"};
    
    // Voting Configuration
    double supermajorityThreshold = 0.67;
    uint64_t votingPeriod = 604800; // 7 days
    
    // Balancing Configuration
    double maxDominanceRatio = 0.6;           // Max ratio one mechanism can dominate
    uint64_t rebalancingInterval = 3600;      // How often to rebalance (1 hour)
    
    // Conflict Resolution
    std::map<ConsensusType, int> consensusPriority = {
        {ConsensusType::PROOF_OF_WORK, 1},
        {ConsensusType::PROOF_OF_STAKE, 2},
        {ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 3},
        {ConsensusType::VOTING_CONSENSUS, 4},
        {ConsensusType::SMART_CONTRACT_VALIDATION, 5}
    };
    
    // Serialization
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

#endif // CONSENSUS_HARMONY_H