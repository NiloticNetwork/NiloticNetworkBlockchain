#ifndef CONSENSUS_CONFLICT_RESOLVER_H
#define CONSENSUS_CONFLICT_RESOLVER_H

#include "consensus_harmony.h"
#include "logger.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <queue>

// Forward declarations
class ConsensusRouter;
class ConsensusMonitor;

/**
 * Conflict severity levels for prioritizing resolution
 */
enum class ConflictSeverity {
    LOW,        // Minor disagreements that don't affect network security
    MEDIUM,     // Moderate conflicts that may impact performance
    HIGH,       // Significant conflicts that could affect network stability
    CRITICAL    // Severe conflicts that threaten network security
};

/**
 * Resolution strategy types
 */
enum class ResolutionStrategy {
    HIERARCHY_BASED,        // Use consensus mechanism hierarchy (Requirements 1.3, 1.4)
    MOST_RESTRICTIVE,       // Use most restrictive validation result (Requirements 1.4)
    CONFIDENCE_WEIGHTED,    // Weight by confidence levels
    MANUAL_INTERVENTION,    // Require manual administrator intervention
    EMERGENCY_FALLBACK      // Emergency mode activation
};

/**
 * Conflict information structure
 */
struct ConflictDetails {
    std::string conflictId;
    std::vector<ConsensusResult> conflictingResults;
    ConflictSeverity severity;
    uint64_t timestamp;
    std::string description;
    std::vector<ConsensusType> involvedMechanisms;
    bool requiresManualIntervention;
    
    ConflictDetails() : severity(ConflictSeverity::MEDIUM), timestamp(0), 
                       requiresManualIntervention(false) {}
    
    ConflictDetails(const std::string& id, const std::vector<ConsensusResult>& results,
                   ConflictSeverity sev = ConflictSeverity::MEDIUM)
        : conflictId(id), conflictingResults(results), severity(sev),
          requiresManualIntervention(false) {
        timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Extract involved mechanisms
        for (const auto& result : results) {
            involvedMechanisms.push_back(result.mechanism);
        }
    }
};

/**
 * Resolution result structure
 */
struct ResolutionResult {
    bool resolved;
    ConsensusResult finalResult;
    ResolutionStrategy strategyUsed;
    std::string resolutionReason;
    uint64_t resolutionTime;
    bool manualInterventionRequired;
    std::vector<std::string> recommendations;
    
    ResolutionResult() : resolved(false), strategyUsed(ResolutionStrategy::HIERARCHY_BASED),
                        resolutionTime(0), manualInterventionRequired(false) {}
    
