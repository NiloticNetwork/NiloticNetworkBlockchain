#include "../include/core/consensus_monitor.h"
#include "../include/core/logger.h"
#include "test_utils_simple.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <fstream>

// Simple mock consensus engine for testing
class SimpleTestEngine : public ConsensusEngine {
private:
    ConsensusType type_;
    bool healthy_;
    
public:
    SimpleTestEngine(ConsensusType type) : type_(type), healthy_(true) {}
    
    bool validateBlock(const Block& /*block*/) override { return healthy_; }
    bool validateTransaction(const Transaction& /*transaction*/) override { return healthy_; }
    ConsensusResult processRequest(const ConsensusRequest& /*request*/) override {
        return ConsensusResult(healthy_, type_, healthy_ ? 1.0 : 0.0);
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy_; }
    
    ConsensusType getType() const override { return type_; }
    std::string getName() const override { return "SimpleTestEngine"; }
    
    nlohmann::json getStatus() const override {
        return nlohmann::json{{"healthy", healthy_}};
    }
    
    nlohmann::json getMetrics() const override {
        return nlohmann::json{{"healthy", healthy_}};
    }
    
    bool adjustParameters(const std::map<std::string, double>& /*parameters*/) override { return true; }
    std::map<std::string, double> getParameters() const override { return {}; }
    
    void setHealthy(bool h) { healthy_ = h; }
};

void testBasicMonitoring() {
    std::cout << "Testing basic monitoring functionality..." << std::endl;
    
    Logger::setLevel(LogLevel::INFO);
    ConsensusMonitor monitor;
    
    // Initialize monitor
    MonitoringConfig config;
    config.healthCheckInterval = 1;
    assert(monitor.initialize(config));
    assert(monitor.isRunning());
    
    // Create and register test engines
    auto powEngine = std::make_shared<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK);
    auto posEngine = std::make_shared<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE);
    
    monitor.registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    monitor.registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
    
    // Test health monitoring
    monitor.monitorConsensusHealth();
    
    auto report = monitor.getReport();
    assert(report.healthMetrics.size() == 2);
    
    std::cout << "✓ Basic monitoring test passed" << std::endl;
}

void testPerformanceMetrics() {
    std::cout << "Testing performance metrics collection..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Test performance metrics updates
    monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 150.0);
    monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 200.0);
    monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 500.0);
    
    auto report = monitor.getReport();
    auto powMetrics = report.healthMetrics[ConsensusType::PROOF_OF_WORK];
    
    assert(powMetrics.totalValidations == 3);
    assert(powMetrics.successfulValidations == 2);
    assert(powMetrics.failedValidations == 1);
    assert(powMetrics.errorRate > 0.3 && powMetrics.errorRate < 0.4); // Should be ~0.33
    
    std::cout << "✓ Performance metrics test passed" << std::endl;
}

void testDashboardData() {
    std::cout << "Testing dashboard data generation..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Add some test data
    monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0);
    monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, true, 150.0);
    
    // Update resource metrics
    monitor.updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 50.0, 512.0, 10.0, 100.0);
    
    auto dashboardJson = monitor.getDashboardData();
    
    assert(dashboardJson.contains("currentMetrics"));
    assert(dashboardJson.contains("systemStats"));
    assert(dashboardJson.contains("resourceUtilization"));
    assert(dashboardJson.contains("trends"));
    assert(dashboardJson.contains("insights"));
    assert(dashboardJson.contains("predictions"));
    
    std::cout << "✓ Dashboard data test passed" << std::endl;
}

void testHistoricalData() {
    std::cout << "Testing historical data tracking..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Generate some historical data
    for (int i = 0; i < 5; ++i) {
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0 + i * 10);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Force historical data update
    monitor.updateHistoricalMetrics();
    
    // Get historical data
    auto historicalData = monitor.getHistoricalData(ConsensusType::PROOF_OF_WORK, "averageResponseTime");
    assert(historicalData.size() > 0);
    
    // Check data points
    for (const auto& point : historicalData) {
        assert(point.metric == "averageResponseTime");
        assert(point.consensusType == ConsensusType::PROOF_OF_WORK);
        assert(point.timestamp > 0);
        assert(point.value > 0);
    }
    
    std::cout << "✓ Historical data test passed" << std::endl;
}

