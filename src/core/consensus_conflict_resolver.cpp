#include "../../include/core/consensus_conflict_resolver.h"
#include "../../include/core/consensus_router.h"
#include "../../include/core/consensus_monitor.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

ConflictResolver::ConflictResolver() : initialized(false) {
    Logger::info("ConflictResolver: Initializing consensus conflict resolver");
}

ConflictResolver::~ConflictResolver() {
    shutdown();
}

bool ConflictResolver::initialize(const ConflictResolverConfig& cfg) {
    std::lock_guard<std::mutex> lock(resolverMutex);
    
    if (initialized) {
        Logger::warning("ConflictResolver: Already initialized");
        return true;
    }
    
    try {
        config = cfg;
        
        // Initialize statistics
        totalConflictsResolved.store(0);
        automaticResolutions.store(0);
        manualInterventions.store(0);
        emergencyActivations.store(0);
        
        strategyUsageCount[ResolutionStrategy::HIERARCHY_BASED] = 0;
        strategyUsageCount[ResolutionStrategy::MOST_RESTRICTIVE] = 0;
        strategyUsageCount[ResolutionStrategy::CONFIDENCE_WEIGHTED] = 0;
        strategyUsageCount[ResolutionStrategy::MANUAL_INTERVENTION] = 0;
        strategyUsageCount[ResolutionStrategy::EMERGENCY_FALLBACK] = 0;
        
        // Clear any existing state
        {
            std::lock_guard<std::mutex> interventionLock(interventionMutex);
            while (!pendingInterventions.empty()) {
                pendingInterventions.pop();
            }
            activeInterventions.clear();
        }
        
        emergencyModeActive.store(false);
        
        initialized = true;
        Logger::info("ConflictResolver: Initialized successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("ConflictResolver: Failed to initialize: " + std::string(e.what()));
        return false;
    }
}

void ConflictResolver::shutdown() {
    std::lock_guard<std::mutex> lock(resolverMutex);
    
    if (!initialized) {
        return;
    }
    
    Logger::info("ConflictResolver: Shutting down");
    
    // Deactivate emergency mode if active
    if (emergencyModeActive.load()) {
        deactivateEmergencyMode();
    }
    
    // Clear pending interventions
    {
        std::lock_guard<std::mutex> interventionLock(interventionMutex);
        while (!pendingInterventions.empty()) {
            pendingInterventions.pop();
        }
        activeInterventions.clear();
    }
    
    initialized = false;
    Logger::info("ConflictResolver: Shutdown complete");
}

ResolutionResult ConflictResolver::resolveConflict(const ConflictDetails& conflict) {
    if (!initialized) {
        Logger::error("ConflictResolver: Not initialized");
        return ResolutionResult(false, ConsensusResult(), ResolutionStrategy::HIERARCHY_BASED, 
                               "Resolver not initialized");
    }
    
    Logger::info("ConflictResolver: Resolving conflict: " + conflict.conflictId);
    
    try {
        // Check if manual intervention is required
        if (requiresManualIntervention(conflict)) {
            Logger::info("ConflictResolver: Manual intervention required for conflict: " + conflict.conflictId);
            
            std::string requestId = requestManualIntervention(conflict, 
                conflict.severity == ConflictSeverity::CRITICAL);
            
            ResolutionResult result;
            result.resolved = false;
            result.manualInterventionRequired = true;
            result.strategyUsed = ResolutionStrategy::MANUAL_INTERVENTION;
            result.resolutionReason = "Manual intervention requested: " + requestId;
            result.recommendations.push_back("Administrator intervention required");
            result.recommendations.push_back("Request ID: " + requestId);
            
            return result;
        }
        
        // Check if emergency mode should be activated
        if (shouldActivateEmergencyMode(conflict)) {
            Logger::warning("ConflictResolver: Activating emergency mode for critical conflict");
            activateEmergencyMode("Critical consensus conflict detected: " + conflict.conflictId);
            
            ResolutionResult result;
            result.resolved = true;
            result.strategyUsed = ResolutionStrategy::EMERGENCY_FALLBACK;
            result.resolutionReason = "Emergency mode activated";
            result.finalResult = ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                                               "Emergency mode - validation rejected");
            
            return result;
        }
        
        // Use automatic resolution with configured strategy
        return resolveConflictWithStrategy(conflict, config.defaultStrategy);
        
    } catch (const std::exception& e) {
        Logger::error("ConflictResolver: Error resolving conflict: " + std::string(e.what()));
        
        ResolutionResult result;
        result.resolved = false;
        result.resolutionReason = "Resolution failed: " + std::string(e.what());
        return result;
    }
}

