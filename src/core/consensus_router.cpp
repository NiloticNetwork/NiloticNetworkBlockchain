#include "consensus_router.h"
#include "block.h"
#include "transaction.h"
#include <algorithm>
#include <numeric>
#include <chrono>

ConsensusRouter::ConsensusRouter() 
    : config(nullptr), totalRequests(0), successfulRoutes(0), 
      conflictCount(0), initialized(false) {
    Logger::info("ConsensusRouter created");
}

ConsensusRouter::~ConsensusRouter() {
    shutdown();
    Logger::info("ConsensusRouter destroyed");
}

bool ConsensusRouter::initialize() {
    return initialize(nullptr);
}

bool ConsensusRouter::initialize(ConsensusConfig* configuration) {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    if (initialized) {
        Logger::warning("ConsensusRouter already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing ConsensusRouter");
        
        config = configuration;
        
        // Set default routing rules
        setDefaultRoutingRules();
        
        // Set default aggregation strategies
        aggregationStrategies[RequestType::BLOCK_VALIDATION] = AggregationStrategy::HIERARCHICAL;
        aggregationStrategies[RequestType::TRANSACTION_VALIDATION] = AggregationStrategy::MOST_RESTRICTIVE;
        aggregationStrategies[RequestType::GOVERNANCE_PROPOSAL] = AggregationStrategy::WEIGHTED_MAJORITY;
        aggregationStrategies[RequestType::SMART_CONTRACT_EXECUTION] = AggregationStrategy::UNANIMOUS;
        aggregationStrategies[RequestType::PARAMETER_ADJUSTMENT] = AggregationStrategy::WEIGHTED_MAJORITY;
        
        // Initialize statistics
        totalRequests = 0;
        successfulRoutes = 0;
        conflictCount = 0;
        engineUsageCount.clear();
        
        initialized = true;
        Logger::info("ConsensusRouter initialized successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize ConsensusRouter: " + std::string(e.what()));
        return false;
    }
}

void ConsensusRouter::shutdown() {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    if (!initialized) {
        return;
    }
    
    Logger::info("Shutting down ConsensusRouter");
    
    // Shutdown all registered engines
    for (auto& [type, engine] : engines) {
        if (engine) {
            try {
                engine->shutdown();
                Logger::info("Consensus engine shut down: " + consensusTypeToString(type));
            } catch (const std::exception& e) {
                Logger::error("Error shutting down engine " + consensusTypeToString(type) + 
                             ": " + std::string(e.what()));
            }
        }
    }
    
    engines.clear();
    routingRules.clear();
    aggregationStrategies.clear();
    
    initialized = false;
    Logger::info("ConsensusRouter shut down successfully");
}

