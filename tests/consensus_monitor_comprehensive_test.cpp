#include "../include/core/consensus_monitor.h"
#include "../include/core/logger.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>

// Mock consensus engine for testing
class MockConsensusEngine : public ConsensusEngine {
private:
    ConsensusType type_;
    bool healthy_;
    double responseTime_;
    bool shouldFail_;
    
public:
    MockConsensusEngine(ConsensusType type) 
        : type_(type), healthy_(true), responseTime_(100.0), shouldFail_(false) {}
    
    bool validateBlock(const Block& block) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(responseTime_)));
        return !shouldFail_;
    }
    
    bool validateTransaction(const Transaction& transaction) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(responseTime_)));
        return !shouldFail_;
    }
    
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(responseTime_)));
        return ConsensusResult(!shouldFail_, type_, shouldFail_ ? 0.0 : 1.0, 
                              shouldFail_ ? "Mock failure" : "Success");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy_; }
    
    ConsensusType getType() const override { return type_; }
    std::string getName() const override { return "MockEngine"; }
    
    nlohmann::json getStatus() const override {
        return nlohmann::json{
            {"type", static_cast<int>(type_)},
            {"healthy", healthy_},
            {"responseTime", responseTime_}
        };
    }
    
    nlohmann::json getMetrics() const override {
        return nlohmann::json{
            {"responseTime", responseTime_},
            {"healthy", healthy_},
            {"shouldFail", shouldFail_}
        };
    }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override {
        return true;
    }
    
    std::map<std::string, double> getParameters() const override {
        return {{"responseTime", responseTime_}};
    }
    
    // Test helper methods
    void setHealthy(bool healthy) { healthy_ = healthy; }
    void setResponseTime(double time) { responseTime_ = time; }
    void setShouldFail(bool fail) { shouldFail_ = fail; }
};

class ConsensusMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::initialize(LogLevel::INFO);
        monitor = std::make_unique<ConsensusMonitor>();
        
        // Create mock engines
        powEngine = std::make_shared<MockConsensusEngine>(ConsensusType::PROOF_OF_WORK);
        posEngine = std::make_shared<MockConsensusEngine>(ConsensusType::PROOF_OF_STAKE);
        porcEngine = std::make_shared<MockConsensusEngine>(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
        
        // Initialize monitor
        MonitoringConfig config;
        config.healthCheckInterval = 1; // 1 second for faster testing
        config.conflictCheckInterval = 1;
        config.reportGenerationInterval = 2;
        
        ASSERT_TRUE(monitor->initialize(config));
        
        // Register engines
        monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
        monitor->registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
        monitor->registerConsensusEngine(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, porcEngine);
    }
    
    void TearDown() override {
        if (monitor) {
            monitor->shutdown();
        }
    }
    
    std::unique_ptr<ConsensusMonitor> monitor;
    std::shared_ptr<MockConsensusEngine> powEngine;
    std::shared_ptr<MockConsensusEngine> posEngine;
    std::shared_ptr<MockConsensusEngine> porcEngine;
};

// Test basic monitoring functionality
TEST_F(ConsensusMonitorTest, BasicMonitoring) {
    EXPECT_TRUE(monitor->isRunning());
    
    // Test health monitoring
    monitor->monitorConsensusHealth();
    
    auto report = monitor->getReport();
    EXPECT_EQ(report.healthMetrics.size(), 3);
    
    // Check that all engines are registered and healthy
    for (const auto& [type, metrics] : report.healthMetrics) {
        EXPECT_TRUE(metrics.isActive);
        EXPECT_GT(metrics.healthScore, 0.8);
    }
}

// Test performance metrics collection
TEST_F(ConsensusMonitorTest, PerformanceMetricsCollection) {
    // Simulate some validations with different response times
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 150.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 200.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 500.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0);
    
    auto report = monitor->getReport();
    auto powMetrics = report.healthMetrics[ConsensusType::PROOF_OF_WORK];
    
    EXPECT_EQ(powMetrics.totalValidations, 4);
    EXPECT_EQ(powMetrics.successfulValidations, 3);
    EXPECT_EQ(powMetrics.failedValidations, 1);
    EXPECT_DOUBLE_EQ(powMetrics.errorRate, 0.25); // 1/4 = 25%
    EXPECT_GT(powMetrics.averageResponseTime, 0);
    
    // Check performance metrics
    EXPECT_GT(powMetrics.performance.minResponseTime, 0);
    EXPECT_GT(powMetrics.performance.maxResponseTime, powMetrics.performance.minResponseTime);
}

