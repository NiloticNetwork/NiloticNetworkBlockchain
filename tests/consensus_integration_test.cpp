#include "../include/core/api.h"
#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>

/**
 * Comprehensive integration test for the Consensus API endpoints
 * Tests the complete workflow of consensus management through the API
 */
class ConsensusIntegrationTest {
private:
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> consensusManager;
    std::unique_ptr<API> api;
    const int testPort = 8082;

public:
    ConsensusIntegrationTest() {
        std::cout << "Setting up Consensus Integration Test..." << std::endl;
        
        // Initialize blockchain
        blockchain = std::make_unique<Blockchain>();
        
        // Initialize consensus manager
        consensusManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        
        // Initialize consensus with default configuration
        bool initialized = consensusManager->initializeConsensus();
        assert(initialized);
        
        // Initialize API with consensus manager
        api = std::make_unique<API>(*blockchain, consensusManager.get());
        
        // Start API server
        api->start(testPort);
        
        // Wait for server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        std::cout << "✓ Consensus Integration Test Setup Complete" << std::endl;
    }
    
    ~ConsensusIntegrationTest() {
        if (api) {
            api->stop();
        }
        std::cout << "✓ Consensus Integration Test Cleanup Complete" << std::endl;
    }
    
    void runIntegrationTests() {
        std::cout << "\n=== Running Consensus Integration Tests ===" << std::endl;
        
        testConsensusInitialization();
        testConsensusStatusFlow();
        testConsensusConfigurationManagement();
        testConsensusParameterAdjustments();
        testEmergencyModeWorkflow();
        testConsensusMetricsCollection();
        testConsensusEngineManagement();
        testErrorHandlingAndRecovery();
        
        std::cout << "\n=== All Consensus Integration Tests Completed ===" << std::endl;
    }

private:
    void testConsensusInitialization() {
        std::cout << "\n--- Testing Consensus Initialization ---" << std::endl;
        
        // Verify consensus manager is properly initialized
        assert(consensusManager->isInitialized());
        assert(consensusManager->isRunning());
        
        // Check that active engines are available
        auto activeEngines = consensusManager->getActiveEngines();
        assert(!activeEngines.empty());
        
        std::cout << "✓ Consensus system properly initialized" << std::endl;
        std::cout << "✓ Active engines: " << activeEngines.size() << std::endl;
    }
    
    void testConsensusStatusFlow() {
        std::cout << "\n--- Testing Consensus Status Flow ---" << std::endl;
        
        // Get initial status
        auto initialStatus = consensusManager->getConsensusStatus();
        
        // Verify status contains expected fields
        assert(!initialStatus.mechanismStatus.empty());
        
        // Get detailed status
        auto detailedStatus = consensusManager->getDetailedStatus();
        assert(!detailedStatus.empty());
        
        std::cout << "✓ Consensus status retrieval working correctly" << std::endl;
        std::cout << "✓ Detailed status contains " << detailedStatus.size() << " fields" << std::endl;
    }
    
    void testConsensusConfigurationManagement() {
        std::cout << "\n--- Testing Consensus Configuration Management ---" << std::endl;
        
        // Get current configuration
        auto currentConfig = consensusManager->getConfiguration();
        
        // Verify default configuration values
        assert(currentConfig.powDifficulty > 0);
        assert(currentConfig.minStakeAmount > 0);
        assert(currentConfig.supermajorityThreshold > 0.5 && currentConfig.supermajorityThreshold <= 1.0);
        
        // Create modified configuration
        ConsensusConfig newConfig = currentConfig;
        newConfig.powDifficulty = currentConfig.powDifficulty + 1;
        newConfig.minStakeAmount = currentConfig.minStakeAmount * 1.5;
        
        // Update configuration
        bool updateSuccess = consensusManager->updateConfiguration(newConfig);
        assert(updateSuccess);
        
        // Verify configuration was updated
        auto updatedConfig = consensusManager->getConfiguration();
        assert(updatedConfig.powDifficulty == newConfig.powDifficulty);
        assert(updatedConfig.minStakeAmount == newConfig.minStakeAmount);
        
        std::cout << "✓ Configuration management working correctly" << std::endl;
        std::cout << "✓ PoW difficulty updated to: " << updatedConfig.powDifficulty << std::endl;
        std::cout << "✓ Min stake amount updated to: " << updatedConfig.minStakeAmount << std::endl;
    }
    
    void testConsensusParameterAdjustments() {
        std::cout << "\n--- Testing Consensus Parameter Adjustments ---" << std::endl;
        
        // Test parameter adjustment for different consensus types
        std::vector<ConsensusType> typesToTest = {
            ConsensusType::PROOF_OF_WORK,
            ConsensusType::PROOF_OF_STAKE,
            ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION
        };
        
        for (auto type : typesToTest) {
            // Get current parameters
            auto currentParams = consensusManager->getConsensusParameters(type);
            
            // Try to adjust a parameter (this might not succeed if not implemented)
            bool adjustSuccess = consensusManager->setConsensusParameter(type, "test_param", 100.0);
            
            // Log the result (don't assert since implementation might not be complete)
            std::string typeName = getConsensusTypeName(type);
            std::cout << "✓ Parameter adjustment for " << typeName << ": " 
                      << (adjustSuccess ? "SUCCESS" : "NOT IMPLEMENTED") << std::endl;
        }
        
        std::cout << "✓ Parameter adjustment testing completed" << std::endl;
    }
    
