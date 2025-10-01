#ifndef CONSENSUS_HARMONY_MANAGER_H
#define CONSENSUS_HARMONY_MANAGER_H

#include "consensus_harmony.h"
#include <memory>
#include <thread>
#include <condition_variable>

// Forward declarations
class ConsensusRouter;
class ConsensusBalancer;
class ConsensusMonitor;
class Blockchain;

/**
 * Central orchestrator for all consensus mechanisms in the Nilotic Blockchain
 * Manages initialization, coordination, and conflict resolution between different consensus engines
 */
class ConsensusHarmonyManager {
private:
    // Core components
    std::unique_ptr<ConsensusRouter> router;
    std::unique_ptr<ConsensusBalancer> balancer;
    std::unique_ptr<ConsensusMonitor> monitor;
    
    // Configuration and state
    ConsensusConfig config;
    ConsensusStatus status;
    std::atomic<bool> initialized;
    std::atomic<bool> running;
    
    // Thread safety
    mutable std::mutex managerMutex;
    std::condition_variable managerCV;
    
    // Background processing
    std::thread managementThread;
    std::atomic<bool> shouldStop;
    
    // Reference to blockchain
    Blockchain* blockchain;

public:
    explicit ConsensusHarmonyManager(Blockchain* bc = nullptr);
    ~ConsensusHarmonyManager();
    
    // Initialization and lifecycle
    bool initializeConsensus();
    bool initializeConsensus(const ConsensusConfig& customConfig);
    void shutdown();
    bool isInitialized() const { return initialized.load(); }
    bool isRunning() const { return running.load(); }
    
    // Core validation methods
    bool validateBlock(const Block& block);
    bool validateTransaction(const Transaction& transaction);
    ConsensusResult processConsensusRequest(const ConsensusRequest& request);
    
    // Consensus mechanism management
    bool registerConsensusEngine(std::unique_ptr<ConsensusEngine> engine);
    bool unregisterConsensusEngine(ConsensusType type);
    std::vector<ConsensusType> getActiveEngines() const;
    
    // Parameter adjustment and balancing
    void adjustConsensusParameters();
    bool setConsensusParameter(ConsensusType type, const std::string& parameter, double value);
    std::map<std::string, double> getConsensusParameters(ConsensusType type) const;
    
    // Status and monitoring
    ConsensusStatus getConsensusStatus() const;
    nlohmann::json getDetailedStatus() const;
    nlohmann::json getMetrics() const;
    
    // Configuration management
    bool updateConfiguration(const ConsensusConfig& newConfig);
    ConsensusConfig getConfiguration() const;
    bool saveConfiguration(const std::string& filename) const;
    bool loadConfiguration(const std::string& filename);
    
    // Conflict resolution
    ConsensusResult resolveConflict(const std::vector<ConsensusResult>& results);
    bool handleConsensusFailure(ConsensusType type, const std::string& error);
    
    // Emergency operations
    bool enterEmergencyMode();
    bool exitEmergencyMode();
    bool isInEmergencyMode() const;
    
    // Blockchain integration
    void setBlockchain(Blockchain* bc) { blockchain = bc; }
    Blockchain* getBlockchain() const { return blockchain; }

private:
    // Internal management
    void managementLoop();
    void performPeriodicTasks();
    void checkSystemHealth();
    void rebalanceConsensus();
    
    // Helper methods
    bool validateConfiguration(const ConsensusConfig& config) const;
    void initializeComponents();
    void shutdownComponents();
    void logEvent(const std::string& event, const nlohmann::json& data = {}) const;
};

#endif // CONSENSUS_HARMONY_MANAGER_H