ResolutionResult ConflictResolver::resolveConflictWithStrategy(const ConflictDetails& conflict, 
                                                              ResolutionStrategy strategy) {
    Logger::info("ConflictResolver: Resolving conflict " + conflict.conflictId + 
                " with strategy: " + strategyToString(strategy));
    
    ResolutionResult result;
    
    try {
        switch (strategy) {
            case ResolutionStrategy::HIERARCHY_BASED:
                result = resolveByHierarchy(conflict.conflictingResults);
                break;
                
            case ResolutionStrategy::MOST_RESTRICTIVE:
                result = resolveByMostRestrictive(conflict.conflictingResults);
                break;
                
            case ResolutionStrategy::CONFIDENCE_WEIGHTED:
                result = resolveByConfidenceWeighting(conflict.conflictingResults);
                break;
                
            case ResolutionStrategy::MANUAL_INTERVENTION:
                {
                    std::string requestId = requestManualIntervention(conflict);
                    result.resolved = false;
                    result.manualInterventionRequired = true;
                    result.strategyUsed = strategy;
                    result.resolutionReason = "Manual intervention requested: " + requestId;
                }
                break;
                
            case ResolutionStrategy::EMERGENCY_FALLBACK:
                activateEmergencyMode("Emergency fallback strategy invoked for: " + conflict.conflictId);
                result.resolved = true;
                result.strategyUsed = strategy;
                result.resolutionReason = "Emergency fallback - validation rejected";
                result.finalResult = ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                                                   "Emergency fallback");
                break;
                
            default:
                Logger::warning("ConflictResolver: Unknown strategy, using hierarchy-based");
                result = resolveByHierarchy(conflict.conflictingResults);
                break;
        }
        
        // Update statistics and log the resolution
        updateStatistics(result);
        logConflictResolution(conflict, result);
        recordConflictHistory(conflict, result);
        
        if (result.resolved) {
            totalConflictsResolved.fetch_add(1);
            if (!result.manualInterventionRequired) {
                automaticResolutions.fetch_add(1);
            }
        }
        
        Logger::info("ConflictResolver: Conflict " + conflict.conflictId + 
                    (result.resolved ? " resolved" : " requires intervention"));
        
        return result;
        
    } catch (const std::exception& e) {
        Logger::error("ConflictResolver: Error in strategy execution: " + std::string(e.what()));
        
        result.resolved = false;
        result.resolutionReason = "Strategy execution failed: " + std::string(e.what());
        return result;
    }
}

ResolutionResult ConflictResolver::resolveByHierarchy(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ResolutionResult(false, ConsensusResult(), ResolutionStrategy::HIERARCHY_BASED,
                               "No results to resolve");
    }
    
    Logger::debug("ConflictResolver: Resolving by hierarchy with " + std::to_string(results.size()) + " results");
    
    // Select result based on consensus mechanism hierarchy
    ConsensusResult selectedResult = selectByHierarchy(results);
    
    ResolutionResult resolution(true, selectedResult, ResolutionStrategy::HIERARCHY_BASED,
                               "Resolved using consensus mechanism hierarchy");
    
    // Add recommendations based on the resolution
    resolution.recommendations.push_back("Used " + consensusTypeToString(selectedResult.mechanism) + 
                                        " as highest priority mechanism");
    
    if (!selectedResult.isValid) {
        resolution.recommendations.push_back("Validation rejected by highest priority mechanism");
    }
    
    return resolution;
}

