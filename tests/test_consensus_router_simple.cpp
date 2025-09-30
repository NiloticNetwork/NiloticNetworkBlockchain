#include "../include/core/consensus_router.h"
#include "../include/core/block.h"
#include "../include/core/transaction.h"
#include <iostream>
#include <cassert>
#include <memory>

// Simple mock consensus engine for testing
class SimpleTestEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::string name;
    bool healthy;
    bool initialized;
    ConsensusResult mockResult;

public:
    SimpleTestEngine(ConsensusType t, const std::string& n, bool valid = true)
        : type(t), name(n), healthy(true), initialized(false) {
        mockResult = ConsensusResult(valid, type, 0.8, "Test result");
    }
    
    void setResult(bool valid, double confidence = 0.8) {
        mockResult = ConsensusResult(valid, type, confidence, "Test result");
    }
    
    // ConsensusEngine interface
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
        return nlohmann::json{{"validations", 0}};
    }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override {
        return true;
    }
    
    std::map<std::string, double> getParameters() const override {
        return {};
    }
};

void testBasicInitialization() {
    std::cout << "Testing basic initialization..." << std::endl;
    
    ConsensusRouter router;
    assert(!router.isInitialized());
    
    assert(router.initialize());
    assert(router.isInitialized());
    
    router.shutdown();
    assert(!router.isInitialized());
    
    std::cout << "✓ Basic initialization test passed" << std::endl;
}

void testEngineRegistration() {
    std::cout << "Testing engine registration..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    // Create and register engines
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(router.registerEngine(std::move(powEngine)));
    assert(router.registerEngine(std::move(posEngine)));
    
    // Check registered engines
    std::vector<ConsensusType> engines = router.getRegisteredEngines();
    assert(engines.size() == 2);
    
    // Test getting specific engine
    ConsensusEngine* engine = router.getEngine(ConsensusType::PROOF_OF_WORK);
    assert(engine != nullptr);
    assert(engine->getType() == ConsensusType::PROOF_OF_WORK);
    
    // Test unregistering
    assert(router.unregisterEngine(ConsensusType::PROOF_OF_WORK));
    engines = router.getRegisteredEngines();
    assert(engines.size() == 1);
    
    router.shutdown();
    std::cout << "✓ Engine registration test passed" << std::endl;
}

void testBasicValidation() {
    std::cout << "Testing basic validation..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    // Register engines with positive results
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW", true);
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS", true);
    
    assert(router.registerEngine(std::move(powEngine)));
    assert(router.registerEngine(std::move(posEngine)));
    
    // Test block validation
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router.validateBlock(testBlock);
    assert(result.isValid);
    assert(result.confidence > 0.0);
    
    // Test transaction validation
    Transaction testTx("sender", "recipient", 100.0);
    result = router.validateTransaction(testTx);
    assert(result.isValid);
    assert(result.confidence > 0.0);
    
    router.shutdown();
    std::cout << "✓ Basic validation test passed" << std::endl;
}

void testRoutingRules() {
    std::cout << "Testing routing rules..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    // Test default routing rules
    std::vector<RoutingRule> rules = router.getRoutingRules();
    assert(rules.size() > 0);
    
    // Test adding custom routing rule
    RoutingRule customRule(RequestType::BLOCK_VALIDATION, 
                          {ConsensusType::PROOF_OF_WORK}, 
                          {ConsensusType::PROOF_OF_STAKE}, 
                          0.8, true);
    
    assert(router.addRoutingRule(customRule));
    
    // Test removing routing rule
    assert(router.removeRoutingRule(RequestType::BLOCK_VALIDATION));
    assert(!router.removeRoutingRule(RequestType::BLOCK_VALIDATION)); // Should fail second time
    
    router.shutdown();
    std::cout << "✓ Routing rules test passed" << std::endl;
}

void testAggregationStrategies() {
    std::cout << "Testing aggregation strategies..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    // Test setting and getting aggregation strategies
    assert(router.setAggregationStrategy(RequestType::BLOCK_VALIDATION, 
                                        AggregationStrategy::UNANIMOUS));
    
    AggregationStrategy strategy = router.getAggregationStrategy(RequestType::BLOCK_VALIDATION);
    assert(strategy == AggregationStrategy::UNANIMOUS);
    
    // Test default strategy for unknown request type
    strategy = router.getAggregationStrategy(RequestType::PARAMETER_ADJUSTMENT);
    assert(strategy == AggregationStrategy::HIERARCHICAL); // Default
    
    router.shutdown();
    std::cout << "✓ Aggregation strategies test passed" << std::endl;
}

void testConflictResolution() {
    std::cout << "Testing conflict resolution..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    // Create engines with conflicting results
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW", true);
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS", false);
    
    powEngine->setResult(true, 0.9);
    posEngine->setResult(false, 0.8);
    
    assert(router.registerEngine(std::move(powEngine)));
    assert(router.registerEngine(std::move(posEngine)));
    
    // Test hierarchical resolution (PoW has higher priority)
    Block testBlock(1, "previous_hash");
    ConsensusResult result = router.validateBlock(testBlock);
    
    // Should resolve to PoW result (higher priority)
    assert(result.isValid);
    assert(result.mechanism == ConsensusType::PROOF_OF_WORK);
    
    router.shutdown();
    std::cout << "✓ Conflict resolution test passed" << std::endl;
}

void testStatistics() {
    std::cout << "Testing statistics..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    // Register engine
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW", true);
    assert(router.registerEngine(std::move(powEngine)));
    
    // Perform some validations
    Block testBlock(1, "previous_hash");
    router.validateBlock(testBlock);
    
    Transaction testTx("sender", "recipient", 100.0);
    router.validateTransaction(testTx);
    
    // Check statistics
    nlohmann::json stats = router.getStatistics();
    assert(stats["totalRequests"].get<uint64_t>() > 0);
    assert(stats["successfulRoutes"].get<uint64_t>() > 0);
    
    // Test statistics reset
    router.resetStatistics();
    stats = router.getStatistics();
    assert(stats["totalRequests"].get<uint64_t>() == 0);
    
    router.shutdown();
    std::cout << "✓ Statistics test passed" << std::endl;
}

void testEngineStatus() {
    std::cout << "Testing engine status..." << std::endl;
    
    ConsensusRouter router;
    assert(router.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(router.registerEngine(std::move(powEngine)));
    assert(router.registerEngine(std::move(posEngine)));
    
    nlohmann::json status = router.getEngineStatus();
    assert(status.contains("PROOF_OF_WORK"));
    assert(status.contains("PROOF_OF_STAKE"));
    
    assert(status["PROOF_OF_WORK"]["healthy"].get<bool>());
    assert(status["PROOF_OF_STAKE"]["healthy"].get<bool>());
    
    router.shutdown();
    std::cout << "✓ Engine status test passed" << std::endl;
}

int main() {
    std::cout << "Running ConsensusRouter tests..." << std::endl;
    std::cout << "=================================" << std::endl;
    
    try {
        testBasicInitialization();
        testEngineRegistration();
        testBasicValidation();
        testRoutingRules();
        testAggregationStrategies();
        testConflictResolution();
        testStatistics();
        testEngineStatus();
        
        std::cout << "=================================" << std::endl;
        std::cout << "✓ All ConsensusRouter tests passed!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "✗ Test failed with unknown exception" << std::endl;
        return 1;
    }
}