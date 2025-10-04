#include "voting_consensus_engine.h"
#include "consensus_harmony_manager.h"
#include "blockchain.h"
#include "logger.h"
#include <iostream>
#include <memory>

/**
 * Demonstration of the Voting Consensus Engine integration
 * This example shows how to:
 * 1. Create and register a voting consensus engine
 * 2. Create governance proposals
 * 3. Cast votes on proposals
 * 4. Tally results and enforce parameter changes
 * 5. Record votes on the blockchain
 */

int main() {
    std::cout << "=== Nilotic Blockchain Voting Consensus Demo ===" << std::endl;
    
    try {
        // Initialize blockchain and consensus harmony manager
        auto blockchain = std::make_unique<Blockchain>();
        auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        
        if (!manager->initializeConsensus()) {
            std::cerr << "Failed to initialize consensus harmony manager" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Consensus Harmony Manager initialized" << std::endl;
        
        // Create and register voting consensus engine
        auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
        VotingConsensusEngine* enginePtr = votingEngine.get();
        
        if (!manager->registerConsensusEngine(std::move(votingEngine))) {
            std::cerr << "Failed to register voting consensus engine" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Voting Consensus Engine registered" << std::endl;
        
        // Display initial engine status
        auto status = enginePtr->getStatus();
        std::cout << "Engine Status: " << status.dump(2) << std::endl;
        
        // Create a governance proposal
        GovernanceProposal proposal;
        proposal.type = ProposalType::PARAMETER_CHANGE;
        proposal.title = "Increase Supermajority Threshold";
        proposal.description = "Proposal to increase the supermajority threshold from 67% to 75% for enhanced security";
        proposal.proposer = "community_leader_1";
        proposal.parameters = {
            {"supermajorityThreshold", "0.75"}
        };
        proposal.requiredThreshold = 0.6; // 60% required to pass
        
        if (!enginePtr->createProposal(proposal)) {
            std::cerr << "Failed to create governance proposal" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Governance proposal created: " << proposal.title << std::endl;
        
        // Get the proposal ID
        auto activeProposals = enginePtr->getActiveProposals();
        if (activeProposals.empty()) {
            std::cerr << "No active proposals found" << std::endl;
            return 1;
        }
        
        std::string proposalId = activeProposals[0].proposalId;
        std::cout << "Proposal ID: " << proposalId << std::endl;
        
        // Simulate voting by different stakeholders
        struct Voter {
            std::string address;
            VoteType vote;
            double votingPower;
            std::string description;
        };
        
        std::vector<Voter> voters = {
            {"validator_node_1", VoteType::YES, 25.0, "Major validator - supports security enhancement"},
            {"validator_node_2", VoteType::YES, 20.0, "Major validator - agrees with proposal"},
            {"community_member_1", VoteType::YES, 15.0, "Community representative - supports change"},
            {"validator_node_3", VoteType::NO, 18.0, "Major validator - prefers current threshold"},
            {"community_member_2", VoteType::YES, 12.0, "Community representative - supports security"},
            {"developer_1", VoteType::ABSTAIN, 8.0, "Core developer - neutral on governance"},
            {"community_member_3", VoteType::NO, 10.0, "Community representative - opposes change"}
        };
        
        std::cout << "\n=== Voting Process ===" << std::endl;
        
        // Cast votes
        for (const auto& voter : voters) {
            if (enginePtr->castVote(proposalId, voter.address, voter.vote, voter.votingPower)) {
                std::string voteStr;
                switch (voter.vote) {
                    case VoteType::YES: voteStr = "YES"; break;
                    case VoteType::NO: voteStr = "NO"; break;
                    case VoteType::ABSTAIN: voteStr = "ABSTAIN"; break;
                }
                std::cout << "✓ " << voter.address << " voted " << voteStr 
                         << " (power: " << voter.votingPower << ") - " << voter.description << std::endl;
            } else {
                std::cout << "✗ Failed to cast vote for " << voter.address << std::endl;
            }
        }
        
        // Tally the votes
        std::cout << "\n=== Vote Tallying ===" << std::endl;
        VotingResults results = enginePtr->tallyVotes(proposalId);
        
        std::cout << "Total Votes: " << results.totalVotes << std::endl;
        std::cout << "Total Voting Power: " << results.totalVotingPower << std::endl;
        std::cout << "YES Votes: " << results.yesVotes << " (power: " << results.yesVotingPower << ")" << std::endl;
        std::cout << "NO Votes: " << results.noVotes << " (power: " << results.noVotingPower << ")" << std::endl;
        std::cout << "ABSTAIN Votes: " << results.abstainVotes << " (power: " << results.abstainVotingPower << ")" << std::endl;
        
        double yesPercentage = (results.yesVotingPower / results.totalVotingPower) * 100.0;
        std::cout << "YES Percentage: " << yesPercentage << "%" << std::endl;
        std::cout << "Required Threshold: " << (proposal.requiredThreshold * 100.0) << "%" << std::endl;
        std::cout << "Proposal Status: " << (results.passed ? "PASSED" : "FAILED") << std::endl;
        
        // If proposal passed, demonstrate parameter enforcement
        if (results.passed) {
            std::cout << "\n=== Parameter Enforcement ===" << std::endl;
            
            // Get current parameters
            auto currentParams = enginePtr->getParameters();
            std::cout << "Current supermajority threshold: " << currentParams["supermajorityThreshold"] << std::endl;
            
            // Enforce the parameter changes
            if (enginePtr->enforceParameterChanges(proposal)) {
                auto newParams = enginePtr->getParameters();
                std::cout << "✓ Parameter changes enforced" << std::endl;
                std::cout << "New supermajority threshold: " << newParams["supermajorityThreshold"] << std::endl;
            } else {
                std::cout << "✗ Failed to enforce parameter changes" << std::endl;
            }
        }
        
        // Display final metrics
        std::cout << "\n=== Final Metrics ===" << std::endl;
        auto metrics = enginePtr->getMetrics();
        std::cout << "Engine Metrics: " << metrics.dump(2) << std::endl;
        
        // Test blockchain integration by validating a voting transaction
        std::cout << "\n=== Blockchain Integration Test ===" << std::endl;
        Transaction votingTx("test_voter", "VOTE_" + proposalId, 1.0);
        
        if (manager->validateTransaction(votingTx)) {
            std::cout << "✓ Voting transaction validated successfully" << std::endl;
        } else {
            std::cout << "✗ Voting transaction validation failed" << std::endl;
        }
        
        // Create a block with voting transactions
        Block testBlock(1, blockchain->getLatestBlock().getHash());
        testBlock.addTransaction(votingTx);
        
        if (manager->validateBlock(testBlock)) {
            std::cout << "✓ Block with voting transactions validated successfully" << std::endl;
        } else {
            std::cout << "✗ Block validation failed" << std::endl;
        }
        
        std::cout << "\n=== Demo Completed Successfully ===" << std::endl;
        
        // Cleanup
        manager->shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}