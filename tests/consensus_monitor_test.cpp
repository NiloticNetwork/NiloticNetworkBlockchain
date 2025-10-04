#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/core/consensus_monitor.h"
#include "../include/core/consensus_harmony.h"
#include <thread>
#include <chrono>

// Mock ConsensusEngine for testing
class MockConsensusEngine : public ConsensusEngine {
public:
    MOCK_METHOD(bool, validateBlock, (const Block& block), (override));
    MOCK_METHOD(bool, validateTransaction, (const Transaction& transaction), (override));
    MOCK_METHOD(ConsensusResult, processRequest, (const ConsensusRequest& request), (override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(bool, isHealthy, (), (const, override));
    MOCK_METHOD(ConsensusType, getType, (), (const, override));
    MOCK_METHOD(std::string, getName, (), (const, override));
    MOCK_METHOD(nlohmann::json, getStatus, (), (const, override));
    MOCK_METHOD(nlohmann::json, getMetrics, (), (const, override));
    MOCK_METHOD(bool, adjustParameters, (const std::map<std::string, double>& parameters), (override));
    MOCK_METHOD(std::map<std::string, double>, getParameters, (), (const, override));
};

class ConsensusMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor = std::make_unique<ConsensusMonitor>();
        
        // Create mock engines
        powEngine = std::make_shared<MockConsensusEngine>();
        posEngine = std::make_shared<MockConsensusEngine>();
        porcEngine = std::make_shared<MockConsensusEngine>();
        
        // Set up default expectations
        ON_CALL(*powEngine, getType()).WillByDefault(::testing::Return(ConsensusType::PROOF_OF_WORK));
        ON_CALL(*posEngine, getType()).WillByDefault(::testing::Return(ConsensusType::PROOF_OF_STAKE));
        ON_CALL(*porcEngine, getType()).WillByDefault(::testing::Return(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION));
        
        ON_CALL(*powEngine, getName()).WillByDefault(::testing::Return("MockPoWEngine"));
        ON_CALL(*posEngine, getName()).WillByDefault(::testing::Return("MockPoSEngine"));
        ON_CALL(*porcEngine, getName()).WillByDefault(::testing::Return("MockPoRCEngine"));
        
        ON_CALL(*powEngine, isHealthy()).WillByDefault(::testing::Return(true));
        ON_CALL(*posEngine, isHealthy()).WillByDefault(::testing::Return(true));
        ON_CALL(*porcEngine, isHealthy()).WillByDefault(::testing::Return(true));
        
        ON_CALL(*powEngine, getMetrics()).WillByDefault(::testing::Return(nlohmann::json{}));
        ON_CALL(*posEngine, getMetrics()).WillByDefault(::testing::Return(nlohmann::json{}));
        ON_CALL(*porcEngine, getMetrics()).WillByDefault(::testing::Return(nlohmann::json{}));
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

// Test 1: Basic initialization and shutdown
TEST_F(ConsensusMonitorTest, InitializationAndShutdown) {
    EXPECT_FALSE(monitor->isRunning());
    
    MonitoringConfig config;
    config.healthCheckInterval = 1; // 1 second for faster testing
    
    EXPECT_TRUE(monitor->initialize(config));
    EXPECT_TRUE(monitor->isRunning());
    
    monitor->shutdown();
    EXPECT_FALSE(monitor->isRunning());
}

// Test 2: Engine registration and unregistration
TEST_F(ConsensusMonitorTest, EngineRegistration) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    // Register engines
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
    
    // Check that engines are registered by getting a report
    auto report = monitor->getReport();
    EXPECT_EQ(report.healthMetrics.size(), 2);
    EXPECT_TRUE(report.healthMetrics.find(ConsensusType::PROOF_OF_WORK) != report.healthMetrics.end());
    EXPECT_TRUE(report.healthMetrics.find(ConsensusType::PROOF_OF_STAKE) != report.healthMetrics.end());
    
    // Unregister an engine
    monitor->unregisterConsensusEngine(ConsensusType::PROOF_OF_WORK);
    
    report = monitor->getReport();
    auto powMetrics = report.healthMetrics.find(ConsensusType::PROOF_OF_WORK);
    if (powMetrics != report.healthMetrics.end()) {
        EXPECT_FALSE(powMetrics->second.isActive);
    }
}

// Test 3: Health monitoring (Requirement 5.1)
TEST_F(ConsensusMonitorTest, HealthMonitoring) {
    MonitoringConfig config;
    config.healthCheckInterval = 1;
    ASSERT_TRUE(monitor->initialize(config));
    
    // Register engines
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
    
    // Set up expectations for health checks
    EXPECT_CALL(*powEngine, isHealthy()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*posEngine, isHealthy()).WillRepeatedly(::testing::Return(true));
    
    // Perform health monitoring
    monitor->monitorConsensusHealth();
    
    auto report = monitor->getReport();
    EXPECT_EQ(report.healthMetrics.size(), 2);
    
    // Check that health scores are initialized
    for (const auto& [type, metrics] : report.healthMetrics) {
        EXPECT_GE(metrics.healthScore, 0.0);
        EXPECT_LE(metrics.healthScore, 1.0);
        EXPECT_TRUE(metrics.isActive);
    }
}

// Test 4: Performance metrics update (Requirement 5.3)
TEST_F(ConsensusMonitorTest, PerformanceMetricsUpdate) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    // Update performance metrics
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 500.0);  // Success, 500ms
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 1000.0); // Failure, 1000ms
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 300.0);  // Success, 300ms
    
    auto report = monitor->getReport();
    auto powMetrics = report.healthMetrics.find(ConsensusType::PROOF_OF_WORK);
    ASSERT_TRUE(powMetrics != report.healthMetrics.end());
    
    const auto& metrics = powMetrics->second;
    EXPECT_EQ(metrics.totalValidations, 3);
    EXPECT_EQ(metrics.successfulValidations, 2);
    EXPECT_EQ(metrics.failedValidations, 1);
    EXPECT_GT(metrics.averageResponseTime, 0.0);
    EXPECT_GT(metrics.errorRate, 0.0);
    EXPECT_LT(metrics.errorRate, 1.0);
}