bool ConsensusRouter::registerEngine(std::unique_ptr<ConsensusEngine> engine) {
    if (!engine) {
        Logger::error("Cannot register null consensus engine");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(routerMutex);
    
    try {
        ConsensusType type = engine->getType();
        std::string name = engine->getName();
        
        Logger::info("Registering consensus engine: " + name);
        
        // Initialize the engine if not already initialized
        if (!engine->isHealthy()) {
            if (!engine->initialize()) {
                Logger::error("Failed to initialize consensus engine: " + name);
                return false;
            }
        }
        
        // Store the engine
        engines[type] = std::move(engine);
        engineUsageCount[type] = 0;
        
        Logger::info("Consensus engine registered successfully: " + name);
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to register consensus engine: " + std::string(e.what()));
        return false;
    }
}

bool ConsensusRouter::unregisterEngine(ConsensusType type) {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    auto it = engines.find(type);
    if (it == engines.end()) {
        Logger::warning("Attempted to unregister non-existent engine: " + consensusTypeToString(type));
        return false;
    }
    
    try {
        Logger::info("Unregistering consensus engine: " + consensusTypeToString(type));
        
        // Shutdown the engine
        if (it->second) {
            it->second->shutdown();
        }
        
        engines.erase(it);
        engineUsageCount.erase(type);
        
        Logger::info("Consensus engine unregistered successfully: " + consensusTypeToString(type));
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to unregister consensus engine: " + std::string(e.what()));
        return false;
    }
}

ConsensusEngine* ConsensusRouter::getEngine(ConsensusType type) const {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    auto it = engines.find(type);
    return (it != engines.end()) ? it->second.get() : nullptr;
}

std::vector<ConsensusType> ConsensusRouter::getRegisteredEngines() const {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    std::vector<ConsensusType> types;
    for (const auto& [type, engine] : engines) {
        types.push_back(type);
    }
    
    return types;
}

std::vector<ConsensusEngine*> ConsensusRouter::getApplicableEngines(const ConsensusRequest& request) {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    std::vector<ConsensusEngine*> applicableEngines;
    
    try {
        // Find applicable routing rule
        RoutingRule rule = findApplicableRule(request);
        
        // Get required mechanisms
        std::vector<ConsensusType> requiredTypes = rule.requiredMechanisms;
        if (requiredTypes.empty()) {
            requiredTypes = determineRequiredMechanisms(request);
        }
        
        // Get optional mechanisms
        std::vector<ConsensusType> optionalTypes = rule.optionalMechanisms;
        if (optionalTypes.empty()) {
            optionalTypes = determineOptionalMechanisms(request);
        }
        
        // Add required engines
        for (ConsensusType type : requiredTypes) {
            auto it = engines.find(type);
            if (it != engines.end() && it->second && isEngineHealthy(type)) {
                applicableEngines.push_back(it->second.get());
            }
        }
        
        // Add optional engines if they're healthy and should be used
        for (ConsensusType type : optionalTypes) {
            auto it = engines.find(type);
            if (it != engines.end() && it->second && 
                isEngineHealthy(type) && shouldUseEngine(type, request)) {
                // Avoid duplicates
                if (std::find_if(applicableEngines.begin(), applicableEngines.end(),
                    [type](ConsensusEngine* engine) { return engine->getType() == type; }) 
                    == applicableEngines.end()) {
                    applicableEngines.push_back(it->second.get());
                }
            }
        }
        
        Logger::debug("Found " + std::to_string(applicableEngines.size()) + 
                     " applicable engines for request: " + request.requestId);
        
    } catch (const std::exception& e) {
        Logger::error("Error determining applicable engines: " + std::string(e.what()));
    }
    
    return applicableEngines;
}

ConsensusResult ConsensusRouter::routeValidation(const ConsensusRequest& request) {
    if (!initialized) {
        Logger::error("ConsensusRouter not initialized");
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "Router not initialized");
    }
    
    totalRequests++;
    
    try {
        Logger::info("Routing validation request: " + request.requestId + 
                    " (type: " + requestTypeToString(request.type) + ")");
        
        // Get applicable engines
        std::vector<ConsensusEngine*> engines = getApplicableEngines(request);
        
        if (engines.empty()) {
            Logger::warning("No applicable engines found for request: " + request.requestId);
            return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                                 "No applicable consensus engines");
        }
        
        // Process request through each engine
        std::vector<ConsensusResult> results;
        for (ConsensusEngine* engine : engines) {
            try {
                ConsensusResult result = engine->processRequest(request);
                result.metadata["engineName"] = engine->getName();
                result.metadata["requestId"] = request.requestId;
                results.push_back(result);
                
                // Update engine usage statistics
                incrementEngineUsage(engine->getType());
                
                Logger::debug("Engine " + engine->getName() + " result: " + 
                             (result.isValid ? "VALID" : "INVALID") + 
                             " (confidence: " + std::to_string(result.confidence) + ")");
                
            } catch (const std::exception& e) {
                Logger::error("Engine " + engine->getName() + " failed: " + std::string(e.what()));
                // Continue with other engines
            }
        }
        
        if (results.empty()) {
            Logger::error("All engines failed for request: " + request.requestId);
            return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                                 "All consensus engines failed");
        }
        
        // Get aggregation strategy for this request type
        AggregationStrategy strategy = getAggregationStrategy(request.type);
        
        // Aggregate results
        ConsensusResult finalResult = routeValidationWithStrategy(request, strategy);
        
        // Update statistics
        updateStatistics(request, finalResult);
        
        if (finalResult.isValid) {
            successfulRoutes++;
        }
        
        Logger::info("Validation routing completed for request: " + request.requestId + 
                    " - Result: " + (finalResult.isValid ? "VALID" : "INVALID") + 
                    " (confidence: " + std::to_string(finalResult.confidence) + ")");
        
        return finalResult;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to route validation request: " + std::string(e.what()));
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                              "Routing failed: " + std::string(e.what()));
    }
}

