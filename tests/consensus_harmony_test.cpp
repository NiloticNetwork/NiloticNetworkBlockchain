#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "consensus_harmony.h"
#include "consensus_harmony_manager.h"
#include "block.h"
#include "transaction.h"
#include "blockchain.h"

// Mock ConsensusEngine for testing
class MockConsensusEngine : public ConsensusEngine {
public:
    MOCK_METHOD(bool, validateBlock, (const Block& block), (override));
    MOCK_METHOD(bool, validateTransaction, (const Transaction& transaction), (override));
    MOCK_METHOD(ConsensusResult, processRequest, (const ConsensusRequest& request), (override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(bool, isHealthy, (), (const, override));
    MOCK_METHOD(ConsensusType, getType, (), (const, override));
    MOCK_METHOD(std::string, getName, (), (const, override));
    MOCK_METHOD(nlohmann::json, getStatus, (), (const, override));
    MOCK_METHOD(nlohmann::json, getMetrics, (), (const, override));
    MOCK_METHOD(bool, adjustParameters, (const std::map<std::string, double>& parameters), (override));
    MOCK_METHOD(std::map<std::string, double>, getParameters, (), (const, override));
};

class ConsensusHarmonyTest : public ::testing::Test {
protected:
    void SetUp() override {
        blockchain = std::make_unique<Blockchain>();
        manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    }
    
    void TearDown() override {
        manager.reset();
        blockchain.reset();
    }
    
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> manager;
};

// Test ConsensusRequest structure
TEST(ConsensusRequestTest, BasicConstruction) {
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, "test_data");
    
    EXPECT_EQ(request.type, RequestType::BLOCK_VALIDATION);
    EXPECT_EQ(request.data, "test_data");
    EXPECT_FALSE(request.requestId.empty());
    EXPECT_GT(request.timestamp, 0);
}

TEST(ConsensusRequestTest, WithRequiredMechanisms) {
    std::vector<ConsensusType> required = {
        ConsensusType::PROOF_OF_WORK,
        ConsensusType::PROOF_OF_STAKE
    };
    
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, "tx_data", required);
    
    EXPECT_EQ(request.requiredMechanisms.size(), 2);
    EXPECT_EQ(request.requiredMechanisms[0], ConsensusType::PROOF_OF_WORK);
    EXPECT_EQ(request.requiredMechanisms[1], ConsensusType::PROOF_OF_STAKE);
}

// Test ConsensusResult structure
TEST(ConsensusResultTest, BasicConstruction) {
    ConsensusResult result(true, ConsensusType::PROOF_OF_WORK, 0.95, "Valid block");
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_WORK);
    EXPECT_DOUBLE_EQ(result.confidence, 0.95);
    EXPECT_EQ(result.reason, "Valid block");
    EXPECT_GT(result.timestamp, 0);
}

TEST(ConsensusResultTest, DefaultConstruction) {
    ConsensusResult result;
    
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_WORK);
    EXPECT_DOUBLE_EQ(result.confidence, 0.0);
    EXPECT_TRUE(result.reason.empty());
}

// Test ConsensusConfig structure
TEST(ConsensusConfigTest, DefaultValues) {
    ConsensusConfig config;
    
    EXPECT_EQ(config.powDifficulty, 4);
    EXPECT_EQ(config.powTargetBlockTime, 600);
    EXPECT_DOUBLE_EQ(config.minStakeAmount, 1000.0);
    EXPECT_EQ(config.stakingPeriod, 86400);
    EXPECT_DOUBLE_EQ(config.minResourceContribution, 100.0);
    EXPECT_DOUBLE_EQ(config.supermajorityThreshold, 0.67);
    EXPECT_EQ(config.votingPeriod, 604800);
    EXPECT_DOUBLE_EQ(config.maxDominanceRatio, 0.6);
    EXPECT_EQ(config.rebalancingInterval, 3600);
}

TEST(ConsensusConfigTest, JsonSerialization) {
    ConsensusConfig config;
    config.powDifficulty = 6;
    config.minStakeAmount = 2000.0;
    config.supermajorityThreshold = 0.75;
    
    nlohmann::json j = config.toJson();
    
    EXPECT_EQ(j["pow"]["difficulty"], 6);
    EXPECT_DOUBLE_EQ(j["pos"]["minStakeAmount"], 2000.0);
    EXPECT_DOUBLE_EQ(j["voting"]["supermajorityThreshold"], 0.75);
}

