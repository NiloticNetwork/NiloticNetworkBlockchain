#include "../include/core/consensus_config_manager.h"
#include "../include/core/consensus_harmony_manager.h"
#include <gtest/gtest.h>
#include <filesystem>

class ConsensusConfigIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        testConfigPath = "integration_test_config.json";
        testBackupDir = "integration_test_backups";
        
        // Clean up any existing test files
        std::filesystem::remove(testConfigPath);
        std::filesystem::remove_all(testBackupDir);
        
        configManager = std::make_unique<ConsensusConfigManager>(testConfigPath, testBackupDir);
        harmonyManager = std::make_unique<ConsensusHarmonyManager>();
    }
    
    void TearDown() override {
        harmonyManager.reset();
        configManager.reset();
        
        // Clean up test files
        std::filesystem::remove(testConfigPath);
        std::filesystem::remove_all(testBackupDir);
    }
    
    std::string testConfigPath;
    std::string testBackupDir;
    std::unique_ptr<ConsensusConfigManager> configManager;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
};

// Test integration between config manager and harmony manager
TEST_F(ConsensusConfigIntegrationTest, BasicIntegrationTest) {
    // Initialize config manager
    ASSERT_TRUE(configManager->initialize());
    
    // Get configuration and verify it's valid
    auto config = configManager->getConfiguration();
    auto validationResult = configManager->validateConfiguration(config);
    EXPECT_TRUE(validationResult.isValid);
    
    // Initialize harmony manager with the configuration
    EXPECT_TRUE(harmonyManager->initializeConsensus(config));
    
    // Verify harmony manager is initialized
    EXPECT_TRUE(harmonyManager->isInitialized());
}

// Test configuration updates affecting harmony manager
TEST_F(ConsensusConfigIntegrationTest, ConfigurationUpdateTest) {
    // Initialize both managers
    ASSERT_TRUE(configManager->initialize());
    auto initialConfig = configManager->getConfiguration();
    ASSERT_TRUE(harmonyManager->initializeConsensus(initialConfig));
    
    // Update configuration through config manager
    EXPECT_TRUE(configManager->setParameter("difficulty", static_cast<uint64_t>(6), 
                                           ConsensusType::PROOF_OF_WORK));
    
    // Get updated configuration
    auto updatedConfig = configManager->getConfiguration();
    EXPECT_EQ(updatedConfig.powDifficulty, 6);
    
    // Update harmony manager with new configuration
    EXPECT_TRUE(harmonyManager->updateConfiguration(updatedConfig));
    
    // Verify the update took effect
    auto harmonyConfig = harmonyManager->getConfiguration();
    EXPECT_EQ(harmonyConfig.powDifficulty, 6);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}