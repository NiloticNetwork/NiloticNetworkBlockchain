#include <iostream>
#include <cassert>
#include <memory>
#include "consensus_harmony.h"
#include "consensus_harmony_manager.h"
#include "blockchain.h"
#include "block.h"
#include "transaction.h"

int main() {
    std::cout << "=== Simple Consensus Harmony Core Infrastructure Tests ===" << std::endl;
    
    int totalTests = 0;
    int passedTests = 0;
    
    // Test 1: ConsensusRequest Creation
    totalTests++;
    std::cout << "Test 1: ConsensusRequest Creation... ";
    try {
        ConsensusRequest request(RequestType::BLOCK_VALIDATION, "test_data");
        if (request.type == RequestType::BLOCK_VALIDATION &&
            request.data == "test_data" &&
            !request.requestId.empty() &&
            request.timestamp > 0) {
            std::cout << "PASSED" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
    }
    
    // Test 2: ConsensusResult Creation
    totalTests++;
    std::cout << "Test 2: ConsensusResult Creation... ";
    try {
        ConsensusResult result(true, ConsensusType::PROOF_OF_WORK, 0.95, "Valid block");
        if (result.isValid &&
            result.mechanism == ConsensusType::PROOF_OF_WORK &&
            result.confidence == 0.95 &&
            result.reason == "Valid block" &&
            result.timestamp > 0) {
            std::cout << "PASSED" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
    }
    
    // Test 3: ConsensusConfig Serialization
    totalTests++;
    std::cout << "Test 3: ConsensusConfig Serialization... ";
    try {
        ConsensusConfig config;
        config.powDifficulty = 6;
        config.minStakeAmount = 2000.0;
        config.supermajorityThreshold = 0.75;
        
        nlohmann::json j = config.toJson();
        
        ConsensusConfig config2;
        config2.fromJson(j);
        
        if (config2.powDifficulty == 6 &&
            config2.minStakeAmount == 2000.0 &&
            config2.supermajorityThreshold == 0.75) {
            std::cout << "PASSED" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
    }
    
    // Test 4: Basic Manager Creation (without initialization to avoid threading issues)
    totalTests++;
    std::cout << "Test 4: Manager Creation... ";
    try {
        auto blockchain = std::make_unique<Blockchain>();
        auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        
        if (!manager->isInitialized() && !manager->isRunning()) {
            std::cout << "PASSED" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
    }
    
    // Test 5: Configuration Management (without initialization)
    totalTests++;
    std::cout << "Test 5: Configuration Management... ";
    try {
        auto blockchain = std::make_unique<Blockchain>();
        auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        
        ConsensusConfig config;
        config.powDifficulty = 8;
        config.minStakeAmount = 3000.0;
        
        // Test configuration update without initialization
        bool result = manager->updateConfiguration(config);
        ConsensusConfig retrievedConfig = manager->getConfiguration();
        
        if (result && 
            retrievedConfig.powDifficulty == 8 &&
            retrievedConfig.minStakeAmount == 3000.0) {
            std::cout << "PASSED" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
    }
    
    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << totalTests << std::endl;
    std::cout << "Passed: " << passedTests << std::endl;
    std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
    std::cout << "Success rate: " << (passedTests * 100.0 / totalTests) << "%" << std::endl;
    
    if (passedTests == totalTests) {
        std::cout << "\n✅ All core infrastructure tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some tests failed." << std::endl;
        return 1;
    }
}