#include "../include/core/consensus_config_manager.h"
#include <iostream>
#include <cassert>
#include <filesystem>

int main() {
    std::cout << "Testing Consensus Configuration Governance Integration...\n";
    std::cout << "========================================================\n\n";
    
    try {
        // Clean up any existing test files
        std::filesystem::remove("test_governance_integration.json");
        std::filesystem::remove_all("test_governance_integration_backups");
        
        // Create configuration manager
        ConsensusConfigManager configManager("test_governance_integration.json", 
                                            "test_governance_integration_backups");
        
        // Test initialization
        std::cout << "1. Testing initialization...\n";
        assert(configManager.initialize());
        std::cout << "✓ Configuration manager initialized\n\n";
        
        // Get initial configuration
        auto initialConfig = configManager.getConfiguration();
        std::cout << "2. Initial configuration:\n";
        std::cout << "   PoW Difficulty: " << initialConfig.powDifficulty << "\n";
        std::cout << "   Min Stake: " << initialConfig.minStakeAmount << "\n";
        std::cout << "   Supermajority Threshold: " << initialConfig.supermajorityThreshold << "\n\n";
        
        // Test governance parameter validation
        std::cout << "3. Testing governance parameter validation...\n";
        nlohmann::json validParams;
        validParams["powDifficulty"] = 5;
        validParams["minStakeAmount"] = 2000.0;
        validParams["supermajorityThreshold"] = 0.7;
        
        assert(configManager.validateGovernanceParameters(validParams));
        std::cout << "✓ Valid governance parameters accepted\n";
        
        nlohmann::json invalidParams;
        invalidParams["unknownParameter"] = 123;
        
        assert(!configManager.validateGovernanceParameters(invalidParams));
        std::cout << "✓ Invalid governance parameters rejected\n\n";
        
        // Test backward compatibility checking
        std::cout << "4. Testing backward compatibility...\n";
        ConsensusConfig compatibleConfig = initialConfig;
        compatibleConfig.powDifficulty = initialConfig.powDifficulty + 1;
        
        assert(configManager.ensureBackwardCompatibility(compatibleConfig));
        std::cout << "✓ Compatible changes accepted\n";
        
        ConsensusConfig incompatibleConfig = initialConfig;
        incompatibleConfig.powDifficulty = initialConfig.powDifficulty * 5;
        
        assert(!configManager.ensureBackwardCompatibility(incompatibleConfig));
        auto issues = configManager.getCompatibilityIssues(incompatibleConfig);
        std::cout << "✓ Incompatible changes rejected (" << issues.size() << " issues found)\n";
        for (const auto& issue : issues) {
            std::cout << "   - " << issue << "\n";
        }
        std::cout << "\n";
        
        // Test applying governance decision
        std::cout << "5. Testing governance decision application...\n";
        nlohmann::json governanceParams;
        governanceParams["powDifficulty"] = initialConfig.powDifficulty + 1;
        governanceParams["minStakeAmount"] = initialConfig.minStakeAmount * 1.2;
        
        assert(configManager.applyGovernanceDecision("PROP_TEST_001", governanceParams, "test"));
        std::cout << "✓ Governance decision applied successfully\n";
        
        // Verify changes
        auto updatedConfig = configManager.getConfiguration();
        assert(updatedConfig.powDifficulty == initialConfig.powDifficulty + 1);
        assert(updatedConfig.minStakeAmount == initialConfig.minStakeAmount * 1.2);
        std::cout << "✓ Configuration changes verified\n";
        std::cout << "   New PoW Difficulty: " << updatedConfig.powDifficulty << "\n";
        std::cout << "   New Min Stake: " << updatedConfig.minStakeAmount << "\n\n";
        
        // Test change tracking
        std::cout << "6. Testing change tracking...\n";
        auto changeLog = configManager.getChangeLog();
        assert(!changeLog.empty());
        std::cout << "✓ Change log contains " << changeLog.size() << " entries\n";
        
        // Find governance-related changes
        int governanceChanges = 0;
        for (const auto& change : changeLog) {
            if (change.source.find("PROP_TEST_001") != std::string::npos) {
                governanceChanges++;
                std::cout << "   - " << change.parameter << ": " << change.oldValue 
                         << " -> " << change.newValue << " (source: " << change.source << ")\n";
            }
        }
        assert(governanceChanges > 0);
        std::cout << "✓ Found " << governanceChanges << " governance-related changes\n\n";
        
        // Test backup creation
        std::cout << "7. Testing backup functionality...\n";
        auto backupHistory = configManager.getBackupHistory();
        assert(backupHistory.size() >= 2); // Initial + governance backup
        std::cout << "✓ " << backupHistory.size() << " backups created\n";
        
        // Find governance backup
        bool foundGovernanceBackup = false;
        for (const auto& backup : backupHistory) {
            if (backup.reason.find("governance") != std::string::npos) {
                foundGovernanceBackup = true;
                std::cout << "   - Governance backup: " << backup.reason << "\n";
                break;
            }
        }
        assert(foundGovernanceBackup);
        std::cout << "✓ Governance backup found\n\n";
        
        // Test configuration persistence
        std::cout << "8. Testing configuration persistence...\n";
        assert(configManager.saveConfiguration());
        std::cout << "✓ Configuration saved successfully\n";
        
        // Create new manager and load configuration
        ConsensusConfigManager newManager("test_governance_integration.json", 
                                         "test_governance_integration_backups");
        assert(newManager.initialize());
        
        auto loadedConfig = newManager.getConfiguration();
        assert(loadedConfig.powDifficulty == updatedConfig.powDifficulty);
        assert(loadedConfig.minStakeAmount == updatedConfig.minStakeAmount);
        std::cout << "✓ Configuration loaded correctly after restart\n\n";
        
        // Test failed governance decision (incompatible)
        std::cout << "9. Testing failed governance decision...\n";
        nlohmann::json incompatibleParams;
        incompatibleParams["powDifficulty"] = initialConfig.powDifficulty * 10; // Too large
        
        assert(!configManager.applyGovernanceDecision("PROP_TEST_002", incompatibleParams, "test"));
        std::cout << "✓ Incompatible governance decision rejected\n";
        
        // Verify configuration wasn't changed
        auto unchangedConfig = configManager.getConfiguration();
        assert(unchangedConfig.powDifficulty == updatedConfig.powDifficulty);
        std::cout << "✓ Configuration remained unchanged after failed decision\n\n";
        
        // Test array parameter governance
        std::cout << "10. Testing array parameter governance...\n";
        nlohmann::json arrayParams;
        arrayParams["acceptedResourceTypes"] = nlohmann::json::array({"COMPUTE", "STORAGE", "BANDWIDTH", "NETWORK"});
        
        assert(configManager.applyGovernanceDecision("PROP_TEST_003", arrayParams, "test"));
        std::cout << "✓ Array parameter governance decision applied\n";
        
        auto arrayConfig = configManager.getConfiguration();
        assert(arrayConfig.acceptedResourceTypes.size() == 4);
        assert(std::find(arrayConfig.acceptedResourceTypes.begin(), 
                        arrayConfig.acceptedResourceTypes.end(), "NETWORK") != 
               arrayConfig.acceptedResourceTypes.end());
        std::cout << "✓ Array parameter updated correctly\n";
        std::cout << "   Resource types: ";
        for (const auto& type : arrayConfig.acceptedResourceTypes) {
            std::cout << type << " ";
        }
        std::cout << "\n\n";
        
        // Clean up test files
        std::filesystem::remove("test_governance_integration.json");
        std::filesystem::remove_all("test_governance_integration_backups");
        
        std::cout << "All governance integration tests passed! ✓\n";
        std::cout << "=========================================\n\n";
        
        std::cout << "Summary of implemented governance features:\n";
        std::cout << "- Governance parameter validation\n";
        std::cout << "- Backward compatibility checking\n";
        std::cout << "- Automatic parameter updates from governance decisions\n";
        std::cout << "- Change tracking and audit logging\n";
        std::cout << "- Configuration backup before governance changes\n";
        std::cout << "- Support for all parameter types (numeric, string, array)\n";
        std::cout << "- Rejection of incompatible changes\n";
        std::cout << "- Persistence of governance-driven configuration changes\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}