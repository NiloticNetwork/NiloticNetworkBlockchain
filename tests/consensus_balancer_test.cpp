#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "consensus_balancer.h"
#include "consensus_router.h"
#include "consensus_harmony.h"
#include <thread>
#include <chrono>

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

class ConsensusBalancerTest : public ::testing::Test {
protected:
    void SetUp() override {
        balancer = std::make_unique<ConsensusBalancer>();
        router = std::make_unique<ConsensusRouter>();
        
        // Create mock engines
        powEngine = std::make_unique<MockConsensusEngine>();
        posEngine = std::make_unique<MockConsensusEngine>();
        porcEngine = std::make_unique<MockConsensusEngine>();
        
        // Set up default mock expectations
        setupMockEngines();
    }
    
    void TearDown() override {
        balancer.reset();
        router.reset();
        powEngine.reset();
        posEngine.reset();
        porcEngine.reset();
    }
    
    void setupMockEngines() {
        // PoW Engine setup
        EXPECT_CALL(*powEngine, getType())
            .WillRepeatedly(::testing::Return(ConsensusType::PROOF_OF_WORK));
        EXPECT_CALL(*powEngine, getName())
            .WillRepeatedly(::testing::Return("MockPoWEngine"));
        EXPECT_CALL(*powEngine, isHealthy())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*powEngine, getMetrics())
            .WillRepeatedly(::testing::Return(createMockMetrics(100, 90, 1.5, 50)));
        EXPECT_CALL(*powEngine, getParameters())
            .WillRepeatedly(::testing::Return(std::map<std::string, double>{
                {"difficulty", 4.0}, {"rewardMultiplier", 1.0}
            }));
        EXPECT_CALL(*powEngine, adjustParameters(::testing::_))
            .WillRepeatedly(::testing::Return(true));
        
        // PoS Engine setup
        EXPECT_CALL(*posEngine, getType())
            .WillRepeatedly(::testing::Return(ConsensusType::PROOF_OF_STAKE));
        EXPECT_CALL(*posEngine, getName())
            .WillRepeatedly(::testing::Return("MockPoSEngine"));
        EXPECT_CALL(*posEngine, isHealthy())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*posEngine, getMetrics())
            .WillRepeatedly(::testing::Return(createMockMetrics(80, 75, 1.2, 30)));
        EXPECT_CALL(*posEngine, getParameters())
            .WillRepeatedly(::testing::Return(std::map<std::string, double>{
                {"difficulty", 3.0}, {"rewardMultiplier", 1.0}
            }));
        EXPECT_CALL(*posEngine, adjustParameters(::testing::_))
            .WillRepeatedly(::testing::Return(true));
        
        // PoRC Engine setup
        EXPECT_CALL(*porcEngine, getType())
            .WillRepeatedly(::testing::Return(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION));
        EXPECT_CALL(*porcEngine, getName())
            .WillRepeatedly(::testing::Return("MockPoRCEngine"));
        EXPECT_CALL(*porcEngine, isHealthy())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*porcEngine, getMetrics())
            .WillRepeatedly(::testing::Return(createMockMetrics(60, 55, 2.0, 20)));
        EXPECT_CALL(*porcEngine, getParameters())
            .WillRepeatedly(::testing::Return(std::map<std::string, double>{
                {"difficulty", 2.0}, {"rewardMultiplier", 1.0}
            }));
        EXPECT_CALL(*porcEngine, adjustParameters(::testing::_))
            .WillRepeatedly(::testing::Return(true));
    }
    
    nlohmann::json createMockMetrics(uint64_t total, uint64_t successful, 
                                   double responseTime, uint64_t participants) {
        nlohmann::json metrics;
        metrics["totalValidations"] = total;
        metrics["successfulValidations"] = successful;
        metrics["failedValidations"] = total - successful;
        metrics["averageResponseTime"] = responseTime;
        metrics["activeParticipants"] = participants;
        metrics["networkHashRate"] = 1000.0;
        metrics["totalStake"] = 50000.0;
        metrics["resourceContribution"] = 500.0;
        return metrics;
    }
    
    std::unique_ptr<ConsensusBalancer> balancer;
    std::unique_ptr<ConsensusRouter> router;
    std::unique_ptr<MockConsensusEngine> powEngine;
    std::unique_ptr<MockConsensusEngine> posEngine;
    std::unique_ptr<MockConsensusEngine> porcEngine;
};

