#ifndef SMART_CONTRACT_CONSENSUS_ENGINE_H
#define SMART_CONTRACT_CONSENSUS_ENGINE_H

#include "consensus_harmony.h"
#include "smart_contract_vm.h"
#include "block.h"
#include "transaction.h"
#include "json.hpp"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>

// Forward declarations
class Blockchain;
class VotingConsensusEngine;

// Smart contract execution result
struct ContractExecutionResult {
    bool success;
    std::string contractAddress;
    std::string newState;
    uint64_t gasUsed;
    std::vector<std::string> logs;
    std::string errorMessage;
    
    ContractExecutionResult() : success(false), gasUsed(0) {}
    
    ContractExecutionResult(bool s, const std::string& addr, const std::string& state, 
                           uint64_t gas, const std::vector<std::string>& l = {}, 
                           const std::string& error = "")
        : success(s), contractAddress(addr), newState(state), gasUsed(gas), logs(l), errorMessage(error) {}
};

// Contract state change validation result
struct StateChangeValidation {
    bool isValid;
    std::string reason;
    std::vector<ConsensusType> validatedBy;
    double confidence;
    
    StateChangeValidation() : isValid(false), confidence(0.0) {}
    
    StateChangeValidation(bool valid, const std::string& r, 
                         const std::vector<ConsensusType>& validators, double conf = 1.0)
        : isValid(valid), reason(r), validatedBy(validators), confidence(conf) {}
};

// Contract governance interaction types
enum class GovernanceInteractionType {
    PARAMETER_PROPOSAL,
    VOTING_PARTICIPATION,
    CONSENSUS_RULE_CHANGE,
    EMERGENCY_ACTION,
    NONE
};

// Contract governance interaction
struct ContractGovernanceInteraction {
    GovernanceInteractionType type;
    std::string contractAddress;
    std::string proposalId;
    std::map<std::string, std::string> parameters;
    bool requiresSupermajority;
    
    ContractGovernanceInteraction() : type(GovernanceInteractionType::NONE), 
                                     requiresSupermajority(false) {}
};

/**
 * Smart Contract Consensus Engine for contract execution validation
 * Implements multi-consensus validation for contract state changes and governance interactions
 */
class SmartContractConsensusEngine : public ConsensusEngine {
private:
    // Core components
    std::unique_ptr<SmartContractVM> vm;
    Blockchain* blockchain;
    VotingConsensusEngine* votingEngine;
    
    // Contract state management
    std::map<std::string, std::string> contractStates;
    std::map<std::string, std::vector<uint8_t>> contractBytecodes;
    std::map<std::string, std::string> contractSources;
    
    // Multi-consensus validation
    std::vector<ConsensusEngine*> validationEngines;
    std::map<std::string, StateChangeValidation> stateValidations;
    
    // Governance integration
    std::map<std::string, ContractGovernanceInteraction> governanceInteractions;
    std::vector<std::string> governanceContracts;
    
    // Configuration
    uint64_t maxGasLimit;
    uint64_t maxContractSize;
    double minValidationConfidence;
    bool requireMultiConsensus;
    bool enableGovernanceValidation;
    
    // Thread safety
    mutable std::mutex contractMutex;
    
    // Status tracking
    bool initialized;
    std::chrono::steady_clock::time_point lastUpdate;
    uint64_t totalExecutions;
    uint64_t successfulExecutions;
    uint64_t failedExecutions;

public:
    explicit SmartContractConsensusEngine(Blockchain* bc = nullptr, 
                                         VotingConsensusEngine* voting = nullptr);
    virtual ~SmartContractConsensusEngine() = default;
    
    // ConsensusEngine interface implementation
    bool validateBlock(const Block& block) override;
    bool validateTransaction(const Transaction& transaction) override;
    ConsensusResult processRequest(const ConsensusRequest& request) override;
    
    bool initialize() override;
    void shutdown() override;
    bool isHealthy() const override;
    
    ConsensusType getType() const override { return ConsensusType::SMART_CONTRACT_VALIDATION; }
    std::string getName() const override { return "SmartContractConsensusEngine"; }
    nlohmann::json getStatus() const override;
    nlohmann::json getMetrics() const override;
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override;
    std::map<std::string, double> getParameters() const override;
    
    // Smart contract execution
    ContractExecutionResult executeContract(const std::string& contractAddress, 
                                          const std::string& input,
                                          const std::string& sender,
                                          uint64_t gasLimit = 1000000);
    
