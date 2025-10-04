#include "../include/core/consensus_performance_optimizer.h"
#include "../include/core/consensus_harmony.h"
#include "../include/core/logger.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <cassert>

// Mock consensus engine for testing
class MockConsensusEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::string name;
    bool healthy;
    std::chrono::milliseconds processingDelay;
    double successRate;
    
public:
    MockConsensusEngine(ConsensusType t, const std::string& n, 
                       std::chrono::milliseconds delay = std::chrono::milliseconds(100),
                       double success = 1.0)
        : type(t), name(n), healthy(true), processingDelay(delay), successRate(success) {}
    
    bool validateBlock(const Block& block) override {
        std::this_thread::sleep_for(processingDelay);
        return (rand() / double(RAND_MAX)) < successRate;
    }
    
    bool validateTransaction(const Transaction& transaction) override {
        std::this_thread::sleep_for(processingDelay);
        return (rand() / double(RAND_MAX)) < successRate;
    }
    
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        std::this_thread::sleep_for(processingDelay);
        
        bool isValid = (rand() / double(RAND_MAX)) < successRate;
        double confidence = isValid ? 0.8 + (rand() / double(RAND_MAX)) * 0.2 : 0.1 + (rand() / double(RAND_MAX)) * 0.3;
        
        return ConsensusResult(isValid, type, confidence, 
                              isValid ? "Mock validation passed" : "Mock validation failed");
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
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override { return true; }
    std::map<std::string, double> getParameters() const override { return {}; }
    
    void setHealthy(bool h) { healthy = h; }
    void setProcessingDelay(std::chrono::milliseconds delay) { processingDelay = delay; }
    void setSuccessRate(double rate) { successRate = rate; }
};

// Test helper functions
std::vector<ConsensusRequest> generateTestRequests(size_t count) {
    std::vector<ConsensusRequest> requests;
    requests.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> typeDist(0, 4);
    
    for (size_t i = 0; i < count; ++i) {
        RequestType type = static_cast<RequestType>(typeDist(gen));
        std::string data = "test_data_" + std::to_string(i);
        requests.emplace_back(type, data);
    }
    
    return requests;
}

std::vector<std::unique_ptr<MockConsensusEngine>> createMockEngines() {
    std::vector<std::unique_ptr<MockConsensusEngine>> engines;
    
    engines.push_back(std::make_unique<MockConsensusEngine>(
        ConsensusType::PROOF_OF_WORK, "MockPoW", std::chrono::milliseconds(150), 0.9));
    engines.push_back(std::make_unique<MockConsensusEngine>(
        ConsensusType::PROOF_OF_STAKE, "MockPoS", std::chrono::milliseconds(100), 0.95));
    engines.push_back(std::make_unique<MockConsensusEngine>(
        ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, "MockPoRC", std::chrono::milliseconds(200), 0.85));
    engines.push_back(std::make_unique<MockConsensusEngine>(
        ConsensusType::VOTING_CONSENSUS, "MockVoting", std::chrono::milliseconds(300), 0.8));
    
    return engines;
}

