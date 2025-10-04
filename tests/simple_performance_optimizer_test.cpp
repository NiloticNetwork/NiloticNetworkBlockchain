#include "../include/core/consensus_performance_optimizer.h"
#include "../include/core/consensus_harmony.h"
#include "../include/core/logger.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <cassert>

// Simple mock consensus engine for testing without blockchain dependencies
class SimpleTestEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::string name;
    bool healthy;
    std::chrono::milliseconds processingDelay;
    double successRate;
    
public:
    SimpleTestEngine(ConsensusType t, const std::string& n, 
                    std::chrono::milliseconds delay = std::chrono::milliseconds(100),
                    double success = 1.0)
        : type(t), name(n), healthy(true), processingDelay(delay), successRate(success) {}
    
    bool validateBlock(const Block& /* block */) override {
        std::this_thread::sleep_for(processingDelay);
        return (rand() / double(RAND_MAX)) < successRate;
    }
    
    bool validateTransaction(const Transaction& /* transaction */) override {
        std::this_thread::sleep_for(processingDelay);
        return (rand() / double(RAND_MAX)) < successRate;
    }
    
    ConsensusResult processRequest(const ConsensusRequest& /* request */) override {
        std::this_thread::sleep_for(processingDelay);
        
        bool isValid = (rand() / double(RAND_MAX)) < successRate;
        double confidence = isValid ? 0.8 + (rand() / double(RAND_MAX)) * 0.2 : 0.1 + (rand() / double(RAND_MAX)) * 0.3;
        
        return ConsensusResult(isValid, type, confidence, 
                              isValid ? "Test validation passed" : "Test validation failed");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy; }
    
    ConsensusType getType() const override { return type; }
    std::string getName() const override { return name; }
    
    nlohmann::json getStatus() const override {
        return nlohmann::json{{"healthy", healthy}, {"name", name}};
    }
    
    nlohmann::json getMetrics() const override {
        return nlohmann::json{{"processingDelay", processingDelay.count()}, {"successRate", successRate}};
    }
    
    bool adjustParameters(const std::map<std::string, double>& /* parameters */) override { return true; }
    std::map<std::string, double> getParameters() const override { return {}; }
    
    void setHealthy(bool h) { healthy = h; }
    void setProcessingDelay(std::chrono::milliseconds delay) { processingDelay = delay; }
    void setSuccessRate(double rate) { successRate = rate; }
};

// Test helper functions
std::vector<ConsensusRequest> generateSimpleTestRequests(size_t count) {
    std::vector<ConsensusRequest> requests;
    requests.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> typeDist(0, 4);
    
    for (size_t i = 0; i < count; ++i) {
        RequestType type = static_cast<RequestType>(typeDist(gen));
        std::string data = "simple_test_data_" + std::to_string(i);
        requests.emplace_back(type, data);
    }
    
    return requests;
}

std::vector<std::unique_ptr<SimpleTestEngine>> createSimpleTestEngines() {
    std::vector<std::unique_ptr<SimpleTestEngine>> engines;
    
    engines.push_back(std::make_unique<SimpleTestEngine>(
        ConsensusType::PROOF_OF_WORK, "SimplePoW", std::chrono::milliseconds(50), 0.9));
    engines.push_back(std::make_unique<SimpleTestEngine>(
        ConsensusType::PROOF_OF_STAKE, "SimplePoS", std::chrono::milliseconds(30), 0.95));
    engines.push_back(std::make_unique<SimpleTestEngine>(
        ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, "SimplePoRC", std::chrono::milliseconds(80), 0.85));
    
    return engines;
}

