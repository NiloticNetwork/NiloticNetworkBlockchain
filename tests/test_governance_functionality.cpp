#include "../include/core/consensus_config_manager.h"
#include <iostream>
#include <cassert>
#include <filesystem>

int main() {
    std::cout << "Testing Consensus Configuration Governance Functionality...\n";
    std::cout << "==========================================================\n\n";
    
    try {
        // Clean up any existing test files
        std::filesystem::remove("test_governance_func.json");
        std::filesystem::remove_all("test_governance_func_backups");
        
        // Create directories
        std::filesystem::create_directories("test_governance_func_backups");
        
        // Create configuration manager with test configuration
        ConsensusConfigManager configManager("test_governance_func.json", 
                                            "test_governance_func_backups");
        
        // Initialize with test configuration to avoid hanging
        auto testConfig = ConsensusConfigManager::getTestConfiguration();
        std::cout << "1. Initializing with test configuration...\n";
        assert(configManager.initialize(testConfig));
        std::cout << "✓ Configuration manager initialized with test config\n";
        std::cout << "   Initial PoW Difficulty: " << testConfig.powDifficulty << "\n";
        std::cout << "   Initial Min Stake: " << testConfig.minStakeAmount << "\n\n";
        
        // Test governance parameter validation
        std::cout << "2. Testing governance parameter validation...\n";
        
        // Valid parameters
        nlohmann::json validParams;
        validParams["powDifficulty"] = 2;
        validParams["minStakeAmount"] = 50.0;
        validParams["supermajorityThreshold"] = 0.65;
        
        assert(configManager.validateGovernanceParameters(validParams));
        std::cout << "✓ Valid governance parameters accepted\n";
        
        // Invalid parameter name
        nlohmann::json invalidNameParams;
        invalidNameParams["unknownParameter"] = 123;
        
        assert(!configManager.validateGovernanceParameters(invalidNameParams));
        std::cout << "✓ Invalid parameter name rejected\n";
        
        // Invalid parameter value
        nlohmann::json invalidValueParams;
        invalidValueParams["supermajorityThreshold"] = 1.5; // > 1.0
        
        assert(!configManager.validateGovernanceParameters(invalidValueParams));
        std::cout << "✓ Invalid parameter value rejected\n\n";
        
        // Test backward compatibility checking
        std::cout << "3. Testing backward compatibility checking...\n";
        
        auto currentConfig = configManager.getConfiguration();
        
        // Compatible change (small increase)
        ConsensusConfig compatibleConfig = currentConfig;
        compatibleConfig.powDifficulty = currentConfig.powDifficulty + 1;
        compatibleConfig.minStakeAmount = currentConfig.minStakeAmount * 1.1;
        
        assert(configManager.ensureBackwardCompatibility(compatibleConfig));
        auto compatIssues = configManager.getCompatibilityIssues(compatibleConfig);
        std::cout << "✓ Compatible changes accepted (" << compatIssues.size() << " issues)\n";
        
        // Incompatible change (large increase)
        ConsensusConfig incompatibleConfig = currentConfig;
        incompatibleConfig.powDifficulty = currentConfig.powDifficulty * 5; // 400% increase
        incompatibleConfig.minStakeAmount = currentConfig.minStakeAmount * 10; // 900% increase
        
        assert(!configManager.ensureBackwardCompatibility(incompatibleConfig));
        auto incompatIssues = configManager.getCompatibilityIssues(incompatibleConfig);
        std::cout << "✓ Incompatible changes rejected (" << incompatIssues.size() << " issues)\n";
        
        for (size_t i = 0; i < std::min(incompatIssues.size(), size_t(3)); ++i) {
            std::cout << "   - " << incompatIssues[i] << "\n";
        }
        std::cout << "\n";
        
        // Test successful governance decision application
        std::cout << "4. Testing successful governance decision...\n";
        
        nlohmann::json governanceParams;
        governanceParams["powDifficulty"] = currentConfig.powDifficulty + 1;
        governanceParams["minStakeAmount"] = currentConfig.minStakeAmount * 1.2;
        governanceParams["supermajorityThreshold"] = 0.65;
        
        assert(configManager.applyGovernanceDecision("PROP_001", governanceParams, "test_governance"));
        std::cout << "✓ Governance decision applied successfully\n";
        
        // Verify changes
        auto updatedConfig = configManager.getConfiguration();
        assert(updatedConfig.powDifficulty == currentConfig.powDifficulty + 1);
        assert(updatedConfig.minStakeAmount == currentConfig.minStakeAmount * 1.2);
        assert(updatedConfig.supermajorityThreshold == 0.65);
        
        std::cout << "✓ Configuration changes verified:\n";
        std::cout << "   PoW Difficulty: " << currentConfig.powDifficulty << " -> " << updatedConfig.powDifficulty << "\n";
        std::cout << "   Min Stake: " << currentConfig.minStakeAmount << " -> " << updatedConfig.minStakeAmount << "\n";
        std::cout << "   Supermajority: " << currentConfig.supermajorityThreshold << " -> " << updatedConfig.supermajorityThreshold << "\n\n";
        
        // Test failed governance decision (incompatible)
        std::cout << "5. Testing failed governance decision (incompatible)...\n";
        
        nlohmann::json incompatibleGovParams;
        incompatibleGovParams["powDifficulty"] = updatedConfig.powDifficulty * 10; // Too large
        
        assert(!configManager.applyGovernanceDecision("PROP_002", incompatibleGovParams, "test_governance"));
        std::cout << "✓ Incompatible governance decision rejected\n";
        
        // Verify configuration unchanged
        auto unchangedConfig = configManager.getConfiguration();
        assert(unchangedConfig.powDifficulty == updatedConfig.powDifficulty);
        std::cout << "✓ Configuration remained unchanged after failed decision\n\n";
        
        // Test failed governance decision (invalid parameters)
        std::cout << "6. Testing failed governance decision (invalid parameters)...\n";
        
        nlohmann::json invalidGovParams;
        invalidGovParams["supermajorityThreshold"] = 1.5; // Invalid value
        
        assert(!configManager.applyGovernanceDecision("PROP_003", invalidGovParams, "test_governance"));
        std::cout << "✓ Invalid governance parameters rejected\n\n";
        
        // Test array parameter governance
        std::cout << "7. Testing array parameter governance...\n";
        
        nlohmann::json arrayParams;
        arrayParams["acceptedResourceTypes"] = nlohmann::json::array({"COMPUTE", "STORAGE", "BANDWIDTH", "NETWORK"});
        
        assert(configManager.applyGovernanceDecision("PROP_004", arrayParams, "test_governance"));
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
        
        // Test change tracking
        std::cout << "8. Testing change tracking...\n";
        
        auto changeLog = configManager.getChangeLog();
        assert(!changeLog.empty());
        std::cout << "✓ Change log contains " << changeLog.size() << " entries\n";
        
        // Count governance-related changes
        int governanceChanges = 0;
        for (const auto& change : changeLog) {
            if (change.source.find("PROP_") != std::string::npos) {
                governanceChanges++;
            }
        }
        assert(governanceChanges > 0);
        std::cout << "✓ Found " << governanceChanges << " governance-related changes\n\n";
        
        // Test backup functionality
        std::cout << "9. Testing backup functionality...\n";
        
        auto backupHistory = configManager.getBackupHistory();
        assert(backupHistory.size() >= 2); // Initial + governance backups
        std::cout << "✓ " << backupHistory.size() << " backups created\n";
        
        // Find governance backups
        int governanceBackups = 0;
        for (const auto& backup : backupHistory) {
            if (backup.reason.find("governance") != std::string::npos) {
                governanceBackups++;
            }
        }
        assert(governanceBackups > 0);
        std::cout << "✓ Found " << governanceBackups << " governance-related backups\n\n";
        
        // Test configuration export/import with governance changes
        std::cout << "10. Testing configuration export/import...\n";
        
        auto exportedConfig = configManager.exportConfiguration();
        assert(!exportedConfig.empty());
        std::cout << "✓ Configuration exported successfully\n";
        
        // Create new manager and import
        ConsensusConfigManager newManager("test_governance_func2.json", "test_governance_func2_backups");
        assert(newManager.initialize());
        assert(newManager.importConfiguration(exportedConfig, "import_test"));
        
        auto importedConfig = newManager.getConfiguration();
        assert(importedConfig.powDifficulty == arrayConfig.powDifficulty);
        assert(importedConfig.minStakeAmount == arrayConfig.minStakeAmount);
        assert(importedConfig.acceptedResourceTypes.size() == arrayConfig.acceptedResourceTypes.size());
        
        std::cout << "✓ Configuration imported and verified successfully\n\n";
        
        // Clean up test files
        std::filesystem::remove("test_governance_func.json");
        std::filesystem::remove_all("test_governance_func_backups");
        std::filesystem::remove("test_governance_func2.json");
        std::filesystem::remove_all("test_governance_func2_backups");
        
        std::cout << "All governance functionality tests passed! ✓\n";
        std::cout << "============================================\n\n";
        
        std::cout << "Summary of implemented governance features:\n";
        std::cout << "✓ Governance parameter validation with type and bounds checking\n";
        std::cout << "✓ Backward compatibility analysis and enforcement\n";
        std::cout << "✓ Automatic parameter updates from governance decisions\n";
        std::cout << "✓ Comprehensive change tracking and audit logging\n";
        std::cout << "✓ Automatic backup creation before governance changes\n";
        std::cout << "✓ Support for all parameter types (numeric, string, array)\n";
        std::cout << "✓ Rejection of incompatible or invalid changes\n";
        std::cout << "✓ Configuration persistence and import/export\n";
        std::cout << "✓ Multiple governance decision support\n";
        std::cout << "✓ Detailed compatibility issue reporting\n";
        
        std::cout << "\nRequirements 3.2 and 3.5 implementation complete:\n";
        std::cout << "- 3.2: Automatic network parameter updates from voting results ✓\n";
        std::cout << "- 3.5: Backward compatibility enforcement for governance changes ✓\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}