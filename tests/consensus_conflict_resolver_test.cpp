#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>
#include "../include/core/consensus_conflict_resolver.h"
#include "../include/core/logger.h"

// Test helper functions
void testBasicInitialization();
void testHierarchyBasedResolution();
void testMostRestrictiveResolution();
void testConfidenceWeightedResolution();
void testManualInterventionRequest();
void testEmergencyModeActivation();
void testConflictSeverityAssessment();
void testResolutionStrategySuggestions();
void testStatisticsAndHistory();
void testConfigurationManagement();
void testCriticalConflictScenarios();
void testMultipleConflictResolution();

// Helper function to create test consensus results
std::vector<ConsensusResult> createTestResults(bool includeConflict = true) {
    std::vector<ConsensusResult> results;
    
    if (includeConflict) {
        // Create conflicting results
        results.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW validation passed"));
        results.push_back(ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS validation failed"));
        results.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.7, "PoRC validation passed"));
    } else {
        // Create non-conflicting results
        results.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW validation passed"));
        results.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS validation passed"));
        results.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.7, "PoRC validation passed"));
    }
    
    return results;
}

ConflictDetails createTestConflict(ConflictSeverity severity = ConflictSeverity::MEDIUM) {
    auto results = createTestResults(true);
    ConflictDetails conflict("TEST_CONFLICT_001", results, severity);
    conflict.description = "Test conflict for unit testing";
    return conflict;
}

int main() {
    std::cout << "Starting ConflictResolver tests..." << std::endl;
    
    try {
        testBasicInitialization();
        testHierarchyBasedResolution();
        testMostRestrictiveResolution();
        testConfidenceWeightedResolution();
        testManualInterventionRequest();
        testEmergencyModeActivation();
        testConflictSeverityAssessment();
        testResolutionStrategySuggestions();
        testStatisticsAndHistory();
        testConfigurationManagement();
        testCriticalConflictScenarios();
        testMultipleConflictResolution();
        
        std::cout << "All ConflictResolver tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

void testBasicInitialization() {
    std::cout << "Testing basic initialization..." << std::endl;
    
    ConflictResolver resolver;
    
    // Test initial state
    assert(!resolver.isInitialized());
    assert(!resolver.isEmergencyModeActive());
    
    // Test initialization
    ConflictResolverConfig config;
    config.enableAutomaticResolution = true;
    config.defaultStrategy = ResolutionStrategy::HIERARCHY_BASED;
    
    bool initResult = resolver.initialize(config);
    assert(initResult);
    assert(resolver.isInitialized());
    
    // Test configuration retrieval
    auto retrievedConfig = resolver.getConfig();
    assert(retrievedConfig.enableAutomaticResolution == config.enableAutomaticResolution);
    assert(retrievedConfig.defaultStrategy == config.defaultStrategy);
    
    // Test shutdown
    resolver.shutdown();
    
    std::cout << "✓ Basic initialization test passed" << std::endl;
}

void testHierarchyBasedResolution() {
    std::cout << "Testing hierarchy-based resolution..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Create test results with different priorities
    std::vector<ConsensusResult> results = {
        ConsensusResult(false, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.9, "PoRC failed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.8, "PoW passed"),
        ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.7, "PoS failed")
    };
    
    ResolutionResult resolution = resolver.resolveByHierarchy(results);
    
    // PoW should be selected as it has highest priority
    assert(resolution.resolved);
    assert(resolution.finalResult.mechanism == ConsensusType::PROOF_OF_WORK);
    assert(resolution.finalResult.isValid == true);
    assert(resolution.strategyUsed == ResolutionStrategy::HIERARCHY_BASED);
    
    std::cout << "✓ Hierarchy-based resolution test passed" << std::endl;
}

void testMostRestrictiveResolution() {
    std::cout << "Testing most restrictive resolution..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Test case 1: Mixed valid/invalid results - should select invalid
    std::vector<ConsensusResult> mixedResults = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW passed"),
        ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS failed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.7, "PoRC passed")
    };
    
    ResolutionResult resolution1 = resolver.resolveByMostRestrictive(mixedResults);
    assert(resolution1.resolved);
    assert(!resolution1.finalResult.isValid); // Should be invalid (most restrictive)
    assert(resolution1.finalResult.mechanism == ConsensusType::PROOF_OF_STAKE);
    
    // Test case 2: All valid results - should select lowest confidence
    std::vector<ConsensusResult> validResults = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW passed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.6, "PoS passed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.8, "PoRC passed")
    };
    
    ResolutionResult resolution2 = resolver.resolveByMostRestrictive(validResults);
    assert(resolution2.resolved);
    assert(resolution2.finalResult.isValid);
    assert(resolution2.finalResult.confidence == 0.6); // Lowest confidence
    
    std::cout << "✓ Most restrictive resolution test passed" << std::endl;
}

