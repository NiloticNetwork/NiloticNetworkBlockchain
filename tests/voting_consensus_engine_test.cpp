#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "voting_consensus_engine.h"
#include "blockchain.h"
#include "block.h"
#include "transaction.h"
#include <memory>
#include <thread>
#include <chrono>

class VotingConsensusEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        blockchain = std::make_unique<Blockchain>();
        engine = std::make_unique<VotingConsensusEngine>(blockchain.get());
        ASSERT_TRUE(engine->initialize());
    }

    void TearDown() override {
        if (engine) {
            engine->shutdown();
        }
    }

    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<VotingConsensusEngine> engine;
};

// Test basic engine initialization and status
TEST_F(VotingConsensusEngineTest, InitializationAndStatus) {
    EXPECT_TRUE(engine->isHealthy());
    EXPECT_EQ(engine->getType(), ConsensusType::VOTING_CONSENSUS);
    EXPECT_EQ(engine->getName(), "VotingConsensusEngine");
    
    auto status = engine->getStatus();
    EXPECT_TRUE(status["initialized"].get<bool>());
    EXPECT_TRUE(status["healthy"].get<bool>());
    EXPECT_EQ(status["type"].get<std::string>(), "VOTING_CONSENSUS");
}

// Test parameter adjustment
TEST_F(VotingConsensusEngineTest, ParameterAdjustment) {
    std::map<std::string, double> params = {
        {"supermajorityThreshold", 0.75},
        {"defaultVotingPeriod", 86400.0}, // 1 day
        {"minVotingPower", 5.0}
    };
    
    EXPECT_TRUE(engine->adjustParameters(params));
    
    auto retrievedParams = engine->getParameters();
    EXPECT_DOUBLE_EQ(retrievedParams["supermajorityThreshold"], 0.75);
    EXPECT_DOUBLE_EQ(retrievedParams["defaultVotingPeriod"], 86400.0);
    EXPECT_DOUBLE_EQ(retrievedParams["minVotingPower"], 5.0);
}

// Test invalid parameter adjustment
TEST_F(VotingConsensusEngineTest, InvalidParameterAdjustment) {
    std::map<std::string, double> invalidParams = {
        {"supermajorityThreshold", 1.5}, // Invalid: > 1.0
        {"defaultVotingPeriod", -100.0}, // Invalid: negative
    };
    
    // Should still return true but not apply invalid values
    EXPECT_TRUE(engine->adjustParameters(invalidParams));
    
    auto params = engine->getParameters();
    EXPECT_NE(params["supermajorityThreshold"], 1.5);
    EXPECT_NE(params["defaultVotingPeriod"], -100.0);
}

// Test proposal creation
TEST_F(VotingConsensusEngineTest, ProposalCreation) {
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Test Proposal";
    proposal.description = "A test governance proposal";
    proposal.proposer = "test_proposer";
    proposal.parameters = {{"testParam", "100"}};
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    auto activeProposals = engine->getActiveProposals();
    EXPECT_EQ(activeProposals.size(), 1);
    EXPECT_EQ(activeProposals[0].title, "Test Proposal");
    EXPECT_EQ(activeProposals[0].proposer, "test_proposer");
}

// Test invalid proposal creation
TEST_F(VotingConsensusEngineTest, InvalidProposalCreation) {
    GovernanceProposal invalidProposal;
    // Missing required fields
    invalidProposal.title = ""; // Empty title
    invalidProposal.proposer = ""; // Empty proposer
    invalidProposal.requiredThreshold = 1.5; // Invalid threshold
    
    EXPECT_FALSE(engine->createProposal(invalidProposal));
    
    auto activeProposals = engine->getActiveProposals();
    EXPECT_EQ(activeProposals.size(), 0);
}

// Test vote casting
TEST_F(VotingConsensusEngineTest, VoteCasting) {
    // First create a proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Test Voting Proposal";
    proposal.description = "A proposal to test voting";
    proposal.proposer = "test_proposer";
    proposal.parameters = {{"testParam", "200"}};
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    auto activeProposals = engine->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Cast votes
    EXPECT_TRUE(engine->castVote(proposalId, "voter1", VoteType::YES, 10.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter2", VoteType::NO, 5.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter3", VoteType::YES, 8.0));
    
    // Verify votes were recorded
    auto votes = engine->getVotes(proposalId);
    EXPECT_EQ(votes.size(), 3);
    
    // Check vote details
    bool foundVoter1 = false, foundVoter2 = false, foundVoter3 = false;
    for (const auto& vote : votes) {
        if (vote.voter == "voter1") {
            foundVoter1 = true;
            EXPECT_EQ(vote.vote, VoteType::YES);
            EXPECT_DOUBLE_EQ(vote.votingPower, 10.0);
        } else if (vote.voter == "voter2") {
            foundVoter2 = true;
            EXPECT_EQ(vote.vote, VoteType::NO);
            EXPECT_DOUBLE_EQ(vote.votingPower, 5.0);
        } else if (vote.voter == "voter3") {
            foundVoter3 = true;
            EXPECT_EQ(vote.vote, VoteType::YES);
            EXPECT_DOUBLE_EQ(vote.votingPower, 8.0);
        }
    }
    EXPECT_TRUE(foundVoter1 && foundVoter2 && foundVoter3);
}