    ContractExecutionResult deployContract(const std::string& sourceCode,
                                         const std::string& deployer,
                                         uint64_t gasLimit = 1000000);
    
    // Multi-consensus validation for state changes
    StateChangeValidation validateStateChange(const std::string& contractAddress,
                                            const std::string& oldState,
                                            const std::string& newState,
                                            const Transaction& transaction);
    
    bool requiresMultiConsensusValidation(const std::string& contractAddress) const;
    void addValidationEngine(ConsensusEngine* engine);
    void removeValidationEngine(ConsensusEngine* engine);
    
    // Contract-governance interaction validation
    bool validateGovernanceInteraction(const ContractGovernanceInteraction& interaction);
    ContractGovernanceInteraction analyzeGovernanceInteraction(const Transaction& transaction);
    bool isGovernanceContract(const std::string& contractAddress) const;
    void registerGovernanceContract(const std::string& contractAddress);
    void unregisterGovernanceContract(const std::string& contractAddress);
    
    // Contract state management
    std::string getContractState(const std::string& contractAddress) const;
    bool setContractState(const std::string& contractAddress, const std::string& state);
    std::vector<uint8_t> getContractBytecode(const std::string& contractAddress) const;
    bool setContractBytecode(const std::string& contractAddress, const std::vector<uint8_t>& bytecode);
    
    // Contract validation
    bool validateContractCode(const std::string& sourceCode) const;
    bool validateContractExecution(const std::string& contractAddress, 
                                 const SmartContractContext& context) const;
    
    // Security and safety
    bool isContractSafe(const std::string& contractAddress) const;
    std::vector<std::string> getSecurityWarnings(const std::string& sourceCode) const;
    bool enforceSecurityPolicies(const std::string& contractAddress) const;
    
    // Configuration management
    void setMaxGasLimit(uint64_t limit) { maxGasLimit = limit; }
    uint64_t getMaxGasLimit() const { return maxGasLimit; }
    void setMaxContractSize(uint64_t size) { maxContractSize = size; }
    uint64_t getMaxContractSize() const { return maxContractSize; }
    void setMinValidationConfidence(double confidence) { minValidationConfidence = confidence; }
    double getMinValidationConfidence() const { return minValidationConfidence; }
    
    // Integration with other engines
    void setBlockchain(Blockchain* bc) { blockchain = bc; }
    Blockchain* getBlockchain() const { return blockchain; }
    void setVotingEngine(VotingConsensusEngine* voting) { votingEngine = voting; }
    VotingConsensusEngine* getVotingEngine() const { return votingEngine; }
    
    // Statistics and monitoring
    uint64_t getTotalExecutions() const { return totalExecutions; }
    uint64_t getSuccessfulExecutions() const { return successfulExecutions; }
    uint64_t getFailedExecutions() const { return failedExecutions; }
    double getSuccessRate() const;
    
    // Utility methods
    std::string generateContractAddress(const std::string& deployer, uint64_t nonce) const;
    bool isContractTransaction(const Transaction& transaction) const;
    bool isContractDeployment(const Transaction& transaction) const;

private:
    // Internal helpers
    bool processContractTransaction(const Transaction& transaction);
    bool processContractDeployment(const Transaction& transaction);
    bool validateContractWithMultiConsensus(const std::string& contractAddress,
                                           const Transaction& transaction);
    
    // Governance interaction helpers
    GovernanceInteractionType detectGovernanceInteractionType(const Transaction& transaction) const;
    bool validateGovernanceProposal(const ContractGovernanceInteraction& interaction);
    bool validateVotingParticipation(const ContractGovernanceInteraction& interaction);
    bool validateConsensusRuleChange(const ContractGovernanceInteraction& interaction);
    
    // Security helpers
    bool checkContractSecurity(const std::string& sourceCode) const;
    bool checkGasLimits(uint64_t gasUsed, uint64_t gasLimit) const;
    bool checkContractSize(const std::string& sourceCode) const;
    
    // State validation helpers
    double calculateValidationConfidence(const std::vector<ConsensusResult>& results) const;
    bool aggregateValidationResults(const std::vector<ConsensusResult>& results) const;
    
    // Logging and monitoring
    void logContractEvent(const std::string& event, const nlohmann::json& data = {}) const;
    void updateExecutionStats(bool success);
    void updateLastActivity();
};

#endif // SMART_CONTRACT_CONSENSUS_ENGINE_H