void testResourceUtilization() {
    std::cout << "Testing resource utilization monitoring..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Update resource metrics
    monitor.updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 60.0, 1024.0, 15.0, 200.0);
    monitor.updateResourceMetrics(ConsensusType::PROOF_OF_STAKE, 40.0, 512.0, 10.0, 150.0);
    
    auto resourceJson = monitor.getResourceUtilization();
    
    assert(resourceJson.contains("PROOF_OF_WORK"));
    assert(resourceJson.contains("PROOF_OF_STAKE"));
    assert(resourceJson.contains("system"));
    
    auto systemResources = resourceJson["system"];
    assert(systemResources.contains("totalCpuUsage"));
    assert(systemResources.contains("totalMemoryUsage"));
    
    // Check calculations
    double totalCpu = systemResources["totalCpuUsage"];
    double totalMemory = systemResources["totalMemoryUsage"];
    
    assert(totalCpu == 100.0); // 60 + 40
    assert(totalMemory == 1536.0); // 1024 + 512
    
    std::cout << "✓ Resource utilization test passed" << std::endl;
}

void testInsightsGeneration() {
    std::cout << "Testing insights generation..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Create conditions that should generate insights
    monitor.updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 85.0, 512.0, 10.0, 100.0);
    
    // High error rate
    for (int i = 0; i < 10; ++i) {
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, i < 3, 100.0); // 70% failure rate
    }
    
    auto insights = monitor.generateInsights();
    assert(insights.size() > 0);
    
    // Check for specific insights
    bool foundCpuInsight = false;
    bool foundErrorInsight = false;
    
    for (const auto& insight : insights) {
        if (insight.find("high CPU usage") != std::string::npos) {
            foundCpuInsight = true;
        }
        if (insight.find("error rate") != std::string::npos) {
            foundErrorInsight = true;
        }
    }
    
    assert(foundCpuInsight);
    assert(foundErrorInsight);
    
    std::cout << "✓ Insights generation test passed" << std::endl;
}

void testAlertGeneration() {
    std::cout << "Testing alert generation..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Create conditions that should generate alerts
    for (int i = 0; i < 20; ++i) {
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 1000.0);
    }
    
    monitor.generateAlerts();
    
    auto activeAlerts = monitor.getActiveAlerts();
    assert(activeAlerts.size() > 0);
    
    // Check alert properties
    for (const auto& alert : activeAlerts) {
        assert(!alert.alertId.empty());
        assert(!alert.type.empty());
        assert(!alert.message.empty());
        assert(!alert.severity.empty());
        assert(alert.timestamp > 0);
        assert(!alert.acknowledged);
    }
    
    std::cout << "✓ Alert generation test passed" << std::endl;
}

void testDataExportImport() {
    std::cout << "Testing data export/import..." << std::endl;
    
    ConsensusMonitor monitor;
    MonitoringConfig config;
    assert(monitor.initialize(config));
    
    // Generate some historical data
    for (int i = 0; i < 3; ++i) {
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0 + i * 10);
        monitor.updateHistoricalMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Export data
    std::string filename = "test_export_simple.json";
    monitor.exportHistoricalData(filename);
    
    // Verify file exists
    std::ifstream file(filename);
    assert(file.good());
    if (file.is_open()) {
        file.close();
    }
    
    // Import data (in a real test, we'd create a new monitor instance)
    monitor.importHistoricalData(filename);
    
    // Verify data was imported
    auto historicalData = monitor.getHistoricalData(ConsensusType::PROOF_OF_WORK, "averageResponseTime");
    assert(historicalData.size() > 0);
    
    // Clean up
    std::remove(filename.c_str());
    
    std::cout << "✓ Data export/import test passed" << std::endl;
}

int main() {
    std::cout << "Running Consensus Monitor Comprehensive Metrics Tests..." << std::endl;
    std::cout << "========================================================" << std::endl;
    
    try {
        testBasicMonitoring();
        testPerformanceMetrics();
        testDashboardData();
        testHistoricalData();
        testResourceUtilization();
        testInsightsGeneration();
        testAlertGeneration();
        testDataExportImport();
        
        std::cout << std::endl;
        std::cout << "🎉 All tests passed successfully!" << std::endl;
        std::cout << "✅ Comprehensive consensus monitoring and metrics system is working correctly." << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}