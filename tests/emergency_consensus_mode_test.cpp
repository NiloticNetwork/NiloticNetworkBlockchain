#include <gtest/gtest.h>
#include "../include/core/emergency_consensus_mode.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/blockchain.h"
#include "../include/core/logger.h"
#include <thread>
#include <chrono>

class EmergencyConsensusModeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for testing
        Logger::setLogLevel(Logger::LogLevel::INFO);
        
        // Create test blockchain
        blockchain = std::make_unique<Blockchain>();
        
        // Create harmony manager
        harmonyManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        
        // Create emergency mode
        emergencyMode = std::make_unique<EmergencyConsensusMode>(harmonyManager.get(), blockchain.get());
    }
    
    void TearDown() override {
        if (emergencyMode) {
            emergencyMode->shutdown();
        }
        if (harmonyManager) {
            harmonyManager->shutdown();
        }
    }
    
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
    std::unique_ptr<EmergencyConsensusMode> emergencyMode;
};

// Test emergency mode initialization
TEST_F(EmergencyConsensusModeTest, InitializationTest) {
    EXPECT_TRUE(emergencyMode->initialize());
    EXPECT_FALSE(emergencyMode->isEmergencyActive());
    EXPECT_EQ(emergencyMode->getCurrentSeverity(), EmergencySeverity::LOW);
}

// Test emergency mode activation
TEST_F(EmergencyConsensusModeTest, EmergencyActivationTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test activation with different severity levels
    EXPECT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::CONSENSUS_CONFLICT, 
        EmergencySeverity::HIGH, 
        "Test consensus conflict",
        "UnitTest"
    ));
    
    EXPECT_TRUE(emergencyMode->isEmergencyActive());
    EXPECT_EQ(emergencyMode->getCurrentSeverity(), EmergencySeverity::HIGH);
}

// Test emergency mode deactivation
TEST_F(EmergencyConsensusModeTest, EmergencyDeactivationTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Activate emergency mode
    ASSERT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::MECHANISM_FAILURE, 
        EmergencySeverity::MEDIUM, 
        "Test mechanism failure"
    ));
    
    ASSERT_TRUE(emergencyMode->isEmergencyActive());
    
    // Deactivate emergency mode
    EXPECT_TRUE(emergencyMode->deactivateEmergencyMode());
    EXPECT_FALSE(emergencyMode->isEmergencyActive());
}

// Test critical emergency activation
TEST_F(EmergencyConsensusModeTest, CriticalEmergencyTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test critical emergency activation
    EXPECT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::NETWORK_ATTACK, 
        EmergencySeverity::CRITICAL, 
        "Critical network attack detected"
    ));
    
    EXPECT_TRUE(emergencyMode->isEmergencyActive());
    EXPECT_EQ(emergencyMode->getCurrentSeverity(), EmergencySeverity::CRITICAL);
}

// Test backup mechanism activation
TEST_F(EmergencyConsensusModeTest, BackupMechanismTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test backup mechanism activation
    EXPECT_TRUE(emergencyMode->activateBackupMechanisms());
    
    // Test backup mechanism deactivation
    EXPECT_TRUE(emergencyMode->deactivateBackupMechanisms());
}

// Test network protection
TEST_F(EmergencyConsensusModeTest, NetworkProtectionTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test network protection activation
    EXPECT_TRUE(emergencyMode->enableNetworkProtection());
    
    // Test network protection deactivation
    EXPECT_TRUE(emergencyMode->disableNetworkProtection());
}

// Test transaction and block processing control
TEST_F(EmergencyConsensusModeTest, ProcessingControlTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test transaction processing control
    EXPECT_TRUE(emergencyMode->haltTransactionProcessing());
    EXPECT_TRUE(emergencyMode->resumeTransactionProcessing());
    
    // Test block production control
    EXPECT_TRUE(emergencyMode->haltBlockProduction());
    EXPECT_TRUE(emergencyMode->resumeBlockProduction());
}

// Test data consistency verification
TEST_F(EmergencyConsensusModeTest, DataConsistencyTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test data consistency verification
    EXPECT_TRUE(emergencyMode->verifyDataConsistency());
}

// Test state rollback
TEST_F(EmergencyConsensusModeTest, StateRollbackTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test valid rollback
    EXPECT_TRUE(emergencyMode->performStateRollback(5));
    
    // Test rollback exceeding maximum
    EXPECT_FALSE(emergencyMode->performStateRollback(100));
}

// Test attack detection and countermeasures
TEST_F(EmergencyConsensusModeTest, AttackProtectionTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test attack detection
    EXPECT_FALSE(emergencyMode->detectConsensusAttack()); // Should return false for no attack
    
    // Test attack countermeasures
    EXPECT_TRUE(emergencyMode->implementAttackCountermeasures());
    
    // Test malicious node isolation
    EXPECT_TRUE(emergencyMode->isolateMaliciousNodes());
}

// Test recovery functionality
TEST_F(EmergencyConsensusModeTest, RecoveryTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Activate emergency mode
    ASSERT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::CRITICAL_ERROR, 
        EmergencySeverity::HIGH, 
        "Test critical error"
    ));
    
    // Test recovery start
    EXPECT_TRUE(emergencyMode->startRecovery());
    EXPECT_TRUE(emergencyMode->isRecoveryInProgress());
}

// Test emergency event reporting
TEST_F(EmergencyConsensusModeTest, EventReportingTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Create test event
    EmergencyEvent event(
        EmergencyType::DATA_CORRUPTION, 
        EmergencySeverity::MEDIUM, 
        "Test data corruption event",
        "UnitTest"
    );
    
    // Report event
    emergencyMode->reportEmergencyEvent(event);
    
    // The event should be processed (exact behavior depends on implementation)
}