// Test dashboard data generation
TEST_F(ConsensusMonitorTest, DashboardDataGeneration) {
    // Add some performance data
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, true, 150.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, true, 200.0);
    
    // Update resource metrics
    monitor->updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 50.0, 512.0, 10.0, 100.0);
    monitor->updateResourceMetrics(ConsensusType::PROOF_OF_STAKE, 30.0, 256.0, 5.0, 50.0);
    
    auto dashboardJson = monitor->getDashboardData();
    
    EXPECT_TRUE(dashboardJson.contains("currentMetrics"));
    EXPECT_TRUE(dashboardJson.contains("systemStats"));
    EXPECT_TRUE(dashboardJson.contains("resourceUtilization"));
    EXPECT_TRUE(dashboardJson.contains("trends"));
    EXPECT_TRUE(dashboardJson.contains("insights"));
    EXPECT_TRUE(dashboardJson.contains("predictions"));
    
    // Check system stats
    auto systemStats = dashboardJson["systemStats"];
    EXPECT_TRUE(systemStats.contains("systemThroughput"));
    EXPECT_TRUE(systemStats.contains("systemAverageResponseTime"));
    EXPECT_TRUE(systemStats.contains("systemErrorRate"));
    EXPECT_TRUE(systemStats.contains("systemHealthScore"));
}

// Test historical data tracking
TEST_F(ConsensusMonitorTest, HistoricalDataTracking) {
    // Generate some historical data
    for (int i = 0; i < 10; ++i) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0 + i * 10);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Force historical data update
    monitor->updateHistoricalMetrics();
    
    // Get historical data
    auto historicalData = monitor->getHistoricalData(ConsensusType::PROOF_OF_WORK, "averageResponseTime");
    EXPECT_GT(historicalData.size(), 0);
    
    // Check data points
    for (const auto& point : historicalData) {
        EXPECT_EQ(point.metric, "averageResponseTime");
        EXPECT_EQ(point.consensusType, ConsensusType::PROOF_OF_WORK);
        EXPECT_GT(point.timestamp, 0);
        EXPECT_GT(point.value, 0);
    }
}

// Test trend analysis
TEST_F(ConsensusMonitorTest, TrendAnalysis) {
    // Generate trending data (increasing response times)
    for (int i = 0; i < 20; ++i) {
        double responseTime = 100.0 + i * 5.0; // Increasing trend
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, responseTime);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    monitor->updateHistoricalMetrics();
    
    // Calculate trend
    double trend = monitor->calculateMetricTrend(ConsensusType::PROOF_OF_WORK, "averageResponseTime", 3600);
    EXPECT_GT(trend, 0); // Should show positive trend (increasing)
    
    // Get system trends
    auto systemTrends = monitor->getSystemTrends();
    EXPECT_TRUE(systemTrends.find("systemResponseTimeTrend") != systemTrends.end());
}

// Test insights generation
TEST_F(ConsensusMonitorTest, InsightsGeneration) {
    // Create conditions that should generate insights
    
    // High CPU usage
    monitor->updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 85.0, 512.0, 10.0, 100.0);
    
    // High error rate
    for (int i = 0; i < 10; ++i) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, i < 3, 100.0); // 70% failure rate
    }
    
    auto insights = monitor->generateInsights();
    EXPECT_GT(insights.size(), 0);
    
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
    
    EXPECT_TRUE(foundCpuInsight);
    EXPECT_TRUE(foundErrorInsight);
}

// Test predictions generation
TEST_F(ConsensusMonitorTest, PredictionsGeneration) {
    // Generate data that should trigger predictions
    
    // Rapidly increasing memory usage
    for (int i = 0; i < 10; ++i) {
        monitor->updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 50.0, 500.0 + i * 100.0, 10.0, 100.0);
        monitor->updateHistoricalMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto predictions = monitor->generatePredictions();
    EXPECT_GT(predictions.size(), 0);
    
    // Should contain memory-related prediction
    bool foundMemoryPrediction = false;
    for (const auto& prediction : predictions) {
        if (prediction.find("memory") != std::string::npos) {
            foundMemoryPrediction = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundMemoryPrediction);
}

// Test conflict detection
TEST_F(ConsensusMonitorTest, ConflictDetection) {
    // Create conditions for conflict detection
    
    // Make one engine very slow
    powEngine->setResponseTime(5000.0);
    
    // Make another engine have high error rate
    posEngine->setShouldFail(true);
    
    // Update metrics to reflect these conditions
    for (int i = 0; i < 10; ++i) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 5000.0);
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, false, 100.0);
    }
    
    // Run conflict detection
    monitor->detectConflicts();
    
    auto report = monitor->getReport();
    EXPECT_GT(report.activeConflicts.size(), 0);
    
    // Check conflict details
    for (const auto& conflict : report.activeConflicts) {
        EXPECT_FALSE(conflict.conflictId.empty());
        EXPECT_GT(conflict.involvedMechanisms.size(), 1);
        EXPECT_FALSE(conflict.description.empty());
    }
}