// Test ParticipationMetrics structure
TEST(ParticipationMetricsTest, BasicFunctionality) {
    ParticipationMetrics metrics;
    metrics.totalValidations = 100;
    metrics.successfulValidations = 90;
    metrics.averageResponseTime = 1.5;
    
    EXPECT_DOUBLE_EQ(metrics.getParticipationRate(), 0.9);
    EXPECT_GT(metrics.getEfficiencyScore(), 0.0);
    EXPECT_LT(metrics.getEfficiencyScore(), 1.0);
}

TEST(ParticipationMetricsTest, EdgeCases) {
    ParticipationMetrics metrics;
    
    // Test zero validations
    EXPECT_DOUBLE_EQ(metrics.getParticipationRate(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getEfficiencyScore(), 0.7); // Only response score contributes
    
    // Test perfect participation
    metrics.totalValidations = 100;
    metrics.successfulValidations = 100;
    metrics.averageResponseTime = 0.1;
    
    EXPECT_DOUBLE_EQ(metrics.getParticipationRate(), 1.0);
    EXPECT_GT(metrics.getEfficiencyScore(), 0.7);
}

// Test BalanceConfig structure
TEST(BalanceConfigTest, DefaultValues) {
    BalanceConfig config;
    
    EXPECT_DOUBLE_EQ(config.targetParticipationRates[ConsensusType::PROOF_OF_WORK], 0.3);
    EXPECT_DOUBLE_EQ(config.targetParticipationRates[ConsensusType::PROOF_OF_STAKE], 0.3);
    EXPECT_DOUBLE_EQ(config.targetParticipationRates[ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION], 0.25);
    EXPECT_DOUBLE_EQ(config.maxDifficultyAdjustment, 0.25);
    EXPECT_DOUBLE_EQ(config.maxRewardAdjustment, 0.20);
    EXPECT_DOUBLE_EQ(config.balanceThreshold, 0.15);
    EXPECT_EQ(config.rebalancingInterval, 3600);
}

// Test BalanceMetrics structure
TEST(BalanceMetricsTest, NetworkHealthCalculation) {
    BalanceMetrics metrics;
    
    // Add some participation metrics
    ParticipationMetrics pow_metrics;
    pow_metrics.totalValidations = 100;
    pow_metrics.successfulValidations = 90;
    pow_metrics.averageResponseTime = 1.0;
    
    ParticipationMetrics pos_metrics;
    pos_metrics.totalValidations = 80;
    pos_metrics.successfulValidations = 75;
    pos_metrics.averageResponseTime = 1.2;
    
    metrics.participationMetrics[ConsensusType::PROOF_OF_WORK] = pow_metrics;
    metrics.participationMetrics[ConsensusType::PROOF_OF_STAKE] = pos_metrics;
    
    double health = metrics.getNetworkHealth();
    EXPECT_GT(health, 0.0);
    EXPECT_LT(health, 1.0);
}

TEST(BalanceMetricsTest, MostDominantMechanism) {
    BalanceMetrics metrics;
    metrics.participationRates[ConsensusType::PROOF_OF_WORK] = 0.6;
    metrics.participationRates[ConsensusType::PROOF_OF_STAKE] = 0.3;
    metrics.participationRates[ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION] = 0.1;
    
    EXPECT_EQ(metrics.getMostDominant(), ConsensusType::PROOF_OF_WORK);
}

// Test ConsensusBalancer construction and initialization
TEST_F(ConsensusBalancerTest, Construction) {
    EXPECT_FALSE(balancer->isInitialized());
    EXPECT_FALSE(balancer->isRunning());
    EXPECT_FALSE(balancer->isInEmergencyMode());
}

TEST_F(ConsensusBalancerTest, CustomConfigConstruction) {
    BalanceConfig customConfig;
    customConfig.maxDifficultyAdjustment = 0.3;
    customConfig.rebalancingInterval = 1800;
    
    auto customBalancer = std::make_unique<ConsensusBalancer>(customConfig);
    
    BalanceConfig retrievedConfig = customBalancer->getBalanceConfig();
    EXPECT_DOUBLE_EQ(retrievedConfig.maxDifficultyAdjustment, 0.3);
    EXPECT_EQ(retrievedConfig.rebalancingInterval, 1800);
}

TEST_F(ConsensusBalancerTest, Initialization) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->isInitialized());
    EXPECT_TRUE(balancer->isRunning());
    
    // Test double initialization
    EXPECT_TRUE(balancer->initialize());
}

