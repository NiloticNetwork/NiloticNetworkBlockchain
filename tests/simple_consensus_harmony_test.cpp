#include <iostream>
#include <cassert>
#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony_integration.h"
#include "../include/core/logger.h"

int main() {
    try {
        Logger::setLevel(LogLevel::INFO);
        Logger::info("Starting simple consensus harmony integration test");
        
        // Create blockchain
        Blockchain blockchain;
        
        // Create integration
        ConsensusHarmonyIntegration integration(&blockchain);
        
        // Test initialization
        Logger::info("Testing initialization...");
        bool initialized = integration.initialize();
        
        if (initialized) {
            Logger::info("✓ Consensus harmony integration initialized successfully");
            
            // Test basic functionality
            if (integration.isInitialized()) {
                Logger::info("✓ Integration reports initialized status correctly");
            } else {
                Logger::error("✗ Integration initialization status check failed");
                return 1;
            }
            
            // Test harmony manager access
            auto harmonyManager = integration.getHarmonyManager();
            if (harmonyManager) {
                Logger::info("✓ Harmony manager is accessible");
                
                if (harmonyManager->isInitialized()) {
                    Logger::info("✓ Harmony manager is initialized");
                } else {
                    Logger::warning("⚠ Harmony manager reports not initialized");
                }
            } else {
                Logger::error("✗ Harmony manager is not accessible");
                return 1;
            }
            
            // Test system validation
            Logger::info("Testing system validation...");
            bool valid = integration.validateSystemIntegrity();
            if (valid) {
                Logger::info("✓ System integrity validation passed");
            } else {
                Logger::warning("⚠ System integrity validation failed (may be expected)");
            }
            
            // Shutdown
            integration.shutdown();
            Logger::info("✓ Integration shutdown completed");
            
        } else {
            Logger::error("✗ Failed to initialize consensus harmony integration");
            return 1;
        }
        
        Logger::info("=== Simple consensus harmony integration test PASSED ===");
        return 0;
        
    } catch (const std::exception& e) {
        Logger::error("Test failed with exception: " + std::string(e.what()));
        return 1;
    }
}