ResolutionResult ConflictResolver::resolveByMostRestrictive(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ResolutionResult(false, ConsensusResult(), ResolutionStrategy::MOST_RESTRICTIVE,
                               "No results to resolve");
    }
    
    Logger::debug("ConflictResolver: Resolving by most restrictive validation");
    
    // Select the most restrictive result
    ConsensusResult selectedResult = selectMostRestrictive(results);
    
    ResolutionResult resolution(true, selectedResult, ResolutionStrategy::MOST_RESTRICTIVE,
                               "Resolved using most restrictive validation");
    
    // Add recommendations
    if (!selectedResult.isValid) {
        resolution.recommendations.push_back("Validation rejected by most restrictive mechanism");
        resolution.recommendations.push_back("At least one mechanism found the request invalid");
    } else {
        resolution.recommendations.push_back("All mechanisms validated successfully");
        resolution.recommendations.push_back("Used lowest confidence result for safety");
    }
    
    return resolution;
}

ResolutionResult ConflictResolver::resolveByConfidenceWeighting(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ResolutionResult(false, ConsensusResult(), ResolutionStrategy::CONFIDENCE_WEIGHTED,
                               "No results to resolve");
    }
    
    Logger::debug("ConflictResolver: Resolving by confidence weighting");
    
    // Select result based on confidence levels
    ConsensusResult selectedResult = selectByConfidence(results);
    
    ResolutionResult resolution(true, selectedResult, ResolutionStrategy::CONFIDENCE_WEIGHTED,
                               "Resolved using confidence-weighted selection");
    
    // Add recommendations
    resolution.recommendations.push_back("Selected result with highest confidence: " + 
                                        std::to_string(selectedResult.confidence));
    resolution.recommendations.push_back("Mechanism: " + consensusTypeToString(selectedResult.mechanism));
    
    return resolution;
}

std::string ConflictResolver::requestManualIntervention(const ConflictDetails& conflict, bool urgent) {
    std::lock_guard<std::mutex> lock(interventionMutex);
    
    ManualInterventionRequest request(conflict, urgent);
    
    // Add suggested strategies
    request.suggestedStrategies = suggestResolutionStrategies(conflict);
    
    // Store the request
    activeInterventions[request.requestId] = request;
    pendingInterventions.push(request);
    
    // Notify administrators
    notifyAdministrators(request);
    
    manualInterventions.fetch_add(1);
    
    Logger::info("ConflictResolver: Manual intervention requested: " + request.requestId + 
                (urgent ? " (URGENT)" : ""));
    
    return request.requestId;
}

bool ConflictResolver::processManualResolution(const std::string& requestId, 
                                              ResolutionStrategy strategy,
                                              const std::string& administratorNotes) {
    std::lock_guard<std::mutex> lock(interventionMutex);
    
    auto it = activeInterventions.find(requestId);
    if (it == activeInterventions.end()) {
        Logger::error("ConflictResolver: Manual intervention request not found: " + requestId);
        return false;
    }
    
    ManualInterventionRequest& request = it->second;
    request.administratorNotes = administratorNotes;
    
    Logger::info("ConflictResolver: Processing manual resolution for: " + requestId + 
                " with strategy: " + strategyToString(strategy));
    
    try {
        // Resolve the conflict with the specified strategy
        ResolutionResult result = resolveConflictWithStrategy(request.conflict, strategy);
        
        // Remove from active interventions
        activeInterventions.erase(it);
        
        // Remove from pending queue as well
        std::queue<ManualInterventionRequest> tempQueue;
        while (!pendingInterventions.empty()) {
            ManualInterventionRequest pending = pendingInterventions.front();
            pendingInterventions.pop();
            if (pending.requestId != requestId) {
                tempQueue.push(pending);
            }
        }
        pendingInterventions = tempQueue;
        
        // Log the manual resolution
        Logger::info("ConflictResolver: Manual resolution completed for: " + requestId);
        
        return result.resolved;
        
    } catch (const std::exception& e) {
        Logger::error("ConflictResolver: Error in manual resolution: " + std::string(e.what()));
        return false;
    }
}

