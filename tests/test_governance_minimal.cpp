#include "../include/core/consensus_config_manager.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing Consensus Configuration Governance (Minimal)...\n";
    
    try {
        // Test static configuration methods first
        std::cout << "1. Testing static configuration methods...\n";
        auto defaultConfig = ConsensusConfigManager::getDefaultConfiguration();
        auto testConfig = ConsensusConfigManager::getTestConfiguration();
        auto prodConfig = ConsensusConfigManager::getProductionConfiguration();
        
        std::cout << "✓ Static configuration methods work\n";
        std::cout << "   Default PoW Difficulty: " << defaultConfig.powDifficulty << "\n";
        std::cout << "   Test PoW Difficulty: " << testConfig.powDifficulty << "\n";
        std::cout << "   Production PoW Difficulty: " << prodConfig.powDifficulty << "\n";
        
        // Test JSON serialization
        std::cout << "\n2. Testing JSON serialization...\n";
        auto json = defaultConfig.toJson();
        std::cout << "✓ Configuration serialized to JSON\n";
        
        ConsensusConfig deserializedConfig;
        deserializedConfig.fromJson(json);
        std::cout << "✓ Configuration deserialized from JSON\n";
        
        assert(deserializedConfig.powDifficulty == defaultConfig.powDifficulty);
        assert(deserializedConfig.minStakeAmount == defaultConfig.minStakeAmount);
        std::cout << "✓ Serialization/deserialization verified\n";
        
        std::cout << "\nAll minimal governance tests passed! ✓\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}