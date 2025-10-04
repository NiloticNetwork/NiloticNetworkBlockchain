#include "../include/core/emergency_consensus_mode.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

// Simple test framework
class SimpleTest {
private:
    static int totalTests;
    static int passedTests;
    static int failedTests;

public:
    static void assertTrue(bool condition, const std::string& testName) {
        totalTests++;
        if (condition) {
            passedTests++;
            std::cout << "[PASS] " << testName << std::endl;
        } else {
            failedTests++;
            std::cout << "[FAIL] " << testName << std::endl;
        }
    }
    
    static void assertFalse(bool condition, const std::string& testName) {
        assertTrue(!condition, testName);
    }
    
    static void assertEqual(int expected, int actual, const std::string& testName) {
        assertTrue(expected == actual, testName + " (expected: " + std::to_string(expected) + ", actual: " + std::to_string(actual) + ")");
    }
    
    static void printSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << failedTests << std::endl;
        std::cout << "Success rate: " << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0) << "%" << std::endl;
    }
};

int SimpleTest::totalTests = 0;
int SimpleTest::passedTests = 0;
int SimpleTest::failedTests = 0;

// Mock blockchain class for testing
class MockBlockchain {
public:
    MockBlockchain() = default;
    ~MockBlockchain() = default;
};

void testEmergencyModeInitialization() {
    std::cout << "\n=== Testing Emergency Mode Initialization ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    SimpleTest::assertFalse(emergencyMode.isEmergencyActive(), "Emergency mode initially inactive");
    SimpleTest::assertTrue(emergencyMode.getCurrentSeverity() == EmergencySeverity::LOW, "Initial severity is LOW");
}

void testEmergencyActivationDeactivation() {
    std::cout << "\n=== Testing Emergency Activation/Deactivation ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test activation
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::CONSENSUS_CONFLICT, 
        EmergencySeverity::HIGH, 
        "Test consensus conflict",
        "UnitTest"
    ), "Emergency mode activation");
    
    SimpleTest::assertTrue(emergencyMode.isEmergencyActive(), "Emergency mode is active after activation");
    SimpleTest::assertTrue(emergencyMode.getCurrentSeverity() == EmergencySeverity::HIGH, "Severity is HIGH after activation");
    
    // Test deactivation
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Emergency mode deactivation");
    SimpleTest::assertFalse(emergencyMode.isEmergencyActive(), "Emergency mode is inactive after deactivation");
}

void testCriticalEmergency() {
    std::cout << "\n=== Testing Critical Emergency ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test critical emergency activation
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::NETWORK_ATTACK, 
        EmergencySeverity::CRITICAL, 
        "Critical network attack detected"
    ), "Critical emergency activation");
    
    SimpleTest::assertTrue(emergencyMode.isEmergencyActive(), "Emergency mode is active");
    SimpleTest::assertTrue(emergencyMode.getCurrentSeverity() == EmergencySeverity::CRITICAL, "Severity is CRITICAL");
    
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Critical emergency deactivation");
}

void testBackupMechanisms() {
    std::cout << "\n=== Testing Backup Mechanisms ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test backup mechanism activation
    SimpleTest::assertTrue(emergencyMode.activateBackupMechanisms(), "Backup mechanism activation");
    
    // Test backup mechanism deactivation
    SimpleTest::assertTrue(emergencyMode.deactivateBackupMechanisms(), "Backup mechanism deactivation");
}

void testNetworkProtection() {
    std::cout << "\n=== Testing Network Protection ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test network protection activation
    SimpleTest::assertTrue(emergencyMode.enableNetworkProtection(), "Network protection activation");
    
    // Test network protection deactivation
    SimpleTest::assertTrue(emergencyMode.disableNetworkProtection(), "Network protection deactivation");
}