std::vector<ManualInterventionRequest> ConflictResolver::getPendingInterventions() {
    std::lock_guard<std::mutex> lock(interventionMutex);
    
    std::vector<ManualInterventionRequest> pending;
    
    // Convert queue to vector for easier access
    std::queue<ManualInterventionRequest> tempQueue = pendingInterventions;
    while (!tempQueue.empty()) {
        pending.push_back(tempQueue.front());
        tempQueue.pop();
    }
    
    return pending;
}

bool ConflictResolver::cancelManualIntervention(const std::string& requestId) {
    std::lock_guard<std::mutex> lock(interventionMutex);
    
    auto it = activeInterventions.find(requestId);
    if (it == activeInterventions.end()) {
        Logger::warning("ConflictResolver: Cannot cancel non-existent intervention: " + requestId);
        return false;
    }
    
    activeInterventions.erase(it);
    
    Logger::info("ConflictResolver: Manual intervention cancelled: " + requestId);
    return true;
}

bool ConflictResolver::activateEmergencyMode(const std::string& reason) {
    std::lock_guard<std::mutex> lock(emergencyMutex);
    
    if (emergencyModeActive.load()) {
        Logger::warning("ConflictResolver: Emergency mode already active");
        return true;
    }
    
    emergencyModeActive.store(true);
    emergencyModeReason = reason;
    emergencyModeActivatedTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    emergencyActivations.fetch_add(1);
    
    Logger::critical("ConflictResolver: EMERGENCY MODE ACTIVATED - " + reason);
    
    // Notify monitor if available
    if (consensusMonitor) {
        // The monitor would handle emergency notifications
    }
    
    return true;
}

bool ConflictResolver::deactivateEmergencyMode() {
    std::lock_guard<std::mutex> lock(emergencyMutex);
    
    if (!emergencyModeActive.load()) {
        Logger::warning("ConflictResolver: Emergency mode not active");
        return false;
    }
    
    uint64_t duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - emergencyModeActivatedTime;
    
    emergencyModeActive.store(false);
    
    Logger::info("ConflictResolver: Emergency mode deactivated after " + 
                std::to_string(duration) + " seconds");
    
    return true;
}

ConflictSeverity ConflictResolver::assessConflictSeverity(const std::vector<ConsensusResult>& results) {
    if (results.empty()) {
        return ConflictSeverity::LOW;
    }
    
    // Calculate confidence difference
    double confidenceDiff = calculateConfidenceDifference(results);
    
    // Check for critical conflicts
    if (isConflictCritical(results)) {
        return ConflictSeverity::CRITICAL;
    }
    
    // Assess based on confidence difference
    if (confidenceDiff > config.criticalConflictThreshold) {
        return ConflictSeverity::HIGH;
    } else if (confidenceDiff > 0.15) {
        return ConflictSeverity::MEDIUM;
    } else {
        return ConflictSeverity::LOW;
    }
}

bool ConflictResolver::requiresManualIntervention(const ConflictDetails& conflict) {
    // Check configuration settings
    if (conflict.severity == ConflictSeverity::CRITICAL && config.requireManualForCritical) {
        return true;
    }
    
    if (conflict.severity == ConflictSeverity::HIGH && config.requireManualForHigh) {
        return true;
    }
    
    // Check if conflict is explicitly marked for manual intervention
    if (conflict.requiresManualIntervention) {
        return true;
    }
    
    // Check if emergency mode is active
    if (emergencyModeActive.load()) {
        return true;
    }
    
    return false;
}

std::vector<ResolutionStrategy> ConflictResolver::suggestResolutionStrategies(const ConflictDetails& conflict) {
    std::vector<ResolutionStrategy> strategies;
    
    // Always suggest hierarchy-based as primary
    strategies.push_back(ResolutionStrategy::HIERARCHY_BASED);
    
    // Suggest most restrictive for security-critical conflicts
    if (conflict.severity >= ConflictSeverity::HIGH) {
        strategies.push_back(ResolutionStrategy::MOST_RESTRICTIVE);
    }
    
    // Suggest confidence weighting for performance conflicts
    if (conflict.severity <= ConflictSeverity::MEDIUM) {
        strategies.push_back(ResolutionStrategy::CONFIDENCE_WEIGHTED);
    }
    
    // Suggest emergency fallback for critical conflicts
    if (conflict.severity == ConflictSeverity::CRITICAL) {
        strategies.push_back(ResolutionStrategy::EMERGENCY_FALLBACK);
    }
    
    return strategies;
}

