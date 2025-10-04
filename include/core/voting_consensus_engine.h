#ifndef VOTING_CONSENSUS_ENGINE_H
#define VOTING_CONSENSUS_ENGINE_H

#include "consensus_harmony.h"
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

// Governance proposal types
enum class ProposalType {
    PARAMETER_CHANGE,
    PROTOCOL_UPGRADE,
    CONSENSUS_RULE_CHANGE,
    NETWORK_CONFIGURATION,
    EMERGENCY_ACTION
};

// Vote types
enum class VoteType {
    YES,
    NO,
    ABSTAIN
};

// Governance proposal structure
struct GovernanceProposal {
    std::string proposalId;
    ProposalType type;
    std::string title;
    std::string description;
    std::map<std::string, std::string> parameters; // parameter name -> new value
    std::string proposer;
    uint64_t creationTime;
    uint64_t votingDeadline;
    double requiredThreshold;                      // Required voting threshold (0.0 - 1.0)
    bool isActive;
    bool isExecuted;
    
    GovernanceProposal() : type(ProposalType::PARAMETER_CHANGE), creationTime(0), 
                          votingDeadline(0), requiredThreshold(0.67), 
                          isActive(false), isExecuted(false) {}
};

// Vote record structure
struct VoteRecord {
    std::string voteId;
    std::string proposalId;
    std::string voter;
    VoteType vote;
    double votingPower;                           // Voting power based on stake/contribution
    uint64_t timestamp;
    std::string transactionHash;                  // Hash of blockchain transaction recording the vote
    
    VoteRecord() : vote(VoteType::ABSTAIN), votingPower(0.0), timestamp(0) {}
};

// Voting results structure
struct VotingResults {
    std::string proposalId;
    uint64_t totalVotes;
    uint64_t yesVotes;
    uint64_t noVotes;
    uint64_t abstainVotes;
    double totalVotingPower;
    double yesVotingPower;
    double noVotingPower;
    double abstainVotingPower;
    double participationRate;
    bool passed;
    
    VotingResults() : totalVotes(0), yesVotes(0), noVotes(0), abstainVotes(0),
                     totalVotingPower(0.0), yesVotingPower(0.0), noVotingPower(0.0),
                     abstainVotingPower(0.0), participationRate(0.0), passed(false) {}
};

/**
 * Voting Consensus Engine for governance and democratic decision-making
 * Implements blockchain-based voting with automatic parameter enforcement
 */
class VotingConsensusEngine : public ConsensusEngine {
private:
    // Core state
    std::map<std::string, GovernanceProposal> proposals;
    std::map<std::string, std::vector<VoteRecord>> votes; // proposalId -> votes
    std::map<std::string, VotingResults> results;
    
    // Configuration
    double supermajorityThreshold;
    uint64_t defaultVotingPeriod;
    double minVotingPower;
    bool requireStakeForVoting;
    
    // Blockchain integration
    Blockchain* blockchain;
    
    // Thread safety
    mutable std::mutex votingMutex;
    
    // Status tracking
    bool initialized;
    std::chrono::steady_clock::time_point lastUpdate;

public:
    explicit VotingConsensusEngine(Blockchain* bc = nullptr);
    virtual ~VotingConsensusEngine() = default;
    
    // ConsensusEngine interface implementation
    bool validateBlock(const Block& block) override;
    bool validateTransaction(const Transaction& transaction) override;
    ConsensusResult processRequest(const ConsensusRequest& request) override;
    
    bool initialize() override;
    void shutdown() override;
    bool isHealthy() const override;
    
    ConsensusType getType() const override { return ConsensusType::VOTING_CONSENSUS; }
    std::string getName() const override { return "VotingConsensusEngine"; }
    nlohmann::json getStatus() const override;
    nlohmann::json getMetrics() const override;
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override;
    std::map<std::string, double> getParameters() const override;
    
    // Governance proposal management
    bool createProposal(const GovernanceProposal& proposal);
    bool updateProposal(const std::string& proposalId, const GovernanceProposal& proposal);
    GovernanceProposal getProposal(const std::string& proposalId) const;
    std::vector<GovernanceProposal> getActiveProposals() const;
    std::vector<GovernanceProposal> getAllProposals() const;
    
    // Voting operations
    bool castVote(const std::string& proposalId, const std::string& voter, 
                  VoteType vote, double votingPower = 1.0);
    bool recordVoteOnBlockchain(const VoteRecord& voteRecord);
    VoteRecord getVote(const std::string& proposalId, const std::string& voter) const;
    std::vector<VoteRecord> getVotes(const std::string& proposalId) const;
    
    // Results and tallying
    VotingResults tallyVotes(const std::string& proposalId);
    VotingResults getResults(const std::string& proposalId) const;
    bool finalizeVoting(const std::string& proposalId);
    
    // Parameter enforcement
    bool executeProposal(const std::string& proposalId);
    bool enforceParameterChanges(const GovernanceProposal& proposal);
    std::vector<std::string> getPendingParameterChanges() const;
    
    // Voting power calculation
    double calculateVotingPower(const std::string& voter) const;
    double getMinimumVotingPower() const { return minVotingPower; }
    void setMinimumVotingPower(double power) { minVotingPower = power; }
    
    // Configuration management
    void setSupermajorityThreshold(double threshold) { supermajorityThreshold = threshold; }
    double getSupermajorityThreshold() const { return supermajorityThreshold; }
    void setDefaultVotingPeriod(uint64_t period) { defaultVotingPeriod = period; }
    uint64_t getDefaultVotingPeriod() const { return defaultVotingPeriod; }
    
    // Blockchain integration
    void setBlockchain(Blockchain* bc) { blockchain = bc; }
    Blockchain* getBlockchain() const { return blockchain; }
    
    // Utility methods
    bool isProposalActive(const std::string& proposalId) const;
    bool hasVotingEnded(const std::string& proposalId) const;
    uint64_t getCurrentTime() const;
    
    // Validation helpers
    bool validateProposal(const GovernanceProposal& proposal) const;
    bool validateVote(const std::string& proposalId, const std::string& voter, VoteType vote) const;
    bool canVote(const std::string& voter) const;

private:
    // Internal helpers
    std::string generateProposalId(const GovernanceProposal& proposal) const;
    std::string generateVoteId(const std::string& proposalId, const std::string& voter) const;
    Transaction createVoteTransaction(const VoteRecord& voteRecord) const;
    bool processVotingTransaction(const Transaction& transaction);
    void updateProposalStatus();
    void logVotingEvent(const std::string& event, const nlohmann::json& data = {}) const;
};

#endif // VOTING_CONSENSUS_ENGINE_H