ConsensusResult ConsensusRouter::routeValidationWithStrategy(const ConsensusRequest& request, 
                                                           AggregationStrategy strategy) {
    // Get applicable engines and their results
    std::vector<ConsensusEngine*> engines = getApplicableEngines(request);
    
    if (engines.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                              "No applicable consensus engines");
    }
    
    // Collect results from all engines
    std::vector<ConsensusResult> results;
    for (ConsensusEngine* engine : engines) {
        try {
            ConsensusResult result = engine->processRequest(request);
            result.metadata["engineName"] = engine->getName();
            results.push_back(result);
        } catch (const std::exception& e) {
            Logger::error("Engine " + engine->getName() + " failed: " + std::string(e.what()));
        }
    }
    
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                              "All consensus engines failed");
    }
    
    // Apply aggregation strategy
    ConsensusResult aggregatedResult;
    switch (strategy) {
        case AggregationStrategy::UNANIMOUS:
            aggregatedResult = aggregateUnanimous(results);
            break;
        case AggregationStrategy::MAJORITY:
            aggregatedResult = aggregateMajority(results);
            break;
        case AggregationStrategy::WEIGHTED_MAJORITY:
            aggregatedResult = aggregateWeightedMajority(results);
            break;
        case AggregationStrategy::MOST_RESTRICTIVE:
            aggregatedResult = aggregateMostRestrictive(results);
            break;
        case AggregationStrategy::HIERARCHICAL:
        default:
            aggregatedResult = aggregateHierarchical(results);
            break;
    }
    
    // Add aggregation metadata
    aggregatedResult.metadata["aggregationStrategy"] = aggregationStrategyToString(strategy);
    aggregatedResult.metadata["engineCount"] = std::to_string(results.size());
    aggregatedResult.metadata["requestId"] = request.requestId;
    
    return aggregatedResult;
}

bool ConsensusRouter::aggregateResults(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return false;
    }
    
    // Simple aggregation - return true if any result is valid
    return std::any_of(results.begin(), results.end(), 
                      [](const ConsensusResult& result) { return result.isValid; });
}

ConflictInfo ConsensusRouter::detectAndResolveConflicts(const std::vector<ConsensusResult>& results,
                                                       AggregationStrategy strategy) {
    ConflictInfo conflictInfo;
    
    if (results.size() <= 1) {
        conflictInfo.hasConflict = false;
        if (!results.empty()) {
            conflictInfo.resolvedResult = results[0];
        }
        return conflictInfo;
    }
    
    // Check for conflicts
    conflictInfo.hasConflict = hasConflicts(results);
    
    if (conflictInfo.hasConflict) {
        conflictCount++;
        
        // Identify conflicting results
        std::vector<ConsensusResult> validResults = filterValidResults(results);
        std::vector<ConsensusResult> invalidResults = filterInvalidResults(results);
        
        conflictInfo.conflictingResults = results;
        
        // Resolve conflict based on strategy
        switch (strategy) {
            case AggregationStrategy::HIERARCHICAL:
                conflictInfo.resolvedResult = resolveConflictByHierarchy(results);
                conflictInfo.resolutionReason = "Resolved using consensus hierarchy";
                break;
            case AggregationStrategy::WEIGHTED_MAJORITY:
                conflictInfo.resolvedResult = resolveConflictByConfidence(results);
                conflictInfo.resolutionReason = "Resolved using weighted confidence";
                break;
            case AggregationStrategy::MOST_RESTRICTIVE:
                conflictInfo.resolvedResult = aggregateMostRestrictive(results);
                conflictInfo.resolutionReason = "Used most restrictive validation";
                break;
            default:
                conflictInfo.resolvedResult = resolveConflictByHierarchy(results);
                conflictInfo.resolutionReason = "Default hierarchical resolution";
                break;
        }
        
        conflictInfo.strategyUsed = strategy;
        
        Logger::warning("Consensus conflict detected and resolved: " + conflictInfo.resolutionReason);
        
    } else {
        // No conflict - use first result or aggregate normally
        conflictInfo.resolvedResult = results[0];
        conflictInfo.resolutionReason = "No conflict detected";
    }
    
    return conflictInfo;
}

