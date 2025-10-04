#include <iostream>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include "../include/core/consensus_balancer.h"

// Simple mock engine for testing
class SimpleTestEngine : public ConsensusEngine {
private:
    ConsensusType type_;
    std::string name_;
    bool healthy_;
    nlohmann::json metrics_;
    std::map<std::string, double> parameters_;

public:
    SimpleTestEngine(ConsensusType type, const std::string& name) 
        : type_(type), name_(name), healthy_(true) {
        // Initialize default metrics
        metrics_["totalValidations"] = 100;
        metrics_["successfulValidations"] = 90;
        metrics_["failedValidations"] = 10;
        metrics_["averageResponseTime"] = 1.5;
        metrics_["activeParticipants"] = 50;
        metrics_["networkHashRate"] = 1000.0;
        metrics_["totalStake"] = 50000.0;
        metrics_["resourceContribution"] = 500.0;
        
        // Initialize default parameters
        parameters_["difficulty"] = 4.0;
        parameters_["rewardMultiplier"] = 1.0;
    }
    
    bool validateBlock(const Block& block) override { return true; }
    bool validateTransaction(const Transaction& transaction) override { return true; }
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        return ConsensusResult(true, type_, 0.9, "Test validation");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy_; }
    ConsensusType getType() const override { return type_; }
    std::string getName() const override { return name_; }
    nlohmann::json getStatus() const override { return nlohmann::json{{"status", "healthy"}}; }
    nlohmann::json getMetrics() const override { return metrics_; }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override {
        for (const auto& [key, value] : parameters) {
            parameters_[key] = value;
        }
        return true;
    }
    
    std::map<std::string, double> getParameters() const override { return parameters_; }
    
    void setHealthy(bool healthy) { healthy_ = healthy; }
    void updateMetrics(const std::string& key, double value) { metrics_[key] = value; }
};

// Test functions
void testBasicConstruction() {
    std::cout << "Testing basic construction..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(!balancer.isInitialized());
    assert(!balancer.isRunning());
    assert(!balancer.isInEmergencyMode());
    
    std::cout << "✓ Basic construction test passed" << std::endl;
}

void testInitialization() {
    std::cout << "Testing initialization..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    assert(balancer.isInitialized());
    assert(balancer.isRunning());
    
    balancer.shutdown();
    assert(!balancer.isInitialized());
    assert(!balancer.isRunning());
    
    std::cout << "✓ Initialization test passed" << std::endl;
}

void testEngineRegistration() {
    std::cout << "Testing engine registration..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    // Test null engine registration
    assert(!balancer.registerEngine(ConsensusType::VOTING_CONSENSUS, nullptr));
    
    assert(balancer.unregisterEngine(ConsensusType::PROOF_OF_WORK));
    assert(!balancer.unregisterEngine(ConsensusType::VOTING_CONSENSUS)); // Not registered
    
    std::cout << "✓ Engine registration test passed" << std::endl;
}

void testBalancing() {
    std::cout << "Testing balancing functionality..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    // Test basic balancing
    balancer.balanceConsensusParticipation();
    
    // Test difficulty adjustment
    balancer.adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1);
    
    // Test reward adjustment
    balancer.adjustRewards(ConsensusType::PROOF_OF_WORK, 1.1);
    
    // Test automatic rebalancing
    assert(balancer.performAutomaticRebalancing());
    
    std::cout << "✓ Balancing functionality test passed" << std::endl;
}

void testMetrics() {
    std::cout << "Testing metrics functionality..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Trigger metrics collection
    balancer.balanceConsensusParticipation();
    
    // Test metrics retrieval
    BalanceMetrics metrics = balancer.getBalanceMetrics();
    assert(metrics.overallBalance >= 0.0 && metrics.overallBalance <= 1.0);
    assert(metrics.networkEfficiency >= 0.0 && metrics.networkEfficiency <= 1.0);
    
    ParticipationMetrics powMetrics = balancer.getParticipationMetrics(ConsensusType::PROOF_OF_WORK);
    assert(powMetrics.totalValidations > 0);
    
    // Test detailed analysis
    nlohmann::json analysis = balancer.getDetailedAnalysis();
    assert(analysis.contains("overallBalance"));
    assert(analysis.contains("networkEfficiency"));
    assert(analysis.contains("participationMetrics"));
    
    std::cout << "✓ Metrics functionality test passed" << std::endl;
}

