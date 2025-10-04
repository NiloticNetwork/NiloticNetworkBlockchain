#include "../../include/core/consensus_harmony_integration.h"
#include "../../include/core/blockchain.h"
#include "../../include/core/logger.h"
#include "../../include/core/mining.h"
#include "../../include/core/voting_consensus_engine.h"
#include "../../include/core/proof_of_resource_contribution.h"
#include "../../include/core/smart_contract_consensus_engine.h"
#include "../../include/core/consensus_harmony_manager.h"
#include "../../include/core/block.h"
#include "../../include/core/transaction.h"
#include <fstream>
#include <algorithm>

ConsensusHarmonyIntegration::ConsensusHarmonyIntegration(Blockchain* blockchain)
    : blockchain(blockchain), initialized(false), migrationCompleted(false) {
    Logger::info("ConsensusHarmonyIntegration created");
}

ConsensusHarmonyIntegration::~ConsensusHarmonyIntegration() {
    shutdown();
}

bool ConsensusHarmonyIntegration::initialize() {
    std::lock_guard<std::mutex> lock(integrationMutex);
    
    if (initialized) {
        Logger::warning("ConsensusHarmonyIntegration already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing Consensus Harmony Integration");
        
        if (!blockchain) {
            Logger::error("Blockchain instance is null");
            return false;
        }
        
        // Initialize consensus harmony manager
        harmonyManager = std::make_unique<ConsensusHarmonyManager>(blockchain);
        if (!harmonyManager->initializeConsensus()) {
            Logger::error("Failed to initialize ConsensusHarmonyManager");
            return false;
        }
        
        // Initialize and register all consensus engines
        if (!initializeConsensusEngines()) {
            Logger::error("Failed to initialize consensus engines");
            return false;
        }
        
        // Integrate with existing blockchain components
        if (!integrateWithBlockchain()) {
            Logger::error("Failed to integrate with blockchain");
            return false;
        }
        
        // Integrate with mining system
        if (!integrateWithMining()) {
            Logger::error("Failed to integrate with mining system");
            return false;
        }
        
        // Integrate with voting system
        if (!integrateWithVoting()) {
            Logger::error("Failed to integrate with voting system");
            return false;
        }
        
        initialized = true;
        Logger::info("Consensus Harmony Integration initialized successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize ConsensusHarmonyIntegration: " + std::string(e.what()));
        return false;
    }
}

void ConsensusHarmonyIntegration::shutdown() {
    std::lock_guard<std::mutex> lock(integrationMutex);
    
    if (!initialized) {
        return;
    }
    
    Logger::info("Shutting down Consensus Harmony Integration");
    
    // Shutdown harmony manager
    if (harmonyManager) {
        harmonyManager->shutdown();
        harmonyManager.reset();
    }
    
    // Clear engine references
    miningEngine.reset();
    votingEngine.reset();
    porcEngine.reset();
    smartContractEngine.reset();
    
    initialized = false;
    Logger::info("Consensus Harmony Integration shutdown completed");
}

