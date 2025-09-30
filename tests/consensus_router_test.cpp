#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/core/consensus_router.h"
#include "../include/core/block.h"
#include "../include/core/transaction.h"
#include <memory>

// Mock ConsensusEngine for testing
class MockConsensusEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::string name;
    bool healthy;
    bool initialized;
    ConsensusResult mockResult;

public:
    MockConsensusEngine(ConsensusType t, const std::string& n, bool h = true)
        : type(t), name(n), healthy(h), initialized(false) {
        mockResult = ConsensusResult(true, type, 1.0, "Mock validation passed");
    }
    
    // Set mock result for testing
    void setMockResult(const ConsensusResult& result) {
        mockResult = result;
    }
    
    // ConsensusEngine interface implementation
    bool validateBlock(const Block& block) override {
        return mockResult.isValid;
    }
    
    bool validateTransaction(const Transaction& transaction) override {
        return mockResult.isValid;
    }
    
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        return mockResult;
    }
    
    bool initialize() override {
        initialized = true;
        return true;
    }
    
    void shutdown() override {
        initialized = false;
    }
    
    bool isHealthy() const override {
        return healthy && initialized;
    }
    
    ConsensusType getType() const override {
        return type;
    }
    
    std::string getName() const override {
        return name;
    }
    
    nlohmann::json getStatus() const override {
        nlohmann::json status;
        status["healthy"] = healthy;
        status["initialized"] = initialized;
        return status;
    }
    
    nlohmann::json getMetrics() const override {
        nlohmann::json metrics;
        metrics["validations"] = 0;
        return metrics;
    }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override {
        return true;
    }
    
    std::map<std::string, double> getParameters() const override {
        return {};
    }
    
    // Test helpers
    void setHealthy(bool h) { healthy = h; }
};

class ConsensusRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        router = std::make_unique<ConsensusRouter>();
        config = std::make_unique<ConsensusConfig>();
        
        // Create mock engines
        powEngine = std::make_unique<MockConsensusEngine>(ConsensusType::PROOF_OF_WORK, "MockPoW");
        posEngine = std::make_unique<MockConsensusEngine>(ConsensusType::PROOF_OF_STAKE, "MockPoS");
        porcEngine = std::make_unique<MockConsensusEngine>(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, "MockPoRC");
        votingEngine = std::make_unique<MockConsensusEngine>(ConsensusType::VOTING_CONSENSUS, "MockVoting");
        scEngine = std::make_unique<MockConsensusEngine>(ConsensusType::SMART_CONTRACT_VALIDATION, "MockSC");
    }
    
    void TearDown() override {
        if (router) {
            router->shutdown();
        }
    }
    
    std::unique_ptr<ConsensusRouter> router;
    std::unique_ptr<ConsensusConfig> config;
    std::unique_ptr<MockConsensusEngine> powEngine;
    std::unique_ptr<MockConsensusEngine> posEngine;
    std::unique_ptr<MockConsensusEngine> porcEngine;
    std::unique_ptr<MockConsensusEngine> votingEngine;
    std::unique_ptr<MockConsensusEngine> scEngine;
};

// Test initialization
TEST_F(ConsensusRouterTest, InitializationTest) {
    EXPECT_FALSE(router->isInitialized());
    
    EXPECT_TRUE(router->initialize(config.get()));
    EXPECT_TRUE(router->isInitialized());
    
    // Test double initialization
    EXPECT_TRUE(router->initialize(config.get()));
}

// Test engine registration
TEST_F(ConsensusRouterTest, EngineRegistrationTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Test registering engines
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    // Test getting registered engines
    std::vector<ConsensusType> registeredTypes = router->getRegisteredEngines();
    EXPECT_EQ(registeredTypes.size(), 2);
    EXPECT_TRUE(std::find(registeredTypes.begin(), registeredTypes.end(), 
                         ConsensusType::PROOF_OF_WORK) != registeredTypes.end());
    EXPECT_TRUE(std::find(registeredTypes.begin(), registeredTypes.end(), 
                         ConsensusType::PROOF_OF_STAKE) != registeredTypes.end());
    
    // Test getting specific engine
    ConsensusEngine* engine = router->getEngine(ConsensusType::PROOF_OF_WORK);
    EXPECT_NE(engine, nullptr);
    EXPECT_EQ(engine->getType(), ConsensusType::PROOF_OF_WORK);
    
    // Test unregistering engine
    EXPECT_TRUE(router->unregisterEngine(ConsensusType::PROOF_OF_WORK));
    registeredTypes = router->getRegisteredEngines();
    EXPECT_EQ(registeredTypes.size(), 1);
}

// Test null engine registration
TEST_F(ConsensusRouterTest, NullEngineRegistrationTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    EXPECT_FALSE(router->registerEngine(nullptr));
}

