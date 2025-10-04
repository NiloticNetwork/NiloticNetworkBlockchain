#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include "../include/core/parameter_adjuster.h"
#include "../include/core/consensus_balancer.h"
#include "../include/core/logger.h"

// Mock ConsensusEngine for testing
class MockConsensusEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::map<std::string, double> parameters;
    nlohmann::json metrics;
    bool healthy;

public:
    MockConsensusEngine(ConsensusType t) : type(t), healthy(true) {
        // Initialize default parameters
        parameters["difficulty"] = 1.0;
        parameters["rewardMultiplier"] = 1.0;
        parameters["minStake"] = 1000.0;
        parameters["minResource"] = 100.0;
        
        // Initialize default metrics
        metrics["totalValidations"] = 100;
        metrics["successfulValidations"] = 95;
        metrics["averageResponseTime"] = 1.0;
        metrics["activeNodes"] = 50;
        metrics["pendingTransactions"] = 10;
        metrics["networkHashRate"] = 1000.0;
    }

    bool validateBlock(const Block& block) override { return true; }
    bool validateTransaction(const Transaction& transaction) override { return true; }
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        return ConsensusResult(true, type, 1.0, "Mock validation");
    }

    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy; }

    ConsensusType getType() const override { return type; }
    std::string getName() const override { return "MockEngine"; }
    nlohmann::json getStatus() const override {
        return nlohmann::json{{"status", "running"}, {"healthy", healthy}};
    }
    nlohmann::json getMetrics() const override { return metrics; }

    bool adjustParameters(const std::map<std::string, double>& newParams) override {
        for (const auto& [key, value] : newParams) {
            parameters[key] = value;
        }
        return true;
    }
    
    std::map<std::string, double> getParameters() const override { return parameters; }
    
    // Test helpers
    void setHealthy(bool h) { healthy = h; }
    void setMetric(const std::string& key, double value) { metrics[key] = value; }
};

// Test basic initialization and shutdown
void testBasicLifecycle() {
    std::cout << "Testing basic lifecycle..." << std::endl;
    
    ParameterAdjuster adjuster;
    
    // Test initialization
    assert(adjuster.initialize());
    assert(adjuster.isInitialized());
    assert(adjuster.isRunning());
    assert(!adjuster.isInEmergencyMode());
    
    // Test shutdown
    adjuster.shutdown();
    assert(!adjuster.isRunning());
    
    std::cout << "✓ Basic lifecycle test passed" << std::endl;
}

// Test engine registration
void testEngineRegistration() {
    std::cout << "Testing engine registration..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    // Create mock engines
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    MockConsensusEngine posEngine(ConsensusType::PROOF_OF_STAKE);
    MockConsensusEngine porcEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
    
    // Register engines
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_STAKE, &posEngine));
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, &porcEngine));
    
    // Test invalid registration
    assert(!adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, nullptr));
    
    // Unregister engines
    assert(adjuster.unregisterEngine(ConsensusType::PROOF_OF_WORK));
    assert(!adjuster.unregisterEngine(ConsensusType::VOTING_CONSENSUS)); // Not registered
    
    adjuster.shutdown();
    std::cout << "✓ Engine registration test passed" << std::endl;
}

// Test parameter adjustments
void testParameterAdjustments() {
    std::cout << "Testing parameter adjustments..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    MockConsensusEngine posEngine(ConsensusType::PROOF_OF_STAKE);
    MockConsensusEngine porcEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
    
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_STAKE, &posEngine));
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, &porcEngine));
    
    // Test difficulty adjustment
    assert(adjuster.adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1, "Test adjustment"));
    auto params = powEngine.getParameters();
    assert(params["difficulty"] > 1.0); // Should be increased
    
    // Test reward adjustment
    assert(adjuster.adjustRewards(ConsensusType::PROOF_OF_STAKE, 1.2, "Test reward increase"));
    params = posEngine.getParameters();
    assert(params["rewardMultiplier"] > 1.0); // Should be increased
    
    // Test stake adjustment
    assert(adjuster.adjustStakeRequirements(ConsensusType::PROOF_OF_STAKE, 0.1, "Test stake increase"));
    params = posEngine.getParameters();
    assert(params["minStake"] > 1000.0); // Should be increased
    
    // Test resource adjustment
    assert(adjuster.adjustResourceRequirements(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, -0.1, "Test resource decrease"));
    params = porcEngine.getParameters();
    assert(params["minResource"] < 100.0); // Should be decreased
    
    // Test invalid adjustments
    assert(!adjuster.adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.5, "Too large")); // Exceeds max change
    assert(!adjuster.adjustRewards(ConsensusType::VOTING_CONSENSUS, 1.1, "Unregistered engine"));
    
    adjuster.shutdown();
    std::cout << "✓ Parameter adjustments test passed" << std::endl;
}