void testConfidenceWeightedResolution() {
    std::cout << "Testing confidence-weighted resolution..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    std::vector<ConsensusResult> results = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.7, "PoW passed"),
        ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.9, "PoS failed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.6, "PoRC passed")
    };
    
    ResolutionResult resolution = resolver.resolveByConfidenceWeighting(results);
    
    // Should select PoS result as it has highest confidence (0.9)
    assert(resolution.resolved);
    assert(resolution.finalResult.mechanism == ConsensusType::PROOF_OF_STAKE);
    assert(resolution.finalResult.confidence == 0.9);
    assert(resolution.strategyUsed == ResolutionStrategy::CONFIDENCE_WEIGHTED);
    
    std::cout << "✓ Confidence-weighted resolution test passed" << std::endl;
}

void testManualInterventionRequest() {
    std::cout << "Testing manual intervention request..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    ConflictDetails conflict = createTestConflict(ConflictSeverity::HIGH);
    
    // Request manual intervention
    std::string requestId = resolver.requestManualIntervention(conflict, true);
    assert(!requestId.empty());
    assert(requestId.find("MANUAL_") == 0);
    
    // Check pending interventions
    auto pending = resolver.getPendingInterventions();
    assert(pending.size() == 1);
    assert(pending[0].requestId == requestId);
    assert(pending[0].urgent == true);
    
    // Process manual resolution
    bool processResult = resolver.processManualResolution(requestId, 
        ResolutionStrategy::MOST_RESTRICTIVE, "Administrator resolved manually");
    assert(processResult);
    
    // Check that intervention is no longer pending
    pending = resolver.getPendingInterventions();
    assert(pending.size() == 0);
    
    std::cout << "✓ Manual intervention request test passed" << std::endl;
}

void testEmergencyModeActivation() {
    std::cout << "Testing emergency mode activation..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Test initial state
    assert(!resolver.isEmergencyModeActive());
    
    // Activate emergency mode
    bool activateResult = resolver.activateEmergencyMode("Test emergency activation");
    assert(activateResult);
    assert(resolver.isEmergencyModeActive());
    
    // Test that emergency mode prevents duplicate activation
    bool duplicateResult = resolver.activateEmergencyMode("Duplicate activation");
    assert(duplicateResult); // Should return true but not change state
    
    // Deactivate emergency mode
    bool deactivateResult = resolver.deactivateEmergencyMode();
    assert(deactivateResult);
    assert(!resolver.isEmergencyModeActive());
    
    // Test deactivating when not active
    bool duplicateDeactivate = resolver.deactivateEmergencyMode();
    assert(!duplicateDeactivate);
    
    std::cout << "✓ Emergency mode activation test passed" << std::endl;
}

void testConflictSeverityAssessment() {
    std::cout << "Testing conflict severity assessment..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Test critical conflict (valid vs invalid results)
    std::vector<ConsensusResult> criticalResults = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW passed"),
        ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS failed")
    };
    
    ConflictSeverity severity1 = resolver.assessConflictSeverity(criticalResults);
    assert(severity1 == ConflictSeverity::CRITICAL);
    
    // Test high severity conflict (large confidence difference)
    std::vector<ConsensusResult> highResults = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW passed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.4, "PoS passed")
    };
    
    ConflictSeverity severity2 = resolver.assessConflictSeverity(highResults);
    assert(severity2 == ConflictSeverity::HIGH);
    
    // Test low severity conflict (small confidence difference)
    std::vector<ConsensusResult> lowResults = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.8, "PoW passed"),
        ConsensusResult(true, ConsensusType::PROOF_OF_STAKE, 0.75, "PoS passed")
    };
    
    ConflictSeverity severity3 = resolver.assessConflictSeverity(lowResults);
    assert(severity3 == ConflictSeverity::LOW);
    
    std::cout << "✓ Conflict severity assessment test passed" << std::endl;
}

void testResolutionStrategySuggestions() {
    std::cout << "Testing resolution strategy suggestions..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Test suggestions for critical conflict
    ConflictDetails criticalConflict = createTestConflict(ConflictSeverity::CRITICAL);
    auto criticalStrategies = resolver.suggestResolutionStrategies(criticalConflict);
    
    assert(!criticalStrategies.empty());
    assert(std::find(criticalStrategies.begin(), criticalStrategies.end(), 
                    ResolutionStrategy::HIERARCHY_BASED) != criticalStrategies.end());
    assert(std::find(criticalStrategies.begin(), criticalStrategies.end(), 
                    ResolutionStrategy::MOST_RESTRICTIVE) != criticalStrategies.end());
    assert(std::find(criticalStrategies.begin(), criticalStrategies.end(), 
                    ResolutionStrategy::EMERGENCY_FALLBACK) != criticalStrategies.end());
    
    // Test suggestions for low severity conflict
    ConflictDetails lowConflict = createTestConflict(ConflictSeverity::LOW);
    auto lowStrategies = resolver.suggestResolutionStrategies(lowConflict);
    
    assert(!lowStrategies.empty());
    assert(std::find(lowStrategies.begin(), lowStrategies.end(), 
                    ResolutionStrategy::HIERARCHY_BASED) != lowStrategies.end());
    assert(std::find(lowStrategies.begin(), lowStrategies.end(), 
                    ResolutionStrategy::CONFIDENCE_WEIGHTED) != lowStrategies.end());
    
    std::cout << "✓ Resolution strategy suggestions test passed" << std::endl;
}