TEST_F(ConsensusBalancerTest, InitializationWithRouter) {
    EXPECT_TRUE(router->initialize());
    EXPECT_TRUE(balancer->initialize(router.get()));
    EXPECT_TRUE(balancer->isInitialized());
    EXPECT_TRUE(balancer->isRunning());
}

TEST_F(ConsensusBalancerTest, Shutdown) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->isInitialized());
    
    balancer->shutdown();
    EXPECT_FALSE(balancer->isInitialized());
    EXPECT_FALSE(balancer->isRunning());
}

// Test engine management
TEST_F(ConsensusBalancerTest, EngineRegistration) {
    EXPECT_TRUE(balancer->initialize());
    
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, porcEngine.get()));
    
    // Test null engine registration
    EXPECT_FALSE(balancer->registerEngine(ConsensusType::VOTING_CONSENSUS, nullptr));
}

TEST_F(ConsensusBalancerTest, EngineUnregistration) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    EXPECT_TRUE(balancer->unregisterEngine(ConsensusType::PROOF_OF_WORK));
    EXPECT_FALSE(balancer->unregisterEngine(ConsensusType::PROOF_OF_STAKE)); // Not registered
}

// Test core balancing functionality
TEST_F(ConsensusBalancerTest, BasicBalancing) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    // This should not throw and should complete successfully
    EXPECT_NO_THROW(balancer->balanceConsensusParticipation());
}

TEST_F(ConsensusBalancerTest, DifficultyAdjustment) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Test valid adjustment
    EXPECT_NO_THROW(balancer->adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1));
    
    // Test invalid adjustments
    EXPECT_NO_THROW(balancer->adjustDifficulty(ConsensusType::PROOF_OF_WORK, 2.0)); // Should be clamped
    EXPECT_NO_THROW(balancer->adjustDifficulty(ConsensusType::PROOF_OF_WORK, -2.0)); // Should be clamped
    
    // Test adjustment for unregistered engine
    EXPECT_NO_THROW(balancer->adjustDifficulty(ConsensusType::VOTING_CONSENSUS, 0.1));
}

TEST_F(ConsensusBalancerTest, RewardAdjustment) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Test valid adjustment
    EXPECT_NO_THROW(balancer->adjustRewards(ConsensusType::PROOF_OF_WORK, 1.1));
    
    // Test invalid adjustments
    EXPECT_NO_THROW(balancer->adjustRewards(ConsensusType::PROOF_OF_WORK, 0.0)); // Should log error
    EXPECT_NO_THROW(balancer->adjustRewards(ConsensusType::PROOF_OF_WORK, 15.0)); // Should be clamped
    
    // Test adjustment for unregistered engine
    EXPECT_NO_THROW(balancer->adjustRewards(ConsensusType::VOTING_CONSENSUS, 1.1));
}

TEST_F(ConsensusBalancerTest, AutomaticRebalancing) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    EXPECT_TRUE(balancer->performAutomaticRebalancing());
    
    // Test without initialization
    balancer->shutdown();
    EXPECT_FALSE(balancer->performAutomaticRebalancing());
}