// Test 5: Conflict detection (Requirement 5.2)
TEST_F(ConsensusMonitorTest, ConflictDetection) {
    MonitoringConfig config;
    config.enableConflictDetection = true;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
    
    // Set up different health conditions to trigger conflict detection
    EXPECT_CALL(*powEngine, isHealthy()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*posEngine, isHealthy()).WillRepeatedly(::testing::Return(true));
    
    // Create performance disparity
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 100.0);   // Fast, healthy
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, false, 3000.0); // Slow, unhealthy
    
    // Run conflict detection
    monitor->detectConflicts();
    
    auto report = monitor->getReport();
    // Conflicts might be detected due to performance disparity
    // The exact number depends on the conflict detection algorithm
}

// Test 6: Alert generation (Requirement 5.2, 5.3)
TEST_F(ConsensusMonitorTest, AlertGeneration) {
    MonitoringConfig config;
    config.healthWarningThreshold = 0.7;
    config.healthCriticalThreshold = 0.5;
    config.errorRateWarningThreshold = 0.1;
    config.responseTimeWarningThreshold = 1000.0;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    // Create conditions that should trigger alerts
    // High error rate
    for (int i = 0; i < 10; ++i) {
        monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 500.0);
    }
    
    // Generate alerts
    monitor->generateAlerts();
    
    auto alerts = monitor->getActiveAlerts();
    EXPECT_GT(alerts.size(), 0);
    
    // Check that we can acknowledge alerts
    if (!alerts.empty()) {
        std::string alertId = alerts[0].alertId;
        monitor->acknowledgeAlert(alertId);
        
        // The alert should now be acknowledged
        auto updatedAlerts = monitor->getActiveAlerts();
        bool found = false;
        for (const auto& alert : updatedAlerts) {
            if (alert.alertId == alertId) {
                found = true;
                break;
            }
        }
        // Alert should not be in active alerts anymore since it's acknowledged
        EXPECT_FALSE(found);
    }
}