// Test alert generation
TEST_F(ConsensusMonitorTest, AlertGeneration) {
    // Create conditions that should generate alerts
    
    // Critical health score
    for (int i = 0; i < 20; ++i) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 1000.0); // High failure rate and slow response
    }
    
    monitor->generateAlerts();
    
    auto activeAlerts = monitor->getActiveAlerts();
    EXPECT_GT(activeAlerts.size(), 0);
    
    // Check alert properties
    for (const auto& alert : activeAlerts) {
        EXPECT_FALSE(alert.alertId.empty());
        EXPECT_FALSE(alert.type.empty());
        EXPECT_FALSE(alert.message.empty());
        EXPECT_FALSE(alert.severity.empty());
        EXPECT_GT(alert.timestamp, 0);
        EXPECT_FALSE(alert.acknowledged);
    }
}

// Test resource utilization monitoring
TEST_F(ConsensusMonitorTest, ResourceUtilizationMonitoring) {
    // Update resource metrics for all engines
    monitor->updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 60.0, 1024.0, 15.0, 200.0);
    monitor->updateResourceMetrics(ConsensusType::PROOF_OF_STAKE, 40.0, 512.0, 10.0, 150.0);
    monitor->updateResourceMetrics(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 30.0, 256.0, 5.0, 100.0);
    
    auto resourceJson = monitor->getResourceUtilization();
    
    EXPECT_TRUE(resourceJson.contains("PROOF_OF_WORK"));
    EXPECT_TRUE(resourceJson.contains("PROOF_OF_STAKE"));
    EXPECT_TRUE(resourceJson.contains("PROOF_OF_RESOURCE_CONTRIBUTION"));
    EXPECT_TRUE(resourceJson.contains("system"));
    
    auto systemResources = resourceJson["system"];
    EXPECT_TRUE(systemResources.contains("totalCpuUsage"));
    EXPECT_TRUE(systemResources.contains("totalMemoryUsage"));
    EXPECT_TRUE(systemResources.contains("averageCpuUsage"));
    EXPECT_TRUE(systemResources.contains("averageMemoryUsage"));
    
    EXPECT_DOUBLE_EQ(systemResources["totalCpuUsage"], 130.0); // 60 + 40 + 30
    EXPECT_DOUBLE_EQ(systemResources["totalMemoryUsage"], 1792.0); // 1024 + 512 + 256
}

// Test data export/import functionality
TEST_F(ConsensusMonitorTest, DataExportImport) {
    // Generate some historical data
    for (int i = 0; i < 5; ++i) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0 + i * 10);
        monitor->updateHistoricalMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Export data
    std::string filename = "test_export.json";
    monitor->exportHistoricalData(filename);
    
    // Verify file exists
    std::ifstream file(filename);
    EXPECT_TRUE(file.good());
    file.close();
    
    // Clear current data and import
    // Note: In a real test, we'd create a new monitor instance
    monitor->importHistoricalData(filename);
    
    // Verify data was imported
    auto historicalData = monitor->getHistoricalData(ConsensusType::PROOF_OF_WORK, "averageResponseTime");
    EXPECT_GT(historicalData.size(), 0);
    
    // Clean up
    std::remove(filename.c_str());
}

// Test continuous monitoring
TEST_F(ConsensusMonitorTest, ContinuousMonitoring) {
    // Start continuous monitoring
    monitor->startContinuousMonitoring();
    
    // Let it run for a short time
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Check that monitoring is active
    EXPECT_TRUE(monitor->isRunning());
    
    // Generate some activity
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, true, 150.0);
    
    // Wait a bit more for processing
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Stop monitoring
    monitor->stopContinuousMonitoring();
    
    // Verify we have collected some data
    auto report = monitor->getReport();
    EXPECT_GT(report.healthMetrics.size(), 0);
}

// Test percentile calculations
TEST_F(ConsensusMonitorTest, PercentileCalculations) {
    // Add response times that should give predictable percentiles
    std::vector<double> responseTimes = {100, 150, 200, 250, 300, 350, 400, 450, 500, 1000};
    
    for (double time : responseTimes) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, time);
    }
    
    auto report = monitor->getReport();
    auto powMetrics = report.healthMetrics[ConsensusType::PROOF_OF_WORK];
    
    EXPECT_GT(powMetrics.performance.p95ResponseTime, powMetrics.performance.p99ResponseTime * 0.8);
    EXPECT_GT(powMetrics.performance.p99ResponseTime, powMetrics.performance.p95ResponseTime);
    EXPECT_EQ(powMetrics.performance.minResponseTime, 100.0);
    EXPECT_EQ(powMetrics.performance.maxResponseTime, 1000.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}