#include "../include/core/consensus_performance_optimizer.h"
#include "../include/core/consensus_harmony.h"
#include "../include/core/logger.h"
#include <iostream>
#include <vector>
#include <chrono>

// Simple test engine
class QuickTestEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::string name;
    
public:
    QuickTestEngine(ConsensusType t, const std::string& n) : type(t), name(n) {}
    
    bool validateBlock(const Block& /* block */) override { return true; }
    bool validateTransaction(const Transaction& /* transaction */) override { return true; }
    
    ConsensusResult processRequest(const ConsensusRequest& /* request */) override {
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return ConsensusResult(true, type, 0.9, "Quick test passed");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return true; }
    
    ConsensusType getType() const override { return type; }
    std::string getName() const override { return name; }
    
    nlohmann::json getStatus() const override {
        return nlohmann::json{{"healthy", true}, {"name", name}};
    }
    
    nlohmann::json getMetrics() const override {
        return nlohmann::json{{"test", true}};
    }
    
    bool adjustParameters(const std::map<std::string, double>& /* parameters */) override { return true; }
    std::map<std::string, double> getParameters() const override { return {}; }
};

int main() {
    std::cout << "=== Quick Performance Optimizer Test ===" << std::endl;
    
    try {
        // Create optimizer with minimal configuration
        OptimizationConfig config;
        config.enablePerformanceMonitoring = false; // Disable background threads
        config.enableMemoryOptimization = false;
        config.enableResultCaching = true;
        config.enableParallelValidation = false; // Keep it simple
        
        ConsensusPerformanceOptimizer optimizer(config);
        
        std::cout << "Initializing optimizer..." << std::endl;
        if (!optimizer.initialize()) {
            std::cout << "ERROR: Failed to initialize optimizer" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Optimizer initialized successfully" << std::endl;
        
        // Test basic caching
        std::cout << "Testing caching..." << std::endl;
        std::string testKey = "quick_test_key";
        ConsensusResult testResult(true, ConsensusType::PROOF_OF_WORK, 0.95, "Cached test result");
        
        ConsensusResult retrievedResult;
        if (optimizer.getCachedResult(testKey, retrievedResult)) {
            std::cout << "ERROR: Should not find result in empty cache" << std::endl;
            return 1;
        }
        
        optimizer.cacheResult(testKey, testResult);
        
        if (!optimizer.getCachedResult(testKey, retrievedResult)) {
            std::cout << "ERROR: Should find cached result" << std::endl;
            return 1;
        }
        
        if (retrievedResult.isValid != testResult.isValid || 
            std::abs(retrievedResult.confidence - testResult.confidence) > 0.001) {
            std::cout << "ERROR: Cached result doesn't match original" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Caching test passed" << std::endl;
        
        // Test with a simple engine
        std::cout << "Testing with consensus engine..." << std::endl;
        QuickTestEngine engine(ConsensusType::PROOF_OF_WORK, "QuickTest");
        std::vector<ConsensusEngine*> engines = {&engine};
        
        ConsensusRequest request(RequestType::BLOCK_VALIDATION, "quick_test_data");
        
        auto startTime = std::chrono::steady_clock::now();
        ConsensusResult result = optimizer.optimizedValidation(request, engines);
        auto endTime = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        if (!result.isValid) {
            std::cout << "ERROR: Validation should have succeeded" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Validation completed in " << duration << "ms" << std::endl;
        
        // Test performance metrics
        std::cout << "Testing performance metrics..." << std::endl;
        auto metrics = optimizer.getPerformanceMetrics();
        
        if (metrics.totalValidations == 0) {
            std::cout << "ERROR: No validations recorded" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Metrics: " << metrics.totalValidations << " validations, " 
                  << metrics.averageValidationTime << "ms average" << std::endl;
        
        // Test optimization report
        std::cout << "Testing optimization report..." << std::endl;
        auto report = optimizer.getOptimizationReport();
        
        if (!report.contains("configuration") || !report.contains("metrics")) {
            std::cout << "ERROR: Optimization report incomplete" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Optimization report generated successfully" << std::endl;
        
        // Shutdown
        std::cout << "Shutting down optimizer..." << std::endl;
        optimizer.shutdown();
        
        std::cout << "✓ All tests passed!" << std::endl;
        std::cout << "🎉 Quick performance test completed successfully!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}