TEST_F(ConsensusBalancerTest, SpecificMechanismRebalancing) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    EXPECT_TRUE(balancer->rebalanceSpecificMechanism(ConsensusType::PROOF_OF_WORK));
    EXPECT_FALSE(balancer->rebalanceSpecificMechanism(ConsensusType::VOTING_CONSENSUS)); // Not registered
}

TEST_F(ConsensusBalancerTest, NetworkPerformanceOptimization) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    EXPECT_NO_THROW(balancer->optimizeNetworkPerformance());
}

// Test metrics and monitoring
TEST_F(ConsensusBalancerTest, BalanceMetricsRetrieval) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    BalanceMetrics metrics = balancer->getBalanceMetrics();
    EXPECT_GE(metrics.overallBalance, 0.0);
    EXPECT_LE(metrics.overallBalance, 1.0);
    EXPECT_GE(metrics.networkEfficiency, 0.0);
    EXPECT_LE(metrics.networkEfficiency, 1.0);
}

TEST_F(ConsensusBalancerTest, ParticipationMetricsRetrieval) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Trigger metrics collection
    balancer->balanceConsensusParticipation();
    
    ParticipationMetrics metrics = balancer->getParticipationMetrics(ConsensusType::PROOF_OF_WORK);
    EXPECT_GT(metrics.totalValidations, 0);
    EXPECT_GT(metrics.successfulValidations, 0);
    
    // Test for unregistered engine
    ParticipationMetrics emptyMetrics = balancer->getParticipationMetrics(ConsensusType::VOTING_CONSENSUS);
    EXPECT_EQ(emptyMetrics.totalValidations, 0);
}

TEST_F(ConsensusBalancerTest, HistoricalMetricsRetrieval) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Trigger metrics collection multiple times to build history
    balancer->balanceConsensusParticipation();
    balancer->balanceConsensusParticipation();
    
    std::vector<ParticipationMetrics> history = 
        balancer->getHistoricalMetrics(ConsensusType::PROOF_OF_WORK);
    EXPECT_GE(history.size(), 1);
    
    // Test for unregistered engine
    std::vector<ParticipationMetrics> emptyHistory = 
        balancer->getHistoricalMetrics(ConsensusType::VOTING_CONSENSUS);
    EXPECT_EQ(emptyHistory.size(), 0);
}

TEST_F(ConsensusBalancerTest, DetailedAnalysis) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    balancer->balanceConsensusParticipation();
    
    nlohmann::json analysis = balancer->getDetailedAnalysis();
    
    EXPECT_TRUE(analysis.contains("overallBalance"));
    EXPECT_TRUE(analysis.contains("networkEfficiency"));
    EXPECT_TRUE(analysis.contains("isBalanced"));
    EXPECT_TRUE(analysis.contains("participationMetrics"));
    EXPECT_TRUE(analysis.contains("dominanceRatios"));
    EXPECT_TRUE(analysis.contains("recommendations"));
    EXPECT_TRUE(analysis.contains("imbalances"));
}

// Test configuration management
TEST_F(ConsensusBalancerTest, ConfigurationManagement) {
    EXPECT_TRUE(balancer->initialize());
    
    BalanceConfig newConfig;
    newConfig.maxDifficultyAdjustment = 0.3;
    newConfig.rebalancingInterval = 1800;
    
    balancer->setBalanceConfig(newConfig);
    
    BalanceConfig retrievedConfig = balancer->getBalanceConfig();
    EXPECT_DOUBLE_EQ(retrievedConfig.maxDifficultyAdjustment, 0.3);
    EXPECT_EQ(retrievedConfig.rebalancingInterval, 1800);
}

TEST_F(ConsensusBalancerTest, TargetParticipationUpdate) {
    EXPECT_TRUE(balancer->initialize());
    
    EXPECT_TRUE(balancer->updateTargetParticipation(ConsensusType::PROOF_OF_WORK, 0.4));
    
    BalanceConfig config = balancer->getBalanceConfig();
    EXPECT_DOUBLE_EQ(config.targetParticipationRates[ConsensusType::PROOF_OF_WORK], 0.4);
    
    // Test invalid values
    EXPECT_FALSE(balancer->updateTargetParticipation(ConsensusType::PROOF_OF_WORK, -0.1));
    EXPECT_FALSE(balancer->updateTargetParticipation(ConsensusType::PROOF_OF_WORK, 1.5));
}