// Routing rule management
bool ConsensusRouter::addRoutingRule(const RoutingRule& rule) {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    // Remove existing rule for the same request type
    removeRoutingRule(rule.requestType);
    
    routingRules.push_back(rule);
    Logger::info("Added routing rule for request type: " + requestTypeToString(rule.requestType));
    
    return true;
}

bool ConsensusRouter::removeRoutingRule(RequestType requestType) {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    auto it = std::remove_if(routingRules.begin(), routingRules.end(),
        [requestType](const RoutingRule& rule) { return rule.requestType == requestType; });
    
    if (it != routingRules.end()) {
        routingRules.erase(it, routingRules.end());
        Logger::info("Removed routing rule for request type: " + requestTypeToString(requestType));
        return true;
    }
    
    return false;
}

std::vector<RoutingRule> ConsensusRouter::getRoutingRules() const {
    std::lock_guard<std::mutex> lock(routerMutex);
    return routingRules;
}

void ConsensusRouter::setDefaultRoutingRules() {
    routingRules.clear();
    
    // Block validation - requires PoW and PoS, optionally PoRC
    routingRules.push_back(RoutingRule(
        RequestType::BLOCK_VALIDATION,
        {ConsensusType::PROOF_OF_WORK, ConsensusType::PROOF_OF_STAKE},
        {ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION},
        0.7, false
    ));
    
    // Transaction validation - requires PoW or PoS, optionally others
    routingRules.push_back(RoutingRule(
        RequestType::TRANSACTION_VALIDATION,
        {ConsensusType::PROOF_OF_WORK},
        {ConsensusType::PROOF_OF_STAKE, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION},
        0.6, false
    ));
    
    // Governance proposals - requires voting consensus
    routingRules.push_back(RoutingRule(
        RequestType::GOVERNANCE_PROPOSAL,
        {ConsensusType::VOTING_CONSENSUS},
        {ConsensusType::PROOF_OF_STAKE},
        0.67, true
    ));
    
    // Smart contract execution - requires smart contract validation
    routingRules.push_back(RoutingRule(
        RequestType::SMART_CONTRACT_EXECUTION,
        {ConsensusType::SMART_CONTRACT_VALIDATION},
        {ConsensusType::PROOF_OF_WORK, ConsensusType::PROOF_OF_STAKE},
        0.8, false
    ));
    
    // Parameter adjustment - requires multiple mechanisms
    routingRules.push_back(RoutingRule(
        RequestType::PARAMETER_ADJUSTMENT,
        {ConsensusType::VOTING_CONSENSUS, ConsensusType::PROOF_OF_STAKE},
        {ConsensusType::PROOF_OF_WORK},
        0.75, true
    ));
    
    Logger::info("Default routing rules set");
}

// Aggregation strategy management
bool ConsensusRouter::setAggregationStrategy(RequestType requestType, AggregationStrategy strategy) {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    aggregationStrategies[requestType] = strategy;
    Logger::info("Set aggregation strategy for " + requestTypeToString(requestType) + 
                ": " + aggregationStrategyToString(strategy));
    
    return true;
}

AggregationStrategy ConsensusRouter::getAggregationStrategy(RequestType requestType) const {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    auto it = aggregationStrategies.find(requestType);
    return (it != aggregationStrategies.end()) ? it->second : AggregationStrategy::HIERARCHICAL;
}

// Validation methods for specific request types
ConsensusResult ConsensusRouter::validateBlock(const Block& block) {
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, block.serialize());
    return routeValidation(request);
}

ConsensusResult ConsensusRouter::validateTransaction(const Transaction& transaction) {
    ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, transaction.serialize());
    return routeValidation(request);
}

ConsensusResult ConsensusRouter::validateGovernanceProposal(const std::string& proposalData) {
    ConsensusRequest request(RequestType::GOVERNANCE_PROPOSAL, proposalData);
    return routeValidation(request);
}

ConsensusResult ConsensusRouter::validateSmartContractExecution(const std::string& contractData) {
    ConsensusRequest request(RequestType::SMART_CONTRACT_EXECUTION, contractData);
    return routeValidation(request);
}

