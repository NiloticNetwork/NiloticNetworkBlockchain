#include "voting_consensus_engine.h"
#include "blockchain.h"
#include "logger.h"
#include <iostream>
#include <memory>

/**
 * Simple demonstration of the Voting Consensus Engine
 * This example shows basic voting functionality without the full harmony system
 */

int main() {
    std::cout << "=== Simple Voting Consensus Demo ===" << std::endl;
    
    try {
        // Initialize blockchain
        auto blockchain = std::make_unique<Blockchain>();
        std::cout << "✓ Blockchain initialized" << std::endl;
        
        // Create voting consensus engine
        auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
        
        if (!votingEngine->initialize()) {
            std::cerr << "Failed to initialize voting consensus engine" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Voting Consensus Engine initialized" << std::endl;
        
        // Display initial engine status
        std::cout << "Getting engine status..." << std::endl;
        auto status = votingEngine->getStatus();
        std::cout << "Engine Status: " << status.dump(2) << std::endl;
        
        // Create a governance proposal
        GovernanceProposal proposal;
        proposal.type = ProposalType::PARAMETER_CHANGE;
        proposal.title = "Increase Supermajority Threshold";
        proposal.description = "Proposal to increase the supermajority threshold from 67% to 75%";
        proposal.proposer = "community_leader_1";
        proposal.parameters = {
            {"supermajorityThreshold", "0.75"}
        };
        proposal.requiredThreshold = 0.6; // 60% required to pass
        
        if (!votingEngine->createProposal(proposal)) {
            std::cerr << "Failed to create governance proposal" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Governance proposal created: " << proposal.title << std::endl;
        
        // Get the proposal ID
        auto activeProposals = votingEngine->getActiveProposals();
        if (activeProposals.empty()) {
            std::cerr << "No active proposals found" << std::endl;
            return 1;
        }
        
        std::string proposalId = activeProposals[0].proposalId;
        std::cout << "Proposal ID: " << proposalId << std::endl;
        
        // Simulate voting
        std::cout << "\n=== Voting Process ===" << std::endl;
        
        // Cast votes
        if (votingEngine->castVote(proposalId, "validator_1", VoteType::YES, 25.0)) {
            std::cout << "✓ validator_1 voted YES (power: 25.0)" << std::endl;
        }
        
        if (votingEngine->castVote(proposalId, "validator_2", VoteType::YES, 20.0)) {
            std::cout << "✓ validator_2 voted YES (power: 20.0)" << std::endl;
        }
        
        if (votingEngine->castVote(proposalId, "validator_3", VoteType::NO, 18.0)) {
            std::cout << "✓ validator_3 voted NO (power: 18.0)" << std::endl;
        }
        
        if (votingEngine->castVote(proposalId, "community_1", VoteType::YES, 15.0)) {
            std::cout << "✓ community_1 voted YES (power: 15.0)" << std::endl;
        }
        
        if (votingEngine->castVote(proposalId, "community_2", VoteType::NO, 10.0)) {
            std::cout << "✓ community_2 voted NO (power: 10.0)" << std::endl;
        }
        
        // Tally the votes
        std::cout << "\n=== Vote Tallying ===" << std::endl;
        VotingResults results = votingEngine->tallyVotes(proposalId);
        
        std::cout << "Total Votes: " << results.totalVotes << std::endl;
        std::cout << "Total Voting Power: " << results.totalVotingPower << std::endl;
        std::cout << "YES Votes: " << results.yesVotes << " (power: " << results.yesVotingPower << ")" << std::endl;
        std::cout << "NO Votes: " << results.noVotes << " (power: " << results.noVotingPower << ")" << std::endl;
        
        double yesPercentage = (results.yesVotingPower / results.totalVotingPower) * 100.0;
        std::cout << "YES Percentage: " << yesPercentage << "%" << std::endl;
        std::cout << "Required Threshold: " << (proposal.requiredThreshold * 100.0) << "%" << std::endl;
        std::cout << "Proposal Status: " << (results.passed ? "PASSED" : "FAILED") << std::endl;
        
        // If proposal passed, demonstrate parameter enforcement
        if (results.passed) {
            std::cout << "\n=== Parameter Enforcement ===" << std::endl;
            
            // Get current parameters
            auto currentParams = votingEngine->getParameters();
            std::cout << "Current supermajority threshold: " << currentParams["supermajorityThreshold"] << std::endl;
            
            // Enforce the parameter changes
            if (votingEngine->enforceParameterChanges(proposal)) {
                auto newParams = votingEngine->getParameters();
                std::cout << "✓ Parameter changes enforced" << std::endl;
                std::cout << "New supermajority threshold: " << newParams["supermajorityThreshold"] << std::endl;
            } else {
                std::cout << "✗ Failed to enforce parameter changes" << std::endl;
            }
        }
        
        // Display final metrics
        std::cout << "\n=== Final Metrics ===" << std::endl;
        auto metrics = votingEngine->getMetrics();
        std::cout << "Engine Metrics: " << metrics.dump(2) << std::endl;
        
        // Test transaction validation
        std::cout << "\n=== Transaction Validation Test ===" << std::endl;
        Transaction votingTx("test_voter", "VOTE_" + proposalId, 1.0);
        
        if (votingEngine->validateTransaction(votingTx)) {
            std::cout << "✓ Voting transaction validated successfully" << std::endl;
        } else {
            std::cout << "✗ Voting transaction validation failed" << std::endl;
        }
        
        // Test block validation
        Block testBlock(1, blockchain->getLatestBlock().getHash());
        testBlock.addTransaction(votingTx);
        
        if (votingEngine->validateBlock(testBlock)) {
            std::cout << "✓ Block with voting transactions validated successfully" << std::endl;
        } else {
            std::cout << "✗ Block validation failed" << std::endl;
        }
        
        std::cout << "\n=== Demo Completed Successfully ===" << std::endl;
        
        // Cleanup
        votingEngine->shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}