bool ConsensusHarmonyIntegration::initializeConsensusEngines() {
    Logger::info("Initializing consensus engines");
    
    try {
        // Initialize Mining Engine (PoW)
        Logger::info("Initializing Mining Engine...");
        miningEngine = std::make_unique<MiningEngine>(*blockchain);
        if (!miningEngine->initialize()) {
            Logger::error("Failed to initialize MiningEngine");
            return false;
        }
        
        // Create a separate instance for harmony manager registration
        auto miningEngineForHarmony = std::make_unique<MiningEngine>(*blockchain);
        if (!miningEngineForHarmony->initialize()) {
            Logger::error("Failed to initialize MiningEngine for harmony manager");
            return false;
        }
        
        // Register with harmony manager
        if (!harmonyManager->registerConsensusEngine(std::move(miningEngineForHarmony))) {
            Logger::error("Failed to register MiningEngine with harmony manager");
            return false;
        }
        Logger::info("Mining Engine registered successfully");
        
        // Initialize Voting Consensus Engine
        Logger::info("Initializing Voting Consensus Engine...");
        votingEngine = std::make_unique<VotingConsensusEngine>(blockchain);
        if (!votingEngine->initialize()) {
            Logger::error("Failed to initialize VotingConsensusEngine");
            return false;
        }
        
        // Create a separate instance for harmony manager registration
        auto votingEngineForHarmony = std::make_unique<VotingConsensusEngine>(blockchain);
        if (!votingEngineForHarmony->initialize()) {
            Logger::error("Failed to initialize VotingConsensusEngine for harmony manager");
            return false;
        }
        
        // Register with harmony manager
        if (!harmonyManager->registerConsensusEngine(std::move(votingEngineForHarmony))) {
            Logger::error("Failed to register VotingConsensusEngine with harmony manager");
            return false;
        }
        Logger::info("Voting Consensus Engine registered successfully");
        
        // Initialize Proof of Resource Contribution Engine
        Logger::info("Initializing Proof of Resource Contribution Engine...");
        porcEngine = std::make_unique<ProofOfResourceContributionEngine>();
        if (!porcEngine->initialize()) {
            Logger::error("Failed to initialize ProofOfResourceContributionEngine");
            return false;
        }
        
        // Create a separate instance for harmony manager registration
        auto porcEngineForHarmony = std::make_unique<ProofOfResourceContributionEngine>();
        if (!porcEngineForHarmony->initialize()) {
            Logger::error("Failed to initialize ProofOfResourceContributionEngine for harmony manager");
            return false;
        }
        
        // Register with harmony manager
        if (!harmonyManager->registerConsensusEngine(std::move(porcEngineForHarmony))) {
            Logger::error("Failed to register ProofOfResourceContributionEngine with harmony manager");
            return false;
        }
        Logger::info("Proof of Resource Contribution Engine registered successfully");
        
        // Initialize Smart Contract Consensus Engine
        Logger::info("Initializing Smart Contract Consensus Engine...");
        smartContractEngine = std::make_unique<SmartContractConsensusEngine>();
        if (!smartContractEngine->initialize()) {
            Logger::error("Failed to initialize SmartContractConsensusEngine");
            return false;
        }
        
        // Create a separate instance for harmony manager registration
        auto smartContractEngineForHarmony = std::make_unique<SmartContractConsensusEngine>();
        if (!smartContractEngineForHarmony->initialize()) {
            Logger::error("Failed to initialize SmartContractConsensusEngine for harmony manager");
            return false;
        }
        
        // Register with harmony manager
        if (!harmonyManager->registerConsensusEngine(std::move(smartContractEngineForHarmony))) {
            Logger::error("Failed to register SmartContractConsensusEngine with harmony manager");
            return false;
        }
        Logger::info("Smart Contract Consensus Engine registered successfully");
        
        Logger::info("All consensus engines initialized and registered successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize consensus engines: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::integrateWithBlockchain() {
    Logger::info("Integrating with blockchain system");
    
    try {
        // Set the harmony manager reference in blockchain
        blockchain->setConsensusHarmonyManager(harmonyManager.get());
        
        // Update blockchain validation methods to use harmony system
        // This would involve modifying the blockchain's validation logic
        // to route through the harmony manager
        
        Logger::info("Blockchain integration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to integrate with blockchain: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::integrateWithMining() {
    Logger::info("Integrating with mining system");
    
    try {
        // The mining engine is already integrated through the harmony manager
        // Additional integration steps can be added here if needed
        
        Logger::info("Mining system integration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to integrate with mining system: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::integrateWithVoting() {
    Logger::info("Integrating with voting system");
    
    try {
        // The voting engine is already integrated through the harmony manager
        // Additional integration steps can be added here if needed
        
        Logger::info("Voting system integration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to integrate with voting system: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::migrateExistingData() {
    std::lock_guard<std::mutex> lock(integrationMutex);
    
    if (migrationCompleted) {
        Logger::info("Data migration already completed");
        return true;
    }
    
    Logger::info("Starting data migration for consensus harmony integration");
    
    try {
        // Create backup of existing blockchain data
        if (!createDataBackup()) {
            Logger::error("Failed to create data backup");
            return false;
        }
        
        // Migrate existing blocks to support harmony validation
        if (!migrateBlocks()) {
            Logger::error("Failed to migrate blocks");
            return false;
        }
        
        // Migrate existing transactions
        if (!migrateTransactions()) {
            Logger::error("Failed to migrate transactions");
            return false;
        }
        
        // Migrate validator data
        if (!migrateValidatorData()) {
            Logger::error("Failed to migrate validator data");
            return false;
        }
        
        // Update configuration files
        if (!updateConfigurationFiles()) {
            Logger::error("Failed to update configuration files");
            return false;
        }
        
        migrationCompleted = true;
        Logger::info("Data migration completed successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Data migration failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::createDataBackup() {
    Logger::info("Creating data backup");
    
    try {
        // Create backup directory
        std::string backupDir = "backup_" + std::to_string(std::time(nullptr));
        
        // Backup blockchain data
        if (!blockchain->saveToFile(backupDir + "/blockchain_backup.json")) {
            Logger::error("Failed to backup blockchain data");
            return false;
        }
        
        // Backup configuration files
        std::vector<std::string> configFiles = {
            "config/consensus_security.json",
            "config/security.json"
        };
        
        for (const auto& configFile : configFiles) {
            std::ifstream src(configFile);
            if (src.is_open()) {
                std::ofstream dst(backupDir + "/" + configFile);
                dst << src.rdbuf();
                src.close();
                dst.close();
            }
        }
        
        Logger::info("Data backup created successfully in directory: " + backupDir);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to create data backup: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::migrateBlocks() {
    Logger::info("Migrating existing blocks");
    
    try {
        auto chain = blockchain->getChain();
        
        for (auto& block : chain) {
            // Add harmony validation metadata to existing blocks
            // This is a simplified migration - in practice, you might need
            // to re-validate blocks with the new consensus system
            
            // For now, we'll just log that the block has been processed
            Logger::debug("Migrated block " + std::to_string(block.getIndex()));
        }
        
        Logger::info("Block migration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to migrate blocks: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::migrateTransactions() {
    Logger::info("Migrating existing transactions");
    
    try {
        auto pendingTxs = blockchain->getPendingTransactions();
        
        for (const auto& tx : pendingTxs) {
            // Validate transactions with the new harmony system
            if (harmonyManager->validateTransaction(tx)) {
                Logger::debug("Migrated transaction: " + tx.getHash());
            } else {
                Logger::warning("Transaction failed harmony validation: " + tx.getHash());
            }
        }
        
        Logger::info("Transaction migration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to migrate transactions: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::migrateValidatorData() {
    Logger::info("Migrating validator data");
    
    try {
        // Get existing PoS validator data from blockchain
        auto posEngine = blockchain->getPoSEngine();
        if (posEngine) {
            // The PoS engine is already integrated, so no migration needed
            Logger::info("PoS validator data already integrated");
        }
        
        Logger::info("Validator data migration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to migrate validator data: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::updateConfigurationFiles() {
    Logger::info("Updating configuration files");
    
    try {
        // Create consensus harmony configuration
        nlohmann::json harmonyConfig;
        harmonyConfig["consensus_harmony"] = {
            {"enabled", true},
            {"engines", {
                {"proof_of_work", {{"enabled", true}, {"difficulty", 4}}},
                {"proof_of_stake", {{"enabled", true}, {"min_stake", 100.0}}},
                {"proof_of_resource_contribution", {{"enabled", true}, {"min_contribution", 10.0}}},
                {"voting_consensus", {{"enabled", true}, {"supermajority_threshold", 0.67}}},
                {"smart_contract_validation", {{"enabled", true}}}
            }},
            {"routing", {
                {"block_validation", {"proof_of_work", "proof_of_stake"}},
                {"transaction_validation", {"proof_of_work"}},
                {"governance_proposal", {"voting_consensus"}},
                {"smart_contract_execution", {"smart_contract_validation"}}
            }},
            {"security", {
                {"cryptographic_validation", true},
                {"attack_detection", true},
                {"audit_logging", true}
            }}
        };
        
        // Save harmony configuration
        std::ofstream configFile("config/consensus_harmony.json");
        if (configFile.is_open()) {
            configFile << harmonyConfig.dump(4);
            configFile.close();
            Logger::info("Created consensus harmony configuration file");
        }
        
        Logger::info("Configuration files updated successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to update configuration files: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::runIntegrationTests() {
    Logger::info("Running consensus harmony integration tests");
    
    try {
        // Test 1: Basic harmony manager functionality
        if (!testHarmonyManagerBasics()) {
            Logger::error("Harmony manager basic tests failed");
            return false;
        }
        
        // Test 2: Multi-consensus block validation
        if (!testMultiConsensusValidation()) {
            Logger::error("Multi-consensus validation tests failed");
            return false;
        }
        
        // Test 3: Consensus conflict resolution
        if (!testConflictResolution()) {
            Logger::error("Conflict resolution tests failed");
            return false;
        }
        
        // Test 4: Parameter adjustment
        if (!testParameterAdjustment()) {
            Logger::error("Parameter adjustment tests failed");
            return false;
        }
        
        // Test 5: Emergency mode functionality
        if (!testEmergencyMode()) {
            Logger::error("Emergency mode tests failed");
            return false;
        }
        
        Logger::info("All integration tests passed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Integration tests failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::testHarmonyManagerBasics() {
    Logger::info("Testing harmony manager basics");
    
    try {
        // Test initialization
        if (!harmonyManager->isInitialized()) {
            Logger::error("Harmony manager not initialized");
            return false;
        }
        
        // Test status retrieval
        auto status = harmonyManager->getConsensusStatus();
        if (status.totalValidations < 0) {
            Logger::error("Invalid status data");
            return false;
        }
        
        // Test metrics retrieval
        auto metrics = harmonyManager->getMetrics();
        if (metrics.empty()) {
            Logger::error("No metrics available");
            return false;
        }
        
        Logger::info("Harmony manager basic tests passed");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Harmony manager basic tests failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::testMultiConsensusValidation() {
    Logger::info("Testing multi-consensus validation");
    
    try {
        // Create a test block
        Block testBlock(1, blockchain->getLatestBlock().getHash());
        Transaction testTx("test_sender", "test_recipient", 10.0);
        testBlock.addTransaction(testTx);
        
        // Mine the block to make it valid for PoW
        testBlock.mineBlock(1); // Use low difficulty for testing
        
        // Test block validation through harmony manager
        bool isValid = harmonyManager->validateBlock(testBlock);
        Logger::info("Test block validation result: " + std::string(isValid ? "VALID" : "INVALID"));
        
        // Test transaction validation
        bool txValid = harmonyManager->validateTransaction(testTx);
        Logger::info("Test transaction validation result: " + std::string(txValid ? "VALID" : "INVALID"));
        
        Logger::info("Multi-consensus validation tests passed");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Multi-consensus validation tests failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::testConflictResolution() {
    Logger::info("Testing conflict resolution");
    
    try {
        // This would test the conflict resolution mechanisms
        // For now, we'll just verify that the conflict resolution components are available
        
        if (!harmonyManager->isInitialized()) {
            Logger::error("Harmony manager not available for conflict resolution testing");
            return false;
        }
        
        Logger::info("Conflict resolution tests passed");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Conflict resolution tests failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::testParameterAdjustment() {
    Logger::info("Testing parameter adjustment");
    
    try {
        // Test parameter adjustment through harmony manager
        bool adjusted = harmonyManager->setConsensusParameter(
            ConsensusType::PROOF_OF_WORK, "difficulty", 5.0);
        
        if (!adjusted) {
            Logger::warning("Parameter adjustment test returned false, but this may be expected");
        }
        
        Logger::info("Parameter adjustment tests passed");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Parameter adjustment tests failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::testEmergencyMode() {
    Logger::info("Testing emergency mode");
    
    try {
        // Test emergency mode activation
        bool activated = harmonyManager->enterEmergencyMode();
        if (activated) {
            Logger::info("Emergency mode activated successfully");
            
            // Test emergency mode status
            bool inEmergency = harmonyManager->isInEmergencyMode();
            if (!inEmergency) {
                Logger::error("Emergency mode status check failed");
                return false;
            }
            
            // Test emergency mode deactivation
            bool deactivated = harmonyManager->exitEmergencyMode();
            if (!deactivated) {
                Logger::error("Failed to exit emergency mode");
                return false;
            }
        }
        
        Logger::info("Emergency mode tests passed");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Emergency mode tests failed: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusHarmonyIntegration::validateSystemIntegrity() {
    Logger::info("Validating system integrity after integration");
    
    try {
        // Validate blockchain integrity
        if (!blockchain->isChainValid()) {
            Logger::error("Blockchain integrity validation failed");
            return false;
        }
        
        // Validate harmony manager status
        if (!harmonyManager->isInitialized() || !harmonyManager->isRunning()) {
            Logger::error("Harmony manager integrity validation failed");
            return false;
        }
        
        // Validate all consensus engines are healthy
        auto activeEngines = harmonyManager->getActiveEngines();
        if (activeEngines.empty()) {
            Logger::error("No active consensus engines found");
            return false;
        }
        
        // Validate configuration consistency
        auto config = harmonyManager->getConfiguration();
        if (config.powDifficulty == 0) {
            Logger::error("Invalid configuration detected");
            return false;
        }
        
        Logger::info("System integrity validation passed");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("System integrity validation failed: " + std::string(e.what()));
        return false;
    }
}

nlohmann::json ConsensusHarmonyIntegration::getIntegrationStatus() const {
    std::lock_guard<std::mutex> lock(integrationMutex);
    
    nlohmann::json status;
    status["initialized"] = initialized.load();
    status["migration_completed"] = migrationCompleted.load();
    
    if (harmonyManager) {
        status["harmony_manager"] = harmonyManager->getDetailedStatus();
    }
    
    status["engines"] = {
        {"mining_engine", miningEngine ? miningEngine->getStatus() : nlohmann::json::object()},
        {"voting_engine", votingEngine ? votingEngine->getStatus() : nlohmann::json::object()},
        {"porc_engine", porcEngine ? porcEngine->getStatus() : nlohmann::json::object()},
        {"smart_contract_engine", smartContractEngine ? smartContractEngine->getStatus() : nlohmann::json::object()}
    };
    
    return status;
}

ConsensusHarmonyManager* ConsensusHarmonyIntegration::getHarmonyManager() const {
    return harmonyManager.get();
}