// Statistics and monitoring
nlohmann::json ConsensusRouter::getStatistics() const {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    nlohmann::json stats;
    stats["totalRequests"] = totalRequests;
    stats["successfulRoutes"] = successfulRoutes;
    stats["conflictCount"] = conflictCount;
    stats["successRate"] = totalRequests > 0 ? 
        static_cast<double>(successfulRoutes) / totalRequests : 0.0;
    stats["conflictRate"] = totalRequests > 0 ? 
        static_cast<double>(conflictCount) / totalRequests : 0.0;
    
    // Engine usage statistics
    nlohmann::json engineUsage;
    for (const auto& [type, count] : engineUsageCount) {
        engineUsage[consensusTypeToString(type)] = count;
    }
    stats["engineUsage"] = engineUsage;
    
    return stats;
}

nlohmann::json ConsensusRouter::getEngineStatus() const {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    nlohmann::json status;
    for (const auto& [type, engine] : engines) {
        nlohmann::json engineStatus;
        engineStatus["name"] = engine->getName();
        engineStatus["type"] = consensusTypeToString(type);
        engineStatus["healthy"] = engine->isHealthy();
        engineStatus["status"] = engine->getStatus();
        engineStatus["metrics"] = engine->getMetrics();
        
        status[consensusTypeToString(type)] = engineStatus;
    }
    
    return status;
}

void ConsensusRouter::resetStatistics() {
    std::lock_guard<std::mutex> lock(routerMutex);
    
    totalRequests = 0;
    successfulRoutes = 0;
    conflictCount = 0;
    engineUsageCount.clear();
    
    Logger::info("ConsensusRouter statistics reset");
}

// Private methods - Internal routing logic
std::vector<ConsensusType> ConsensusRouter::determineRequiredMechanisms(const ConsensusRequest& request) {
    std::vector<ConsensusType> required;
    
    switch (request.type) {
        case RequestType::BLOCK_VALIDATION:
            required = {ConsensusType::PROOF_OF_WORK, ConsensusType::PROOF_OF_STAKE};
            break;
        case RequestType::TRANSACTION_VALIDATION:
            required = {ConsensusType::PROOF_OF_WORK};
            break;
        case RequestType::GOVERNANCE_PROPOSAL:
            required = {ConsensusType::VOTING_CONSENSUS};
            break;
        case RequestType::SMART_CONTRACT_EXECUTION:
            required = {ConsensusType::SMART_CONTRACT_VALIDATION};
            break;
        case RequestType::PARAMETER_ADJUSTMENT:
            required = {ConsensusType::VOTING_CONSENSUS, ConsensusType::PROOF_OF_STAKE};
            break;
    }
    
    return required;
}

std::vector<ConsensusType> ConsensusRouter::determineOptionalMechanisms(const ConsensusRequest& request) {
    std::vector<ConsensusType> optional;
    
    switch (request.type) {
        case RequestType::BLOCK_VALIDATION:
            optional = {ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION};
            break;
        case RequestType::TRANSACTION_VALIDATION:
            optional = {ConsensusType::PROOF_OF_STAKE, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION};
            break;
        case RequestType::GOVERNANCE_PROPOSAL:
            optional = {ConsensusType::PROOF_OF_STAKE};
            break;
        case RequestType::SMART_CONTRACT_EXECUTION:
            optional = {ConsensusType::PROOF_OF_WORK, ConsensusType::PROOF_OF_STAKE};
            break;
        case RequestType::PARAMETER_ADJUSTMENT:
            optional = {ConsensusType::PROOF_OF_WORK};
            break;
    }
    
    return optional;
}

RoutingRule ConsensusRouter::findApplicableRule(const ConsensusRequest& request) {
    for (const RoutingRule& rule : routingRules) {
        if (rule.requestType == request.type) {
            return rule;
        }
    }
    
    // Return default rule if no specific rule found
    return RoutingRule(request.type, determineRequiredMechanisms(request), 
                      determineOptionalMechanisms(request));
}

// Result aggregation algorithms
ConsensusResult ConsensusRouter::aggregateUnanimous(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No results to aggregate");
    }
    
    // All results must be valid for unanimous agreement
    bool allValid = std::all_of(results.begin(), results.end(),
        [](const ConsensusResult& result) { return result.isValid; });
    
    if (!allValid) {
        return ConsensusResult(false, results[0].mechanism, 0.0, "Unanimous agreement failed");
    }
    
    // Calculate average confidence
    double avgConfidence = calculateAverageConfidence(results);
    
    return ConsensusResult(true, results[0].mechanism, avgConfidence, "Unanimous agreement");
}