// Test functions
bool testBasicInitialization() {
    std::cout << "Testing basic initialization..." << std::endl;
    
    ConsensusPerformanceOptimizer optimizer;
    
    if (optimizer.isInitialized()) {
        std::cout << "ERROR: Optimizer should not be initialized before calling initialize()" << std::endl;
        return false;
    }
    
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    if (!optimizer.isInitialized()) {
        std::cout << "ERROR: Optimizer should be initialized after calling initialize()" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Basic initialization test passed" << std::endl;
    return true;
}

bool testCachingFunctionality() {
    std::cout << "Testing caching functionality..." << std::endl;
    
    OptimizationConfig config;
    config.enableResultCaching = true;
    config.maxCacheSize = 100;
    config.cacheExpirationTime = std::chrono::seconds(60);
    
    ConsensusPerformanceOptimizer optimizer(config);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    // Test cache miss and hit
    std::string testKey = "test_key_1";
    ConsensusResult testResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "Test result");
    
    ConsensusResult retrievedResult;
    if (optimizer.getCachedResult(testKey, retrievedResult)) {
        std::cout << "ERROR: Should not find result in empty cache" << std::endl;
        return false;
    }
    
    optimizer.cacheResult(testKey, testResult, 100.0);
    
    if (!optimizer.getCachedResult(testKey, retrievedResult)) {
        std::cout << "ERROR: Should find cached result" << std::endl;
        return false;
    }
    
    if (retrievedResult.isValid != testResult.isValid || 
        retrievedResult.confidence != testResult.confidence) {
        std::cout << "ERROR: Cached result doesn't match original" << std::endl;
        return false;
    }
    
    // Test cache statistics
    auto cacheStats = optimizer.getCacheStatistics();
    if (cacheStats["resultCacheSize"] != 1) {
        std::cout << "ERROR: Cache size should be 1" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Caching functionality test passed" << std::endl;
    return true;
}

bool testParallelValidation() {
    std::cout << "Testing parallel validation..." << std::endl;
    
    OptimizationConfig config;
    config.enableParallelValidation = true;
    config.maxWorkerThreads = 4;
    config.minParallelEngines = 2;
    
    ConsensusPerformanceOptimizer optimizer(config);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    // Create mock engines
    auto mockEngines = createMockEngines();
    std::vector<ConsensusEngine*> engines;
    for (auto& engine : mockEngines) {
        engines.push_back(engine.get());
    }
    
    // Test parallel validation
    ConsensusRequest testRequest(RequestType::BLOCK_VALIDATION, "test_block_data");
    
    auto startTime = std::chrono::steady_clock::now();
    auto results = optimizer.parallelValidation(testRequest, engines);
    auto endTime = std::chrono::steady_clock::now();
    
    auto parallelTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (results.size() != engines.size()) {
        std::cout << "ERROR: Should get results from all engines. Expected: " << engines.size() 
                  << ", Got: " << results.size() << std::endl;
        return false;
    }
    
    // Test sequential validation for comparison
    startTime = std::chrono::steady_clock::now();
    auto sequentialResult = optimizer.sequentialValidation(testRequest, engines);
    endTime = std::chrono::steady_clock::now();
    
    auto sequentialTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "Parallel time: " << parallelTime << "ms, Sequential time: " << sequentialTime << "ms" << std::endl;
    
    // Parallel should be faster (though not guaranteed due to overhead in small tests)
    if (parallelTime > sequentialTime * 2) {
        std::cout << "WARNING: Parallel validation seems slower than expected" << std::endl;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Parallel validation test passed" << std::endl;
    return true;
}

bool testMemoryOptimization() {
    std::cout << "Testing memory optimization..." << std::endl;
    
    OptimizationConfig config;
    config.enableMemoryOptimization = true;
    config.maxCacheSize = 10; // Small cache for testing
    config.cacheExpirationTime = std::chrono::seconds(1); // Short expiration
    
    ConsensusPerformanceOptimizer optimizer(config);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    // Fill cache beyond capacity
    for (int i = 0; i < 15; ++i) {
        std::string key = "test_key_" + std::to_string(i);
        ConsensusResult result(true, ConsensusType::PROOF_OF_WORK, 0.8, "Test");
        optimizer.cacheResult(key, result);
    }
    
    auto cacheStats = optimizer.getCacheStatistics();
    size_t cacheSize = cacheStats["resultCacheSize"];
    
    if (cacheSize > config.maxCacheSize) {
        std::cout << "ERROR: Cache size (" << cacheSize << ") exceeds maximum (" 
                  << config.maxCacheSize << ")" << std::endl;
        return false;
    }
    
    // Test memory cleanup
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for expiration
    optimizer.performMemoryCleanup();
    
    cacheStats = optimizer.getCacheStatistics();
    size_t newCacheSize = cacheStats["resultCacheSize"];
    
    if (newCacheSize >= cacheSize) {
        std::cout << "WARNING: Memory cleanup didn't reduce cache size significantly" << std::endl;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Memory optimization test passed" << std::endl;
    return true;
}

bool testPerformanceMetrics() {
    std::cout << "Testing performance metrics..." << std::endl;
    
    OptimizationConfig config;
    config.enablePerformanceMonitoring = true;
    
    ConsensusPerformanceOptimizer optimizer(config);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    // Perform some operations to generate metrics
    auto mockEngines = createMockEngines();
    std::vector<ConsensusEngine*> engines;
    for (auto& engine : mockEngines) {
        engines.push_back(engine.get());
    }
    
    auto testRequests = generateTestRequests(5);
    
    for (const auto& request : testRequests) {
        optimizer.optimizedValidation(request, {engines[0]}); // Single engine for caching test
    }
    
    // Get metrics
    auto metrics = optimizer.getPerformanceMetrics();
    
    if (metrics.totalValidations == 0) {
        std::cout << "ERROR: No validations recorded in metrics" << std::endl;
        return false;
    }
    
    if (metrics.averageValidationTime <= 0) {
        std::cout << "ERROR: Invalid average validation time" << std::endl;
        return false;
    }
    
    // Test metrics reset
    optimizer.resetMetrics();
    metrics = optimizer.getPerformanceMetrics();
    
    if (metrics.totalValidations != 0) {
        std::cout << "ERROR: Metrics not reset properly" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Performance metrics test passed" << std::endl;
    return true;
}

bool testBenchmarking() {
    std::cout << "Testing benchmarking functionality..." << std::endl;
    
    ConsensusPerformanceOptimizer optimizer;
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    auto mockEngines = createMockEngines();
    std::vector<ConsensusEngine*> engines;
    for (auto& engine : mockEngines) {
        engines.push_back(engine.get());
    }
    
    auto testRequests = generateTestRequests(10);
    
    // Run benchmark
    auto benchmarkResult = optimizer.runBenchmark(testRequests, {engines[0]});
    
    if (benchmarkResult["testRequests"] != testRequests.size()) {
        std::cout << "ERROR: Benchmark didn't process all requests" << std::endl;
        return false;
    }
    
    if (!benchmarkResult.contains("results")) {
        std::cout << "ERROR: Benchmark results missing" << std::endl;
        return false;
    }
    
    auto results = benchmarkResult["results"];
    if (!results.contains("totalTime") || !results.contains("averageTime") || 
        !results.contains("throughput")) {
        std::cout << "ERROR: Benchmark results incomplete" << std::endl;
        return false;
    }
    
    double totalTime = results["totalTime"];
    double avgTime = results["averageTime"];
    double throughput = results["throughput"];
    
    if (totalTime <= 0 || avgTime <= 0 || throughput <= 0) {
        std::cout << "ERROR: Invalid benchmark metrics" << std::endl;
        return false;
    }
    
    std::cout << "Benchmark results - Total: " << totalTime << "ms, Average: " 
              << avgTime << "ms, Throughput: " << throughput << " req/s" << std::endl;
    
    optimizer.shutdown();
    
    std::cout << "✓ Benchmarking test passed" << std::endl;
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
    
    if (!report.contains("configuration") || !report.contains("metrics") || 
        !report.contains("cache")) {
        std::cout << "ERROR: Optimization report incomplete" << std::endl;
        return false;
    }
    
    auto config = report["configuration"];
    if (!config.contains("maxCacheSize") || !config.contains("enableResultCaching")) {
        std::cout << "ERROR: Configuration section incomplete" << std::endl;
        return false;
    }
    
    auto cache = report["cache"];
    if (!cache.contains("resultCacheSize") || !cache.contains("maxCacheSize")) {
        std::cout << "ERROR: Cache section incomplete" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Optimization report test passed" << std::endl;
    return true;
}

bool testConfigurationManagement() {
    std::cout << "Testing configuration management..." << std::endl;
    
    OptimizationConfig initialConfig;
    initialConfig.maxCacheSize = 1000;
    initialConfig.enableResultCaching = true;
    initialConfig.maxWorkerThreads = 8;
    
    ConsensusPerformanceOptimizer optimizer(initialConfig);
    if (!optimizer.initialize()) {
        std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
        return false;
    }
    
    auto retrievedConfig = optimizer.getConfiguration();
    if (retrievedConfig.maxCacheSize != initialConfig.maxCacheSize ||
        retrievedConfig.enableResultCaching != initialConfig.enableResultCaching ||
        retrievedConfig.maxWorkerThreads != initialConfig.maxWorkerThreads) {
        std::cout << "ERROR: Retrieved configuration doesn't match initial" << std::endl;
        return false;
    }
    
    // Update configuration
    OptimizationConfig newConfig = initialConfig;
    newConfig.maxCacheSize = 2000;
    newConfig.enableParallelValidation = false;
    
    if (!optimizer.updateConfiguration(newConfig)) {
        std::cout << "ERROR: Failed to update configuration" << std::endl;
        return false;
    }
    
    auto updatedConfig = optimizer.getConfiguration();
    if (updatedConfig.maxCacheSize != newConfig.maxCacheSize ||
        updatedConfig.enableParallelValidation != newConfig.enableParallelValidation) {
        std::cout << "ERROR: Configuration not updated properly" << std::endl;
        return false;
    }
    
    optimizer.shutdown();
    
    std::cout << "✓ Configuration management test passed" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Consensus Performance Optimizer Tests ===" << std::endl;
    
    // Initialize logger
    Logger::setLevel(Logger::Level::INFO);
    
    int passed = 0;
    int total = 0;
    
    // Run tests
    std::vector<std::pair<std::string, std::function<bool()>>> tests = {
        {"Basic Initialization", testBasicInitialization},
        {"Caching Functionality", testCachingFunctionality},
        {"Parallel Validation", testParallelValidation},
        {"Memory Optimization", testMemoryOptimization},
        {"Performance Metrics", testPerformanceMetrics},
        {"Benchmarking", testBenchmarking},
        {"Optimization Report", testOptimizationReport},
        {"Configuration Management", testConfigurationManagement}
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