void testStatisticsAndHistory() {
    std::cout << "Testing statistics and history..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Initial statistics should be zero
    auto initialStats = resolver.getStatistics();
    assert(initialStats["totalConflictsResolved"] == 0);
    assert(initialStats["automaticResolutions"] == 0);
    
    // Resolve a conflict to generate statistics
    ConflictDetails conflict = createTestConflict();
    ResolutionResult result = resolver.resolveConflict(conflict);
    
    // Check updated statistics
    auto updatedStats = resolver.getStatistics();
    assert(updatedStats["totalConflictsResolved"] == 1);
    
    // Check conflict history
    auto history = resolver.getConflictHistory();
    assert(history.is_array());
    assert(history.size() == 1);
    
    // Reset statistics
    resolver.resetStatistics();
    auto resetStats = resolver.getStatistics();
    assert(resetStats["totalConflictsResolved"] == 0);
    
    std::cout << "✓ Statistics and history test passed" << std::endl;
}

void testConfigurationManagement() {
    std::cout << "Testing configuration management..." << std::endl;
    
    ConflictResolver resolver;
    
    // Test custom configuration
    ConflictResolverConfig customConfig;
    customConfig.enableAutomaticResolution = false;
    customConfig.defaultStrategy = ResolutionStrategy::MOST_RESTRICTIVE;
    customConfig.requireManualForCritical = true;
    customConfig.resolutionTimeout = 60;
    
    resolver.initialize(customConfig);
    
    auto retrievedConfig = resolver.getConfig();
    assert(retrievedConfig.enableAutomaticResolution == false);
    assert(retrievedConfig.defaultStrategy == ResolutionStrategy::MOST_RESTRICTIVE);
    assert(retrievedConfig.requireManualForCritical == true);
    assert(retrievedConfig.resolutionTimeout == 60);
    
    // Test configuration update
    ConflictResolverConfig updatedConfig = customConfig;
    updatedConfig.resolutionTimeout = 120;
    
    resolver.updateConfig(updatedConfig);
    auto newConfig = resolver.getConfig();
    assert(newConfig.resolutionTimeout == 120);
    
    std::cout << "✓ Configuration management test passed" << std::endl;
}

void testCriticalConflictScenarios() {
    std::cout << "Testing critical conflict scenarios..." << std::endl;
    
    ConflictResolver resolver;
    ConflictResolverConfig config;
    config.requireManualForCritical = false; // Allow automatic resolution for testing
    config.enableEmergencyMode = true;
    resolver.initialize(config);
    
    // Create critical conflict with PoW and PoS disagreement
    std::vector<ConsensusResult> criticalResults = {
        ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 0.9, "PoW validation passed"),
        ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 0.8, "PoS validation failed")
    };
    
    ConflictDetails criticalConflict("CRITICAL_001", criticalResults, ConflictSeverity::CRITICAL);
    criticalConflict.description = "Critical PoW vs PoS conflict";
    
    // Resolve the critical conflict
    ResolutionResult result = resolver.resolveConflict(criticalConflict);
    
    // Should either resolve automatically or activate emergency mode
    assert(result.resolved || resolver.isEmergencyModeActive());
    
    if (result.resolved && result.strategyUsed != ResolutionStrategy::EMERGENCY_FALLBACK) {
        // If resolved normally, should use hierarchy (PoW has higher priority)
        assert(result.finalResult.mechanism == ConsensusType::PROOF_OF_WORK);
        assert(result.finalResult.isValid == true);
    } else if (result.resolved && result.strategyUsed == ResolutionStrategy::EMERGENCY_FALLBACK) {
        // Emergency fallback should reject validation for safety
        assert(result.finalResult.isValid == false);
    }
    
    std::cout << "✓ Critical conflict scenarios test passed" << std::endl;
}

void testMultipleConflictResolution() {
    std::cout << "Testing multiple conflict resolution..." << std::endl;
    
    ConflictResolver resolver;
    resolver.initialize();
    
    // Create multiple conflicts with different severities
    std::vector<ConflictDetails> conflicts = {
        createTestConflict(ConflictSeverity::LOW),
        createTestConflict(ConflictSeverity::MEDIUM),
        createTestConflict(ConflictSeverity::HIGH)
    };
    
    conflicts[0].conflictId = "MULTI_001";
    conflicts[1].conflictId = "MULTI_002";
    conflicts[2].conflictId = "MULTI_003";
    
    // Resolve all conflicts
    std::vector<ResolutionResult> results;
    for (const auto& conflict : conflicts) {
        results.push_back(resolver.resolveConflict(conflict));
    }
    
    // Check that all conflicts were processed
    assert(results.size() == 3);
    
    // Check statistics
    auto stats = resolver.getStatistics();
    assert(stats["totalConflictsResolved"].get<uint64_t>() >= 2); // At least 2 should be resolved automatically
    
    // Check history
    auto history = resolver.getConflictHistory();
    assert(history.size() >= 2);
    
    std::cout << "✓ Multiple conflict resolution test passed" << std::endl;
}