// Test functions
bool testBasicFunctionality() {
    std::cout << "Testing basic functionality..." << std::endl;
    
    ConsensusPerformanceOptimizer optimizer;
    
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    if (!optimizer.isInitialized()) {
        std::cout << "ERROR: Optimizer should be initialized" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Basic functionality test passed" << std::endl;
    return true;
}

bool testCaching() {
    std::cout << "Testing caching..." << std::endl;
    
    OptimizationConfig config;
    config.enableResultCaching = true;
    config.maxCacheSize = 10;
    
    ConsensusPerformanceOptimizer optimizer(config);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    // Test cache operations
    std::string testKey = "test_cache_key";
    ConsensusResult testResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "Cached result");
    
    ConsensusResult retrievedResult;
    if (optimizer.getCachedResult(testKey, retrievedResult)) {
        std::cout << "ERROR: Should not find result in empty cache" << std::endl;
        return false;
    }
    
    optimizer.cacheResult(testKey, testResult);
    
    if (!optimizer.getCachedResult(testKey, retrievedResult)) {
        std::cout << "ERROR: Should find cached result" << std::endl;
        return false;
    }
    
    if (retrievedResult.isValid != testResult.isValid) {
        std::cout << "ERROR: Cached result doesn't match" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Caching test passed" << std::endl;
    return true;
}

bool testParallelProcessing() {
    std::cout << "Testing parallel processing..." << std::endl;
    
    OptimizationConfig config;
    config.enableParallelValidation = true;
    config.maxWorkerThreads = 4;
    config.minParallelEngines = 2;
    
    ConsensusPerformanceOptimizer optimizer(config);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    auto testEngines = createSimpleTestEngines();
    std::vector<ConsensusEngine*> engines;
    for (auto& engine : testEngines) {
        engines.push_back(engine.get());
    }
    
    ConsensusRequest testRequest(RequestType::BLOCK_VALIDATION, "parallel_test_data");
    
    // Test parallel validation
    auto startTime = std::chrono::steady_clock::now();
    auto results = optimizer.parallelValidation(testRequest, engines);
    auto endTime = std::chrono::steady_clock::now();
    
    auto parallelTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (results.size() != engines.size()) {
        std::cout << "ERROR: Expected " << engines.size() << " results, got " << results.size() << std::endl;
        return false;
    }
    
    std::cout << "Parallel validation completed in " << parallelTime << "ms" << std::endl;
    
    optimizer.shutdown();
    
    std::cout << "✓ Parallel processing test passed" << std::endl;
    return true;
}

bool testPerformanceMetrics() {
    std::cout << "Testing performance metrics..." << std::endl;
    
    ConsensusPerformanceOptimizer optimizer;
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    // Perform some operations
    auto testEngines = createSimpleTestEngines();
    std::vector<ConsensusEngine*> engines = {testEngines[0].get()};
    
    auto testRequests = generateSimpleTestRequests(3);
    
    for (const auto& request : testRequests) {
        optimizer.optimizedValidation(request, engines);
    }
    
    // Check metrics
    auto metrics = optimizer.getPerformanceMetrics();
    
    if (metrics.totalValidations == 0) {
        std::cout << "ERROR: No validations recorded" << std::endl;
        return false;
    }
    
    std::cout << "Total validations: " << metrics.totalValidations << std::endl;
    std::cout << "Average validation time: " << metrics.averageValidationTime << "ms" << std::endl;
    
    optimizer.shutdown();
    
    std::cout << "✓ Performance metrics test passed" << std::endl;
    return true;
}

bool testOptimizationReport() {
    std::cout << "Testing optimization report..." << std::endl;
    
    ConsensusPerformanceOptimizer optimizer;
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    auto report = optimizer.getOptimizationReport();
    
    if (!report.contains("configuration") || !report.contains("metrics") || !report.contains("cache")) {
        std::cout << "ERROR: Optimization report incomplete" << std::endl;
        return false;
    }
    
    std::cout << "Optimization report generated successfully" << std::endl;
    
    optimizer.shutdown();
    
    std::cout << "✓ Optimization report test passed" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Simple Consensus Performance Optimizer Tests ===" << std::endl;
    
    // Initialize logger
    // Logger::setLevel(Logger::Level::INFO); // Commented out for test build
    
    int passed = 0;
    int total = 0;
    
    // Run tests
    std::vector<std::pair<std::string, std::function<bool()>>> tests = {
        {"Basic Functionality", testBasicFunctionality},
        {"Caching", testCaching},
        {"Parallel Processing", testParallelProcessing},
        {"Performance Metrics", testPerformanceMetrics},
        {"Optimization Report", testOptimizationReport}
    };
    
    for (const auto& [testName, testFunc] : tests) {
        total++;
        std::cout << "\n--- " << testName << " ---" << std::endl;
        
        try {
            if (testFunc()) {
                passed++;
                std::cout << "✓ " << testName << " PASSED" << std::endl;
            } else {
                std::cout << "✗ " << testName << " FAILED" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ " << testName << " FAILED with exception: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    std::cout << "Success Rate: " << (100.0 * passed / total) << "%" << std::endl;
    
    if (passed == total) {
        std::cout << "🎉 All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some tests failed!" << std::endl;
        return 1;
    }
}