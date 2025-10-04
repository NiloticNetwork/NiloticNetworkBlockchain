#include "../include/core/consensus_config_manager.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

class ConsensusConfigGovernanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        testConfigPath = "test_governance_config.json";
        testBackupDir = "test_governance_backups";
        
        // Clean up any existing test files
        std::filesystem::remove(testConfigPath);
        std::filesystem::remove_all(testBackupDir);
        
        configManager = std::make_unique<ConsensusConfigManager>(testConfigPath, testBackupDir);
    }
    
    void TearDown() override {
        configManager.reset();
        
        // Clean up test files
        std::filesystem::remove(testConfigPath);
        std::filesystem::remove_all(testBackupDir);
    }
    
    std::string testConfigPath;
    std::string testBackupDir;
    std::unique_ptr<ConsensusConfigManager> configManager;
};

// Test governance parameter validation
TEST_F(ConsensusConfigGovernanceTest, GovernanceParameterValidationTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Test valid governance parameters
    nlohmann::json validParams;
    validParams["powDifficulty"] = 5;
    validParams["minStakeAmount"] = 2000.0;
    validParams["supermajorityThreshold"] = 0.7;
    
    EXPECT_TRUE(configManager->validateGovernanceParameters(validParams));
    
    // Test invalid parameters
    nlohmann::json invalidParams;
    invalidParams["unknownParameter"] = 123;
    
    EXPECT_FALSE(configManager->validateGovernanceParameters(invalidParams));
    
    // Test parameter with invalid value
    nlohmann::json invalidValueParams;
    invalidValueParams["supermajorityThreshold"] = 1.5; // Invalid: > 1.0
    
    EXPECT_FALSE(configManager->validateGovernanceParameters(invalidValueParams));
}

// Test governance decision application
TEST_F(ConsensusConfigGovernanceTest, ApplyGovernanceDecisionTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Get initial configuration
    auto initialConfig = configManager->getConfiguration();
    
    // Create governance decision
    nlohmann::json governanceParams;
    governanceParams["powDifficulty"] = initialConfig.powDifficulty + 1;
    governanceParams["minStakeAmount"] = initialConfig.minStakeAmount * 1.5;
    governanceParams["supermajorityThreshold"] = 0.75;
    
    // Apply governance decision
    EXPECT_TRUE(configManager->applyGovernanceDecision("PROP_001", governanceParams, "test_governance"));
    
    // Verify changes were applied
    auto updatedConfig = configManager->getConfiguration();
    EXPECT_EQ(updatedConfig.powDifficulty, initialConfig.powDifficulty + 1);
    EXPECT_EQ(updatedConfig.minStakeAmount, initialConfig.minStakeAmount * 1.5);
    EXPECT_EQ(updatedConfig.supermajorityThreshold, 0.75);
    
    // Verify change log
    auto changeLog = configManager->getChangeLog();
    EXPECT_GE(changeLog.size(), 3); // At least 3 changes
    
    // Check that backup was created
    auto backupHistory = configManager->getBackupHistory();
    EXPECT_GE(backupHistory.size(), 2); // Initial + governance backup
}

// Test backward compatibility checking
TEST_F(ConsensusConfigGovernanceTest, BackwardCompatibilityTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto currentConfig = configManager->getConfiguration();
    
    // Test compatible changes
    ConsensusConfig compatibleConfig = currentConfig;
    compatibleConfig.powDifficulty = currentConfig.powDifficulty + 1; // Small increase
    compatibleConfig.minStakeAmount = currentConfig.minStakeAmount * 1.2; // 20% increase
    
    EXPECT_TRUE(configManager->ensureBackwardCompatibility(compatibleConfig));
    auto compatibilityIssues = configManager->getCompatibilityIssues(compatibleConfig);
    EXPECT_TRUE(compatibilityIssues.empty());
    
    // Test incompatible changes
    ConsensusConfig incompatibleConfig = currentConfig;
    incompatibleConfig.powDifficulty = currentConfig.powDifficulty * 3; // 200% increase
    incompatibleConfig.minStakeAmount = currentConfig.minStakeAmount * 10; // 900% increase
    incompatibleConfig.supermajorityThreshold = 0.95; // Very high threshold
    
    EXPECT_FALSE(configManager->ensureBackwardCompatibility(incompatibleConfig));
    auto issues = configManager->getCompatibilityIssues(incompatibleConfig);
    EXPECT_FALSE(issues.empty());
    EXPECT_GE(issues.size(), 3); // Should have multiple issues
}

// Test governance decision with compatibility issues
TEST_F(ConsensusConfigGovernanceTest, GovernanceDecisionCompatibilityTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto initialConfig = configManager->getConfiguration();
    
    // Create governance decision with compatibility issues
    nlohmann::json incompatibleParams;
    incompatibleParams["powDifficulty"] = initialConfig.powDifficulty * 5; // Too large increase
    incompatibleParams["minStakeAmount"] = initialConfig.minStakeAmount * 10; // Too large increase
    
    // Should fail due to compatibility issues
    EXPECT_FALSE(configManager->applyGovernanceDecision("PROP_002", incompatibleParams, "test_governance"));
    
    // Verify configuration wasn't changed
    auto currentConfig = configManager->getConfiguration();
    EXPECT_EQ(currentConfig.powDifficulty, initialConfig.powDifficulty);
    EXPECT_EQ(currentConfig.minStakeAmount, initialConfig.minStakeAmount);
}