// Test routing rules
TEST_F(ConsensusRouterTest, RoutingRulesTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Test default routing rules
    std::vector<RoutingRule> rules = router->getRoutingRules();
    EXPECT_GT(rules.size(), 0);
    
    // Test adding custom routing rule
    RoutingRule customRule(RequestType::BLOCK_VALIDATION, 
                          {ConsensusType::PROOF_OF_WORK}, 
                          {ConsensusType::PROOF_OF_STAKE}, 
                          0.8, true);
    
    EXPECT_TRUE(router->addRoutingRule(customRule));
    
    // Test removing routing rule
    EXPECT_TRUE(router->removeRoutingRule(RequestType::BLOCK_VALIDATION));
    EXPECT_FALSE(router->removeRoutingRule(RequestType::BLOCK_VALIDATION)); // Should fail second time
}

// Test aggregation strategies
TEST_F(ConsensusRouterTest, AggregationStrategyTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Test setting and getting aggregation strategies
    EXPECT_TRUE(router->setAggregationStrategy(RequestType::BLOCK_VALIDATION, 
                                              AggregationStrategy::UNANIMOUS));
    
    AggregationStrategy strategy = router->getAggregationStrategy(RequestType::BLOCK_VALIDATION);
    EXPECT_EQ(strategy, AggregationStrategy::UNANIMOUS);
    
    // Test default strategy for unknown request type
    strategy = router->getAggregationStrategy(RequestType::PARAMETER_ADJUSTMENT);
    EXPECT_EQ(strategy, AggregationStrategy::HIERARCHICAL); // Default
}

// Test applicable engines determination
TEST_F(ConsensusRouterTest, ApplicableEnginesTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Register engines
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(porcEngine)));
    
    // Test block validation request
    ConsensusRequest blockRequest(RequestType::BLOCK_VALIDATION, "test_block_data");
    std::vector<ConsensusEngine*> engines = router->getApplicableEngines(blockRequest);
    
    // Should include PoW and PoS (required) and potentially PoRC (optional)
    EXPECT_GE(engines.size(), 2);
    
    // Test transaction validation request
    ConsensusRequest txRequest(RequestType::TRANSACTION_VALIDATION, "test_tx_data");
    engines = router->getApplicableEngines(txRequest);
    
    // Should include at least PoW (required)
    EXPECT_GE(engines.size(), 1);
}

// Test basic validation routing
TEST_F(ConsensusRouterTest, BasicValidationRoutingTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Register engines with positive results
    powEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW valid"));
    posEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS valid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    // Test block validation
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router->validateBlock(testBlock);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_GT(result.confidence, 0.0);
    
    // Test transaction validation
    Transaction testTx("sender", "recipient", 100.0);
    result = router->validateTransaction(testTx);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_GT(result.confidence, 0.0);
}

// Test conflict detection and resolution
TEST_F(ConsensusRouterTest, ConflictResolutionTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Set up conflicting results
    powEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW valid"));
    posEngine->setMockResult(ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS invalid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    // Test hierarchical resolution (PoW has higher priority)
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router->validateBlock(testBlock);
    
    // Should resolve to PoW result (higher priority)
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::PROOF_OF_WORK);
}

