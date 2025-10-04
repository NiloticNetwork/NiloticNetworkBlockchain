#ifndef CONSENSUS_HARMONY_INTEGRATION_H
#define CONSENSUS_HARMONY_INTEGRATION_H

#include "consensus_harmony.h"
#include "json.hpp"
#include <memory>
#include <mutex>
#include <string>

// Forward declarations
class Blockchain;
class ConsensusHarmonyManager;
class MiningEngine;
class VotingConsensusEngine;
class ProofOfResourceContributionEngine;
class SmartContractConsensusEngine;

/**
 * Consensus Harmony Integration - Integrates all consensus components into the main blockchain system
 * Handles migration of existing data and provides comprehensive system integration
 */
class ConsensusHarmonyIntegration {
private:
    // Core components
    Blockchain* blockchain;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
    
    // Consensus engines
    std::unique_ptr<MiningEngine> miningEngine;
    std::unique_ptr<VotingConsensusEngine> votingEngine;
    std::unique_ptr<ProofOfResourceContributionEngine> porcEngine;
    std::unique_ptr<SmartContractConsensusEngine> smartContractEngine;
    
    // State tracking
    std::atomic<bool> initialized;
    std::atomic<bool> migrationCompleted;
    
    // Thread safety
    mutable std::mutex integrationMutex;

public:
    explicit ConsensusHarmonyIntegration(Blockchain* blockchain);
    ~ConsensusHarmonyIntegration();
    
    // Lifecycle management
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized.load(); }
    
    // Integration methods
    bool initializeConsensusEngines();
    bool integrateWithBlockchain();
    bool integrateWithMining();
    bool integrateWithVoting();
    
    // Data migration
    bool migrateExistingData();
    bool createDataBackup();
    bool migrateBlocks();
    bool migrateTransactions();
    bool migrateValidatorData();
    bool updateConfigurationFiles();
    
    // Testing and validation
    bool runIntegrationTests();
    bool testHarmonyManagerBasics();
    bool testMultiConsensusValidation();
    bool testConflictResolution();
    bool testParameterAdjustment();
    bool testEmergencyMode();
    bool validateSystemIntegrity();
    
    // Status and monitoring
    nlohmann::json getIntegrationStatus() const;
    bool isMigrationCompleted() const { return migrationCompleted.load(); }
    
    // Component access
    ConsensusHarmonyManager* getHarmonyManager() const;
    MiningEngine* getMiningEngine() const { return miningEngine.get(); }
    VotingConsensusEngine* getVotingEngine() const { return votingEngine.get(); }
    ProofOfResourceContributionEngine* getPorcEngine() const { return porcEngine.get(); }
    SmartContractConsensusEngine* getSmartContractEngine() const { return smartContractEngine.get(); }
};

#endif // CONSENSUS_HARMONY_INTEGRATION_H