TEST(ConsensusConfigTest, JsonDeserialization) {
    nlohmann::json j;
    j["pow"]["difficulty"] = 8;
    j["pos"]["minStakeAmount"] = 1500.0;
    j["voting"]["supermajorityThreshold"] = 0.8;
    
    ConsensusConfig config;
    config.fromJson(j);
    
    EXPECT_EQ(config.powDifficulty, 8);
    EXPECT_DOUBLE_EQ(config.minStakeAmount, 1500.0);
    EXPECT_DOUBLE_EQ(config.supermajorityThreshold, 0.8);
}

// Test ConsensusHarmonyManager
TEST_F(ConsensusHarmonyTest, ManagerConstruction) {
    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
    EXPECT_EQ(manager->getBlockchain(), blockchain.get());
}

TEST_F(ConsensusHarmonyTest, ManagerInitialization) {
    EXPECT_TRUE(manager->initializeConsensus());
    EXPECT_TRUE(manager->isInitialized());
    EXPECT_TRUE(manager->isRunning());
    
    // Test double initialization
    EXPECT_TRUE(manager->initializeConsensus());
}

TEST_F(ConsensusHarmonyTest, ManagerInitializationWithCustomConfig) {
    ConsensusConfig config;
    config.powDifficulty = 6;
    config.minStakeAmount = 2000.0;
    
    EXPECT_TRUE(manager->initializeConsensus(config));
    
    ConsensusConfig retrievedConfig = manager->getConfiguration();
    EXPECT_EQ(retrievedConfig.powDifficulty, 6);
    EXPECT_DOUBLE_EQ(retrievedConfig.minStakeAmount, 2000.0);
}

TEST_F(ConsensusHarmonyTest, ManagerShutdown) {
    EXPECT_TRUE(manager->initializeConsensus());
    EXPECT_TRUE(manager->isInitialized());
    
    manager->shutdown();
    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
}

TEST_F(ConsensusHarmonyTest, BlockValidation) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    // Create a test block
    Block testBlock(1, "previous_hash");
    Transaction tx("sender", "recipient", 100.0);
    testBlock.addTransaction(tx);
    
    // Test block validation
    bool result = manager->validateBlock(testBlock);
    EXPECT_TRUE(result); // Should pass basic validation
}

TEST_F(ConsensusHarmonyTest, TransactionValidation) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    // Create a test transaction
    Transaction testTx("sender", "recipient", 50.0);
    
    // Test transaction validation
    bool result = manager->validateTransaction(testTx);
    EXPECT_TRUE(result); // Should pass basic validation
}

TEST_F(ConsensusHarmonyTest, ConsensusRequestProcessing) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, "test_block_data");
    ConsensusResult result = manager->processConsensusRequest(request);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_DOUBLE_EQ(result.confidence, 1.0);
    EXPECT_EQ(result.metadata["requestId"], request.requestId);
}

TEST_F(ConsensusHarmonyTest, StatusAndMetrics) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    ConsensusStatus status = manager->getConsensusStatus();
    EXPECT_EQ(status.totalValidations, 0);
    EXPECT_EQ(status.successfulValidations, 0);
    EXPECT_EQ(status.conflictCount, 0);
    
    // Perform some validations
    Block testBlock(1, "hash");
    manager->validateBlock(testBlock);
    
    Transaction testTx("sender", "recipient", 100.0);
    manager->validateTransaction(testTx);
    
    // Check updated status
    status = manager->getConsensusStatus();
    EXPECT_EQ(status.totalValidations, 2);
    EXPECT_EQ(status.successfulValidations, 2);
    
    // Test detailed status
    nlohmann::json detailedStatus = manager->getDetailedStatus();
    EXPECT_TRUE(detailedStatus["initialized"]);
    EXPECT_TRUE(detailedStatus["running"]);
    EXPECT_EQ(detailedStatus["totalValidations"], 2);
    
    // Test metrics
    nlohmann::json metrics = manager->getMetrics();
    EXPECT_DOUBLE_EQ(metrics["validationSuccessRate"], 1.0);
    EXPECT_DOUBLE_EQ(metrics["conflictRate"], 0.0);
}

