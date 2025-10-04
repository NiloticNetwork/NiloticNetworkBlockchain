#include "../include/core/consensus_config_manager.h"
#include <iostream>
#include <cassert>
#include <filesystem>

void testBasicFunctionality() {
    std::cout << "Testing basic configuration manager functionality...\n";
    
    // Clean up any existing test files
    std::filesystem::remove("test_config.json");
    std::filesystem::remove_all("test_backups");
    
    // Create configuration manager
    ConsensusConfigManager configManager("test_config.json", "test_backups");
    
    // Test initialization
    assert(configManager.initialize());
    std::cout << "✓ Initialization successful\n";
    
    // Test getting configuration
    auto config = configManager.getConfiguration();
    assert(config.powDifficulty > 0);
    assert(config.minStakeAmount > 0);
    std::cout << "✓ Configuration retrieval successful\n";
    
    // Test parameter setting
    assert(configManager.setParameter("difficulty", static_cast<uint64_t>(6), 
                                     ConsensusType::PROOF_OF_WORK));
    
    auto updatedConfig = configManager.getConfiguration();
    assert(updatedConfig.powDifficulty == 6);
    std::cout << "✓ Parameter setting successful\n";
    
    // Test validation
    auto validationResult = configManager.validateConfiguration(config);
    assert(validationResult.isValid);
    std::cout << "✓ Configuration validation successful\n";
    
    // Test invalid parameter (should fail)
    assert(!configManager.setParameter("difficulty", static_cast<uint64_t>(0), 
                                      ConsensusType::PROOF_OF_WORK));
    std::cout << "✓ Invalid parameter correctly rejected\n";
    
    // Test backup creation
    assert(configManager.createBackup("test_backup"));
    std::cout << "✓ Backup creation successful\n";
    
    // Test configuration persistence
    assert(configManager.saveConfiguration());
    std::cout << "✓ Configuration saving successful\n";
    
    // Test self-test
    assert(configManager.performSelfTest());
    std::cout << "✓ Self-test successful\n";
    
    // Clean up test files
    std::filesystem::remove("test_config.json");
    std::filesystem::remove_all("test_backups");
    
    std::cout << "All basic functionality tests passed!\n\n";
}

void testDefaultConfigurations() {
    std::cout << "Testing default configurations...\n";
    
    auto defaultConfig = ConsensusConfigManager::getDefaultConfiguration();
    auto testConfig = ConsensusConfigManager::getTestConfiguration();
    auto prodConfig = ConsensusConfigManager::getProductionConfiguration();
    
    // Verify they are different
    assert(defaultConfig.powDifficulty != testConfig.powDifficulty);
    assert(testConfig.powDifficulty != prodConfig.powDifficulty);
    std::cout << "✓ Default configurations are distinct\n";
    
    // Verify test config has lower values (easier for testing)
    assert(testConfig.powDifficulty < defaultConfig.powDifficulty);
    assert(testConfig.minStakeAmount < defaultConfig.minStakeAmount);
    std::cout << "✓ Test configuration has appropriate values\n";
    
    // Verify production config has higher security values
    assert(prodConfig.powDifficulty >= defaultConfig.powDifficulty);
    assert(prodConfig.minStakeAmount >= defaultConfig.minStakeAmount);
    std::cout << "✓ Production configuration has appropriate values\n";
    
    std::cout << "Default configuration tests passed!\n\n";
}

void testParameterValidation() {
    std::cout << "Testing parameter validation...\n";
    
    ConsensusConfigManager configManager("validation_test_config.json", "validation_test_backups");
    assert(configManager.initialize());
    
    // Test valid parameters
    auto result = configManager.validateParameter("difficulty", 4.0, ConsensusType::PROOF_OF_WORK);
    assert(result.isValid);
    std::cout << "✓ Valid parameter correctly accepted\n";
    
    // Test invalid parameters
    result = configManager.validateParameter("difficulty", 0.0, ConsensusType::PROOF_OF_WORK);
    assert(!result.isValid);
    std::cout << "✓ Invalid parameter correctly rejected\n";
    
    result = configManager.validateParameter("supermajorityThreshold", 1.5, ConsensusType::VOTING_CONSENSUS);
    assert(!result.isValid);
    std::cout << "✓ Invalid voting threshold correctly rejected\n";
    
    // Clean up
    std::filesystem::remove("validation_test_config.json");
    std::filesystem::remove_all("validation_test_backups");
    
    std::cout << "Parameter validation tests passed!\n\n";
}

void testConfigurationComparison() {
    std::cout << "Testing configuration comparison...\n";
    
    ConsensusConfigManager configManager("comparison_test_config.json", "comparison_test_backups");
    assert(configManager.initialize());
    
    auto config1 = ConsensusConfigManager::getDefaultConfiguration();
    auto config2 = config1;
    config2.powDifficulty = 10;
    config2.minStakeAmount = 5000.0;
    
    auto differences = configManager.compareConfigurations(config1, config2);
    assert(differences.size() == 2);
    std::cout << "✓ Configuration comparison detected correct number of differences\n";
    
    // Clean up
    std::filesystem::remove("comparison_test_config.json");
    std::filesystem::remove_all("comparison_test_backups");
    
    std::cout << "Configuration comparison tests passed!\n\n";
}

int main() {
    std::cout << "Consensus Configuration Manager Simple Tests\n";
    std::cout << "==========================================\n\n";
    
    try {
        testBasicFunctionality();
        testDefaultConfigurations();
        testParameterValidation();
        testConfigurationComparison();
        
        std::cout << "🎉 All tests passed successfully!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception\n";
        return 1;
    }
}