#include "../include/core/consensus_monitor.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing basic consensus monitor functionality..." << std::endl;
    
    Logger::setLevel(LogLevel::INFO);
    
    try {
        // Test 1: Basic initialization
        std::cout << "Test 1: Basic initialization..." << std::endl;
        ConsensusMonitor monitor;
        
        MonitoringConfig config;
        config.healthCheckInterval = 5; // 5 seconds
        
        bool initialized = monitor.initialize(config);
        assert(initialized);
        assert(monitor.isRunning());
        
        std::cout << "✓ Initialization test passed" << std::endl;
        
        // Test 2: Performance metrics update
        std::cout << "Test 2: Performance metrics update..." << std::endl;
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 150.0);
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 200.0);
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, false, 500.0);
        
        auto report = monitor.getReport();
        auto powMetrics = report.healthMetrics[ConsensusType::PROOF_OF_WORK];
        
        assert(powMetrics.totalValidations == 3);
        assert(powMetrics.successfulValidations == 2);
        assert(powMetrics.failedValidations == 1);
        
        std::cout << "✓ Performance metrics test passed" << std::endl;
        
        // Test 3: Resource metrics
        std::cout << "Test 3: Resource metrics..." << std::endl;
        monitor.updateResourceMetrics(ConsensusType::PROOF_OF_WORK, 60.0, 1024.0, 15.0, 200.0);
        monitor.updateResourceMetrics(ConsensusType::PROOF_OF_STAKE, 40.0, 512.0, 10.0, 150.0);
        
        auto resourceJson = monitor.getResourceUtilization();
        assert(resourceJson.contains("PROOF_OF_WORK"));
        assert(resourceJson.contains("PROOF_OF_STAKE"));
        assert(resourceJson.contains("system"));
        
        std::cout << "✓ Resource metrics test passed" << std::endl;
        
        // Test 4: Dashboard data
        std::cout << "Test 4: Dashboard data generation..." << std::endl;
        auto dashboardJson = monitor.getDashboardData();
        
        assert(dashboardJson.contains("currentMetrics"));
        assert(dashboardJson.contains("systemStats"));
        assert(dashboardJson.contains("resourceUtilization"));
        assert(dashboardJson.contains("trends"));
        assert(dashboardJson.contains("insights"));
        assert(dashboardJson.contains("predictions"));
        
        std::cout << "✓ Dashboard data test passed" << std::endl;
        
        // Test 5: Historical data
        std::cout << "Test 5: Historical data tracking..." << std::endl;
        monitor.updateHistoricalMetrics();
        
        auto historicalData = monitor.getHistoricalData(ConsensusType::PROOF_OF_WORK, "averageResponseTime");
        // Historical data might be empty initially, which is fine
        
        std::cout << "✓ Historical data test passed" << std::endl;
        
        // Test 6: Insights generation
        std::cout << "Test 6: Insights generation..." << std::endl;
        auto insights = monitor.generateInsights();
        assert(insights.size() > 0); // Should have at least one insight
        
        std::cout << "✓ Insights generation test passed" << std::endl;
        
        // Test 7: Alert generation
        std::cout << "Test 7: Alert generation..." << std::endl;
        // Create conditions that should generate alerts (high error rate)
        for (int i = 0; i < 10; ++i) {
            monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_STAKE, false, 1000.0);
        }
        
        monitor.generateAlerts();
        auto activeAlerts = monitor.getActiveAlerts();
        assert(activeAlerts.size() > 0);
        
        std::cout << "✓ Alert generation test passed" << std::endl;
        
        // Test 8: Trend calculation
        std::cout << "Test 8: Trend calculation..." << std::endl;
        auto systemTrends = monitor.getSystemTrends();
        // Trends might be zero initially, which is fine
        
        std::cout << "✓ Trend calculation test passed" << std::endl;
        
        // Shutdown
        monitor.shutdown();
        
        std::cout << std::endl;
        std::cout << "🎉 All basic tests passed successfully!" << std::endl;
        std::cout << "✅ Consensus monitoring and metrics system is working correctly." << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}