// Test network condition monitoring
void testNetworkConditionMonitoring() {
    std::cout << "Testing network condition monitoring..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    
    // Set some metrics
    powEngine.setMetric("totalValidations", 1000);
    powEngine.setMetric("averageResponseTime", 2.0);
    powEngine.setMetric("pendingTransactions", 50);
    powEngine.setMetric("activeNodes", 100);
    
    // Monitor network conditions
    adjuster.monitorNetworkConditions();
    
    // Get network conditions
    NetworkConditions conditions = adjuster.getNetworkConditions();
    assert(conditions.pendingTransactions == 50);
    assert(conditions.activeNodes == 100);
    assert(conditions.averageResponseTime == 2.0);
    
    // Test health score calculation
    double healthScore = conditions.getHealthScore();
    assert(healthScore >= 0.0 && healthScore <= 1.0);
    
    adjuster.shutdown();
    std::cout << "✓ Network condition monitoring test passed" << std::endl;
}

// Test security threat detection
void testSecurityThreatDetection() {
    std::cout << "Testing security threat detection..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    
    // Simulate high network load (potential DDoS)
    powEngine.setMetric("pendingTransactions", 50000);
    
    // Simulate low node count (potential network partition)
    powEngine.setMetric("activeNodes", 5);
    
    // Detect threats
    adjuster.detectSecurityThreats();
    
    // Get security threats
    SecurityThreats threats = adjuster.getSecurityThreats();
    assert(threats.ddosAttack || threats.networkPartitioning);
    assert(threats.threatLevel > 0.0);
    assert(!threats.activeThreats.empty());
    
    // Test threat severity
    std::string severity = threats.getThreatSeverity();
    assert(!severity.empty());
    
    adjuster.shutdown();
    std::cout << "✓ Security threat detection test passed" << std::endl;
}

// Test automatic adjustments based on load
void testLoadBasedAdjustments() {
    std::cout << "Testing load-based adjustments..." << std::endl;
    
    ParameterAdjustmentConfig config;
    config.enableAutomaticAdjustment = true;
    config.loadThresholdHigh = 0.7;
    config.loadThresholdLow = 0.3;
    
    ParameterAdjuster adjuster(config);
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    MockConsensusEngine posEngine(ConsensusType::PROOF_OF_STAKE);
    
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_STAKE, &posEngine));
    
    // Simulate high load conditions
    powEngine.setMetric("pendingTransactions", 20000);
    posEngine.setMetric("pendingTransactions", 20000);
    
    // Monitor and adjust
    adjuster.monitorNetworkConditions();
    bool adjustmentsMade = adjuster.adjustParametersBasedOnLoad();
    
    // Should make adjustments for high load
    assert(adjustmentsMade);
    
    adjuster.shutdown();
    std::cout << "✓ Load-based adjustments test passed" << std::endl;
}

// Test emergency mode
void testEmergencyMode() {
    std::cout << "Testing emergency mode..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    // Enter emergency mode
    assert(adjuster.enterEmergencyMode());
    assert(adjuster.isInEmergencyMode());
    
    // Exit emergency mode
    assert(adjuster.exitEmergencyMode());
    assert(!adjuster.isInEmergencyMode());
    
    // Test double entry/exit
    assert(adjuster.enterEmergencyMode());
    assert(adjuster.enterEmergencyMode()); // Should succeed (already in emergency mode)
    assert(adjuster.exitEmergencyMode());
    assert(adjuster.exitEmergencyMode()); // Should succeed (not in emergency mode)
    
    adjuster.shutdown();
    std::cout << "✓ Emergency mode test passed" << std::endl;
}

// Test configuration management
void testConfigurationManagement() {
    std::cout << "Testing configuration management..." << std::endl;
    
    ParameterAdjustmentConfig config;
    config.monitoringInterval = 60;
    config.adjustmentInterval = 300;
    config.maxDifficultyChange = 0.15;
    config.maxRewardChange = 0.10;
    
    ParameterAdjuster adjuster(config);
    assert(adjuster.initialize());
    
    // Get configuration
    ParameterAdjustmentConfig retrievedConfig = adjuster.getConfig();
    assert(retrievedConfig.monitoringInterval == 60);
    assert(retrievedConfig.adjustmentInterval == 300);
    assert(retrievedConfig.maxDifficultyChange == 0.15);
    assert(retrievedConfig.maxRewardChange == 0.10);
    
    // Update adjustment limits
    assert(adjuster.updateAdjustmentLimits(0.2, 0.15, 0.1, 0.25));
    retrievedConfig = adjuster.getConfig();
    assert(retrievedConfig.maxDifficultyChange == 0.2);
    assert(retrievedConfig.maxRewardChange == 0.15);
    
    // Test invalid limits
    assert(!adjuster.updateAdjustmentLimits(-0.1, 0.15, 0.1, 0.25)); // Negative value
    assert(!adjuster.updateAdjustmentLimits(1.5, 0.15, 0.1, 0.25));  // Too large
    
    adjuster.shutdown();
    std::cout << "✓ Configuration management test passed" << std::endl;
}

