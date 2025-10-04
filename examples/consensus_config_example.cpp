#include "../include/core/consensus_config_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Consensus Configuration Manager Example\n";
    std::cout << "======================================\n\n";
    
    // Create configuration manager
    ConsensusConfigManager configManager("example_config.json", "example_backups");
    
    // Initialize with default configuration
    std::cout << "1. Initializing configuration manager...\n";
    if (!configManager.initialize()) {
        std::cerr << "Failed to initialize configuration manager\n";
        return 1;
    }
    std::cout << "   ✓ Configuration manager initialized successfully\n\n";
    
    // Display current configuration
    std::cout << "2. Current configuration:\n";
    auto config = configManager.getConfiguration();
    std::cout << "   PoW Difficulty: " << config.powDifficulty << "\n";
    std::cout << "   PoW Target Block Time: " << config.powTargetBlockTime << " seconds\n";
    std::cout << "   PoS Min Stake: " << config.minStakeAmount << "\n";
    std::cout << "   Voting Threshold: " << config.supermajorityThreshold << "\n\n";
    
    // Register a change callback
    std::cout << "3. Registering change callback...\n";
    configManager.registerChangeCallback([](const ConfigChangeEvent& event) {
        std::cout << "   📝 Configuration changed: " << event.parameter 
                  << " from " << event.oldValue << " to " << event.newValue 
                  << " (source: " << event.source << ")\n";
    });
    std::cout << "   ✓ Change callback registered\n\n";
    
    // Update some parameters
    std::cout << "4. Updating parameters...\n";
    
    if (configManager.setParameter("difficulty", static_cast<uint64_t>(6), 
                                  ConsensusType::PROOF_OF_WORK, "example")) {
        std::cout << "   ✓ PoW difficulty updated to 6\n";
    }
    
    if (configManager.setParameter("minStakeAmount", 2000.0, 
                                  ConsensusType::PROOF_OF_STAKE, "example")) {
        std::cout << "   ✓ PoS minimum stake updated to 2000.0\n";
    }
    
    if (configManager.setParameter("supermajorityThreshold", 0.75, 
                                  ConsensusType::VOTING_CONSENSUS, "example")) {
        std::cout << "   ✓ Voting threshold updated to 0.75\n";
    }
    
    std::cout << "\n";
    
    // Create a backup
    std::cout << "5. Creating configuration backup...\n";
    if (configManager.createBackup("example_backup")) {
        std::cout << "   ✓ Backup created successfully\n";
    }
    std::cout << "\n";
    
    // Display updated configuration
    std::cout << "6. Updated configuration:\n";
    config = configManager.getConfiguration();
    std::cout << "   PoW Difficulty: " << config.powDifficulty << "\n";
    std::cout << "   PoW Target Block Time: " << config.powTargetBlockTime << " seconds\n";
    std::cout << "   PoS Min Stake: " << config.minStakeAmount << "\n";
    std::cout << "   Voting Threshold: " << config.supermajorityThreshold << "\n\n";
    
    // Test parameter validation
    std::cout << "7. Testing parameter validation...\n";
    
    // Try to set an invalid parameter (should fail)
    if (!configManager.setParameter("difficulty", static_cast<uint64_t>(0), 
                                   ConsensusType::PROOF_OF_WORK, "example")) {
        std::cout << "   ✓ Invalid difficulty (0) correctly rejected\n";
    }
    
    // Try to set an invalid voting threshold (should fail)
    if (!configManager.setParameter("supermajorityThreshold", 1.5, 
                                   ConsensusType::VOTING_CONSENSUS, "example")) {
        std::cout << "   ✓ Invalid voting threshold (1.5) correctly rejected\n";
    }
    
    std::cout << "\n";
    
    // Display change log
    std::cout << "8. Configuration change log:\n";
    auto changeLog = configManager.getChangeLog();
    for (const auto& change : changeLog) {
        std::cout << "   - " << change.parameter << ": " << change.oldValue 
                  << " → " << change.newValue << " (source: " << change.source << ")\n";
    }
    std::cout << "\n";
    
    // Export configuration
    std::cout << "9. Exporting configuration...\n";
    auto exportedConfig = configManager.exportConfiguration();
    std::cout << "   ✓ Configuration exported (size: " << exportedConfig.dump().size() << " bytes)\n\n";
    
    // Display status and diagnostics
    std::cout << "10. System status:\n";
    auto status = configManager.getStatus();
    std::cout << "   Backup count: " << status["backup_count"] << "\n";
    std::cout << "   Change log size: " << status["change_log_size"] << "\n";
    std::cout << "   Registered parameters: " << status["registered_parameters"] << "\n";
    
    auto diagnostics = configManager.getDiagnostics();
    std::cout << "   Configuration valid: " << (diagnostics["configuration_valid"].get<bool>() ? "Yes" : "No") << "\n";
    std::cout << "   Config file exists: " << (diagnostics["config_file_exists"].get<bool>() ? "Yes" : "No") << "\n\n";
    
    // Perform self-test
    std::cout << "11. Performing self-test...\n";
    if (configManager.performSelfTest()) {
        std::cout << "   ✓ Self-test passed\n";
    } else {
        std::cout << "   ✗ Self-test failed\n";
    }
    
    std::cout << "\n✅ Example completed successfully!\n";
    std::cout << "\nConfiguration files created:\n";
    std::cout << "  - example_config.json (main configuration)\n";
    std::cout << "  - example_backups/ (backup directory)\n";
    
    return 0;
}