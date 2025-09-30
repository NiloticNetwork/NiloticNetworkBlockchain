#ifndef CONSENSUS_ROUTER_H
#define CONSENSUS_ROUTER_H

#include "consensus_harmony.h"
#include "logger.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <functional>

// Forward declarations
class Block;
class Transaction;

/**
 * Routing rule structure for determining which consensus mechanisms to use
 */
struct RoutingRule {
    RequestType requestType;
    std::vector<ConsensusType> requiredMechanisms;
    std::vector<ConsensusType> optionalMechanisms;
    double minimumConfidence;
    bool requireAllMechanisms;
    
    RoutingRule() : requestType(RequestType::BLOCK_VALIDATION), 
                   minimumConfidence(0.5), requireAllMechanisms(false) {}
    
    RoutingRule(RequestType type, const std::vector<ConsensusType>& required,
               const std::vector<ConsensusType>& optional = {},
               double minConf = 0.5, bool requireAll = false)
        : requestType(type), requiredMechanisms(required), 
          optionalMechanisms(optional), minimumConfidence(minConf),
          requireAllMechanisms(requireAll) {}
};

/**
 * Aggregation strategy for combining consensus results
 */
enum class AggregationStrategy {
    UNANIMOUS,          // All mechanisms must agree
    MAJORITY,           // Majority of mechanisms must agree
    WEIGHTED_MAJORITY,  // Weighted majority based on confidence
    MOST_RESTRICTIVE,   // Use most restrictive validation result
    HIERARCHICAL        // Use consensus hierarchy for conflicts
};

/**
 * Conflict detection and resolution information
 */
struct ConflictInfo {
    bool hasConflict;
    std::vector<ConsensusResult> conflictingResults;
    ConsensusResult resolvedResult;
    std::string resolutionReason;
    AggregationStrategy strategyUsed;
    
    ConflictInfo() : hasConflict(false), strategyUsed(AggregationStrategy::HIERARCHICAL) {}
};

/**
 * Consensus Router - Routes validation requests to appropriate consensus mechanisms
 * Implements mechanism selection logic, routing rules, and result aggregation
 */
class ConsensusRouter {
private:
    // Registered consensus engines
    std::map<ConsensusType, std::unique_ptr<ConsensusEngine>> engines;
    
    // Routing configuration
    std::vector<RoutingRule> routingRules;
    std::map<RequestType, AggregationStrategy> aggregationStrategies;
    ConsensusConfig* config;
    
    // Thread safety
    mutable std::mutex routerMutex;
    
    // Statistics
    uint64_t totalRequests;
    uint64_t successfulRoutes;
    uint64_t conflictCount;
    std::map<ConsensusType, uint64_t> engineUsageCount;
    
    // Initialization state
    bool initialized;

public:
    ConsensusRouter();
    ~ConsensusRouter();
    
    // Lifecycle management
    bool initialize();
    bool initialize(ConsensusConfig* configuration);
    void shutdown();
    bool isInitialized() const { return initialized; }
    
    // Engine management
    bool registerEngine(std::unique_ptr<ConsensusEngine> engine);
    bool unregisterEngine(ConsensusType type);
    ConsensusEngine* getEngine(ConsensusType type) const;
    std::vector<ConsensusType> getRegisteredEngines() const;
    
    // Core routing functionality
    std::vector<ConsensusEngine*> getApplicableEngines(const ConsensusRequest& request);
    ConsensusResult routeValidation(const ConsensusRequest& request);
    bool aggregateResults(const std::vector<ConsensusResult>& results);
    
    // Advanced routing methods
    ConsensusResult routeValidationWithStrategy(const ConsensusRequest& request, 
                                               AggregationStrategy strategy);
    ConflictInfo detectAndResolveConflicts(const std::vector<ConsensusResult>& results,
                                          AggregationStrategy strategy);
    
    // Routing rule management
    bool addRoutingRule(const RoutingRule& rule);
    bool removeRoutingRule(RequestType requestType);
    std::vector<RoutingRule> getRoutingRules() const;
    void setDefaultRoutingRules();
    
    // Aggregation strategy management
    bool setAggregationStrategy(RequestType requestType, AggregationStrategy strategy);
    AggregationStrategy getAggregationStrategy(RequestType requestType) const;
    
    // Validation methods for specific request types
    ConsensusResult validateBlock(const Block& block);
    ConsensusResult validateTransaction(const Transaction& transaction);
    ConsensusResult validateGovernanceProposal(const std::string& proposalData);
    ConsensusResult validateSmartContractExecution(const std::string& contractData);
    
    // Statistics and monitoring
    nlohmann::json getStatistics() const;
    nlohmann::json getEngineStatus() const;
    void resetStatistics();
    
    // Configuration
    void setConfiguration(ConsensusConfig* configuration) { config = configuration; }
    ConsensusConfig* getConfiguration() const { return config; }

private:
    // Internal routing logic
    std::vector<ConsensusType> determineRequiredMechanisms(const ConsensusRequest& request);
    std::vector<ConsensusType> determineOptionalMechanisms(const ConsensusRequest& request);
    RoutingRule findApplicableRule(const ConsensusRequest& request);
    
    // Result aggregation algorithms
    ConsensusResult aggregateUnanimous(const std::vector<ConsensusResult>& results);
    ConsensusResult aggregateMajority(const std::vector<ConsensusResult>& results);
    ConsensusResult aggregateWeightedMajority(const std::vector<ConsensusResult>& results);
    ConsensusResult aggregateMostRestrictive(const std::vector<ConsensusResult>& results);
    ConsensusResult aggregateHierarchical(const std::vector<ConsensusResult>& results);
    
    // Conflict detection and resolution
    bool hasConflicts(const std::vector<ConsensusResult>& results);
    ConsensusResult resolveConflictByHierarchy(const std::vector<ConsensusResult>& results);
    ConsensusResult resolveConflictByConfidence(const std::vector<ConsensusResult>& results);
    
    // Helper methods
    double calculateAverageConfidence(const std::vector<ConsensusResult>& results);
    std::vector<ConsensusResult> filterValidResults(const std::vector<ConsensusResult>& results);
    std::vector<ConsensusResult> filterInvalidResults(const std::vector<ConsensusResult>& results);
    int getConsensusPriority(ConsensusType type) const;
    std::string consensusTypeToString(ConsensusType type) const;
    std::string requestTypeToString(RequestType type) const;
    std::string aggregationStrategyToString(AggregationStrategy strategy) const;
    
    // Validation helpers
    bool isEngineHealthy(ConsensusType type) const;
    bool shouldUseEngine(ConsensusType type, const ConsensusRequest& request) const;
    
    // Statistics helpers
    void updateStatistics(const ConsensusRequest& request, const ConsensusResult& result);
    void incrementEngineUsage(ConsensusType type);
};

#endif // CONSENSUS_ROUTER_H