void testProcessingControl() {
    std::cout << "\n=== Testing Processing Control ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test transaction processing control
    SimpleTest::assertTrue(emergencyMode.haltTransactionProcessing(), "Halt transaction processing");
    SimpleTest::assertTrue(emergencyMode.resumeTransactionProcessing(), "Resume transaction processing");
    
    // Test block production control
    SimpleTest::assertTrue(emergencyMode.haltBlockProduction(), "Halt block production");
    SimpleTest::assertTrue(emergencyMode.resumeBlockProduction(), "Resume block production");
}

void testDataConsistency() {
    std::cout << "\n=== Testing Data Consistency ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test data consistency verification
    SimpleTest::assertTrue(emergencyMode.verifyDataConsistency(), "Data consistency verification");
    
    // Test state rollback
    SimpleTest::assertTrue(emergencyMode.performStateRollback(5), "Valid state rollback");
    SimpleTest::assertFalse(emergencyMode.performStateRollback(100), "Invalid state rollback (exceeds maximum)");
}

void testAttackProtection() {
    std::cout << "\n=== Testing Attack Protection ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test attack detection (should return false for no attack)
    SimpleTest::assertFalse(emergencyMode.detectConsensusAttack(), "Attack detection (no attack)");
    
    // Test attack countermeasures
    SimpleTest::assertTrue(emergencyMode.implementAttackCountermeasures(), "Attack countermeasures");
    
    // Test malicious node isolation
    SimpleTest::assertTrue(emergencyMode.isolateMaliciousNodes(), "Malicious node isolation");
}

void testRecoveryFunctionality() {
    std::cout << "\n=== Testing Recovery Functionality ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Activate emergency mode
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::CRITICAL_ERROR, 
        EmergencySeverity::HIGH, 
        "Test critical error"
    ), "Emergency activation for recovery test");
    
    // Test recovery start
    SimpleTest::assertTrue(emergencyMode.startRecovery(), "Recovery start");
    SimpleTest::assertTrue(emergencyMode.isRecoveryInProgress(), "Recovery in progress");
    
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Emergency deactivation after recovery");
}

void testConfigurationManagement() {
    std::cout << "\n=== Testing Configuration Management ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Get default configuration
    EmergencyConfig config = emergencyMode.getConfiguration();
    SimpleTest::assertTrue(config.maxConsensusConflicts > 0, "Default max consensus conflicts > 0");
    SimpleTest::assertTrue(config.maxMechanismFailures > 0, "Default max mechanism failures > 0");
    
    // Update configuration
    config.maxConsensusConflicts = 10;
    SimpleTest::assertTrue(emergencyMode.updateConfiguration(config), "Configuration update");
    
    // Verify configuration update
    EmergencyConfig updatedConfig = emergencyMode.getConfiguration();
    SimpleTest::assertTrue(updatedConfig.maxConsensusConflicts == 10, "Configuration update verification");
}

void testStatusAndMetrics() {
    std::cout << "\n=== Testing Status and Metrics ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Get initial status
    nlohmann::json status = emergencyMode.getEmergencyStatus();
    SimpleTest::assertFalse(status["emergencyActive"], "Initial emergency status is inactive");
    
    // Get initial metrics
    nlohmann::json metrics = emergencyMode.getEmergencyMetrics();
    SimpleTest::assertTrue(metrics["totalEmergencyActivations"] == 0, "Initial emergency activations is 0");
    
    // Activate emergency mode
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::MANUAL_ACTIVATION, 
        EmergencySeverity::LOW, 
        "Test activation"
    ), "Emergency activation for metrics test");
    
    // Check updated status
    status = emergencyMode.getEmergencyStatus();
    SimpleTest::assertTrue(status["emergencyActive"], "Emergency status is active after activation");
    SimpleTest::assertTrue(status["currentSeverity"] == "LOW", "Current severity is LOW");
    
    // Deactivate and check metrics
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Emergency deactivation for metrics test");
    metrics = emergencyMode.getEmergencyMetrics();
    SimpleTest::assertTrue(metrics["totalEmergencyActivations"] == 1, "Total emergency activations is 1");
}

