#include "../include/core/consensus_config_manager.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

class ConsensusConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        testConfigPath = "test_config.json";
        testBackupDir = "test_backups";
        
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

// Test initialization
TEST_F(ConsensusConfigManagerTest, InitializationTest) {
    EXPECT_TRUE(configManager->initialize());
    
    // Check that default configuration is loaded
    auto config = configManager->getConfiguration();
    EXPECT_GT(config.powDifficulty, 0);
    EXPECT_GT(config.powTargetBlockTime, 0);
    EXPECT_GT(config.minStakeAmount, 0);
    
    // Check that config file was created
    EXPECT_TRUE(std::filesystem::exists(testConfigPath));
    
    // Check that backup directory was created
    EXPECT_TRUE(std::filesystem::exists(testBackupDir));
}

TEST_F(ConsensusConfigManagerTest, InitializationWithCustomConfigTest) {
    ConsensusConfig customConfig = ConsensusConfigManager::getTestConfiguration();
    customConfig.powDifficulty = 5;
    customConfig.minStakeAmount = 500.0;
    
    EXPECT_TRUE(configManager->initialize(customConfig));
    
    auto loadedConfig = configManager->getConfiguration();
    EXPECT_EQ(loadedConfig.powDifficulty, 5);
    EXPECT_EQ(loadedConfig.minStakeAmount, 500.0);
}

// Test configuration validation
TEST_F(ConsensusConfigManagerTest, ConfigurationValidationTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Test valid configuration
    ConsensusConfig validConfig = ConsensusConfigManager::getDefaultConfiguration();
    auto result = configManager->validateConfiguration(validConfig);
    EXPECT_TRUE(result.isValid);
    EXPECT_TRUE(result.errors.empty());
    
    // Test invalid configuration
    ConsensusConfig invalidConfig = validConfig;
    invalidConfig.powDifficulty = 0; // Invalid difficulty
    invalidConfig.supermajorityThreshold = 1.5; // Invalid threshold
    
    result = configManager->validateConfiguration(invalidConfig);
    EXPECT_FALSE(result.isValid);
    EXPECT_FALSE(result.errors.empty());
}

// Test parameter validation
TEST_F(ConsensusConfigManagerTest, ParameterValidationTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Test valid parameters
    auto result = configManager->validateParameter("difficulty", 4.0, ConsensusType::PROOF_OF_WORK);
    EXPECT_TRUE(result.isValid);
    
    // Test invalid parameters
    result = configManager->validateParameter("difficulty", 0.0, ConsensusType::PROOF_OF_WORK);
    EXPECT_FALSE(result.isValid);
    
    result = configManager->validateParameter("supermajorityThreshold", 1.5, ConsensusType::VOTING_CONSENSUS);
    EXPECT_FALSE(result.isValid);
}

// Test parameter setting and getting
TEST_F(ConsensusConfigManagerTest, ParameterSetGetTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Test setting PoW difficulty
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(6), 
                                           ConsensusType::PROOF_OF_WORK));
    
    auto config = configManager->getConfiguration();
    EXPECT_EQ(config.powDifficulty, 6);
    
    // Test setting PoS minimum stake
    EXPECT_TRUE(configManager->setParameter("minStakeAmount", 2000.0, 
                                           ConsensusType::PROOF_OF_STAKE));
    
    config = configManager->getConfiguration();
    EXPECT_EQ(config.minStakeAmount, 2000.0);
    
    // Test setting voting threshold
    EXPECT_TRUE(configManager->setParameter("supermajorityThreshold", 0.75, 
                                           ConsensusType::VOTING_CONSENSUS));
    
    config = configManager->getConfiguration();
    EXPECT_EQ(config.supermajorityThreshold, 0.75);
}

// Test safety bounds
TEST_F(ConsensusConfigManagerTest, SafetyBoundsTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Set custom safety bounds
    ConfigBounds bounds(2.0, 10.0, true, "Test bounds");
    EXPECT_TRUE(configManager->setSafetyBounds("difficulty", ConsensusType::PROOF_OF_WORK, bounds));
    
    // Test parameter within bounds
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(5), 
                                           ConsensusType::PROOF_OF_WORK));
    
    // Test parameter outside bounds (should fail)
    EXPECT_FALSE(configManager->setParameter("difficulty", static_cast<uint64_t>(15), 
                                            ConsensusType::PROOF_OF_WORK));
    
    // Get and verify bounds
    auto retrievedBounds = configManager->getSafetyBounds("difficulty", ConsensusType::PROOF_OF_WORK);
    EXPECT_EQ(retrievedBounds.minValue, 2.0);
    EXPECT_EQ(retrievedBounds.maxValue, 10.0);
}

