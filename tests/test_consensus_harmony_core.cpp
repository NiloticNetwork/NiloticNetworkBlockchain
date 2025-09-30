#include <iostream>
#include <cassert>
#include <memory>
#include <functional>
#include <cstdio>
#include <thread>
#include <chrono>
#include "consensus_harmony.h"
#include "consensus_harmony_manager.h"
#include "blockchain.h"
#include "block.h"
#include "transaction.h"

// Simple test framework
class TestRunner {
private:
    int totalTests = 0;
    int passedTests = 0;
    
public:
    void runTest(const std::string& testName, std::function<bool()> testFunc) {
        totalTests++;
        std::cout << "Running test: " << testName << "... ";
        std::cout.flush();
        
        try {
            if (testFunc()) {
                std::cout << "PASSED" << std::endl;
                passedTests++;
            } else {
                std::cout << "FAILED" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
        }
    }
    
    void printSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
        std::cout << "Success rate: " << (passedTests * 100.0 / totalTests) << "%" << std::endl;
    }
};

// Test functions
bool testConsensusRequestCreation() {
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, "test_data");
    
    return request.type == RequestType::BLOCK_VALIDATION &&
           request.data == "test_data" &&
           !request.requestId.empty() &&
           request.timestamp > 0;
}

bool testConsensusResultCreation() {
    ConsensusResult result(true, ConsensusType::PROOF_OF_WORK, 0.95, "Valid block");
    
    return result.isValid &&
           result.mechanism == ConsensusType::PROOF_OF_WORK &&
           result.confidence == 0.95 &&
           result.reason == "Valid block" &&
           result.timestamp > 0;
}

bool testConsensusConfigSerialization() {
    ConsensusConfig config;
    config.powDifficulty = 6;
    config.minStakeAmount = 2000.0;
    config.supermajorityThreshold = 0.75;
    
    nlohmann::json j = config.toJson();
    
    ConsensusConfig config2;
    config2.fromJson(j);
    
    return config2.powDifficulty == 6 &&
           config2.minStakeAmount == 2000.0 &&
           config2.supermajorityThreshold == 0.75;
}

bool testConsensusHarmonyManagerInitialization() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    // Test initial state
    if (manager->isInitialized() || manager->isRunning()) {
        return false;
    }
    
    // Test initialization
    if (!manager->initializeConsensus()) {
        return false;
    }
    
    if (!manager->isInitialized() || !manager->isRunning()) {
        return false;
    }
    
    // Test shutdown - explicit shutdown before destruction
    manager->shutdown();
    
    // Give a moment for threads to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    return !manager->isInitialized() && !manager->isRunning();
}

bool testConsensusHarmonyManagerWithCustomConfig() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    ConsensusConfig config;
    config.powDifficulty = 8;
    config.minStakeAmount = 3000.0;
    
    if (!manager->initializeConsensus(config)) {
        return false;
    }
    
    ConsensusConfig retrievedConfig = manager->getConfiguration();
    
    bool result = retrievedConfig.powDifficulty == 8 &&
                  retrievedConfig.minStakeAmount == 3000.0;
    
    manager->shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    return result;
}

bool testBlockValidation() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    if (!manager->initializeConsensus()) {
        return false;
    }
    
    // Create a test block
    Block testBlock(1, "previous_hash");
    Transaction tx("sender", "recipient", 100.0);
    testBlock.addTransaction(tx);
    
    // Test block validation
    bool result = manager->validateBlock(testBlock);
    
    manager->shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    return result;
}

bool testTransactionValidation() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    if (!manager->initializeConsensus()) {
        return false;
    }
    
    // Create a test transaction
    Transaction testTx("sender", "recipient", 50.0);
    
    // Test transaction validation
    bool result = manager->validateTransaction(testTx);
    
    manager->shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    return result;
}

bool testConsensusRequestProcessing() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    if (!manager->initializeConsensus()) {
        return false;
    }
    
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, "test_block_data");
    ConsensusResult result = manager->processConsensusRequest(request);
    
    bool testResult = result.isValid &&
                      result.confidence > 0.0 &&
                      result.metadata.find("requestId") != result.metadata.end();
    
    manager->shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    return testResult;
}

bool testStatusAndMetrics() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    if (!manager->initializeConsensus()) {
        return false;
    }
    
    // Perform some validations
    Block testBlock(1, "hash");
    manager->validateBlock(testBlock);
    
    Transaction testTx("sender", "recipient", 100.0);
    manager->validateTransaction(testTx);
    
    // Check status
    ConsensusStatus status = manager->getConsensusStatus();
    if (status.totalValidations != 2 || status.successfulValidations != 2) {
        manager->shutdown();
        return false;
    }
    
    // Test detailed status
    nlohmann::json detailedStatus = manager->getDetailedStatus();
    if (!detailedStatus["initialized"] || !detailedStatus["running"]) {
        manager->shutdown();
        return false;
    }
    
    // Test metrics
    nlohmann::json metrics = manager->getMetrics();
    bool result = metrics["validationSuccessRate"] == 1.0 &&
                  metrics["conflictRate"] == 0.0;
    
    manager->shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    return result;
}

bool testConfigurationPersistence() {
    auto blockchain = std::make_unique<Blockchain>();
    auto manager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    
    if (!manager->initializeConsensus()) {
        return false;
    }
    
    ConsensusConfig config;
    config.powDifficulty = 7;
    config.minStakeAmount = 2500.0;
    manager->updateConfiguration(config);
    
    // Save configuration
    std::string filename = "test_consensus_config.json";
    if (!manager->saveConfiguration(filename)) {
        manager->shutdown();
        return false;
    }
    
    manager->shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Create new manager and load configuration
    auto newManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
    if (!newManager->loadConfiguration(filename)) {
        std::remove(filename.c_str());
        return false;
    }
    
    ConsensusConfig loadedConfig = newManager->getConfiguration();
    
    // Clean up
    std::remove(filename.c_str());
    
    return loadedConfig.powDifficulty == 7 &&
           loadedConfig.minStakeAmount == 2500.0;
}

int main() {
    std::cout << "=== Consensus Harmony Core Infrastructure Tests ===" << std::endl;
    
    TestRunner runner;
    
    // Test data structures
    runner.runTest("ConsensusRequest Creation", testConsensusRequestCreation);
    runner.runTest("ConsensusResult Creation", testConsensusResultCreation);
    runner.runTest("ConsensusConfig Serialization", testConsensusConfigSerialization);
    
    // Test ConsensusHarmonyManager
    runner.runTest("Manager Initialization", testConsensusHarmonyManagerInitialization);
    runner.runTest("Manager with Custom Config", testConsensusHarmonyManagerWithCustomConfig);
    runner.runTest("Block Validation", testBlockValidation);
    runner.runTest("Transaction Validation", testTransactionValidation);
    runner.runTest("Consensus Request Processing", testConsensusRequestProcessing);
    runner.runTest("Status and Metrics", testStatusAndMetrics);
    runner.runTest("Configuration Persistence", testConfigurationPersistence);
    
    runner.printSummary();
    
    return 0;
}