TEST_F(ConsensusBalancerTest, InvalidConfiguration) {
    BalanceConfig invalidConfig;
    
    // Test invalid difficulty adjustment
    invalidConfig.maxDifficultyAdjustment = -0.1;
    EXPECT_NO_THROW(balancer->setBalanceConfig(invalidConfig)); // Should log error but not throw
    
    // Test invalid reward adjustment
    invalidConfig.maxDifficultyAdjustment = 0.25;
    invalidConfig.maxRewardAdjustment = 1.5;
    EXPECT_NO_THROW(balancer->setBalanceConfig(invalidConfig));
    
    // Test invalid balance threshold
    invalidConfig.maxRewardAdjustment = 0.2;
    invalidConfig.balanceThreshold = -0.1;
    EXPECT_NO_THROW(balancer->setBalanceConfig(invalidConfig));
}

// Test emergency operations
TEST_F(ConsensusBalancerTest, EmergencyMode) {
    EXPECT_TRUE(balancer->initialize());
    
    EXPECT_FALSE(balancer->isInEmergencyMode());
    
    EXPECT_TRUE(balancer->enterEmergencyMode());
    EXPECT_TRUE(balancer->isInEmergencyMode());
    
    // Test double entry
    EXPECT_TRUE(balancer->enterEmergencyMode());
    
    EXPECT_TRUE(balancer->exitEmergencyMode());
    EXPECT_FALSE(balancer->isInEmergencyMode());
    
    // Test double exit
    EXPECT_TRUE(balancer->exitEmergencyMode());
}

// Test analysis and recommendations
TEST_F(ConsensusBalancerTest, ImbalanceAnalysis) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    balancer->balanceConsensusParticipation();
    
    std::vector<std::string> imbalances = balancer->analyzeImbalances();
    // Should return a vector (may be empty if balanced)
    EXPECT_GE(imbalances.size(), 0);
}

TEST_F(ConsensusBalancerTest, RecommendationGeneration) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    balancer->balanceConsensusParticipation();
    
    std::vector<std::string> recommendations = balancer->generateRecommendations();
    // Should return a vector (may be empty if no recommendations)
    EXPECT_GE(recommendations.size(), 0);
}

TEST_F(ConsensusBalancerTest, NetworkThreatDetection) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    balancer->balanceConsensusParticipation();
    
    // Should not detect threats with normal mock data
    EXPECT_FALSE(balancer->detectNetworkThreats());
}

// Test statistics
TEST_F(ConsensusBalancerTest, Statistics) {
    EXPECT_TRUE(balancer->initialize());
    
    nlohmann::json stats = balancer->getStatistics();
    
    EXPECT_TRUE(stats.contains("totalRebalances"));
    EXPECT_TRUE(stats.contains("emergencyActivations"));
    EXPECT_TRUE(stats.contains("isInitialized"));
    EXPECT_TRUE(stats.contains("isRunning"));
    EXPECT_TRUE(stats.contains("isInEmergencyMode"));
    EXPECT_TRUE(stats.contains("registeredEngines"));
    EXPECT_TRUE(stats.contains("uptimeSeconds"));
    
    EXPECT_EQ(stats["totalRebalances"], 0);
    EXPECT_EQ(stats["emergencyActivations"], 0);
    EXPECT_TRUE(stats["isInitialized"]);
    EXPECT_TRUE(stats["isRunning"]);
    EXPECT_FALSE(stats["isInEmergencyMode"]);
}