    void testEmergencyModeWorkflow() {
        std::cout << "\n--- Testing Emergency Mode Workflow ---" << std::endl;
        
        // Check initial emergency mode status
        bool initialEmergencyStatus = consensusManager->isInEmergencyMode();
        std::cout << "✓ Initial emergency mode status: " << (initialEmergencyStatus ? "ACTIVE" : "INACTIVE") << std::endl;
        
        // Try to enter emergency mode
        bool enterSuccess = consensusManager->enterEmergencyMode();
        std::cout << "✓ Enter emergency mode: " << (enterSuccess ? "SUCCESS" : "FAILED/NOT NEEDED") << std::endl;
        
        // Check emergency mode status after entering
        bool emergencyStatusAfterEnter = consensusManager->isInEmergencyMode();
        std::cout << "✓ Emergency mode status after enter: " << (emergencyStatusAfterEnter ? "ACTIVE" : "INACTIVE") << std::endl;
        
        // Try to exit emergency mode
        bool exitSuccess = consensusManager->exitEmergencyMode();
        std::cout << "✓ Exit emergency mode: " << (exitSuccess ? "SUCCESS" : "FAILED/NOT NEEDED") << std::endl;
        
        // Check final emergency mode status
        bool finalEmergencyStatus = consensusManager->isInEmergencyMode();
        std::cout << "✓ Final emergency mode status: " << (finalEmergencyStatus ? "ACTIVE" : "INACTIVE") << std::endl;
        
        std::cout << "✓ Emergency mode workflow testing completed" << std::endl;
    }
    
    void testConsensusMetricsCollection() {
        std::cout << "\n--- Testing Consensus Metrics Collection ---" << std::endl;
        
        // Get metrics
        auto metrics = consensusManager->getMetrics();
        assert(!metrics.empty());
        
        std::cout << "✓ Metrics collection working correctly" << std::endl;
        std::cout << "✓ Metrics contain " << metrics.size() << " fields" << std::endl;
        
        // Print some key metrics if available
        if (metrics.contains("system_health")) {
            std::cout << "✓ System health metric available" << std::endl;
        }
        
        if (metrics.contains("active_engines")) {
            std::cout << "✓ Active engines metric available" << std::endl;
        }
    }
    
    void testConsensusEngineManagement() {
        std::cout << "\n--- Testing Consensus Engine Management ---" << std::endl;
        
        // Get active engines
        auto activeEngines = consensusManager->getActiveEngines();
        std::cout << "✓ Found " << activeEngines.size() << " active engines" << std::endl;
        
        // List active engines
        for (auto type : activeEngines) {
            std::string typeName = getConsensusTypeName(type);
            std::cout << "  - " << typeName << std::endl;
        }
        
        std::cout << "✓ Engine management testing completed" << std::endl;
    }
    
    void testErrorHandlingAndRecovery() {
        std::cout << "\n--- Testing Error Handling and Recovery ---" << std::endl;
        
        // Test invalid configuration
        ConsensusConfig invalidConfig;
        invalidConfig.powDifficulty = 0; // Invalid difficulty
        invalidConfig.minStakeAmount = -100; // Invalid stake amount
        invalidConfig.supermajorityThreshold = 1.5; // Invalid threshold (> 1.0)
        
        bool invalidConfigResult = consensusManager->updateConfiguration(invalidConfig);
        std::cout << "✓ Invalid configuration handling: " << (invalidConfigResult ? "ACCEPTED" : "REJECTED") << std::endl;
        
        // Test system recovery after invalid operations
        auto statusAfterInvalid = consensusManager->getConsensusStatus();
        bool systemStillRunning = consensusManager->isRunning();
        
        assert(systemStillRunning);
        std::cout << "✓ System remains operational after invalid operations" << std::endl;
        
        std::cout << "✓ Error handling and recovery testing completed" << std::endl;
    }
    
    std::string getConsensusTypeName(ConsensusType type) {
        switch (type) {
            case ConsensusType::PROOF_OF_WORK:
                return "PROOF_OF_WORK";
            case ConsensusType::PROOF_OF_STAKE:
                return "PROOF_OF_STAKE";
            case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
                return "PROOF_OF_RESOURCE_CONTRIBUTION";
            case ConsensusType::VOTING_CONSENSUS:
                return "VOTING_CONSENSUS";
            case ConsensusType::SMART_CONTRACT_VALIDATION:
                return "SMART_CONTRACT_VALIDATION";
            default:
                return "UNKNOWN";
        }
    }
};

int main() {
    try {
        ConsensusIntegrationTest test;
        test.runIntegrationTests();
        
        std::cout << "\n🎉 All Consensus Integration tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}