// Test configuration persistence
TEST_F(ConsensusConfigManagerTest, ConfigurationPersistenceTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Modify configuration
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(7), 
                                           ConsensusType::PROOF_OF_WORK));
    EXPECT_TRUE(configManager->setParameter("minStakeAmount", 1500.0, 
                                           ConsensusType::PROOF_OF_STAKE));
    
    // Save configuration
    EXPECT_TRUE(configManager->saveConfiguration());
    
    // Create new manager and load configuration
    auto newManager = std::make_unique<ConsensusConfigManager>(testConfigPath, testBackupDir);
    EXPECT_TRUE(newManager->initialize());
    
    auto loadedConfig = newManager->getConfiguration();
    EXPECT_EQ(loadedConfig.powDifficulty, 7);
    EXPECT_EQ(loadedConfig.minStakeAmount, 1500.0);
}

// Test backup and restore functionality
TEST_F(ConsensusConfigManagerTest, BackupRestoreTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Get initial configuration
    auto initialConfig = configManager->getConfiguration();
    
    // Create backup
    EXPECT_TRUE(configManager->createBackup("test_backup"));
    
    // Modify configuration
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(8), 
                                           ConsensusType::PROOF_OF_WORK));
    
    auto modifiedConfig = configManager->getConfiguration();
    EXPECT_NE(modifiedConfig.powDifficulty, initialConfig.powDifficulty);
    
    // Restore from backup
    auto backups = configManager->getBackupHistory();
    EXPECT_FALSE(backups.empty());
    
    EXPECT_TRUE(configManager->restoreFromBackup(0));
    
    auto restoredConfig = configManager->getConfiguration();
    EXPECT_EQ(restoredConfig.powDifficulty, initialConfig.powDifficulty);
}

// Test change tracking
TEST_F(ConsensusConfigManagerTest, ChangeTrackingTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Make some changes
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(6), 
                                           ConsensusType::PROOF_OF_WORK, "test_source"));
    EXPECT_TRUE(configManager->setParameter("minStakeAmount", 2000.0, 
                                           ConsensusType::PROOF_OF_STAKE, "test_source"));
    
    // Check change log
    auto changeLog = configManager->getChangeLog();
    EXPECT_GE(changeLog.size(), 2);
    
    // Check specific changes
    bool foundDifficultyChange = false;
    bool foundStakeChange = false;
    
    for (const auto& change : changeLog) {
        if (change.parameter == "difficulty" && change.source == "test_source") {
            foundDifficultyChange = true;
        }
        if (change.parameter == "minStakeAmount" && change.source == "test_source") {
            foundStakeChange = true;
        }
    }
    
    EXPECT_TRUE(foundDifficultyChange);
    EXPECT_TRUE(foundStakeChange);
}

// Test change callbacks
TEST_F(ConsensusConfigManagerTest, ChangeCallbackTest) {
    ASSERT_TRUE(configManager->initialize());
    
    bool callbackCalled = false;
    std::string changedParameter;
    
    // Register callback
    configManager->registerChangeCallback([&](const ConfigChangeEvent& event) {
        callbackCalled = true;
        changedParameter = event.parameter;
    });
    
    // Make a change
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(5), 
                                           ConsensusType::PROOF_OF_WORK));
    
    // Verify callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(changedParameter, "difficulty");
}

// Test configuration export/import
TEST_F(ConsensusConfigManagerTest, ExportImportTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Modify configuration
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(9), 
                                           ConsensusType::PROOF_OF_WORK));
    EXPECT_TRUE(configManager->setParameter("minStakeAmount", 3000.0, 
                                           ConsensusType::PROOF_OF_STAKE));
    
    // Export configuration
    auto exportedJson = configManager->exportConfiguration();
    EXPECT_FALSE(exportedJson.empty());
    
    // Create new manager and import
    auto newManager = std::make_unique<ConsensusConfigManager>("test_config2.json", "test_backups2");
    EXPECT_TRUE(newManager->initialize());
    EXPECT_TRUE(newManager->importConfiguration(exportedJson, "import_test"));
    
    auto importedConfig = newManager->getConfiguration();
    EXPECT_EQ(importedConfig.powDifficulty, 9);
    EXPECT_EQ(importedConfig.minStakeAmount, 3000.0);
    
    // Clean up
    std::filesystem::remove("test_config2.json");
    std::filesystem::remove_all("test_backups2");
}

