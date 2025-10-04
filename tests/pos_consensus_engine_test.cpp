#include <gtest/gtest.h>
#include <memory>
#include "../include/core/pos_consensus_engine.h"
#include "../include/core/block.h"
#include "../include/core/transaction.h"
#include "../include/core/blockchain.h"

class PoSConsensusEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        posEngine = std::make_unique<PoSConsensusEngine>();
        posEngine->initialize();
        
        // Set up test account balances
        std::map<std::string, double> testBalances;
        testBalances["validator1"] = 5000.0;
        testBalances["validator2"] = 3000.0;
        testBalances["validator3"] = 2000.0;
        testBalances["user1"] = 1000.0;
        
        posEngine->setAccountBalances(testBalances);
    }
    
    void TearDown() override {
        posEngine->shutdown();
    }
    
    std::unique_ptr<PoSConsensusEngine> posEngine;
};

// Test PoS engine initialization
TEST_F(PoSConsensusEngineTest, InitializationTest) {
    EXPECT_TRUE(posEngine->isHealthy());
    EXPECT_EQ(posEngine->getType(), ConsensusType::PROOF_OF_STAKE);
    EXPECT_EQ(posEngine->getName(), "Proof of Stake Consensus Engine");
}

// Test staking tokens
TEST_F(PoSConsensusEngineTest, StakeTokensTest) {
    // Test successful staking
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    // Test insufficient balance
    EXPECT_FALSE(posEngine->stakeTokens("user1", 2000.0));
    
    // Test below minimum stake
    EXPECT_FALSE(posEngine->stakeTokens("validator2", 500.0));
    
    // Verify validator was added
    auto validators = posEngine->getValidators();
    EXPECT_EQ(validators.size(), 1);
    EXPECT_EQ(validators["validator1"].stakedAmount, 2000.0);
    EXPECT_TRUE(validators["validator1"].isActive);
}

// Test unstaking tokens
TEST_F(PoSConsensusEngineTest, UnstakeTokensTest) {
    // First stake some tokens
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    // Test successful unstaking
    EXPECT_TRUE(posEngine->unstakeTokens("validator1", 500.0));
    
    // Test unstaking more than staked
    EXPECT_FALSE(posEngine->unstakeTokens("validator1", 2000.0));
    
    // Test unstaking from non-validator
    EXPECT_FALSE(posEngine->unstakeTokens("user1", 100.0));
    
    // Verify stake amount
    auto validatorInfo = posEngine->getValidatorInfo("validator1");
    EXPECT_EQ(validatorInfo.stakedAmount, 1500.0);
}

// Test validator selection
TEST_F(PoSConsensusEngineTest, ValidatorSelectionTest) {
    // Add multiple validators
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 3000.0));
    EXPECT_TRUE(posEngine->stakeTokens("validator2", 2000.0));
    EXPECT_TRUE(posEngine->stakeTokens("validator3", 1500.0));
    
    // Test single validator selection
    std::string selectedValidator = posEngine->selectValidator();
    EXPECT_FALSE(selectedValidator.empty());
    
    // Test multiple validator selection
    std::vector<std::string> selectedValidators = posEngine->selectValidators(2);
    EXPECT_EQ(selectedValidators.size(), 2);
    
    // Verify selected validators are different
    if (selectedValidators.size() == 2) {
        EXPECT_NE(selectedValidators[0], selectedValidators[1]);
    }
}

// Test block validation
TEST_F(PoSConsensusEngineTest, BlockValidationTest) {
    // Add a validator
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    // Create a test block
    Block testBlock(1, "previous_hash");
    testBlock.setValidator("validator1");
    
    // Test successful validation
    EXPECT_TRUE(posEngine->validateBlock(testBlock));
    
    // Test validation with unknown validator
    Block invalidBlock(2, "previous_hash");
    invalidBlock.setValidator("unknown_validator");
    EXPECT_FALSE(posEngine->validateBlock(invalidBlock));
}

// Test transaction validation
TEST_F(PoSConsensusEngineTest, TransactionValidationTest) {
    // Add a validator
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    // Create valid transaction
    Transaction validTx("validator1", "user1", 500.0);
    EXPECT_TRUE(posEngine->validateTransaction(validTx));
    
    // Create transaction with insufficient balance (considering staked amount)
    Transaction invalidTx("validator1", "user1", 4000.0);
    EXPECT_FALSE(posEngine->validateTransaction(invalidTx));
    
    // Test coinbase transaction
    Transaction coinbaseTx("COINBASE", "validator1", 100.0);
    EXPECT_TRUE(posEngine->validateTransaction(coinbaseTx));
}

// Test cross-mechanism coordination
TEST_F(PoSConsensusEngineTest, CrossMechanismCoordinationTest) {
    // Add validators
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 3000.0));
    EXPECT_TRUE(posEngine->stakeTokens("validator2", 2000.0));
    
    // Test coordination with PoW
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, "test_data", 
                           {ConsensusType::PROOF_OF_WORK, ConsensusType::PROOF_OF_STAKE});
    
    EXPECT_TRUE(posEngine->coordinateWithMechanism(ConsensusType::PROOF_OF_WORK, request));
    
    // Test validator selection for coordination
    std::vector<ConsensusType> mechanisms = {ConsensusType::PROOF_OF_WORK};
    std::vector<std::string> coordinatingValidators = 
        posEngine->selectValidatorsForCoordination(mechanisms);
    
    // Should return empty since validators don't have mechanism support configured
    EXPECT_TRUE(coordinatingValidators.empty());
}