TEST_F(ConsensusHarmonyTest, ConfigurationManagement) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    ConsensusConfig newConfig;
    newConfig.powDifficulty = 8;
    newConfig.minStakeAmount = 3000.0;
    
    EXPECT_TRUE(manager->updateConfiguration(newConfig));
    
    ConsensusConfig retrievedConfig = manager->getConfiguration();
    EXPECT_EQ(retrievedConfig.powDifficulty, 8);
    EXPECT_DOUBLE_EQ(retrievedConfig.minStakeAmount, 3000.0);
}

TEST_F(ConsensusHarmonyTest, ConfigurationPersistence) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    ConsensusConfig config;
    config.powDifficulty = 7;
    config.minStakeAmount = 2500.0;
    manager->updateConfiguration(config);
    
    // Save configuration
    std::string filename = "test_consensus_config.json";
    EXPECT_TRUE(manager->saveConfiguration(filename));
    
    // Create new manager and load configuration
    auto newManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    EXPECT_TRUE(newManager->loadConfiguration(filename));
    
    ConsensusConfig loadedConfig = newManager->getConfiguration();
    EXPECT_EQ(loadedConfig.powDifficulty, 7);
    EXPECT_DOUBLE_EQ(loadedConfig.minStakeAmount, 2500.0);
    
    // Clean up
    std::remove(filename.c_str());
}

TEST_F(ConsensusHarmonyTest, ValidationWithoutInitialization) {
    // Test validation without initialization
    Block testBlock(1, "hash");
    EXPECT_FALSE(manager->validateBlock(testBlock));
    
    Transaction testTx("sender", "recipient", 100.0);
    EXPECT_FALSE(manager->validateTransaction(testTx));
    
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, "data");
    ConsensusResult result = manager->processConsensusRequest(request);
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.reason, "Manager not initialized");
}

// Test invalid configurations
TEST_F(ConsensusHarmonyTest, InvalidConfiguration) {
    ConsensusConfig invalidConfig;
    
    // Test invalid difficulty
    invalidConfig.powDifficulty = 0;
    EXPECT_FALSE(manager->initializeConsensus(invalidConfig));
    
    // Test invalid stake amount
    invalidConfig.powDifficulty = 4;
    invalidConfig.minStakeAmount = -100.0;
    EXPECT_FALSE(manager->initializeConsensus(invalidConfig));
    
    // Test invalid supermajority threshold
    invalidConfig.minStakeAmount = 1000.0;
    invalidConfig.supermajorityThreshold = 0.4; // Too low
    EXPECT_FALSE(manager->initializeConsensus(invalidConfig));
    
    invalidConfig.supermajorityThreshold = 1.5; // Too high
    EXPECT_FALSE(manager->initializeConsensus(invalidConfig));
}

// Test consensus engine registration
TEST_F(ConsensusHarmonyTest, ConsensusEngineRegistration) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    auto mockEngine = std::make_unique<MockConsensusEngine>();
    
    // Set up mock expectations
    EXPECT_CALL(*mockEngine, getType())
        .WillRepeatedly(::testing::Return(ConsensusType::PROOF_OF_WORK));
    EXPECT_CALL(*mockEngine, getName())
        .WillRepeatedly(::testing::Return("MockPoWEngine"));
    EXPECT_CALL(*mockEngine, initialize())
        .WillOnce(::testing::Return(true));
    
    EXPECT_TRUE(manager->registerConsensusEngine(std::move(mockEngine)));
    
    // Test null engine registration
    EXPECT_FALSE(manager->registerConsensusEngine(nullptr));
}

TEST_F(ConsensusHarmonyTest, ConsensusEngineUnregistration) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    EXPECT_TRUE(manager->unregisterConsensusEngine(ConsensusType::PROOF_OF_WORK));
}

// Performance test
TEST_F(ConsensusHarmonyTest, PerformanceTest) {
    EXPECT_TRUE(manager->initializeConsensus());
    
    const int numValidations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numValidations; ++i) {
        Transaction tx("sender" + std::to_string(i), "recipient", 100.0);
        manager->validateTransaction(tx);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 1000 validations in reasonable time (less than 1 second)
    EXPECT_LT(duration.count(), 1000);
    
    ConsensusStatus status = manager->getConsensusStatus();
    EXPECT_EQ(status.totalValidations, numValidations);
    EXPECT_EQ(status.successfulValidations, numValidations);
}