#include <gtest/gtest.h>
#include "voting_consensus_engine.h"
#include "consensus_harmony_manager.h"
#include "blockchain.h"
#include <memory>

class VotingConsensusIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        blockchain = std::make_unique<Blockchain>();
        manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        ASSERT_TRUE(manager->initializeConsensus());
    }

    void TearDown() override {
        if (manager) {
            manager->shutdown();
        }
    }

    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> manager;
};

// Test integration of VotingConsensusEngine with ConsensusHarmonyManager
TEST_F(VotingConsensusIntegrationTest, VotingEngineRegistration) {
    // Create and register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Verify engine is registered
    auto activeEngines = manager->getActiveEngines();
    bool foundVotingEngine = false;
    for (const auto& type : activeEngines) {
        if (type == ConsensusType::VOTING_CONSENSUS) {
            foundVotingEngine = true;
            break;
        }
    }
    EXPECT_TRUE(foundVotingEngine);
}

// Test governance proposal processing through the harmony system
TEST_F(VotingConsensusIntegrationTest, GovernanceProposalProcessing) {
    // Register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    VotingConsensusEngine* enginePtr = votingEngine.get();
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Create a governance proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Integration Test Proposal";
    proposal.description = "Test proposal for integration testing";
    proposal.proposer = "integration_test";
    proposal.parameters = {{"testParam", "500"}};
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(enginePtr->createProposal(proposal));
    
    // Verify proposal was created
    auto activeProposals = enginePtr->getActiveProposals();
    EXPECT_EQ(activeProposals.size(), 1);
    EXPECT_EQ(activeProposals[0].title, "Integration Test Proposal");
}

// Test voting transaction validation through the harmony system
TEST_F(VotingConsensusIntegrationTest, VotingTransactionValidation) {
    // Register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Create a voting transaction
    Transaction votingTx("voter1", "VOTE_PROP123", 1.0);
    
    // Validate through the harmony manager
    EXPECT_TRUE(manager->validateTransaction(votingTx));
}

// Test block validation with voting transactions
TEST_F(VotingConsensusIntegrationTest, BlockValidationWithVotingTransactions) {
    // Register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Create a block with voting transactions
    Block testBlock(1, "previous_hash");
    
    Transaction regularTx("sender1", "recipient1", 100.0);
    Transaction votingTx("voter1", "VOTE_PROP123", 1.0);
    
    testBlock.addTransaction(regularTx);
    testBlock.addTransaction(votingTx);
    
    // Validate through the harmony manager
    EXPECT_TRUE(manager->validateBlock(testBlock));
}

// Test consensus status reporting with voting engine
TEST_F(VotingConsensusIntegrationTest, ConsensusStatusReporting) {
    // Register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Get detailed status
    auto status = manager->getDetailedStatus();
    
    EXPECT_TRUE(status["initialized"].get<bool>());
    EXPECT_TRUE(status["running"].get<bool>());
    
    // Check that voting consensus is listed in mechanisms
    EXPECT_TRUE(status.contains("mechanisms"));
    EXPECT_TRUE(status["mechanisms"].contains("VOTING_CONSENSUS"));
    EXPECT_TRUE(status["mechanisms"]["VOTING_CONSENSUS"]["active"].get<bool>());
}

// Test parameter enforcement through governance
TEST_F(VotingConsensusIntegrationTest, ParameterEnforcementIntegration) {
    // Register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    VotingConsensusEngine* enginePtr = votingEngine.get();
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Create a parameter change proposal
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Parameter Change Test";
    proposal.proposer = "test_proposer";
    proposal.parameters = {
        {"supermajorityThreshold", "0.75"}
    };
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(enginePtr->createProposal(proposal));
    
    auto activeProposals = enginePtr->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Cast votes to pass the proposal
    EXPECT_TRUE(enginePtr->castVote(proposalId, "voter1", VoteType::YES, 40.0));
    EXPECT_TRUE(enginePtr->castVote(proposalId, "voter2", VoteType::YES, 30.0));
    EXPECT_TRUE(enginePtr->castVote(proposalId, "voter3", VoteType::NO, 20.0));
    
    // Tally votes
    VotingResults results = enginePtr->tallyVotes(proposalId);
    EXPECT_TRUE(results.passed);
    
    // Get original parameters
    auto originalParams = enginePtr->getParameters();
    double originalThreshold = originalParams["supermajorityThreshold"];
    
    // Execute the proposal (enforce parameter changes)
    EXPECT_TRUE(enginePtr->enforceParameterChanges(proposal));
    
    // Verify parameter was changed
    auto newParams = enginePtr->getParameters();
    EXPECT_DOUBLE_EQ(newParams["supermajorityThreshold"], 0.75);
    EXPECT_NE(newParams["supermajorityThreshold"], originalThreshold);
}

// Test metrics collection integration
TEST_F(VotingConsensusIntegrationTest, MetricsCollectionIntegration) {
    // Register voting consensus engine
    auto votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
    VotingConsensusEngine* enginePtr = votingEngine.get();
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(votingEngine)));
    
    // Create some activity
    GovernanceProposal proposal;
    proposal.type = ProposalType::PARAMETER_CHANGE;
    proposal.title = "Metrics Test Proposal";
    proposal.proposer = "metrics_test";
    proposal.requiredThreshold = 0.6;
    
    EXPECT_TRUE(enginePtr->createProposal(proposal));
    
    auto activeProposals = enginePtr->getActiveProposals();
    ASSERT_EQ(activeProposals.size(), 1);
    std::string proposalId = activeProposals[0].proposalId;
    
    // Cast some votes
    EXPECT_TRUE(enginePtr->castVote(proposalId, "voter1", VoteType::YES));
    EXPECT_TRUE(enginePtr->castVote(proposalId, "voter2", VoteType::NO));
    
    // Get metrics from the engine
    auto engineMetrics = enginePtr->getMetrics();
    EXPECT_EQ(engineMetrics["proposals"]["total"].get<int>(), 1);
    EXPECT_EQ(engineMetrics["votes"]["total"].get<int>(), 2);
    
    // Get metrics from the manager
    auto managerMetrics = manager->getMetrics();
    EXPECT_TRUE(managerMetrics.contains("activeEngineCount"));
    EXPECT_GE(managerMetrics["activeEngineCount"].get<int>(), 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}