// Test consensus result processing
TEST_F(PoSConsensusEngineTest, ProcessRequestTest) {
    // Add a validator
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    // Test block validation request
    ConsensusRequest blockRequest(RequestType::BLOCK_VALIDATION, "block_data");
    ConsensusResult result = posEngine->processRequest(blockRequest);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_STAKE);
    EXPECT_GT(result.confidence, 0.0);
    
    // Test transaction validation request
    ConsensusRequest txRequest(RequestType::TRANSACTION_VALIDATION, "tx_data");
    ConsensusResult txResult = posEngine->processRequest(txRequest);
    
    EXPECT_TRUE(txResult.isValid);
    EXPECT_EQ(txResult.mechanism, ConsensusType::PROOF_OF_STAKE);
}

// Test parameter adjustment
TEST_F(PoSConsensusEngineTest, ParameterAdjustmentTest) {
    std::map<std::string, double> newParameters;
    newParameters["minStakeAmount"] = 1500.0;
    newParameters["slashingPenalty"] = 0.15;
    
    EXPECT_TRUE(posEngine->adjustParameters(newParameters));
    
    // Verify parameters were updated
    auto parameters = posEngine->getParameters();
    EXPECT_EQ(parameters["minStakeAmount"], 1500.0);
    EXPECT_EQ(parameters["slashingPenalty"], 0.15);
}

// Test validator slashing
TEST_F(PoSConsensusEngineTest, ValidatorSlashingTest) {
    // Add a validator
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    // Test slashing
    EXPECT_TRUE(posEngine->slashValidator("validator1", 0.1)); // 10% penalty
    
    // Verify stake was reduced
    auto validatorInfo = posEngine->getValidatorInfo("validator1");
    EXPECT_EQ(validatorInfo.stakedAmount, 1800.0); // 2000 - (2000 * 0.1)
    EXPECT_LT(validatorInfo.reputationScore, 1.0);
    
    // Test slashing non-existent validator
    EXPECT_FALSE(posEngine->slashValidator("unknown", 0.1));
}

// Test reputation updates
TEST_F(PoSConsensusEngineTest, ReputationUpdateTest) {
    // Add a validator
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 2000.0));
    
    auto initialInfo = posEngine->getValidatorInfo("validator1");
    double initialReputation = initialInfo.reputationScore;
    
    // Test successful validation reputation update
    posEngine->updateValidatorReputation("validator1", true);
    auto updatedInfo = posEngine->getValidatorInfo("validator1");
    EXPECT_GE(updatedInfo.reputationScore, initialReputation);
    
    // Test failed validation reputation update
    posEngine->updateValidatorReputation("validator1", false);
    auto penalizedInfo = posEngine->getValidatorInfo("validator1");
    EXPECT_LT(penalizedInfo.reputationScore, updatedInfo.reputationScore);
}

// Test status and metrics
TEST_F(PoSConsensusEngineTest, StatusAndMetricsTest) {
    // Add validators
    EXPECT_TRUE(posEngine->stakeTokens("validator1", 3000.0));
    EXPECT_TRUE(posEngine->stakeTokens("validator2", 2000.0));
    
    // Test status
    nlohmann::json status = posEngine->getStatus();
    EXPECT_EQ(status["type"], "PROOF_OF_STAKE");
    EXPECT_TRUE(status["healthy"]);
    EXPECT_EQ(status["totalValidators"], 2);
    EXPECT_EQ(status["totalStake"], 5000.0);
    
    // Test metrics
    nlohmann::json metrics = posEngine->getMetrics();
    EXPECT_TRUE(metrics.contains("totalValidations"));
    EXPECT_TRUE(metrics.contains("successfulValidations"));
    EXPECT_TRUE(metrics.contains("validators"));
    EXPECT_EQ(metrics["validators"].size(), 2);
}

// Integration test with Blockchain class
TEST_F(PoSConsensusEngineTest, BlockchainIntegrationTest) {
    Blockchain blockchain;
    
    // Test staking through blockchain
    EXPECT_TRUE(blockchain.stakeTokens("GENESIS", 2000.0));
    
    // Test validator selection through blockchain
    std::string selectedValidator = blockchain.selectValidator();
    EXPECT_FALSE(selectedValidator.empty());
    
    // Test PoS status through blockchain
    nlohmann::json posStatus = blockchain.getPoSStatus();
    EXPECT_TRUE(posStatus.contains("type"));
    EXPECT_EQ(posStatus["type"], "PROOF_OF_STAKE");
}

// Test cross-mechanism validation
TEST_F(PoSConsensusEngineTest, CrossMechanismValidationTest) {
    Blockchain blockchain;
    
    // Add some validators
    EXPECT_TRUE(blockchain.stakeTokens("GENESIS", 2000.0));
    
    // Create a test block
    Block testBlock(1, blockchain.getLatestBlock().getHash());
    testBlock.setValidator("GENESIS");
    
    // Test multi-consensus validation
    std::vector<ConsensusType> mechanisms = {ConsensusType::PROOF_OF_STAKE};
    EXPECT_TRUE(blockchain.validateBlockWithMultipleConsensus(testBlock, mechanisms));
    
    // Test validator selection for coordination
    std::vector<std::string> coordinatingValidators = 
        blockchain.selectValidatorsForCoordination(mechanisms, 1);
    
    // Should have at least one validator available
    EXPECT_FALSE(coordinatingValidators.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}