// Test unanimous aggregation
TEST_F(ConsensusRouterTest, UnanimousAggregationTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Set unanimous strategy for block validation
    EXPECT_TRUE(router->setAggregationStrategy(RequestType::BLOCK_VALIDATION, 
                                              AggregationStrategy::UNANIMOUS));
    
    // Set up agreeing results
    powEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW valid"));
    posEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS valid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router->validateBlock(testBlock);
    
    EXPECT_TRUE(result.isValid);
    
    // Now test with disagreeing results
    router->unregisterEngine(ConsensusType::PROOF_OF_STAKE);
    posEngine = std::make_unique<MockConsensusEngine>(ConsensusType::PROOF_OF_STAKE, "MockPoS");
    posEngine->setMockResult(ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS invalid"));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    result = router->validateBlock(testBlock);
    EXPECT_FALSE(result.isValid); // Should fail unanimous requirement
}

// Test majority aggregation
TEST_F(ConsensusRouterTest, MajorityAggregationTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Set majority strategy
    EXPECT_TRUE(router->setAggregationStrategy(RequestType::BLOCK_VALIDATION, 
                                              AggregationStrategy::MAJORITY));
    
    // Set up majority valid results (2 valid, 1 invalid)
    powEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW valid"));
    posEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS valid"));
    porcEngine->setMockResult(ConsensusResult(false, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.7, "PoRC invalid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(porcEngine)));
    
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router->validateBlock(testBlock);
    
    EXPECT_TRUE(result.isValid); // Majority should win
}

// Test most restrictive aggregation
TEST_F(ConsensusRouterTest, MostRestrictiveAggregationTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Set most restrictive strategy
    EXPECT_TRUE(router->setAggregationStrategy(RequestType::TRANSACTION_VALIDATION, 
                                              AggregationStrategy::MOST_RESTRICTIVE));
    
    // Set up mixed results
    powEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW valid"));
    posEngine->setMockResult(ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS invalid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    Transaction testTx("sender", "recipient", 100.0);
    ConsensusResult result = router->validateTransaction(testTx);
    
    EXPECT_FALSE(result.isValid); // Most restrictive should reject
}

// Test weighted majority aggregation
TEST_F(ConsensusRouterTest, WeightedMajorityAggregationTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Set weighted majority strategy
    EXPECT_TRUE(router->setAggregationStrategy(RequestType::GOVERNANCE_PROPOSAL, 
                                              AggregationStrategy::WEIGHTED_MAJORITY));
    
    // Set up weighted results (high confidence valid vs low confidence invalid)
    votingEngine->setMockResult(ConsensusResult(true, ConsensusType::VOTING_CONSENSUS, 0.9, "Voting valid"));
    posEngine->setMockResult(ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.3, "PoS invalid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(votingEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    ConsensusResult result = router->validateGovernanceProposal("test_proposal");
    
    EXPECT_TRUE(result.isValid); // Higher weighted confidence should win
}

// Test statistics
TEST_F(ConsensusRouterTest, StatisticsTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Register engines
    powEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW valid"));
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    
    // Perform some validations
    Block testBlock(1, "previous_hash");
    router->validateBlock(testBlock);
    
    Transaction testTx("sender", "recipient", 100.0);
    router->validateTransaction(testTx);
    
    // Check statistics
    nlohmann::json stats = router->getStatistics();
    EXPECT_GT(stats["totalRequests"].get<uint64_t>(), 0);
    EXPECT_GT(stats["successfulRoutes"].get<uint64_t>(), 0);
    
    // Test statistics reset
    router->resetStatistics();
    stats = router->getStatistics();
    EXPECT_EQ(stats["totalRequests"].get<uint64_t>(), 0);
}

// Test engine status
TEST_F(ConsensusRouterTest, EngineStatusTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    nlohmann::json status = router->getEngineStatus();
    EXPECT_TRUE(status.contains("PROOF_OF_WORK"));
    EXPECT_TRUE(status.contains("PROOF_OF_STAKE"));
    
    EXPECT_TRUE(status["PROOF_OF_WORK"]["healthy"].get<bool>());
    EXPECT_TRUE(status["PROOF_OF_STAKE"]["healthy"].get<bool>());
}

// Test unhealthy engine handling
TEST_F(ConsensusRouterTest, UnhealthyEngineTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Set one engine as unhealthy
    powEngine->setHealthy(false);
    posEngine->setMockResult(ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS valid"));
    
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    EXPECT_TRUE(router->registerEngine(std::move(posEngine)));
    
    // Test that unhealthy engines are not used
    ConsensusRequest blockRequest(RequestType::BLOCK_VALIDATION, "test_block_data");
    std::vector<ConsensusEngine*> engines = router->getApplicableEngines(blockRequest);
    
    // Should only include healthy engines
    for (ConsensusEngine* engine : engines) {
        EXPECT_TRUE(engine->isHealthy());
    }
}

// Test empty results handling
TEST_F(ConsensusRouterTest, EmptyResultsTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    // Don't register any engines
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router->validateBlock(testBlock);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.reason, "No applicable consensus engines");
}

// Test governance proposal validation
TEST_F(ConsensusRouterTest, GovernanceProposalTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    votingEngine->setMockResult(ConsensusResult(true, ConsensusType::VOTING_CONSENSUS, 0.85, "Proposal approved"));
    EXPECT_TRUE(router->registerEngine(std::move(votingEngine)));
    
    ConsensusResult result = router->validateGovernanceProposal("increase_block_size");
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::VOTING_CONSENSUS);
}

// Test smart contract execution validation
TEST_F(ConsensusRouterTest, SmartContractExecutionTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    
    scEngine->setMockResult(ConsensusResult(true, ConsensusType::SMART_CONTRACT_VALIDATION, 0.95, "Contract executed"));
    EXPECT_TRUE(router->registerEngine(std::move(scEngine)));
    
    ConsensusResult result = router->validateSmartContractExecution("contract_bytecode");
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.mechanism, ConsensusType::SMART_CONTRACT_VALIDATION);
}

// Test shutdown behavior
TEST_F(ConsensusRouterTest, ShutdownTest) {
    EXPECT_TRUE(router->initialize(config.get()));
    EXPECT_TRUE(router->registerEngine(std::move(powEngine)));
    
    EXPECT_TRUE(router->isInitialized());
    
    router->shutdown();
    EXPECT_FALSE(router->isInitialized());
    
    // Test that operations fail after shutdown
    std::vector<ConsensusType> engines = router->getRegisteredEngines();
    EXPECT_EQ(engines.size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}