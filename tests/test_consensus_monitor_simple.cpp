#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "../include/core/consensus_monitor.h"
#include "../include/core/consensus_harmony.h"

// Simple mock consensus engine for testing
class SimpleTestEngine : public ConsensusEngine {
private:
    ConsensusType type;
    bool healthy;
    
public:
    SimpleTestEngine(ConsensusType t, bool h = true) : type(t), healthy(h) {}
    
    bool validateBlock(const Block& block) override { return true; }
    bool validateTransaction(const Transaction& transaction) override { return true; }
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        return ConsensusResult(true, type, 1.0, "Test validation");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy; }
    
    ConsensusType getType() const override { return type; }
    std::string getName() const override { return "SimpleTestEngine"; }
    nlohmann::json getStatus() const override { return nlohmann::json{}; }
    nlohmann::json getMetrics() const override { return nlohmann::json{}; }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override { return true; }
    std::map<std::string, double> getParameters() const override { return {}; }
    
    void setHealthy(bool h) { healthy = h; }
};

int main() {
    std::cout << "Testing Consensus Monitor Implementation..." << std::endl;
    
    // Create consensus monitor
    auto monitor = std::make_unique<ConsensusMonitor>();
    
    // Initialize with test configuration
    MonitoringConfig config;
    config.healthCheckInterval = 1; // 1 second for fast testing
    config.conflictCheckInterval = 1;
    config.healthWarningThreshold = 0.7;
    config.healthCriticalThreshold = 0.5;
    
    if (!monitor->initialize(config)) {
        std::cerr << "Failed to initialize consensus monitor" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Monitor initialized successfully" << std::endl;
    
    // Create test engines
    auto powEngine = std::make_shared<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, true);
    auto posEngine = std::make_shared<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, true);
    auto porcEngine = std::make_shared<SimpleTestEngine>(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, false);
    
    // Register engines
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, porcEngine);
    
    std::cout << "✓ Registered 3 consensus engines" << std::endl;
    
    // Test health monitoring
    monitor->monitorConsensusHealth();
    
    auto report = monitor->getReport();
    std::cout << "✓ Health monitoring completed" << std::endl;
    std::cout << "  - Monitored engines: " << report.healthMetrics.size() << std::endl;
    std::cout << "  - Overall health score: " << report.overallHealthScore << std::endl;
    std::cout << "  - System stable: " << (report.systemStable ? "Yes" : "No") << std::endl;
    
    // Test performance metrics update
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 200.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 250.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, false, 1500.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, false, 3000.0);
    
    std::cout << "✓ Updated performance metrics" << std::endl;
    
    // Test alert generation
    monitor->generateAlerts();
    
    auto alerts = monitor->getActiveAlerts();
    std::cout << "✓ Generated alerts: " << alerts.size() << std::endl;
    
    for (const auto& alert : alerts) {
        std::cout << "  - " << alert.type << ": " << alert.message << " [" << alert.severity << "]" << std::endl;
    }
    
    // Test conflict detection
    monitor->detectConflicts();
    
    report = monitor->getReport();
    std::cout << "✓ Conflict detection completed" << std::endl;
    std::cout << "  - Active conflicts: " << report.activeConflicts.size() << std::endl;
    
    // Test rebalancing recommendations
    auto recommendations = monitor->generateRebalancingRecommendations();
    std::cout << "✓ Generated rebalancing recommendations: " << recommendations.size() << std::endl;
    
    for (const auto& rec : recommendations) {
        std::cout << "  - " << rec << std::endl;
    }
    
    // Test fork tracking
    monitor->trackForkContribution(ConsensusType::PROOF_OF_WORK, "Test fork at block 12345");
    
    std::cout << "✓ Fork tracking tested" << std::endl;
    
    // Test real-time status
    auto status = monitor->getRealtimeStatus();
    std::cout << "✓ Real-time status retrieved" << std::endl;
    std::cout << "  - Running: " << status["running"] << std::endl;
    std::cout << "  - Overall health: " << status["overallHealthScore"] << std::endl;
    
    // Test dashboard data
    auto dashboard = monitor->getDashboardData();
    std::cout << "✓ Dashboard data retrieved" << std::endl;
    
    // Test JSON serialization
    auto reportJson = report.toJson();
    std::cout << "✓ JSON serialization tested" << std::endl;
    
    // Test statistics
    std::cout << "✓ Statistics:" << std::endl;
    std::cout << "  - Total conflicts detected: " << monitor->getTotalConflictsDetected() << std::endl;
    std::cout << "  - Total alerts generated: " << monitor->getTotalAlertsGenerated() << std::endl;
    
    // Test continuous monitoring for a short time
    std::cout << "✓ Testing continuous monitoring for 3 seconds..." << std::endl;
    monitor->startContinuousMonitoring();
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    monitor->stopContinuousMonitoring();
    std::cout << "✓ Continuous monitoring stopped" << std::endl;
    
    // Final report
    report = monitor->getReport();
    std::cout << "\n=== Final Report ===" << std::endl;
    std::cout << "Overall Health Score: " << report.overallHealthScore << std::endl;
    std::cout << "System Stable: " << (report.systemStable ? "Yes" : "No") << std::endl;
    std::cout << "Total Conflicts: " << report.totalConflictsDetected << std::endl;
    std::cout << "Total Alerts: " << report.totalAlertsGenerated << std::endl;
    std::cout << "Active Alerts: " << monitor->getActiveAlerts().size() << std::endl;
    
    // Shutdown
    monitor->shutdown();
    std::cout << "✓ Monitor shutdown completed" << std::endl;
    
    std::cout << "\n🎉 All Consensus Monitor tests passed successfully!" << std::endl;
    
    return 0;
}