nlohmann::json ConflictResolver::getStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex);
    
    nlohmann::json stats;
    
    stats["totalConflictsResolved"] = totalConflictsResolved.load();
    stats["automaticResolutions"] = automaticResolutions.load();
    stats["manualInterventions"] = manualInterventions.load();
    stats["emergencyActivations"] = emergencyActivations.load();
    
    // Strategy usage statistics
    nlohmann::json strategyStats;
    for (const auto& [strategy, count] : strategyUsageCount) {
        strategyStats[strategyToString(strategy)] = count;
    }
    stats["strategyUsage"] = strategyStats;
    
    // Current state
    stats["emergencyModeActive"] = emergencyModeActive.load();
    if (emergencyModeActive.load()) {
        stats["emergencyModeReason"] = emergencyModeReason;
        stats["emergencyModeActivatedTime"] = emergencyModeActivatedTime;
    }
    
    // Pending interventions
    {
        std::lock_guard<std::mutex> interventionLock(interventionMutex);
        stats["pendingInterventions"] = pendingInterventions.size();
        stats["activeInterventions"] = activeInterventions.size();
    }
    
    return stats;
}

nlohmann::json ConflictResolver::getConflictHistory() const {
    std::lock_guard<std::mutex> lock(historyMutex);
    
    nlohmann::json history = nlohmann::json::array();
    
    for (size_t i = 0; i < resolvedConflicts.size() && i < resolutionHistory.size(); ++i) {
        nlohmann::json entry;
        
        const auto& conflict = resolvedConflicts[i];
        const auto& resolution = resolutionHistory[i];
        
        entry["conflictId"] = conflict.conflictId;
        entry["timestamp"] = conflict.timestamp;
        entry["severity"] = severityToString(conflict.severity);
        entry["description"] = conflict.description;
        entry["resolved"] = resolution.resolved;
        entry["strategy"] = strategyToString(resolution.strategyUsed);
        entry["resolutionReason"] = resolution.resolutionReason;
        entry["resolutionTime"] = resolution.resolutionTime;
        entry["manualInterventionRequired"] = resolution.manualInterventionRequired;
        
        history.push_back(entry);
    }
    
    return history;
}

void ConflictResolver::resetStatistics() {
    std::lock_guard<std::mutex> lock(statisticsMutex);
    
    totalConflictsResolved.store(0);
    automaticResolutions.store(0);
    manualInterventions.store(0);
    emergencyActivations.store(0);
    
    for (auto& [strategy, count] : strategyUsageCount) {
        count = 0;
    }
    
    Logger::info("ConflictResolver: Statistics reset");
}

void ConflictResolver::updateConfig(const ConflictResolverConfig& newConfig) {
    std::lock_guard<std::mutex> lock(resolverMutex);
    
    config = newConfig;
    Logger::info("ConflictResolver: Configuration updated");
}

// Private helper methods

std::string ConflictResolver::generateConflictId() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "CONFLICT_" + std::to_string(timestamp);
}

int ConflictResolver::getConsensusPriority(ConsensusType type) const {
    // Default priority order (lower number = higher priority)
    switch (type) {
        case ConsensusType::PROOF_OF_WORK:
            return 1;
        case ConsensusType::PROOF_OF_STAKE:
            return 2;
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
            return 3;
        case ConsensusType::VOTING_CONSENSUS:
            return 4;
        case ConsensusType::SMART_CONTRACT_VALIDATION:
            return 5;
        default:
            return 999;
    }
}

double ConflictResolver::calculateConfidenceDifference(const std::vector<ConsensusResult>& results) const {
    if (results.size() < 2) {
        return 0.0;
    }
    
    double maxConfidence = 0.0;
    double minConfidence = 1.0;
    
    for (const auto& result : results) {
        maxConfidence = std::max(maxConfidence, result.confidence);
        minConfidence = std::min(minConfidence, result.confidence);
    }
    
    return maxConfidence - minConfidence;
}

