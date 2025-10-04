#ifndef CONSENSUS_HARMONY_MANAGER_H
#define CONSENSUS_HARMONY_MANAGER_H

#include "consensus_harmony.h"
#include "consensus_router.h"
#include "consensus_balancer.h"
#include "consensus_monitor.h"
#include "emergency_consensus_mode.h"
#include "consensus_security_validator.h"
#include "consensus_security_auditor.h"
#include "consensus_performance_optimizer.h"
#include <memory>
#include <thread>
#include <condition_variable>

// Forward declarations
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
    std::unique_ptr<EmergencyConsensusMode> emergencyMode;
    
    // Security components
    std::unique_ptr<ConsensusSecurityValidator> securityValidator;
    std::unique_ptr<ConsensusSecurityAuditor> securityAuditor;
    
    // Performance optimization
    std::unique_ptr<ConsensusPerformanceOptimizer> performanceOptimizer;
    
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
    bool enterEmergencyMode(EmergencyType type, EmergencySeverity severity, 
                           const std::string& description, const std::string& source = "");
    bool exitEmergencyMode();
    bool isInEmergencyMode() const;
    EmergencyConsensusMode* getEmergencyMode() const { return emergencyMode.get(); }
    
    // Security operations
    SecurityValidationResult validateSecurity(const ConsensusRequest& request);
    SecurityValidationResult validateBlockSecurity(const Block& block);
    SecurityValidationResult validateTransactionSecurity(const Transaction& transaction);
    void logSecurityEvent(const std::string& event, const std::string& source, 
                         const nlohmann::json& details = {});
    nlohmann::json getSecurityMetrics() const;
    nlohmann::json getSecurityReport() const;
    bool enableSecurityFeature(const std::string& feature, bool enable);
    
    // Security component access
    ConsensusSecurityValidator* getSecurityValidator() const { return securityValidator.get(); }
    ConsensusSecurityAuditor* getSecurityAuditor() const { return securityAuditor.get(); }
    
    // Performance optimization access
    ConsensusPerformanceOptimizer* getPerformanceOptimizer() const { return performanceOptimizer.get(); }
    
    // Performance optimization methods
    bool enablePerformanceOptimization(bool enable);
    bool updateOptimizationConfiguration(const OptimizationConfig& config);
    OptimizationConfig getOptimizationConfiguration() const;
    nlohmann::json getPerformanceReport() const;
    nlohmann::json runPerformanceBenchmark(const std::vector<ConsensusRequest>& testRequests);
    
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