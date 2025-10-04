#include "../include/core/consensus_monitor.h"
#include "../include/core/logger.h"
#include <iostream>

int main() {
    std::cout << "Minimal consensus monitor test..." << std::endl;
    
    Logger::setLevel(LogLevel::INFO);
    
    try {
        ConsensusMonitor monitor;
        
        MonitoringConfig config;
        config.healthCheckInterval = 10; // 10 seconds
        
        std::cout << "Initializing monitor..." << std::endl;
        bool initialized = monitor.initialize(config);
        std::cout << "Initialized: " << (initialized ? "true" : "false") << std::endl;
        
        std::cout << "Updating performance metrics..." << std::endl;
        monitor.updatePerformanceMetrics(ConsensusType::PROOF_OF_WORK, true, 150.0);
        std::cout << "Performance metrics updated" << std::endl;
        
        std::cout << "Getting report..." << std::endl;
        // This might be where it hangs
        auto report = monitor.getReport();
        std::cout << "Report obtained, health metrics count: " << report.healthMetrics.size() << std::endl;
        
        std::cout << "Shutting down..." << std::endl;
        monitor.shutdown();
        
        std::cout << "✅ Test completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
}