bool ConflictResolver::isConflictCritical(const std::vector<ConsensusResult>& results) const {
    // Check if there are both valid and invalid results
    bool hasValid = false;
    bool hasInvalid = false;
    
    for (const auto& result : results) {
        if (result.isValid) {
            hasValid = true;
        } else {
            hasInvalid = true;
        }
        
        if (hasValid && hasInvalid) {
            return true; // Critical conflict: mechanisms disagree on validity
        }
    }
    
    return false;
}

ConsensusResult ConflictResolver::selectByHierarchy(const std::vector<ConsensusResult>& results) const {
    if (results.empty()) {
        return ConsensusResult();
    }
    
    // Sort by priority (lower priority number = higher priority)
    auto sortedResults = results;
    std::sort(sortedResults.begin(), sortedResults.end(),
        [this](const ConsensusResult& a, const ConsensusResult& b) {
            return getConsensusPriority(a.mechanism) < getConsensusPriority(b.mechanism);
        });
    
    return sortedResults[0];
}

ConsensusResult ConflictResolver::selectMostRestrictive(const std::vector<ConsensusResult>& results) const {
    if (results.empty()) {
        return ConsensusResult();
    }
    
    // If any result is invalid, return the invalid result with highest confidence
    std::vector<ConsensusResult> invalidResults;
    for (const auto& result : results) {
        if (!result.isValid) {
            invalidResults.push_back(result);
        }
    }
    
    if (!invalidResults.empty()) {
        auto maxConfidenceInvalid = std::max_element(invalidResults.begin(), invalidResults.end(),
            [](const ConsensusResult& a, const ConsensusResult& b) {
                return a.confidence < b.confidence;
            });
        return *maxConfidenceInvalid;
    }
    
    // All results are valid, return the one with lowest confidence (most restrictive)
    auto minConfidenceValid = std::min_element(results.begin(), results.end(),
        [](const ConsensusResult& a, const ConsensusResult& b) {
            return a.confidence < b.confidence;
        });
    
    return *minConfidenceValid;
}

ConsensusResult ConflictResolver::selectByConfidence(const std::vector<ConsensusResult>& results) const {
    if (results.empty()) {
        return ConsensusResult();
    }
    
    // Return the result with highest confidence
    auto maxConfidenceResult = std::max_element(results.begin(), results.end(),
        [](const ConsensusResult& a, const ConsensusResult& b) {
            return a.confidence < b.confidence;
        });
    
    return *maxConfidenceResult;
}

void ConflictResolver::processInterventionQueue() {
    std::lock_guard<std::mutex> lock(interventionMutex);
    
    // Remove expired interventions
    auto it = activeInterventions.begin();
    while (it != activeInterventions.end()) {
        if (isInterventionExpired(it->second)) {
            Logger::warning("ConflictResolver: Manual intervention expired: " + it->first);
            it = activeInterventions.erase(it);
        } else {
            ++it;
        }
    }
}

bool ConflictResolver::isInterventionExpired(const ManualInterventionRequest& request) const {
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return (currentTime - request.requestTime) > config.manualInterventionTimeout;
}

void ConflictResolver::notifyAdministrators(const ManualInterventionRequest& request) {
    if (!config.notifyAdministrators) {
        return;
    }
    
    std::string urgencyStr = request.urgent ? " [URGENT]" : "";
    Logger::warning("ConflictResolver: ADMINISTRATOR NOTIFICATION" + urgencyStr + 
                   " - Manual intervention required: " + request.requestId);
    
    // In a real implementation, this would send emails, SMS, or other notifications
}

bool ConflictResolver::shouldActivateEmergencyMode(const ConflictDetails& conflict) const {
    if (!config.enableEmergencyMode) {
        return false;
    }
    
    // Activate for critical conflicts involving security mechanisms
    if (conflict.severity == ConflictSeverity::CRITICAL) {
        for (const auto& mechanism : conflict.involvedMechanisms) {
            if (mechanism == ConsensusType::PROOF_OF_WORK || 
                mechanism == ConsensusType::PROOF_OF_STAKE) {
                return true;
            }
        }
    }
    
    return false;
}

