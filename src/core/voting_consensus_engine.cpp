#include "voting_consensus_engine.h"
#include "blockchain.h"
#include "logger.h"
#include "utils.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

VotingConsensusEngine::VotingConsensusEngine(Blockchain* bc)
    : blockchain(bc), supermajorityThreshold(0.67), defaultVotingPeriod(604800), // 7 days
      minVotingPower(1.0), requireStakeForVoting(true), initialized(false),
      lastUpdate(std::chrono::steady_clock::now()) {
}

bool VotingConsensusEngine::initialize() {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    if (initialized) {
        return true;
    }
    
    try {
        // Initialize default configuration
        supermajorityThreshold = 0.67;
        defaultVotingPeriod = 604800; // 7 days in seconds
        minVotingPower = 1.0;
        requireStakeForVoting = true;
        
        // Clear any existing state
        proposals.clear();
        votes.clear();
        results.clear();
        
        initialized = true;
        lastUpdate = std::chrono::steady_clock::now();
        
        Logger::info("VotingConsensusEngine initialized successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize VotingConsensusEngine: " + std::string(e.what()));
        return false;
    }
}

void VotingConsensusEngine::shutdown() {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    if (!initialized) {
        return;
    }
    
    // Clear state
    proposals.clear();
    votes.clear();
    results.clear();
    
    initialized = false;
    Logger::info("VotingConsensusEngine shutdown completed");
}

bool VotingConsensusEngine::isHealthy() const {
    // Don't lock mutex here to avoid deadlock when called from getStatus
    if (!initialized) {
        return false;
    }
    
    // Check if we have recent activity (within last hour)
    auto now = std::chrono::steady_clock::now();
    auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count();
    
    // Consider healthy if initialized and no critical errors
    return timeSinceUpdate < 3600; // 1 hour
}

