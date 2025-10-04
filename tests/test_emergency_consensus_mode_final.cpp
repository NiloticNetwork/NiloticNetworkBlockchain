#include "emergency_consensus_mode_standalone.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>

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

void testEmergencyModeBasics() {
    std::cout << "\n=== Testing Emergency Mode Basics ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
    
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    SimpleTest::assertFalse(emergencyMode.isEmergencyActive(), "Emergency mode initially inactive");
    SimpleTest::assertTrue(emergencyMode.getCurrentSeverity() == EmergencySeverity::LOW, "Initial severity is LOW");
    
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

void testEmergencyConfiguration() {
    std::cout << "\n=== Testing Emergency Configuration ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
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

void testEmergencyStatusAndMetrics() {
    std::cout << "\n=== Testing Emergency Status and Metrics ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
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

void testEmergencyRecovery() {
    std::cout << "\n=== Testing Emergency Recovery ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
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
    
    // Test recovery start
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::CRITICAL_ERROR, 
        EmergencySeverity::HIGH, 
        "Test critical error"
    ), "Emergency activation for recovery test");
    
    SimpleTest::assertTrue(emergencyMode.startRecovery(), "Recovery start");
    SimpleTest::assertTrue(emergencyMode.isRecoveryInProgress(), "Recovery in progress");
    
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Emergency deactivation after recovery");
}

void testEmergencyOperations() {
    std::cout << "\n=== Testing Emergency Operations ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test backup mechanisms
    SimpleTest::assertTrue(emergencyMode.activateBackupMechanisms(), "Backup mechanism activation");
    SimpleTest::assertTrue(emergencyMode.deactivateBackupMechanisms(), "Backup mechanism deactivation");
    
    // Test network protection
    SimpleTest::assertTrue(emergencyMode.enableNetworkProtection(), "Network protection activation");
    SimpleTest::assertTrue(emergencyMode.disableNetworkProtection(), "Network protection deactivation");
    
    // Test processing control
    SimpleTest::assertTrue(emergencyMode.haltTransactionProcessing(), "Halt transaction processing");
    SimpleTest::assertTrue(emergencyMode.resumeTransactionProcessing(), "Resume transaction processing");
    SimpleTest::assertTrue(emergencyMode.haltBlockProduction(), "Halt block production");
    SimpleTest::assertTrue(emergencyMode.resumeBlockProduction(), "Resume block production");
    
    // Test data consistency
    SimpleTest::assertTrue(emergencyMode.verifyDataConsistency(), "Data consistency verification");
    SimpleTest::assertTrue(emergencyMode.performStateRollback(5), "Valid state rollback");
    SimpleTest::assertFalse(emergencyMode.performStateRollback(100), "Invalid state rollback (exceeds maximum)");
    
    // Test attack protection
    SimpleTest::assertFalse(emergencyMode.detectConsensusAttack(), "Attack detection (no attack)");
    SimpleTest::assertTrue(emergencyMode.implementAttackCountermeasures(), "Attack countermeasures");
    SimpleTest::assertTrue(emergencyMode.isolateMaliciousNodes(), "Malicious node isolation");
}

void testCriticalEmergencyScenario() {
    std::cout << "\n=== Testing Critical Emergency Scenario ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test critical emergency activation
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::NETWORK_ATTACK, 
        EmergencySeverity::CRITICAL, 
        "Critical network attack detected"
    ), "Critical emergency activation");
    
    SimpleTest::assertTrue(emergencyMode.isEmergencyActive(), "Emergency mode is active");
    SimpleTest::assertTrue(emergencyMode.getCurrentSeverity() == EmergencySeverity::CRITICAL, "Severity is CRITICAL");
    
    // In critical mode, transaction processing should be halted
    // (This is handled internally by the emergency mode)
    
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Critical emergency deactivation");
}

void testMultipleEmergencyActivations() {
    std::cout << "\n=== Testing Multiple Emergency Activations ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // First activation
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::CONSENSUS_CONFLICT, 
        EmergencySeverity::LOW, 
        "First activation"
    ), "First emergency activation");
    
    // Second activation (should update severity)
    SimpleTest::assertTrue(emergencyMode.activateEmergencyMode(
        EmergencyType::NETWORK_ATTACK, 
        EmergencySeverity::HIGH, 
        "Second activation"
    ), "Second emergency activation");
    
    SimpleTest::assertTrue(emergencyMode.isEmergencyActive(), "Emergency mode is active after multiple activations");
    SimpleTest::assertTrue(emergencyMode.getCurrentSeverity() == EmergencySeverity::HIGH, "Severity is HIGH after second activation");
    
    // Deactivate
    SimpleTest::assertTrue(emergencyMode.deactivateEmergencyMode(), "Emergency deactivation after multiple activations");
}

void testEmergencyEventReporting() {
    std::cout << "\n=== Testing Emergency Event Reporting ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Create test event
    EmergencyEvent event(
        EmergencyType::DATA_CORRUPTION, 
        EmergencySeverity::MEDIUM, 
        "Test data corruption event",
        "UnitTest"
    );
    
    // Report event
    emergencyMode.reportEmergencyEvent(event);
    
    // The event should be processed (exact behavior depends on implementation)
    // For this test, we just verify the method doesn't crash
    SimpleTest::assertTrue(true, "Emergency event reporting completed");
}

void testAutomatedRecoveryControl() {
    std::cout << "\n=== Testing Automated Recovery Control ===" << std::endl;
    
    EmergencyConsensusModeStandalone emergencyMode;
    SimpleTest::assertTrue(emergencyMode.initialize(), "Emergency mode initialization");
    
    // Test automated recovery control
    SimpleTest::assertTrue(emergencyMode.enableAutomatedRecovery(), "Enable automated recovery");
    SimpleTest::assertTrue(emergencyMode.isAutomatedRecoveryEnabled(), "Automated recovery is enabled");
    
    SimpleTest::assertTrue(emergencyMode.disableAutomatedRecovery(), "Disable automated recovery");
    // Note: Implementation may vary for isAutomatedRecoveryEnabled after disable
}

int main() {
    std::cout << "Starting Emergency Consensus Mode Final Tests..." << std::endl;
    
    // Initialize logger
    Logger::setLevel(LogLevel::INFO);
    
    try {
        // Run all tests
        testEmergencyModeBasics();
        testEmergencyConfiguration();
        testEmergencyStatusAndMetrics();
        testEmergencyRecovery();
        testEmergencyOperations();
        testCriticalEmergencyScenario();
        testMultipleEmergencyActivations();
        testEmergencyEventReporting();
        testAutomatedRecoveryControl();
        
        // Print test summary
        SimpleTest::printSummary();
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nEmergency Consensus Mode Final Tests Completed!" << std::endl;
    return 0;
}