    ResolutionResult(bool res, const ConsensusResult& result, ResolutionStrategy strategy,
                    const std::string& reason = "")
        : resolved(res), finalResult(result), strategyUsed(strategy), 
          resolutionReason(reason), manualInterventionRequired(false) {
        resolutionTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

/**
 * Manual intervention request structure
 */
struct ManualInterventionRequest {
    std::string requestId;
    ConflictDetails conflict;
    std::vector<ResolutionStrategy> suggestedStrategies;
    uint64_t requestTime;
    bool urgent;
    std::string administratorNotes;
    
    ManualInterventionRequest() : requestTime(0), urgent(false) {}
    
    ManualInterventionRequest(const ConflictDetails& conf, bool isUrgent = false)
        : conflict(conf), urgent(isUrgent) {
        requestTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        requestId = "MANUAL_" + std::to_string(requestTime) + "_" + conflict.conflictId;
    }
};

/**
 * Configuration for conflict resolution
 */
struct ConflictResolverConfig {
    // Automatic resolution settings
    bool enableAutomaticResolution = true;
    ResolutionStrategy defaultStrategy = ResolutionStrategy::HIERARCHY_BASED;
    
    // Severity thresholds for manual intervention
    bool requireManualForCritical = true;
    bool requireManualForHigh = false;
    
    // Timeout settings (in seconds)
    uint64_t resolutionTimeout = 30;
    uint64_t manualInterventionTimeout = 300; // 5 minutes
    
    // Confidence thresholds
    double minimumConfidenceForResolution = 0.5;
    double criticalConflictThreshold = 0.3; // Confidence difference that triggers critical severity
    
    // Emergency settings
    bool enableEmergencyMode = true;
    uint64_t emergencyModeTimeout = 60; // 1 minute
    
    // Logging and monitoring
    bool logAllConflicts = true;
    bool notifyAdministrators = true;
};

/**
 * ConflictResolver - Handles consensus conflicts with hierarchy-based resolution
 * 
 * This class implements comprehensive conflict resolution for the consensus harmony system:
 * - Automatic conflict resolution using hierarchy and most restrictive validation
 * - Manual intervention capabilities for critical conflicts
 * - Emergency mode activation for severe network threats
 * 
 * Requirements addressed: 1.3, 1.4, 3.4
 */
class ConflictResolver {
public:
    ConflictResolver();
    ~ConflictResolver();
    
    // Initialization and configuration
    bool initialize(const ConflictResolverConfig& config = ConflictResolverConfig{});
    void shutdown();
    bool isInitialized() const { return initialized; }
    
    // Core conflict resolution (Requirements 1.3, 1.4)
    ResolutionResult resolveConflict(const ConflictDetails& conflict);
    ResolutionResult resolveConflictWithStrategy(const ConflictDetails& conflict, 
                                                ResolutionStrategy strategy);
    
    // Automatic resolution methods (Requirement 1.4)
    ResolutionResult resolveByHierarchy(const std::vector<ConsensusResult>& results);
    ResolutionResult resolveByMostRestrictive(const std::vector<ConsensusResult>& results);
    ResolutionResult resolveByConfidenceWeighting(const std::vector<ConsensusResult>& results);
    
    // Manual intervention capabilities (Requirement 3.4)
    std::string requestManualIntervention(const ConflictDetails& conflict, bool urgent = false);
    bool processManualResolution(const std::string& requestId, ResolutionStrategy strategy,
                                const std::string& administratorNotes = "");
    std::vector<ManualInterventionRequest> getPendingInterventions();
    bool cancelManualIntervention(const std::string& requestId);
    
    // Emergency mode and critical conflict handling
    bool activateEmergencyMode(const std::string& reason);
    bool deactivateEmergencyMode();
    bool isEmergencyModeActive() const { return emergencyModeActive.load(); }
    
    // Conflict analysis and detection
    ConflictSeverity assessConflictSeverity(const std::vector<ConsensusResult>& results);
    bool requiresManualIntervention(const ConflictDetails& conflict);
    std::vector<ResolutionStrategy> suggestResolutionStrategies(const ConflictDetails& conflict);
    
    // Integration with other components
    void setConsensusRouter(std::shared_ptr<ConsensusRouter> router) { consensusRouter = router; }
    void setConsensusMonitor(std::shared_ptr<ConsensusMonitor> monitor) { consensusMonitor = monitor; }
    
    // Statistics and monitoring
    nlohmann::json getStatistics() const;
    nlohmann::json getConflictHistory() const;
    void resetStatistics();
    
    // Configuration management
    void updateConfig(const ConflictResolverConfig& newConfig);
    ConflictResolverConfig getConfig() const { return config; }

private:
    // Configuration
    ConflictResolverConfig config;
    bool initialized;
    
    // Component references
    std::shared_ptr<ConsensusRouter> consensusRouter;
    std::shared_ptr<ConsensusMonitor> consensusMonitor;
    
    // Manual intervention management
    std::queue<ManualInterventionRequest> pendingInterventions;
    std::map<std::string, ManualInterventionRequest> activeInterventions;
    mutable std::mutex interventionMutex;
    
    // Emergency mode state
    std::atomic<bool> emergencyModeActive{false};
    std::string emergencyModeReason;
    uint64_t emergencyModeActivatedTime;
    mutable std::mutex emergencyMutex;
    
    // Statistics
    std::atomic<uint64_t> totalConflictsResolved{0};
    std::atomic<uint64_t> automaticResolutions{0};
    std::atomic<uint64_t> manualInterventions{0};
    std::atomic<uint64_t> emergencyActivations{0};
    std::map<ResolutionStrategy, uint64_t> strategyUsageCount;
    mutable std::mutex statisticsMutex;
    
    // Conflict history
    std::vector<ConflictDetails> resolvedConflicts;
    std::vector<ResolutionResult> resolutionHistory;
    mutable std::mutex historyMutex;
    
    // Thread safety
    mutable std::mutex resolverMutex;
    
    // Private helper methods
    std::string generateConflictId() const;
    int getConsensusPriority(ConsensusType type) const;
    double calculateConfidenceDifference(const std::vector<ConsensusResult>& results) const;
    bool isConflictCritical(const std::vector<ConsensusResult>& results) const;
    
    // Resolution algorithm implementations
    ConsensusResult selectByHierarchy(const std::vector<ConsensusResult>& results) const;
    ConsensusResult selectMostRestrictive(const std::vector<ConsensusResult>& results) const;
    ConsensusResult selectByConfidence(const std::vector<ConsensusResult>& results) const;
    
    // Manual intervention helpers
    void processInterventionQueue();
    bool isInterventionExpired(const ManualInterventionRequest& request) const;
    void notifyAdministrators(const ManualInterventionRequest& request);
    
    // Emergency mode helpers
    void handleEmergencyMode();
    bool shouldActivateEmergencyMode(const ConflictDetails& conflict) const;
    
    // Logging and monitoring helpers
    void logConflictResolution(const ConflictDetails& conflict, const ResolutionResult& result);
    void updateStatistics(const ResolutionResult& result);
    void recordConflictHistory(const ConflictDetails& conflict, const ResolutionResult& result);
    
    // Utility methods
    std::string consensusTypeToString(ConsensusType type) const;
    std::string severityToString(ConflictSeverity severity) const;
    std::string strategyToString(ResolutionStrategy strategy) const;
    ConflictSeverity stringToSeverity(const std::string& severity) const;
    ResolutionStrategy stringToStrategy(const std::string& strategy) const;
};

#endif // CONSENSUS_CONFLICT_RESOLVER_H