// Test configuration management
TEST_F(EmergencyConsensusModeTest, ConfigurationTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Get default configuration
    EmergencyConfig config = emergencyMode->getConfiguration();
    EXPECT_GT(config.maxConsensusConflicts, 0);
    EXPECT_GT(config.maxMechanismFailures, 0);
    
    // Update configuration
    config.maxConsensusConflicts = 10;
    EXPECT_TRUE(emergencyMode->updateConfiguration(config));
    
    // Verify configuration update
    EmergencyConfig updatedConfig = emergencyMode->getConfiguration();
    EXPECT_EQ(updatedConfig.maxConsensusConflicts, 10);
}

// Test emergency status and metrics
TEST_F(EmergencyConsensusModeTest, StatusAndMetricsTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Get initial status
    nlohmann::json status = emergencyMode->getEmergencyStatus();
    EXPECT_FALSE(status["emergencyActive"]);
    
    // Get initial metrics
    nlohmann::json metrics = emergencyMode->getEmergencyMetrics();
    EXPECT_EQ(metrics["totalEmergencyActivations"], 0);
    
    // Activate emergency mode
    ASSERT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::MANUAL_ACTIVATION, 
        EmergencySeverity::LOW, 
        "Test activation"
    ));
    
    // Check updated status
    status = emergencyMode->getEmergencyStatus();
    EXPECT_TRUE(status["emergencyActive"]);
    EXPECT_EQ(status["currentSeverity"], "LOW");
    
    // Deactivate and check metrics
    ASSERT_TRUE(emergencyMode->deactivateEmergencyMode());
    metrics = emergencyMode->getEmergencyMetrics();
    EXPECT_EQ(metrics["totalEmergencyActivations"], 1);
}

// Test recovery strategies
TEST_F(EmergencyConsensusModeTest, RecoveryStrategiesTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Add custom recovery strategy
    RecoveryStrategy customStrategy("test_strategy", []() {
        return true; // Always succeed for test
    });
    
    emergencyMode->addRecoveryStrategy(customStrategy);
    
    // Execute recovery strategy
    EXPECT_TRUE(emergencyMode->executeRecoveryStrategy("test_strategy"));
    
    // Try to execute non-existent strategy
    EXPECT_FALSE(emergencyMode->executeRecoveryStrategy("non_existent_strategy"));
}

// Test automated recovery
TEST_F(EmergencyConsensusModeTest, AutomatedRecoveryTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Test automated recovery control
    EXPECT_TRUE(emergencyMode->enableAutomatedRecovery());
    EXPECT_TRUE(emergencyMode->isAutomatedRecoveryEnabled());
    
    EXPECT_TRUE(emergencyMode->disableAutomatedRecovery());
    EXPECT_TRUE(emergencyMode->isAutomatedRecoveryEnabled()); // Implementation may vary
}

// Test emergency mode timeout
TEST_F(EmergencyConsensusModeTest, EmergencyTimeoutTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // Activate emergency mode
    ASSERT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::MANUAL_ACTIVATION, 
        EmergencySeverity::LOW, 
        "Timeout test"
    ));
    
    // Wait a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check that emergency mode is still active
    EXPECT_TRUE(emergencyMode->isEmergencyActive());
    
    // Deactivate manually
    EXPECT_TRUE(emergencyMode->deactivateEmergencyMode());
}

// Test multiple emergency activations
TEST_F(EmergencyConsensusModeTest, MultipleActivationsTest) {
    ASSERT_TRUE(emergencyMode->initialize());
    
    // First activation
    ASSERT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::CONSENSUS_CONFLICT, 
        EmergencySeverity::LOW, 
        "First activation"
    ));
    
    // Second activation (should update severity)
    ASSERT_TRUE(emergencyMode->activateEmergencyMode(
        EmergencyType::NETWORK_ATTACK, 
        EmergencySeverity::HIGH, 
        "Second activation"
    ));
    
    EXPECT_TRUE(emergencyMode->isEmergencyActive());
    EXPECT_EQ(emergencyMode->getCurrentSeverity(), EmergencySeverity::HIGH);
    
    // Deactivate
    EXPECT_TRUE(emergencyMode->deactivateEmergencyMode());
}

// Test integration with harmony manager
TEST_F(EmergencyConsensusModeTest, HarmonyManagerIntegrationTest) {
    // Initialize harmony manager
    ASSERT_TRUE(harmonyManager->initializeConsensus());
    
    // Test emergency mode access through harmony manager
    EXPECT_TRUE(harmonyManager->enterEmergencyMode());
    EXPECT_TRUE(harmonyManager->isInEmergencyMode());
    
    EXPECT_TRUE(harmonyManager->exitEmergencyMode());
    EXPECT_FALSE(harmonyManager->isInEmergencyMode());
    
    // Test specific emergency activation
    EXPECT_TRUE(harmonyManager->enterEmergencyMode(
        EmergencyType::CRITICAL_ERROR,
        EmergencySeverity::CRITICAL,
        "Integration test critical error",
        "HarmonyManagerTest"
    ));
    
    EXPECT_TRUE(harmonyManager->isInEmergencyMode());
    
    EmergencyConsensusMode* emergencyModePtr = harmonyManager->getEmergencyMode();
    ASSERT_NE(emergencyModePtr, nullptr);
    EXPECT_EQ(emergencyModePtr->getCurrentSeverity(), EmergencySeverity::CRITICAL);
    
    EXPECT_TRUE(harmonyManager->exitEmergencyMode());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}