void testConfiguration() {
    std::cout << "Testing configuration management..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    BalanceConfig config = balancer.getBalanceConfig();
    assert(config.maxDifficultyAdjustment == 0.25);
    
    // Update configuration
    config.maxDifficultyAdjustment = 0.3;
    config.rebalancingInterval = 1800;
    balancer.setBalanceConfig(config);
    
    BalanceConfig updatedConfig = balancer.getBalanceConfig();
    assert(updatedConfig.maxDifficultyAdjustment == 0.3);
    assert(updatedConfig.rebalancingInterval == 1800);
    
    // Test target participation update
    assert(balancer.updateTargetParticipation(ConsensusType::PROOF_OF_WORK, 0.4));
    assert(!balancer.updateTargetParticipation(ConsensusType::PROOF_OF_WORK, -0.1)); // Invalid
    
    std::cout << "✓ Configuration management test passed" << std::endl;
}

void testEmergencyMode() {
    std::cout << "Testing emergency mode..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    assert(!balancer.isInEmergencyMode());
    
    assert(balancer.enterEmergencyMode());
    assert(balancer.isInEmergencyMode());
    
    assert(balancer.exitEmergencyMode());
    assert(!balancer.isInEmergencyMode());
    
    std::cout << "✓ Emergency mode test passed" << std::endl;
}

void testStatistics() {
    std::cout << "Testing statistics..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    nlohmann::json stats = balancer.getStatistics();
    assert(stats.contains("totalRebalances"));
    assert(stats.contains("emergencyActivations"));
    assert(stats.contains("isInitialized"));
    assert(stats.contains("isRunning"));
    
    assert(stats["isInitialized"] == true);
    assert(stats["isRunning"] == true);
    assert(stats["totalRebalances"] == 0);
    
    balancer.resetStatistics();
    
    std::cout << "✓ Statistics test passed" << std::endl;
}

void testErrorHandling() {
    std::cout << "Testing error handling..." << std::endl;
    
    ConsensusBalancer balancer;
    
    // Test operations without initialization
    balancer.balanceConsensusParticipation(); // Should not crash
    balancer.adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1); // Should not crash
    assert(!balancer.performAutomaticRebalancing()); // Should return false
    
    // Test with unhealthy engine
    assert(balancer.initialize());
    auto unhealthyEngine = std::make_unique<SimpleTestEngine>(ConsensusType::VOTING_CONSENSUS, "Unhealthy");
    unhealthyEngine->setHealthy(false);
    assert(!balancer.registerEngine(ConsensusType::VOTING_CONSENSUS, unhealthyEngine.get()));
    
    std::cout << "✓ Error handling test passed" << std::endl;
}

void testPerformance() {
    std::cout << "Testing performance..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    const int numOperations = 100;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        balancer.balanceConsensusParticipation();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 100 operations in reasonable time (less than 5 seconds)
    assert(duration.count() < 5000);
    
    std::cout << "✓ Performance test passed (completed " << numOperations 
              << " operations in " << duration.count() << "ms)" << std::endl;
}

int main() {
    std::cout << "Running Consensus Balancer Tests..." << std::endl;
    std::cout << "===================================" << std::endl;
    
    try {
        testBasicConstruction();
        testInitialization();
        testEngineRegistration();
        testBalancing();
        testMetrics();
        testConfiguration();
        testEmergencyMode();
        testStatistics();
        testErrorHandling();
        testPerformance();
        
        std::cout << std::endl;
        std::cout << "===================================" << std::endl;
        std::cout << "All tests passed successfully! ✓" << std::endl;
        std::cout << "===================================" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << std::endl;
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << std::endl;
        std::cout << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}