void ConflictResolver::logConflictResolution(const ConflictDetails& conflict, 
                                           const ResolutionResult& result) {
    if (!config.logAllConflicts) {
        return;
    }
    
    std::string logMessage = "Conflict " + conflict.conflictId + " " + 
                           (result.resolved ? "resolved" : "unresolved") + 
                           " using " + strategyToString(result.strategyUsed) + 
                           " - " + result.resolutionReason;
    
    Logger::info("ConflictResolver: " + logMessage);
}

void ConflictResolver::updateStatistics(const ResolutionResult& result) {
    std::lock_guard<std::mutex> lock(statisticsMutex);
    
    strategyUsageCount[result.strategyUsed]++;
}

void ConflictResolver::recordConflictHistory(const ConflictDetails& conflict, 
                                           const ResolutionResult& result) {
    std::lock_guard<std::mutex> lock(historyMutex);
    
    resolvedConflicts.push_back(conflict);
    resolutionHistory.push_back(result);
    
    // Keep only the last 1000 entries to prevent unlimited growth
    if (resolvedConflicts.size() > 1000) {
        resolvedConflicts.erase(resolvedConflicts.begin());
        resolutionHistory.erase(resolutionHistory.begin());
    }
}

std::string ConflictResolver::consensusTypeToString(ConsensusType type) const {
    switch (type) {
        case ConsensusType::PROOF_OF_WORK:
            return "PROOF_OF_WORK";
        case ConsensusType::PROOF_OF_STAKE:
            return "PROOF_OF_STAKE";
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
            return "PROOF_OF_RESOURCE_CONTRIBUTION";
        case ConsensusType::VOTING_CONSENSUS:
            return "VOTING_CONSENSUS";
        case ConsensusType::SMART_CONTRACT_VALIDATION:
            return "SMART_CONTRACT_VALIDATION";
        default:
            return "UNKNOWN";
    }
}

std::string ConflictResolver::severityToString(ConflictSeverity severity) const {
    switch (severity) {
        case ConflictSeverity::LOW:
            return "LOW";
        case ConflictSeverity::MEDIUM:
            return "MEDIUM";
        case ConflictSeverity::HIGH:
            return "HIGH";
        case ConflictSeverity::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

std::string ConflictResolver::strategyToString(ResolutionStrategy strategy) const {
    switch (strategy) {
        case ResolutionStrategy::HIERARCHY_BASED:
            return "HIERARCHY_BASED";
        case ResolutionStrategy::MOST_RESTRICTIVE:
            return "MOST_RESTRICTIVE";
        case ResolutionStrategy::CONFIDENCE_WEIGHTED:
            return "CONFIDENCE_WEIGHTED";
        case ResolutionStrategy::MANUAL_INTERVENTION:
            return "MANUAL_INTERVENTION";
        case ResolutionStrategy::EMERGENCY_FALLBACK:
            return "EMERGENCY_FALLBACK";
        default:
            return "UNKNOWN";
    }
}

ConflictSeverity ConflictResolver::stringToSeverity(const std::string& severity) const {
    if (severity == "LOW") return ConflictSeverity::LOW;
    if (severity == "MEDIUM") return ConflictSeverity::MEDIUM;
    if (severity == "HIGH") return ConflictSeverity::HIGH;
    if (severity == "CRITICAL") return ConflictSeverity::CRITICAL;
    return ConflictSeverity::MEDIUM;
}

ResolutionStrategy ConflictResolver::stringToStrategy(const std::string& strategy) const {
    if (strategy == "HIERARCHY_BASED") return ResolutionStrategy::HIERARCHY_BASED;
    if (strategy == "MOST_RESTRICTIVE") return ResolutionStrategy::MOST_RESTRICTIVE;
    if (strategy == "CONFIDENCE_WEIGHTED") return ResolutionStrategy::CONFIDENCE_WEIGHTED;
    if (strategy == "MANUAL_INTERVENTION") return ResolutionStrategy::MANUAL_INTERVENTION;
    if (strategy == "EMERGENCY_FALLBACK") return ResolutionStrategy::EMERGENCY_FALLBACK;
    return ResolutionStrategy::HIERARCHY_BASED;
}