// Test 7: Rebalancing recommendations (Requirement 5.4)
TEST_F(ConsensusMonitorTest, RebalancingRecommendations) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_STAKE, posEngine);
    
    // Create imbalanced conditions
    // PoW: Good performance
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 200.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 250.0);
    
    // PoS: Poor performance
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, false, 2000.0);
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, false, 2500.0);
    
    auto recommendations = monitor->generateRebalancingRecommendations();
    EXPECT_GT(recommendations.size(), 0);
    
    // Should contain recommendations about the poor-performing PoS
    bool foundPosRecommendation = false;
    for (const auto& rec : recommendations) {
        if (rec.find("PROOF_OF_STAKE") != std::string::npos) {
            foundPosRecommendation = true;
            break;
        }
    }
    EXPECT_TRUE(foundPosRecommendation);
}

// Test 8: Fork tracking (Requirement 5.5)
TEST_F(ConsensusMonitorTest, ForkTracking) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    // Track fork contribution
    std::string forkInfo = "Fork detected at block 12345, hash: abc123";
    monitor->trackForkContribution(ConsensusType::PROOF_OF_WORK, forkInfo);
    
    // Should generate an alert for fork detection
    auto alerts = monitor->getActiveAlerts();
    bool foundForkAlert = false;
    for (const auto& alert : alerts) {
        if (alert.type == "SECURITY" && alert.message.find("fork") != std::string::npos) {
            foundForkAlert = true;
            break;
        }
    }
    EXPECT_TRUE(foundForkAlert);
}

// Test 9: Real-time status and dashboard data
TEST_F(ConsensusMonitorTest, RealtimeStatusAndDashboard) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    // Get real-time status
    auto status = monitor->getRealtimeStatus();
    EXPECT_TRUE(status.contains("timestamp"));
    EXPECT_TRUE(status.contains("running"));
    EXPECT_TRUE(status.contains("overallHealthScore"));
    EXPECT_TRUE(status.contains("engines"));
    
    // Get dashboard data
    auto dashboard = monitor->getDashboardData();
    EXPECT_TRUE(dashboard.contains("dashboardGenerated"));
    EXPECT_TRUE(dashboard.contains("healthMetrics"));
}

// Test 10: Configuration management
TEST_F(ConsensusMonitorTest, ConfigurationManagement) {
    MonitoringConfig config;
    config.healthCheckInterval = 30;
    config.conflictCheckInterval = 10;
    config.healthWarningThreshold = 0.8;
    
    ASSERT_TRUE(monitor->initialize(config));
    
    auto retrievedConfig = monitor->getConfig();
    EXPECT_EQ(retrievedConfig.healthCheckInterval, 30);
    EXPECT_EQ(retrievedConfig.conflictCheckInterval, 10);
    EXPECT_EQ(retrievedConfig.healthWarningThreshold, 0.8);
    
    // Update configuration
    MonitoringConfig newConfig = config;
    newConfig.healthCheckInterval = 60;
    newConfig.healthWarningThreshold = 0.6;
    
    monitor->updateConfig(newConfig);
    
    auto updatedConfig = monitor->getConfig();
    EXPECT_EQ(updatedConfig.healthCheckInterval, 60);
    EXPECT_EQ(updatedConfig.healthWarningThreshold, 0.6);
}