ConsensusResult ConsensusRouter::aggregateMajority(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No results to aggregate");
    }
    
    std::vector<ConsensusResult> validResults = filterValidResults(results);
    std::vector<ConsensusResult> invalidResults = filterInvalidResults(results);
    
    bool majorityValid = validResults.size() > invalidResults.size();
    
    if (majorityValid) {
        double avgConfidence = calculateAverageConfidence(validResults);
        return ConsensusResult(true, validResults[0].mechanism, avgConfidence, "Majority agreement");
    } else {
        double avgConfidence = calculateAverageConfidence(invalidResults);
        return ConsensusResult(false, invalidResults[0].mechanism, avgConfidence, "Majority rejection");
    }
}

ConsensusResult ConsensusRouter::aggregateWeightedMajority(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No results to aggregate");
    }
    
    double totalValidWeight = 0.0;
    double totalInvalidWeight = 0.0;
    
    for (const ConsensusResult& result : results) {
        if (result.isValid) {
            totalValidWeight += result.confidence;
        } else {
            totalInvalidWeight += result.confidence;
        }
    }
    
    bool weightedMajorityValid = totalValidWeight > totalInvalidWeight;
    double totalWeight = totalValidWeight + totalInvalidWeight;
    double confidence = totalWeight > 0 ? 
        (weightedMajorityValid ? totalValidWeight / totalWeight : totalInvalidWeight / totalWeight) : 0.0;
    
    return ConsensusResult(weightedMajorityValid, results[0].mechanism, confidence, 
                          "Weighted majority decision");
}

ConsensusResult ConsensusRouter::aggregateMostRestrictive(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No results to aggregate");
    }
    
    // If any result is invalid, the aggregated result is invalid
    std::vector<ConsensusResult> invalidResults = filterInvalidResults(results);
    if (!invalidResults.empty()) {
        // Return the invalid result with highest confidence
        auto maxConfidenceResult = std::max_element(invalidResults.begin(), invalidResults.end(),
            [](const ConsensusResult& a, const ConsensusResult& b) {
                return a.confidence < b.confidence;
            });
        
        ConsensusResult result = *maxConfidenceResult;
        result.reason = "Most restrictive validation (rejected)";
        return result;
    }
    
    // All results are valid, return the one with lowest confidence
    auto minConfidenceResult = std::min_element(results.begin(), results.end(),
        [](const ConsensusResult& a, const ConsensusResult& b) {
            return a.confidence < b.confidence;
        });
    
    ConsensusResult result = *minConfidenceResult;
    result.reason = "Most restrictive validation (accepted)";
    return result;
}

ConsensusResult ConsensusRouter::aggregateHierarchical(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No results to aggregate");
    }
    
    // Sort results by consensus priority (lower number = higher priority)
    std::vector<ConsensusResult> sortedResults = results;
    std::sort(sortedResults.begin(), sortedResults.end(),
        [this](const ConsensusResult& a, const ConsensusResult& b) {
            return getConsensusPriority(a.mechanism) < getConsensusPriority(b.mechanism);
        });
    
    // Return the result from the highest priority mechanism
    ConsensusResult result = sortedResults[0];
    result.reason = "Hierarchical consensus decision";
    return result;
}

// Conflict detection and resolution
bool ConsensusRouter::hasConflicts(const std::vector<ConsensusResult>& results) {
    if (results.size() <= 1) {
        return false;
    }
    
    bool hasValid = false;
    bool hasInvalid = false;
    
    for (const ConsensusResult& result : results) {
        if (result.isValid) {
            hasValid = true;
        } else {
            hasInvalid = true;
        }
        
        if (hasValid && hasInvalid) {
            return true;
        }
    }
    
    return false;
}

ConsensusResult ConsensusRouter::resolveConflictByHierarchy(const std::vector<ConsensusResult>& results) {
    return aggregateHierarchical(results);
}

ConsensusResult ConsensusRouter::resolveConflictByConfidence(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No results to resolve");
    }
    
    // Return the result with highest confidence
    auto maxConfidenceResult = std::max_element(results.begin(), results.end(),
        [](const ConsensusResult& a, const ConsensusResult& b) {
            return a.confidence < b.confidence;
        });
    
    ConsensusResult result = *maxConfidenceResult;
    result.reason = "Resolved by highest confidence";
    return result;
}