// Test governance decision with invalid parameters
TEST_F(ConsensusConfigGovernanceTest, GovernanceDecisionInvalidParametersTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto initialConfig = configManager->getConfiguration();
    
    // Create governance decision with invalid parameters
    nlohmann::json invalidParams;
    invalidParams["supermajorityThreshold"] = 1.5; // Invalid: > 1.0
    invalidParams["powDifficulty"] = 0; // Invalid: < 1
    
    // Should fail due to invalid parameters
    EXPECT_FALSE(configManager->applyGovernanceDecision("PROP_003", invalidParams, "test_governance"));
    
    // Verify configuration wasn't changed
    auto currentConfig = configManager->getConfiguration();
    EXPECT_EQ(currentConfig.supermajorityThreshold, initialConfig.supermajorityThreshold);
    EXPECT_EQ(currentConfig.powDifficulty, initialConfig.powDifficulty);
}

// Test resource type compatibility
TEST_F(ConsensusConfigGovernanceTest, ResourceTypeCompatibilityTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto currentConfig = configManager->getConfiguration();
    
    // Test removing a resource type
    ConsensusConfig modifiedConfig = currentConfig;
    if (!modifiedConfig.acceptedResourceTypes.empty()) {
        modifiedConfig.acceptedResourceTypes.pop_back(); // Remove last resource type
        
        auto issues = configManager->getCompatibilityIssues(modifiedConfig);
        EXPECT_FALSE(issues.empty());
        
        // Should contain warning about removing resource type
        bool foundResourceTypeWarning = false;
        for (const auto& issue : issues) {
            if (issue.find("Removing resource type") != std::string::npos) {
                foundResourceTypeWarning = true;
                break;
            }
        }
        EXPECT_TRUE(foundResourceTypeWarning);
    }
    
    // Test adding a resource type (should be compatible)
    ConsensusConfig addedTypeConfig = currentConfig;
    addedTypeConfig.acceptedResourceTypes.push_back("NEW_RESOURCE_TYPE");
    
    auto addIssues = configManager->getCompatibilityIssues(addedTypeConfig);
    // Adding resource types should not cause compatibility issues
    bool hasResourceTypeIssue = false;
    for (const auto& issue : addIssues) {
        if (issue.find("resource type") != std::string::npos) {
            hasResourceTypeIssue = true;
            break;
        }
    }
    EXPECT_FALSE(hasResourceTypeIssue);
}

// Test governance decision with array parameters
TEST_F(ConsensusConfigGovernanceTest, GovernanceDecisionArrayParametersTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto initialConfig = configManager->getConfiguration();
    
    // Create governance decision with array parameter
    nlohmann::json arrayParams;
    arrayParams["acceptedResourceTypes"] = nlohmann::json::array({"COMPUTE", "STORAGE", "BANDWIDTH", "NETWORK"});
    
    // Apply governance decision
    EXPECT_TRUE(configManager->applyGovernanceDecision("PROP_004", arrayParams, "test_governance"));
    
    // Verify changes were applied
    auto updatedConfig = configManager->getConfiguration();
    EXPECT_EQ(updatedConfig.acceptedResourceTypes.size(), 4);
    EXPECT_TRUE(std::find(updatedConfig.acceptedResourceTypes.begin(), 
                         updatedConfig.acceptedResourceTypes.end(), "NETWORK") != 
                updatedConfig.acceptedResourceTypes.end());
}

// Test multiple governance decisions
TEST_F(ConsensusConfigGovernanceTest, MultipleGovernanceDecisionsTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto initialConfig = configManager->getConfiguration();
    
    // First governance decision
    nlohmann::json firstParams;
    firstParams["powDifficulty"] = initialConfig.powDifficulty + 1;
    
    EXPECT_TRUE(configManager->applyGovernanceDecision("PROP_005", firstParams, "test_governance"));
    
    auto configAfterFirst = configManager->getConfiguration();
    EXPECT_EQ(configAfterFirst.powDifficulty, initialConfig.powDifficulty + 1);
    
    // Second governance decision
    nlohmann::json secondParams;
    secondParams["minStakeAmount"] = initialConfig.minStakeAmount * 1.5;
    
    EXPECT_TRUE(configManager->applyGovernanceDecision("PROP_006", secondParams, "test_governance"));
    
    auto configAfterSecond = configManager->getConfiguration();
    EXPECT_EQ(configAfterSecond.powDifficulty, initialConfig.powDifficulty + 1); // Should remain
    EXPECT_EQ(configAfterSecond.minStakeAmount, initialConfig.minStakeAmount * 1.5);
    
    // Verify multiple backups were created
    auto backupHistory = configManager->getBackupHistory();
    EXPECT_GE(backupHistory.size(), 3); // Initial + 2 governance backups
    
    // Verify change log contains both decisions
    auto changeLog = configManager->getChangeLog();
    bool foundFirstDecision = false, foundSecondDecision = false;
    for (const auto& change : changeLog) {
        if (change.source.find("PROP_005") != std::string::npos) {
            foundFirstDecision = true;
        }
        if (change.source.find("PROP_006") != std::string::npos) {
            foundSecondDecision = true;
        }
    }
    EXPECT_TRUE(foundFirstDecision);
    EXPECT_TRUE(foundSecondDecision);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}