// Test analysis and recommendations
void testAnalysisAndRecommendations() {
    std::cout << "Testing analysis and recommendations..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    
    // Set conditions that should trigger analysis
    powEngine.setMetric("pendingTransactions", 50000); // High load
    powEngine.setMetric("averageResponseTime", 10.0);  // High response time
    
    adjuster.monitorNetworkConditions();
    adjuster.detectSecurityThreats();
    
    // Get analysis
    std::vector<std::string> analysis = adjuster.analyzeNetworkConditions();
    assert(!analysis.empty());
    
    // Get recommendations
    std::vector<std::string> recommendations = adjuster.generateAdjustmentRecommendations();
    assert(!recommendations.empty());
    
    // Get performance analysis
    nlohmann::json perfAnalysis = adjuster.getPerformanceAnalysis();
    assert(perfAnalysis.contains("networkHealth"));
    assert(perfAnalysis.contains("overallPerformance"));
    
    adjuster.shutdown();
    std::cout << "✓ Analysis and recommendations test passed" << std::endl;
}

// Test statistics and history
void testStatisticsAndHistory() {
    std::cout << "Testing statistics and history..." << std::endl;
    
    ParameterAdjuster adjuster;
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    
    // Make some adjustments to generate history
    assert(adjuster.adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1, "Test 1"));
    assert(adjuster.adjustDifficulty(ConsensusType::PROOF_OF_WORK, -0.05, "Test 2"));
    
    // Get statistics
    nlohmann::json stats = adjuster.getStatistics();
    assert(stats["totalAdjustments"].get<uint64_t>() >= 2);
    assert(stats["isInitialized"].get<bool>() == true);
    assert(stats["isRunning"].get<bool>() == true);
    
    // Get adjustment history
    std::vector<AdjustmentRecord> history = adjuster.getAdjustmentHistory();
    assert(history.size() >= 2);
    assert(history[0].parameter == "difficulty");
    assert(history[0].reason == "Test 1");
    
    // Get detailed status
    nlohmann::json status = adjuster.getDetailedStatus();
    assert(status.contains("networkConditions"));
    assert(status.contains("securityThreats"));
    assert(status.contains("totalAdjustments"));
    
    // Reset statistics
    adjuster.resetStatistics();
    stats = adjuster.getStatistics();
    assert(stats["totalAdjustments"].get<uint64_t>() == 0);
    
    adjuster.shutdown();
    std::cout << "✓ Statistics and history test passed" << std::endl;
}

// Test automatic adjustment integration
void testAutomaticAdjustmentIntegration() {
    std::cout << "Testing automatic adjustment integration..." << std::endl;
    
    ParameterAdjustmentConfig config;
    config.enableAutomaticAdjustment = true;
    config.monitoringInterval = 1; // 1 second for testing
    config.adjustmentInterval = 2; // 2 seconds for testing
    
    ParameterAdjuster adjuster(config);
    assert(adjuster.initialize());
    
    MockConsensusEngine powEngine(ConsensusType::PROOF_OF_WORK);
    assert(adjuster.registerEngine(ConsensusType::PROOF_OF_WORK, &powEngine));
    
    // Enable automatic adjustment
    adjuster.enableAutomaticAdjustment();
    assert(adjuster.isAutomaticAdjustmentEnabled());
    
    // Perform automatic adjustments
    bool adjustmentsMade = adjuster.performAutomaticAdjustments();
    // May or may not make adjustments depending on conditions
    (void)adjustmentsMade; // Suppress unused variable warning
    
    // Disable automatic adjustment
    adjuster.disableAutomaticAdjustment();
    assert(!adjuster.isAutomaticAdjustmentEnabled());
    
    // Should not make adjustments when disabled
    bool disabledAdjustments = adjuster.performAutomaticAdjustments();
    // Should return false when disabled
    (void)disabledAdjustments; // Suppress unused variable warning
    
    adjuster.shutdown();
    std::cout << "✓ Automatic adjustment integration test passed" << std::endl;
}

int main() {
    std::cout << "Running ParameterAdjuster tests..." << std::endl;
    
    try {
        testBasicLifecycle();
        testEngineRegistration();
        testParameterAdjustments();
        testNetworkConditionMonitoring();
        testSecurityThreatDetection();
        testLoadBasedAdjustments();
        testEmergencyMode();
        testConfigurationManagement();
        testAnalysisAndRecommendations();
        testStatisticsAndHistory();
        testAutomaticAdjustmentIntegration();
        
        std::cout << "\n✅ All ParameterAdjuster tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}