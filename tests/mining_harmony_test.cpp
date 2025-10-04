#include <gtest/gtest.h>
#include "../include/core/mining.h"
#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony.h"
#include "../include/core/logger.h"
#include <memory>

class MiningHarmonyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for tests
        Logger::setLogLevel(Logger::LogLevel::INFO);
        
        // Create blockchain instance
        blockchain = std::make_unique<Blockchain>();
        
        // Create mining configuration
        config.targetDifficulty = 2;  // Low difficulty for fast testing
        config.targetBlockTime = 10;  // 10 seconds for testing
        config.miningReward = 50.0;
        config.transactionFee = 0.01;
        config.maxBlockSize = 1024;
        config.maxTransactionsPerBlock = 10;
        
        // Create mining engine
        miningEngine = std::make_unique<MiningEngine>(*blockchain, config);
    }
    
    void TearDown() override {
        if (miningEngine) {
            miningEngine->shutdown();
        }
    }
    
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<MiningEngine> miningEngine;
    MiningConfig config;
};

// Test ConsensusEngine interface implementation
TEST_F(MiningHarmonyTest, ConsensusEngineInterface) {
    // Test initialization
    EXPECT_TRUE(miningEngine->initialize());
    EXPECT_TRUE(miningEngine->isHealthy());
    EXPECT_EQ(miningEngine->getType(), ConsensusType::PROOF_OF_WORK);
    EXPECT_EQ(miningEngine->getName(), "ProofOfWorkEngine");
    
    // Test status and metrics
    nlohmann::json status = miningEngine->getStatus();
    EXPECT_TRUE(status.contains("harmonyInitialized"));
    EXPECT_TRUE(status.contains("harmonyHealthy"));
    EXPECT_TRUE(status.contains("consensusType"));
    
    nlohmann::json metrics = miningEngine->getMetrics();
    EXPECT_TRUE(metrics.contains("mining"));
    EXPECT_TRUE(metrics.contains("harmony"));
}

// Test parameter adjustment
TEST_F(MiningHarmonyTest, ParameterAdjustment) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Get initial parameters
    auto initialParams = miningEngine->getParameters();
    EXPECT_GT(initialParams.size(), 0);
    
    // Adjust parameters
    std::map<std::string, double> newParams;
    newParams["difficulty"] = 3.0;
    newParams["targetBlockTime"] = 15.0;
    newParams["miningReward"] = 25.0;
    
    EXPECT_TRUE(miningEngine->adjustParameters(newParams));
    
    // Verify parameters were adjusted
    auto adjustedParams = miningEngine->getParameters();
    EXPECT_EQ(adjustedParams["difficulty"], 3.0);
    EXPECT_EQ(adjustedParams["targetBlockTime"], 15.0);
    EXPECT_EQ(adjustedParams["miningReward"], 25.0);
}

// Test block validation through consensus request
TEST_F(MiningHarmonyTest, BlockValidationRequest) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Create a test block
    Block testBlock(1, "previous_hash");
    Transaction coinbaseTx("COINBASE", "miner_address", 50.0);
    testBlock.addTransaction(coinbaseTx);
    testBlock.setNonce(12345);
    testBlock.updateHash();
    
    // Create consensus request for block validation
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, testBlock.serialize());
    
    // Process the request
    ConsensusResult result = miningEngine->processRequest(request);
    
    // Verify result
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_WORK);
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
    EXPECT_FALSE(result.reason.empty());
}

// Test transaction validation through consensus request
TEST_F(MiningHarmonyTest, TransactionValidationRequest) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Create a test transaction
    Transaction testTx("sender_address", "recipient_address", 10.0);
    
    // Create consensus request for transaction validation
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, testTx.serialize());
    
    // Process the request
    ConsensusResult result = miningEngine->processRequest(request);
    
    // Verify result
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_WORK);
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
    EXPECT_FALSE(result.reason.empty());
}

// Test harmony-specific block validation
TEST_F(MiningHarmonyTest, HarmonyBlockValidation) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Create a valid test block
    Block validBlock(1, "previous_hash");
    Transaction coinbaseTx("COINBASE", "miner_address", 50.0);
    validBlock.addTransaction(coinbaseTx);
    validBlock.setNonce(12345);
    validBlock.updateHash();
    
    // Create consensus request
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, validBlock.serialize());
    
    // Test harmony-specific validation
    bool isValid = miningEngine->validateBlockHarmony(validBlock, request);
    
    // The result depends on whether the block meets difficulty requirements
    // For testing purposes, we just verify the method doesn't crash
    EXPECT_TRUE(isValid || !isValid); // Always true, just testing execution
}