// Test duplicate vote prevention
TEST_F(VotingConsensusEngineTest, DuplicateVotePrevention) {
    // Create a proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Duplicate Vote Test";
    proposal.proposer = "test_proposer";
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    auto activeProposals = engine->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Cast first vote
    EXPECT_TRUE(engine->castVote(proposalId, "voter1", VoteType::YES, 10.0));
    
    // Try to cast duplicate vote - should fail
    EXPECT_FALSE(engine->castVote(proposalId, "voter1", VoteType::NO, 5.0));
    
    // Verify only one vote was recorded
    auto votes = engine->getVotes(proposalId);
    EXPECT_EQ(votes.size(), 1);
    EXPECT_EQ(votes[0].vote, VoteType::YES); // Original vote should remain
}

// Test vote tallying
TEST_F(VotingConsensusEngineTest, VoteTallying) {
    // Create a proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Tally Test Proposal";
    proposal.proposer = "test_proposer";
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    auto activeProposals = engine->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Cast votes with different voting powers
    EXPECT_TRUE(engine->castVote(proposalId, "voter1", VoteType::YES, 30.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter2", VoteType::YES, 20.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter3", VoteType::NO, 15.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter4", VoteType::ABSTAIN, 5.0));
    
    // Tally votes
    VotingResults results = engine->tallyVotes(proposalId);
    
    // Verify tallying results
    EXPECT_EQ(results.totalVotes, 4);
    EXPECT_EQ(results.yesVotes, 2);
    EXPECT_EQ(results.noVotes, 1);
    EXPECT_EQ(results.abstainVotes, 1);
    
    EXPECT_DOUBLE_EQ(results.totalVotingPower, 70.0);
    EXPECT_DOUBLE_EQ(results.yesVotingPower, 50.0);
    EXPECT_DOUBLE_EQ(results.noVotingPower, 15.0);
    EXPECT_DOUBLE_EQ(results.abstainVotingPower, 5.0);
    
    // Check if proposal passed (50/70 = 0.714 > 0.6 threshold)
    EXPECT_TRUE(results.passed);
}

// Test proposal that doesn't pass
TEST_F(VotingConsensusEngineTest, ProposalDoesNotPass) {
    // Create a proposal with high threshold
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "High Threshold Proposal";
    proposal.proposer = "test_proposer";
    proposal.requiredThreshold = 0.8; // High threshold
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    auto activeProposals = engine->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Cast votes where YES doesn't meet threshold
    EXPECT_TRUE(engine->castVote(proposalId, "voter1", VoteType::YES, 30.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter2", VoteType::NO, 40.0));
    EXPECT_TRUE(engine->castVote(proposalId, "voter3", VoteType::ABSTAIN, 10.0));
    
    // Tally votes
    VotingResults results = engine->tallyVotes(proposalId);
    
    // Check that proposal didn't pass (30/80 = 0.375 < 0.8 threshold)
    EXPECT_FALSE(results.passed);
}

// Test transaction validation
TEST_F(VotingConsensusEngineTest, TransactionValidation) {
    // Create a regular transaction (should pass)
    Transaction regularTx("sender1", "recipient1", 100.0);
    EXPECT_TRUE(engine->validateTransaction(regularTx));
    
    // Create a voting transaction
    Transaction votingTx("voter1", "VOTE_PROP123", 1.0);
    // Should pass validation even if proposal doesn't exist (other validation will catch it)
    EXPECT_TRUE(engine->validateTransaction(votingTx));
}

// Test block validation
TEST_F(VotingConsensusEngineTest, BlockValidation) {
    // Create a block with regular transactions
    Block testBlock(1, "previous_hash");
    
    Transaction tx1("sender1", "recipient1", 50.0);
    Transaction tx2("sender2", "recipient2", 75.0);
    
    testBlock.addTransaction(tx1);
    testBlock.addTransaction(tx2);
    
    EXPECT_TRUE(engine->validateBlock(testBlock));
}

// Test consensus request processing
TEST_F(VotingConsensusEngineTest, ConsensusRequestProcessing) {
    // Test block validation request
    Block testBlock(1, "previous_hash");
    Transaction tx("sender", "recipient", 100.0);
    testBlock.addTransaction(tx);
    
    ConsensusRequest blockRequest(RequestType::BLOCK_VALIDATION, testBlock.serialize());
    ConsensusResult result = engine->processRequest(blockRequest);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::VOTING_CONSENSUS);
    EXPECT_DOUBLE_EQ(result.confidence, 1.0);
    
    // Test transaction validation request
    ConsensusRequest txRequest(RequestType::TRANSACTION_VALIDATION, tx.serialize());
    ConsensusResult txResult = engine->processRequest(txRequest);
    
    EXPECT_TRUE(txResult.isValid);
    EXPECT_EQ(txResult.mechanism, ConsensusType::VOTING_CONSENSUS);
}

// Test metrics collection
TEST_F(VotingConsensusEngineTest, MetricsCollection) {
    // Create some proposals and votes
    GovernanceProposal proposal1;
    proposal1.type = ProposalType::PARAMETER_CHANGE;
    proposal1.title = "Proposal 1";
    proposal1.proposer = "proposer1";
    proposal1.requiredThreshold = 0.6;
    
    GovernanceProposal proposal2;
    proposal2.type = ProposalType::PROTOCOL_UPGRADE;
    proposal2.title = "Proposal 2";
    proposal2.proposer = "proposer2";
    proposal2.requiredThreshold = 0.7;
    
    EXPECT_TRUE(engine->createProposal(proposal1));
    EXPECT_TRUE(engine->createProposal(proposal2));
    
    auto activeProposals = engine->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 2);
    
    // Cast some votes
    EXPECT_TRUE(engine->castVote(activeProposals[0].proposalId, "voter1", VoteType::YES));
    EXPECT_TRUE(engine->castVote(activeProposals[0].proposalId, "voter2", VoteType::NO));
    EXPECT_TRUE(engine->castVote(activeProposals[1].proposalId, "voter3", VoteType::YES));
    
    // Check metrics
    auto metrics = engine->getMetrics();
    
    EXPECT_EQ(metrics["proposals"]["total"].get<int>(), 2);
    EXPECT_EQ(metrics["proposals"]["active"].get<int>(), 2);
    EXPECT_EQ(metrics["proposals"]["executed"].get<int>(), 0);
    
    EXPECT_EQ(metrics["votes"]["total"].get<int>(), 3);
    EXPECT_EQ(metrics["votes"]["proposals_with_votes"].get<int>(), 2);
}

// Test engine shutdown and reinitialization
TEST_F(VotingConsensusEngineTest, ShutdownAndReinitialization) {
    // Create some state
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Test Proposal";
    proposal.proposer = "test_proposer";
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    EXPECT_EQ(engine->getActiveProposals().size(), 1);
    
    // Shutdown
    engine->shutdown();
    EXPECT_FALSE(engine->isHealthy());
    
    // Reinitialize
    EXPECT_TRUE(engine->initialize());
    EXPECT_TRUE(engine->isHealthy());
    
    // State should be cleared after reinitialization
    EXPECT_EQ(engine->getActiveProposals().size(), 0);
}

// Test concurrent vote casting (thread safety)
TEST_F(VotingConsensusEngineTest, ConcurrentVoteCasting) {
    // Create a proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Concurrent Test Proposal";
    proposal.proposer = "test_proposer";
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    auto activeProposals = engine->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Launch multiple threads to cast votes concurrently
    const int numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successfulVotes(0);
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            std::string voter = "voter" + std::to_string(i);
            if (engine->castVote(proposalId, voter, VoteType::YES, 1.0)) {
                successfulVotes++;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All votes should have been successful
    EXPECT_EQ(successfulVotes.load(), numThreads);
    
    auto votes = engine->getVotes(proposalId);
    EXPECT_EQ(votes.size(), numThreads);
}

// Test parameter enforcement
TEST_F(VotingConsensusEngineTest, ParameterEnforcement) {
    // Create a parameter change proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Parameter Change Proposal";
    proposal.proposer = "test_proposer";
    proposal.parameters = {
        {"supermajorityThreshold", "0.75"},
        {"minVotingPower", "2.0"}
    };
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(engine->createProposal(proposal));
    
    // Get original parameters
    auto originalParams = engine->getParameters();
    double originalThreshold = originalParams["supermajorityThreshold"];
    double originalMinPower = originalParams["minVotingPower"];
    
    // Test parameter enforcement
    EXPECT_TRUE(engine->enforceParameterChanges(proposal));
    
    // Verify parameters were changed
    auto newParams = engine->getParameters();
    EXPECT_DOUBLE_EQ(newParams["supermajorityThreshold"], 0.75);
    EXPECT_DOUBLE_EQ(newParams["minVotingPower"], 2.0);
    
    // Verify they're different from original
    EXPECT_NE(newParams["supermajorityThreshold"], originalThreshold);
    EXPECT_NE(newParams["minVotingPower"], originalMinPower);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}