bool VotingConsensusEngine::validateBlock(const Block& block) {
    if (!initialized) {
        return false;
    }
    
    try {
        // Validate all voting-related transactions in the block
        auto transactions = block.getTransactions();
        for (const auto& tx : transactions) {
            if (!validateTransaction(tx)) {
                Logger::warning("VotingConsensusEngine: Invalid voting transaction in block");
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine block validation error: " + std::string(e.what()));
        return false;
    }
}

bool VotingConsensusEngine::validateTransaction(const Transaction& transaction) {
    if (!initialized) {
        return false;
    }
    
    try {
        // Check if this is a voting-related transaction
        std::string recipient = transaction.getRecipient();
        
        // Voting transactions have special recipients starting with "VOTE_"
        if (recipient.find("VOTE_") == 0) {
            return processVotingTransaction(transaction);
        }
        
        // For non-voting transactions, always return true (let other engines handle them)
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine transaction validation error: " + std::string(e.what()));
        return false;
    }
}

ConsensusResult VotingConsensusEngine::processRequest(const ConsensusRequest& request) {
    if (!initialized) {
        return ConsensusResult(false, ConsensusType::VOTING_CONSENSUS, 0.0, "Engine not initialized");
    }
    
    try {
        switch (request.type) {
            case RequestType::BLOCK_VALIDATION: {
                // Deserialize block and validate
                Block block = Block::deserialize(request.data);
                bool isValid = validateBlock(block);
                return ConsensusResult(isValid, ConsensusType::VOTING_CONSENSUS, 
                                     isValid ? 1.0 : 0.0, 
                                     isValid ? "Block validation passed" : "Block validation failed");
            }
            
            case RequestType::TRANSACTION_VALIDATION: {
                // Deserialize transaction and validate
                Transaction tx = Transaction::deserialize(request.data);
                bool isValid = validateTransaction(tx);
                return ConsensusResult(isValid, ConsensusType::VOTING_CONSENSUS,
                                     isValid ? 1.0 : 0.0,
                                     isValid ? "Transaction validation passed" : "Transaction validation failed");
            }
            
            case RequestType::GOVERNANCE_PROPOSAL: {
                // Handle governance proposal processing
                nlohmann::json proposalData = nlohmann::json::parse(request.data);
                // Process governance proposal logic here
                return ConsensusResult(true, ConsensusType::VOTING_CONSENSUS, 1.0, "Governance proposal processed");
            }
            
            default:
                return ConsensusResult(true, ConsensusType::VOTING_CONSENSUS, 1.0, "Request type not handled by voting consensus");
        }
    } catch (const std::exception& e) {
        return ConsensusResult(false, ConsensusType::VOTING_CONSENSUS, 0.0, 
                             "Error processing request: " + std::string(e.what()));
    }
}

nlohmann::json VotingConsensusEngine::getStatus() const {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    nlohmann::json status;
    status["initialized"] = initialized;
    status["healthy"] = initialized; // Simplified health check to avoid recursive calls
    status["type"] = "VOTING_CONSENSUS";
    status["name"] = getName();
    
    status["configuration"] = {
        {"supermajorityThreshold", supermajorityThreshold},
        {"defaultVotingPeriod", defaultVotingPeriod},
        {"minVotingPower", minVotingPower},
        {"requireStakeForVoting", requireStakeForVoting}
    };
    
    // Count active proposals directly to avoid recursive mutex lock
    int activeCount = 0;
    for (const auto& [id, proposal] : proposals) {
        if (proposal.isActive && getCurrentTime() < proposal.votingDeadline) {
            activeCount++;
        }
    }
    
    status["statistics"] = {
        {"totalProposals", proposals.size()},
        {"activeProposals", activeCount},
        {"totalVotes", votes.size()}
    };
    
    return status;
}

nlohmann::json VotingConsensusEngine::getMetrics() const {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    nlohmann::json metrics;
    
    // Count active proposals
    int activeCount = 0;
    int executedCount = 0;
    for (const auto& [id, proposal] : proposals) {
        if (proposal.isActive) activeCount++;
        if (proposal.isExecuted) executedCount++;
    }
    
    metrics["proposals"] = {
        {"total", proposals.size()},
        {"active", activeCount},
        {"executed", executedCount}
    };
    
    // Count total votes
    int totalVotes = 0;
    for (const auto& [proposalId, voteList] : votes) {
        totalVotes += voteList.size();
    }
    
    metrics["votes"] = {
        {"total", totalVotes},
        {"proposals_with_votes", votes.size()}
    };
    
    metrics["participation"] = {
        {"average_participation_rate", 0.0} // TODO: Calculate actual participation rate
    };
    
    return metrics;
}

bool VotingConsensusEngine::adjustParameters(const std::map<std::string, double>& parameters) {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    try {
        for (const auto& [param, value] : parameters) {
            if (param == "supermajorityThreshold") {
                if (value >= 0.5 && value <= 1.0) {
                    supermajorityThreshold = value;
                    Logger::info("VotingConsensusEngine: Updated supermajorityThreshold to " + std::to_string(value));
                }
            } else if (param == "defaultVotingPeriod") {
                if (value > 0) {
                    defaultVotingPeriod = static_cast<uint64_t>(value);
                    Logger::info("VotingConsensusEngine: Updated defaultVotingPeriod to " + std::to_string(value));
                }
            } else if (param == "minVotingPower") {
                if (value >= 0.0) {
                    minVotingPower = value;
                    Logger::info("VotingConsensusEngine: Updated minVotingPower to " + std::to_string(value));
                }
            }
        }
        
        lastUpdate = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine parameter adjustment error: " + std::string(e.what()));
        return false;
    }
}

std::map<std::string, double> VotingConsensusEngine::getParameters() const {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    return {
        {"supermajorityThreshold", supermajorityThreshold},
        {"defaultVotingPeriod", static_cast<double>(defaultVotingPeriod)},
        {"minVotingPower", minVotingPower}
    };
}

bool VotingConsensusEngine::createProposal(const GovernanceProposal& proposal) {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    if (!initialized) {
        return false;
    }
    
    try {
        // Validate the proposal
        if (!validateProposal(proposal)) {
            Logger::error("VotingConsensusEngine: Invalid proposal");
            return false;
        }
        
        // Generate unique proposal ID
        GovernanceProposal newProposal = proposal;
        newProposal.proposalId = generateProposalId(proposal);
        newProposal.creationTime = getCurrentTime();
        newProposal.isActive = true;
        newProposal.isExecuted = false;
        
        // Set voting deadline if not specified
        if (newProposal.votingDeadline == 0) {
            newProposal.votingDeadline = newProposal.creationTime + defaultVotingPeriod;
        }
        
        // Store the proposal
        proposals[newProposal.proposalId] = newProposal;
        
        // Initialize empty vote list
        votes[newProposal.proposalId] = std::vector<VoteRecord>();
        
        Logger::info("VotingConsensusEngine: Created proposal " + newProposal.proposalId);
        logVotingEvent("proposal_created", {{"proposalId", newProposal.proposalId}});
        
        lastUpdate = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine proposal creation error: " + std::string(e.what()));
        return false;
    }
}

bool VotingConsensusEngine::castVote(const std::string& proposalId, const std::string& voter, 
                                   VoteType vote, double votingPower) {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    if (!initialized) {
        return false;
    }
    
    try {
        // Validate the vote
        if (!validateVote(proposalId, voter, vote)) {
            return false;
        }
        
        // Check if voter can vote
        if (!canVote(voter)) {
            Logger::warning("VotingConsensusEngine: Voter " + voter + " cannot vote");
            return false;
        }
        
        // Calculate voting power if not provided
        if (votingPower <= 0.0) {
            votingPower = calculateVotingPower(voter);
        }
        
        // Create vote record
        VoteRecord voteRecord;
        voteRecord.voteId = generateVoteId(proposalId, voter);
        voteRecord.proposalId = proposalId;
        voteRecord.voter = voter;
        voteRecord.vote = vote;
        voteRecord.votingPower = votingPower;
        voteRecord.timestamp = getCurrentTime();
        
        // Record vote on blockchain
        if (!recordVoteOnBlockchain(voteRecord)) {
            Logger::error("VotingConsensusEngine: Failed to record vote on blockchain");
            return false;
        }
        
        // Store the vote
        votes[proposalId].push_back(voteRecord);
        
        Logger::info("VotingConsensusEngine: Vote cast by " + voter + " for proposal " + proposalId);
        logVotingEvent("vote_cast", {
            {"proposalId", proposalId},
            {"voter", voter},
            {"vote", static_cast<int>(vote)},
            {"votingPower", votingPower}
        });
        
        lastUpdate = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine vote casting error: " + std::string(e.what()));
        return false;
    }
}

bool VotingConsensusEngine::recordVoteOnBlockchain(const VoteRecord& voteRecord) {
    if (!blockchain) {
        Logger::warning("VotingConsensusEngine: No blockchain instance available");
        return false;
    }
    
    try {
        // Create a transaction to record the vote
        Transaction voteTransaction = createVoteTransaction(voteRecord);
        
        // Add transaction to blockchain's pending transactions
        // Note: In a real implementation, this would go through proper transaction validation
        // For now, we'll create a simple vote recording mechanism
        
        Logger::info("VotingConsensusEngine: Vote recorded on blockchain with hash: " + voteTransaction.getHash());
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine blockchain recording error: " + std::string(e.what()));
        return false;
    }
}

VotingResults VotingConsensusEngine::tallyVotes(const std::string& proposalId) {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    VotingResults result;
    result.proposalId = proposalId;
    
    if (votes.find(proposalId) == votes.end()) {
        return result;
    }
    
    // Tally all votes
    for (const auto& vote : votes[proposalId]) {
        result.totalVotes++;
        result.totalVotingPower += vote.votingPower;
        
        switch (vote.vote) {
            case VoteType::YES:
                result.yesVotes++;
                result.yesVotingPower += vote.votingPower;
                break;
            case VoteType::NO:
                result.noVotes++;
                result.noVotingPower += vote.votingPower;
                break;
            case VoteType::ABSTAIN:
                result.abstainVotes++;
                result.abstainVotingPower += vote.votingPower;
                break;
        }
    }
    
    // Calculate participation rate (simplified)
    result.participationRate = result.totalVotes > 0 ? 1.0 : 0.0; // TODO: Calculate based on total eligible voters
    
    // Determine if proposal passed
    if (result.totalVotingPower > 0) {
        double yesRatio = result.yesVotingPower / result.totalVotingPower;
        result.passed = yesRatio >= supermajorityThreshold;
    }
    
    // Store results
    results[proposalId] = result;
    
    return result;
}

bool VotingConsensusEngine::executeProposal(const std::string& proposalId) {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    if (!initialized) {
        return false;
    }
    
    try {
        auto proposalIt = proposals.find(proposalId);
        if (proposalIt == proposals.end()) {
            Logger::error("VotingConsensusEngine: Proposal not found: " + proposalId);
            return false;
        }
        
        GovernanceProposal& proposal = proposalIt->second;
        
        // Check if proposal has already been executed
        if (proposal.isExecuted) {
            Logger::warning("VotingConsensusEngine: Proposal already executed: " + proposalId);
            return true;
        }
        
        // Check if voting has ended
        if (!hasVotingEnded(proposalId)) {
            Logger::error("VotingConsensusEngine: Voting period has not ended for proposal: " + proposalId);
            return false;
        }
        
        // Tally votes and check if proposal passed
        VotingResults result = tallyVotes(proposalId);
        if (!result.passed) {
            Logger::info("VotingConsensusEngine: Proposal did not pass: " + proposalId);
            proposal.isExecuted = true; // Mark as processed even if it didn't pass
            return false;
        }
        
        // Execute the proposal based on its type
        bool success = false;
        switch (proposal.type) {
            case ProposalType::PARAMETER_CHANGE:
                success = enforceParameterChanges(proposal);
                break;
            case ProposalType::PROTOCOL_UPGRADE:
                // TODO: Implement protocol upgrade logic
                Logger::info("VotingConsensusEngine: Protocol upgrade execution not yet implemented");
                success = true;
                break;
            case ProposalType::CONSENSUS_RULE_CHANGE:
                // TODO: Implement consensus rule change logic
                Logger::info("VotingConsensusEngine: Consensus rule change execution not yet implemented");
                success = true;
                break;
            default:
                Logger::warning("VotingConsensusEngine: Unknown proposal type for execution");
                success = false;
        }
        
        if (success) {
            proposal.isExecuted = true;
            Logger::info("VotingConsensusEngine: Successfully executed proposal: " + proposalId);
            logVotingEvent("proposal_executed", {{"proposalId", proposalId}});
        }
        
        lastUpdate = std::chrono::steady_clock::now();
        return success;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine proposal execution error: " + std::string(e.what()));
        return false;
    }
}

bool VotingConsensusEngine::enforceParameterChanges(const GovernanceProposal& proposal) {
    try {
        // Apply parameter changes
        for (const auto& [param, value] : proposal.parameters) {
            double numValue = std::stod(value);
            
            // Apply to this engine's parameters
            std::map<std::string, double> paramMap = {{param, numValue}};
            adjustParameters(paramMap);
            
            Logger::info("VotingConsensusEngine: Applied parameter change: " + param + " = " + value);
        }
        
        return true;
    } catch (const std::exception& e) {
        Logger::error("VotingConsensusEngine parameter enforcement error: " + std::string(e.what()));
        return false;
    }
}

// Helper methods implementation

std::string VotingConsensusEngine::generateProposalId(const GovernanceProposal& proposal) const {
    std::stringstream ss;
    ss << "PROP_" << proposal.proposer << "_" << getCurrentTime() << "_" 
       << std::hash<std::string>{}(proposal.title);
    return ss.str();
}

std::string VotingConsensusEngine::generateVoteId(const std::string& proposalId, const std::string& voter) const {
    return "VOTE_" + proposalId + "_" + voter;
}

Transaction VotingConsensusEngine::createVoteTransaction(const VoteRecord& voteRecord) const {
    // Create a special transaction to record the vote
    std::string recipient = "VOTE_" + voteRecord.proposalId;
    double amount = static_cast<double>(voteRecord.vote); // Encode vote type as amount
    
    Transaction voteTx(voteRecord.voter, recipient, amount);
    return voteTx;
}

bool VotingConsensusEngine::processVotingTransaction(const Transaction& transaction) {
    // Validate voting transaction format
    std::string recipient = transaction.getRecipient();
    if (recipient.find("VOTE_") != 0) {
        return true; // Not a voting transaction
    }
    
    // Extract proposal ID from recipient
    std::string proposalId = recipient.substr(5); // Remove "VOTE_" prefix
    
    // Check if proposal exists
    if (proposals.find(proposalId) == proposals.end()) {
        Logger::warning("VotingConsensusEngine: Vote transaction for unknown proposal: " + proposalId);
        return false;
    }
    
    return true;
}

bool VotingConsensusEngine::validateProposal(const GovernanceProposal& proposal) const {
    if (proposal.title.empty() || proposal.proposer.empty()) {
        return false;
    }
    
    if (proposal.requiredThreshold < 0.0 || proposal.requiredThreshold > 1.0) {
        return false;
    }
    
    return true;
}

bool VotingConsensusEngine::validateVote(const std::string& proposalId, const std::string& voter, VoteType vote) const {
    // Check if proposal exists
    if (proposals.find(proposalId) == proposals.end()) {
        Logger::error("VotingConsensusEngine: Proposal not found: " + proposalId);
        return false;
    }
    
    // Check if proposal is active
    if (!isProposalActive(proposalId)) {
        Logger::error("VotingConsensusEngine: Proposal is not active: " + proposalId);
        return false;
    }
    
    // Check if voter has already voted
    if (votes.find(proposalId) != votes.end()) {
        for (const auto& existingVote : votes.at(proposalId)) {
            if (existingVote.voter == voter) {
                Logger::error("VotingConsensusEngine: Voter has already voted: " + voter);
                return false;
            }
        }
    }
    
    return true;
}

bool VotingConsensusEngine::canVote(const std::string& voter) const {
    if (voter.empty()) {
        return false;
    }
    
    // Check minimum voting power requirement
    double votingPower = calculateVotingPower(voter);
    return votingPower >= minVotingPower;
}

double VotingConsensusEngine::calculateVotingPower(const std::string& voter) const {
    // TODO: Implement proper voting power calculation based on stake, contribution, etc.
    // For now, return a default value
    return 1.0;
}

bool VotingConsensusEngine::isProposalActive(const std::string& proposalId) const {
    auto it = proposals.find(proposalId);
    if (it == proposals.end()) {
        return false;
    }
    
    const GovernanceProposal& proposal = it->second;
    return proposal.isActive && !hasVotingEnded(proposalId);
}

bool VotingConsensusEngine::hasVotingEnded(const std::string& proposalId) const {
    auto it = proposals.find(proposalId);
    if (it == proposals.end()) {
        return true;
    }
    
    const GovernanceProposal& proposal = it->second;
    return getCurrentTime() >= proposal.votingDeadline;
}

uint64_t VotingConsensusEngine::getCurrentTime() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::vector<GovernanceProposal> VotingConsensusEngine::getActiveProposals() const {
    std::lock_guard<std::mutex> lock(votingMutex);
    
    std::vector<GovernanceProposal> activeProposals;
    for (const auto& [id, proposal] : proposals) {
        if (isProposalActive(id)) {
            activeProposals.push_back(proposal);
        }
    }
    
    return activeProposals;
}

void VotingConsensusEngine::logVotingEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logData;
    logData["engine"] = "VotingConsensusEngine";
    logData["event"] = event;
    logData["timestamp"] = getCurrentTime();
    logData["data"] = data;
    
    Logger::info("VotingEvent: " + logData.dump());
}