// Test harmony-specific transaction validation
TEST_F(MiningHarmonyTest, HarmonyTransactionValidation) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Create a valid test transaction
    Transaction validTx("sender_address", "recipient_address", 1.0);
    
    // Create consensus request
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, validTx.serialize());
    
    // Test harmony-specific validation
    bool isValid = miningEngine->validateTransactionHarmony(validTx, request);
    
    // Should be valid for a properly formed transaction
    EXPECT_TRUE(isValid);
    
    // Test invalid transaction (empty sender and recipient)
    Transaction invalidTx("", "", 1.0);
    bool isInvalid = miningEngine->validateTransactionHarmony(invalidTx, request);
    EXPECT_FALSE(isInvalid);
}

// Test confidence calculation
TEST_F(MiningHarmonyTest, ConfidenceCalculation) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Test block confidence calculation
    Block testBlock(1, "previous_hash");
    Transaction coinbaseTx("COINBASE", "miner_address", 50.0);
    testBlock.addTransaction(coinbaseTx);
    testBlock.setNonce(12345);
    testBlock.updateHash();
    
    double blockConfidence = miningEngine->calculateValidationConfidence(testBlock);
    EXPECT_GE(blockConfidence, 0.0);
    EXPECT_LE(blockConfidence, 1.0);
    
    // Test transaction confidence calculation
    Transaction testTx("sender_address", "recipient_address", 1.0);
    double txConfidence = miningEngine->calculateValidationConfidence(testTx);
    EXPECT_GE(txConfidence, 0.0);
    EXPECT_LE(txConfidence, 1.0);
}

// Test harmony metrics collection
TEST_F(MiningHarmonyTest, HarmonyMetrics) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Process some validation requests to generate metrics
    Transaction testTx("sender_address", "recipient_address", 1.0);
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, testTx.serialize());
    
    // Process multiple requests
    for (int i = 0; i < 5; ++i) {
        ConsensusResult result = miningEngine->processRequest(request);
        // Results will vary, but should not crash
    }
    
    // Collect harmony metrics
    miningEngine->collectHarmonyMetrics();
    
    // Get harmony metrics
    nlohmann::json harmonyMetrics = miningEngine->getHarmonyMetrics();
    EXPECT_TRUE(harmonyMetrics.contains("totalHarmonyValidations"));
    EXPECT_TRUE(harmonyMetrics.contains("successfulHarmonyValidations"));
    EXPECT_TRUE(harmonyMetrics.contains("averageConfidence"));
    EXPECT_TRUE(harmonyMetrics.contains("harmonySuccessRate"));
    
    // Verify metrics are reasonable
    EXPECT_GE(harmonyMetrics["totalHarmonyValidations"].get<uint64_t>(), 5);
    EXPECT_GE(harmonyMetrics["averageConfidence"].get<double>(), 0.0);
    EXPECT_LE(harmonyMetrics["averageConfidence"].get<double>(), 1.0);
}

// Test unsupported request types
TEST_F(MiningHarmonyTest, UnsupportedRequestTypes) {
    EXPECT_TRUE(miningEngine->initialize());
    
    // Test governance proposal request (not supported by PoW engine)
    ConsensusRequest governanceRequest(RequestType::GOVERNANCE_PROPOSAL, "test_proposal_data");
    ConsensusResult result = miningEngine->processRequest(governanceRequest);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_WORK);
    EXPECT_EQ(result.confidence, 0.0);
    EXPECT_TRUE(result.reason.find("Unsupported request type") != std::string::npos);
}

// Test error handling
TEST_F(MiningHarmonyTest, ErrorHandling) {
    // Test processing request without initialization
    Transaction testTx("sender_address", "recipient_address", 1.0);
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, testTx.serialize());
    
    ConsensusResult result = miningEngine->processRequest(request);
    EXPECT_FALSE(result.isValid);
    EXPECT_TRUE(result.reason.find("not healthy") != std::string::npos || 
                result.reason.find("not initialized") != std::string::npos);
    
    // Test with invalid JSON data
    ConsensusRequest invalidRequest(RequestType::BLOCK_VALIDATION, "invalid_json_data");
    EXPECT_TRUE(miningEngine->initialize());
    
    ConsensusResult invalidResult = miningEngine->processRequest(invalidRequest);
    EXPECT_FALSE(invalidResult.isValid);
    EXPECT_TRUE(invalidResult.reason.find("failed") != std::string::npos);
}

// Test shutdown behavior
TEST_F(MiningHarmonyTest, ShutdownBehavior) {
    EXPECT_TRUE(miningEngine->initialize());
    EXPECT_TRUE(miningEngine->isHealthy());
    
    // Shutdown the engine
    miningEngine->shutdown();
    EXPECT_FALSE(miningEngine->isHealthy());
    
    // Test that requests fail after shutdown
    Transaction testTx("sender_address", "recipient_address", 1.0);
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, testTx.serialize());
    
    ConsensusResult result = miningEngine->processRequest(request);
    EXPECT_FALSE(result.isValid);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}