// Test configuration comparison
TEST_F(ConsensusConfigManagerTest, ConfigurationComparisonTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto config1 = ConsensusConfigManager::getDefaultConfiguration();
    auto config2 = config1;
    config2.powDifficulty = 10;
    config2.minStakeAmount = 5000.0;
    
    auto differences = configManager->compareConfigurations(config1, config2);
    EXPECT_EQ(differences.size(), 2);
    
    bool foundDifficultyDiff = false;
    bool foundStakeDiff = false;
    
    for (const auto& diff : differences) {
        if (diff.find("powDifficulty") != std::string::npos) {
            foundDifficultyDiff = true;
        }
        if (diff.find("minStakeAmount") != std::string::npos) {
            foundStakeDiff = true;
        }
    }
    
    EXPECT_TRUE(foundDifficultyDiff);
    EXPECT_TRUE(foundStakeDiff);
}

// Test default configurations
TEST_F(ConsensusConfigManagerTest, DefaultConfigurationsTest) {
    auto defaultConfig = ConsensusConfigManager::getDefaultConfiguration();
    auto testConfig = ConsensusConfigManager::getTestConfiguration();
    auto prodConfig = ConsensusConfigManager::getProductionConfiguration();
    
    // Verify they are different
    EXPECT_NE(defaultConfig.powDifficulty, testConfig.powDifficulty);
    EXPECT_NE(testConfig.powDifficulty, prodConfig.powDifficulty);
    
    // Verify test config has lower values (easier for testing)
    EXPECT_LT(testConfig.powDifficulty, defaultConfig.powDifficulty);
    EXPECT_LT(testConfig.minStakeAmount, defaultConfig.minStakeAmount);
    
    // Verify production config has higher security values
    EXPECT_GE(prodConfig.powDifficulty, defaultConfig.powDifficulty);
    EXPECT_GE(prodConfig.minStakeAmount, defaultConfig.minStakeAmount);
}

// Test self-test functionality
TEST_F(ConsensusConfigManagerTest, SelfTestTest) {
    ASSERT_TRUE(configManager->initialize());
    EXPECT_TRUE(configManager->performSelfTest());
}

// Test status and diagnostics
TEST_F(ConsensusConfigManagerTest, StatusDiagnosticsTest) {
    ASSERT_TRUE(configManager->initialize());
    
    auto status = configManager->getStatus();
    EXPECT_TRUE(status["initialized"].get<bool>());
    EXPECT_EQ(status["config_file"].get<std::string>(), testConfigPath);
    
    auto diagnostics = configManager->getDiagnostics();
    EXPECT_TRUE(diagnostics["configuration_valid"].get<bool>());
    EXPECT_TRUE(diagnostics["config_file_exists"].get<bool>());
}

// Test parameter registry
TEST_F(ConsensusConfigManagerTest, ParameterRegistryTest) {
    ASSERT_TRUE(configManager->initialize());
    
    // Get all registered parameters
    auto allParams = configManager->getRegisteredParameters();
    EXPECT_FALSE(allParams.empty());
    
    // Get PoW-specific parameters
    auto powParams = configManager->getRegisteredParameters(ConsensusType::PROOF_OF_WORK);
    EXPECT_FALSE(powParams.empty());
    
    // Register custom parameter
    ConfigParameter customParam("testParam", "double", ConfigBounds(0, 100), "test", "Test parameter");
    EXPECT_TRUE(configManager->registerParameter(customParam));
    
    auto updatedParams = configManager->getRegisteredParameters();
    EXPECT_GT(updatedParams.size(), allParams.size());
    
    // Unregister parameter
    EXPECT_TRUE(configManager->unregisterParameter("testParam"));
    
    auto finalParams = configManager->getRegisteredParameters();
    EXPECT_EQ(finalParams.size(), allParams.size());
}

// Test concurrent access
TEST_F(ConsensusConfigManagerTest, ConcurrentAccessTest) {
    ASSERT_TRUE(configManager->initialize());
    
    const int numThreads = 10;
    const int operationsPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    // Launch threads that perform concurrent operations
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                // Alternate between different operations
                if (j % 3 == 0) {
                    if (configManager->setParameter("difficulty", static_cast<uint64_t>(4 + (i % 5)), 
                                                   ConsensusType::PROOF_OF_WORK)) {
                        successCount++;
                    }
                } else if (j % 3 == 1) {
                    auto config = configManager->getConfiguration();
                    if (config.powDifficulty > 0) {
                        successCount++;
                    }
                } else {
                    if (configManager->createBackup("concurrent_test_" + std::to_string(i))) {
                        successCount++;
                    }
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify that most operations succeeded (some may fail due to validation)
    EXPECT_GT(successCount.load(), numThreads * operationsPerThread / 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}