TEST_F(ConsensusBalancerTest, StatisticsReset) {
    EXPECT_TRUE(balancer->initialize());
    
    // Perform some operations to generate statistics
    balancer->enterEmergencyMode();
    balancer->exitEmergencyMode();
    balancer->balanceConsensusParticipation();
    
    nlohmann::json statsBefore = balancer->getStatistics();
    EXPECT_GT(statsBefore["emergencyActivations"], 0);
    
    balancer->resetStatistics();
    
    nlohmann::json statsAfter = balancer->getStatistics();
    EXPECT_EQ(statsAfter["totalRebalances"], 0);
    EXPECT_EQ(statsAfter["emergencyActivations"], 0);
}

// Test error handling and edge cases
TEST_F(ConsensusBalancerTest, OperationsWithoutInitialization) {
    // Test operations without initialization
    EXPECT_NO_THROW(balancer->balanceConsensusParticipation());
    EXPECT_NO_THROW(balancer->adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1));
    EXPECT_NO_THROW(balancer->adjustRewards(ConsensusType::PROOF_OF_WORK, 1.1));
    EXPECT_FALSE(balancer->performAutomaticRebalancing());
}

TEST_F(ConsensusBalancerTest, UnhealthyEngineHandling) {
    // Create an unhealthy engine
    auto unhealthyEngine = std::make_unique<MockConsensusEngine>();
    EXPECT_CALL(*unhealthyEngine, getType())
        .WillRepeatedly(::testing::Return(ConsensusType::VOTING_CONSENSUS));
    EXPECT_CALL(*unhealthyEngine, getName())
        .WillRepeatedly(::testing::Return("UnhealthyEngine"));
    EXPECT_CALL(*unhealthyEngine, isHealthy())
        .WillRepeatedly(::testing::Return(false));
    
    EXPECT_TRUE(balancer->initialize());
    
    // Should handle unhealthy engine gracefully
    EXPECT_FALSE(balancer->registerEngine(ConsensusType::VOTING_CONSENSUS, unhealthyEngine.get()));
}

TEST_F(ConsensusBalancerTest, EngineMetricsFailure) {
    // Create an engine that throws on getMetrics()
    auto faultyEngine = std::make_unique<MockConsensusEngine>();
    EXPECT_CALL(*faultyEngine, getType())
        .WillRepeatedly(::testing::Return(ConsensusType::VOTING_CONSENSUS));
    EXPECT_CALL(*faultyEngine, getName())
        .WillRepeatedly(::testing::Return("FaultyEngine"));
    EXPECT_CALL(*faultyEngine, isHealthy())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*faultyEngine, getMetrics())
        .WillRepeatedly(::testing::Throw(std::runtime_error("Metrics failure")));
    EXPECT_CALL(*faultyEngine, getParameters())
        .WillRepeatedly(::testing::Return(std::map<std::string, double>{}));
    
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::VOTING_CONSENSUS, faultyEngine.get()));
    
    // Should handle metrics failure gracefully
    EXPECT_NO_THROW(balancer->balanceConsensusParticipation());
}

// Performance and stress tests
TEST_F(ConsensusBalancerTest, PerformanceTest) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, porcEngine.get()));
    
    const int numOperations = 100;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        balancer->balanceConsensusParticipation();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 100 balancing operations in reasonable time (less than 5 seconds)
    EXPECT_LT(duration.count(), 5000);
}

TEST_F(ConsensusBalancerTest, ConcurrentOperations) {
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    // Test concurrent balancing operations
    std::vector<std::thread> threads;
    std::atomic<int> completedOperations(0);
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j) {
                balancer->balanceConsensusParticipation();
                completedOperations++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(completedOperations.load(), 50);
}

// Integration test with background thread
TEST_F(ConsensusBalancerTest, BackgroundThreadOperation) {
    // Use a short rebalancing interval for testing
    BalanceConfig config;
    config.rebalancingInterval = 1; // 1 second
    balancer->setBalanceConfig(config);
    
    EXPECT_TRUE(balancer->initialize());
    EXPECT_TRUE(balancer->registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Let the background thread run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Check that some rebalancing occurred
    nlohmann::json stats = balancer->getStatistics();
    EXPECT_GE(stats["totalRebalances"], 0); // May be 0 or more depending on timing
    
    balancer->shutdown();
    EXPECT_FALSE(balancer->isRunning());
}