// Helper methods
double ConsensusRouter::calculateAverageConfidence(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return 0.0;
    }
    
    double totalConfidence = std::accumulate(results.begin(), results.end(), 0.0,
        [](double sum, const ConsensusResult& result) {
            return sum + result.confidence;
        });
    
    return totalConfidence / results.size();
}

std::vector<ConsensusResult> ConsensusRouter::filterValidResults(const std::vector<ConsensusResult>& results) {
    std::vector<ConsensusResult> validResults;
    std::copy_if(results.begin(), results.end(), std::back_inserter(validResults),
        [](const ConsensusResult& result) { return result.isValid; });
    return validResults;
}

std::vector<ConsensusResult> ConsensusRouter::filterInvalidResults(const std::vector<ConsensusResult>& results) {
    std::vector<ConsensusResult> invalidResults;
    std::copy_if(results.begin(), results.end(), std::back_inserter(invalidResults),
        [](const ConsensusResult& result) { return !result.isValid; });
    return invalidResults;
}

int ConsensusRouter::getConsensusPriority(ConsensusType type) const {
    if (config && config->consensusPriority.find(type) != config->consensusPriority.end()) {
        return config->consensusPriority.at(type);
    }
    
    // Default priorities
    switch (type) {
        case ConsensusType::PROOF_OF_WORK: return 1;
        case ConsensusType::PROOF_OF_STAKE: return 2;
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION: return 3;
        case ConsensusType::VOTING_CONSENSUS: return 4;
        case ConsensusType::SMART_CONTRACT_VALIDATION: return 5;
        default: return 999;
    }
}

std::string ConsensusRouter::consensusTypeToString(ConsensusType type) const {
    switch (type) {
        case ConsensusType::PROOF_OF_WORK: return "PROOF_OF_WORK";
        case ConsensusType::PROOF_OF_STAKE: return "PROOF_OF_STAKE";
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION: return "PROOF_OF_RESOURCE_CONTRIBUTION";
        case ConsensusType::VOTING_CONSENSUS: return "VOTING_CONSENSUS";
        case ConsensusType::SMART_CONTRACT_VALIDATION: return "SMART_CONTRACT_VALIDATION";
        default: return "UNKNOWN";
    }
}

std::string ConsensusRouter::requestTypeToString(RequestType type) const {
    switch (type) {
        case RequestType::BLOCK_VALIDATION: return "BLOCK_VALIDATION";
        case RequestType::TRANSACTION_VALIDATION: return "TRANSACTION_VALIDATION";
        case RequestType::PARAMETER_ADJUSTMENT: return "PARAMETER_ADJUSTMENT";
        case RequestType::GOVERNANCE_PROPOSAL: return "GOVERNANCE_PROPOSAL";
        case RequestType::SMART_CONTRACT_EXECUTION: return "SMART_CONTRACT_EXECUTION";
        default: return "UNKNOWN";
    }
}

std::string ConsensusRouter::aggregationStrategyToString(AggregationStrategy strategy) const {
    switch (strategy) {
        case AggregationStrategy::UNANIMOUS: return "UNANIMOUS";
        case AggregationStrategy::MAJORITY: return "MAJORITY";
        case AggregationStrategy::WEIGHTED_MAJORITY: return "WEIGHTED_MAJORITY";
        case AggregationStrategy::MOST_RESTRICTIVE: return "MOST_RESTRICTIVE";
        case AggregationStrategy::HIERARCHICAL: return "HIERARCHICAL";
        default: return "UNKNOWN";
    }
}

// Validation helpers
bool ConsensusRouter::isEngineHealthy(ConsensusType type) const {
    auto it = engines.find(type);
    return (it != engines.end() && it->second && it->second->isHealthy());
}

bool ConsensusRouter::shouldUseEngine(ConsensusType type, const ConsensusRequest& request) const {
    // For now, use all healthy engines
    // This could be enhanced with more sophisticated logic
    return isEngineHealthy(type);
}

// Statistics helpers
void ConsensusRouter::updateStatistics(const ConsensusRequest& request, const ConsensusResult& result) {
    // Statistics are updated in the calling methods
    // This method could be used for additional custom statistics
}

void ConsensusRouter::incrementEngineUsage(ConsensusType type) {
    engineUsageCount[type]++;
}