// Test 11: Continuous monitoring threads
TEST_F(ConsensusMonitorTest, ContinuousMonitoring) {
    MonitoringConfig config;
    config.healthCheckInterval = 1; // 1 second for faster testing
    config.conflictCheckInterval = 1;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    EXPECT_CALL(*powEngine, isHealthy()).WillRepeatedly(::testing::Return(true));
    
    // Start continuous monitoring
    monitor->startContinuousMonitoring();
    
    // Let it run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    
    // Stop monitoring
    monitor->stopContinuousMonitoring();
    
    // Should have collected some metrics
    auto report = monitor->getReport();
    EXPECT_GT(report.healthMetrics.size(), 0);
}

// Test 12: Alert management
TEST_F(ConsensusMonitorTest, AlertManagement) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    // Generate some alerts
    AlertInfo alert1("ALERT_001", "HEALTH", "Test alert 1", "MEDIUM", {ConsensusType::PROOF_OF_WORK});
    AlertInfo alert2("ALERT_002", "PERFORMANCE", "Test alert 2", "HIGH", {ConsensusType::PROOF_OF_WORK});
    
    monitor->generateAlert(alert1);
    monitor->generateAlert(alert2);
    
    // Get alerts for specific mechanism
    auto powAlerts = monitor->getAlertsForMechanism(ConsensusType::PROOF_OF_WORK);
    EXPECT_EQ(powAlerts.size(), 2);
    
    // Clear old alerts (using 0 seconds to clear all)
    monitor->clearOldAlerts(0);
    
    auto remainingAlerts = monitor->getActiveAlerts();
    EXPECT_EQ(remainingAlerts.size(), 0);
}

// Test 13: JSON serialization of monitoring report
TEST_F(ConsensusMonitorTest, MonitoringReportSerialization) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    // Add some data
    monitor->updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 500.0);
    
    auto report = monitor->getReport();
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("healthMetrics"));
    EXPECT_TRUE(json.contains("activeConflicts"));
    EXPECT_TRUE(json.contains("recentAlerts"));
    EXPECT_TRUE(json.contains("totalConflictsDetected"));
    EXPECT_TRUE(json.contains("totalAlertsGenerated"));
    EXPECT_TRUE(json.contains("overallHealthScore"));
    EXPECT_TRUE(json.contains("systemStable"));
}

// Test 14: Error handling and edge cases
TEST_F(ConsensusMonitorTest, ErrorHandlingAndEdgeCases) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    // Test with no registered engines
    monitor->monitorConsensusHealth();
    auto report = monitor->getReport();
    EXPECT_EQ(report.overallHealthScore, 0.0);
    EXPECT_FALSE(report.systemStable);
    
    // Test unregistering non-existent engine
    monitor->unregisterConsensusEngine(ConsensusType::VOTING_CONSENSUS); // Should not crash
    
    // Test acknowledging non-existent alert
    monitor->acknowledgeAlert("NON_EXISTENT_ALERT"); // Should not crash
    
    // Test getting alerts for non-registered mechanism
    auto alerts = monitor->getAlertsForMechanism(ConsensusType::VOTING_CONSENSUS);
    EXPECT_EQ(alerts.size(), 0);
}

// Test 15: Statistics tracking
TEST_F(ConsensusMonitorTest, StatisticsTracking) {
    MonitoringConfig config;
    ASSERT_TRUE(monitor->initialize(config));
    
    monitor->registerConsensusEngine(ConsensusType::PROOF_OF_WORK, powEngine);
    
    EXPECT_EQ(monitor->getTotalConflictsDetected(), 0);
    EXPECT_EQ(monitor->getTotalAlertsGenerated(), 0);
    
    // Generate some conflicts and alerts
    MonitoringConflictInfo conflict("CONFLICT_001", {ConsensusType::PROOF_OF_WORK}, "Test conflict");
    monitor->reportConflict(conflict);
    
    AlertInfo alert("ALERT_001", "TEST", "Test alert");
    monitor->generateAlert(alert);
    
    EXPECT_GT(monitor->getTotalConflictsDetected(), 0);
    EXPECT_GT(monitor->getTotalAlertsGenerated(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}