void testRecoveryStrategies() {
    std::cout << "\n=== Testing Recovery Strategies ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Add custom recovery strategy
    RecoveryStrategy customStrategy("test_strategy", []() {
        return true; // Always succeed for test
    });
    
    emergencyMode.addRecoveryStrategy(customStrategy);
    
    // Execute recovery strategy
    SimpleTest::assertTrue(emergencyMode.executeRecoveryStrategy("test_strategy"), "Execute existing recovery strategy");
    
    // Try to execute non-existent strategy
    SimpleTest::assertFalse(emergencyMode.executeRecoveryStrategy("non_existent_strategy"), "Execute non-existent recovery strategy");
}

void testAutomatedRecovery() {
    std::cout << "\n=== Testing Automated Recovery ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    EmergencyConsensusMode emergencyMode(&harmonyManager, reinterpret_cast<Blockchain*>(&blockchain));
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test automated recovery control
    SimpleTest::assertTrue(emergencyMode.enableAutomatedRecovery(), "Enable automated recovery");
    SimpleTest::assertTrue(emergencyMode.isAutomatedRecoveryEnabled(), "Automated recovery is enabled");
    
    SimpleTest::assertTrue(emergencyMode.disableAutomatedRecovery(), "Disable automated recovery");
    // Note: Implementation may vary for isAutomatedRecoveryEnabled after disable
}

void testHarmonyManagerIntegration() {
    std::cout << "\n=== Testing Harmony Manager Integration ===" << std::endl;
    
    MockBlockchain blockchain;
    ConsensusHarmonyManager harmonyManager(reinterpret_cast<Blockchain*>(&blockchain));
    
    // Initialize harmony manager
    SimpleTest::assertTrue(harmonyManager.initializeConsensus(), "Harmony manager initialization");
    
    // Test emergency mode access through harmony manager
    SimpleTest::assertTrue(harmonyManager.enterEmergencyMode(), "Enter emergency mode via harmony manager");
    SimpleTest::assertTrue(harmonyManager.isInEmergencyMode(), "Emergency mode is active via harmony manager");
    
    SimpleTest::assertTrue(harmonyManager.exitEmergencyMode(), "Exit emergency mode via harmony manager");
    SimpleTest::assertFalse(harmonyManager.isInEmergencyMode(), "Emergency mode is inactive via harmony manager");
    
    // Test specific emergency activation
    SimpleTest::assertTrue(harmonyManager.enterEmergencyMode(
        EmergencyType::CRITICAL_ERROR,
        EmergencySeverity::CRITICAL,
        "Integration test critical error",
        "HarmonyManagerTest"
    ), "Specific emergency activation via harmony manager");
    
    SimpleTest::assertTrue(harmonyManager.isInEmergencyMode(), "Emergency mode is active after specific activation");
    
    EmergencyConsensusMode* emergencyModePtr = harmonyManager.getEmergencyMode();
    SimpleTest::assertTrue(emergencyModePtr != nullptr, "Emergency mode pointer is not null");
    SimpleTest::assertTrue(emergencyModePtr->getCurrentSeverity() == EmergencySeverity::CRITICAL, "Current severity is CRITICAL");
    
    SimpleTest::assertTrue(harmonyManager.exitEmergencyMode(), "Exit emergency mode after specific activation");
}

int main() {
    std::cout << "Starting Emergency Consensus Mode Tests..." << std::endl;
    
    // Initialize logger
    Logger::setLevel(LogLevel::INFO);
    
    try {
        // Run all tests
        testEmergencyModeInitialization();
        testEmergencyActivationDeactivation();
        testCriticalEmergency();
        testBackupMechanisms();
        testNetworkProtection();
        testProcessingControl();
        testDataConsistency();
        testAttackProtection();
        testRecoveryFunctionality();
        testConfigurationManagement();
        testStatusAndMetrics();
        testRecoveryStrategies();
        testAutomatedRecovery();
        testHarmonyManagerIntegration();
        
        // Print test summary
        SimpleTest::printSummary();
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nEmergency Consensus Mode Tests Completed!" << std::endl;
    return 0;
}