#include "../include/core/consensus_config_manager.h"
#include <iostream>

int main() {
    std::cout << "Testing ConsensusConfigManager basic instantiation...\n";
    
    try {
        // Test default configurations
        auto defaultConfig = ConsensusConfigManager::getDefaultConfiguration();
        std::cout << "✓ Default configuration created\n";
        std::cout << "  PoW Difficulty: " << defaultConfig.powDifficulty << "\n";
        std::cout << "  Min Stake: " << defaultConfig.minStakeAmount << "\n";
        
        auto testConfig = ConsensusConfigManager::getTestConfiguration();
        std::cout << "✓ Test configuration created\n";
        std::cout << "  PoW Difficulty: " << testConfig.powDifficulty << "\n";
        std::cout << "  Min Stake: " << testConfig.minStakeAmount << "\n";
        
        auto prodConfig = ConsensusConfigManager::getProductionConfiguration();
        std::cout << "✓ Production configuration created\n";
        std::cout << "  PoW Difficulty: " << prodConfig.powDifficulty << "\n";
        std::cout << "  Min Stake: " << prodConfig.minStakeAmount << "\n";
        
        // Test basic validation
        ConsensusConfigManager configManager("minimal_test_config.json", "minimal_test_backups");
        auto validationResult = configManager.validateConfiguration(defaultConfig);
        std::cout << "✓ Configuration validation: " << (validationResult.isValid ? "VALID" : "INVALID") << "\n";
        
        if (!validationResult.errors.empty()) {
            std::cout << "  Errors:\n";
            for (const auto& error : validationResult.errors) {
                std::cout << "    - " << error << "\n";
            }
        }
        
        std::cout << "\n🎉 Basic functionality test completed successfully!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}