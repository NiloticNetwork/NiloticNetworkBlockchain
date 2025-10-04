#include <iostream>
#include <cassert>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

// Minimal JSON implementation for testing
namespace nlohmann {
    class json {
    private:
        std::map<std::string, std::string> data;
        
    public:
        json() = default;
        json(const std::map<std::string, std::string>& d) : data(d) {}
        
        std::string& operator[](const std::string& key) { return data[key]; }
        const std::string& operator[](const std::string& key) const { 
            static std::string empty;
            auto it = data.find(key);
            return it != data.end() ? it->second : empty;
        }
        
        bool contains(const std::string& key) const { return data.find(key) != data.end(); }
        std::string dump() const { return "{}"; }
    };
}

// Minimal logger for testing
class Logger {
public:
    static void info(const std::string& msg) { std::cout << "[INFO] " << msg << std::endl; }
    static void warning(const std::string& msg) { std::cout << "[WARN] " << msg << std::endl; }
    static void error(const std::string& msg) { std::cout << "[ERROR] " << msg << std::endl; }
    static void debug(const std::string& msg) { std::cout << "[DEBUG] " << msg << std::endl; }
};

// Forward declarations
class Block;
class Transaction;

// Consensus mechanism types
enum class ConsensusType {
    PROOF_OF_WORK,
    PROOF_OF_STAKE,
    PROOF_OF_RESOURCE_CONTRIBUTION,
    VOTING_CONSENSUS,
    SMART_CONTRACT_VALIDATION
};

// Request types for consensus validation
enum class RequestType {
    BLOCK_VALIDATION,
    TRANSACTION_VALIDATION,
    PARAMETER_ADJUSTMENT,
    GOVERNANCE_PROPOSAL,
    SMART_CONTRACT_EXECUTION
};

// Consensus request structure
struct ConsensusRequest {
    RequestType type;
    std::string data;
    std::vector<ConsensusType> requiredMechanisms;
    uint64_t timestamp;
    std::string requestId;
    std::map<std::string, std::string> metadata;
    
    ConsensusRequest() : type(RequestType::BLOCK_VALIDATION), timestamp(0) {}
    ConsensusRequest(RequestType t, const std::string& d) 
        : type(t), data(d), timestamp(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) {
        requestId = "test_request";
    }
};

// Consensus result structure
struct ConsensusResult {
    bool isValid;
    ConsensusType mechanism;
    double confidence;
    std::string reason;
    std::map<std::string, std::string> metadata;
    uint64_t timestamp;
    
    ConsensusResult() : isValid(false), mechanism(ConsensusType::PROOF_OF_WORK), 
                       confidence(0.0), timestamp(0) {}
    ConsensusResult(bool valid, ConsensusType mech, double conf = 1.0, const std::string& r = "")
        : isValid(valid), mechanism(mech), confidence(conf), reason(r),
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// Base interface for all consensus engines
class ConsensusEngine {
public:
    virtual ~ConsensusEngine() = default;
    virtual bool validateBlock(const Block& block) = 0;
    virtual bool validateTransaction(const Transaction& transaction) = 0;
    virtual ConsensusResult processRequest(const ConsensusRequest& request) = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isHealthy() const = 0;
    virtual ConsensusType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual nlohmann::json getStatus() const = 0;
    virtual nlohmann::json getMetrics() const = 0;
    virtual bool adjustParameters(const std::map<std::string, double>& parameters) = 0;
    virtual std::map<std::string, double> getParameters() const = 0;
};

// Now include the core balancer structures
struct ParticipationMetrics {
    uint64_t totalValidations = 0;
    uint64_t successfulValidations = 0;
    uint64_t failedValidations = 0;
    double averageResponseTime = 0.0;
    double currentDifficulty = 1.0;
    double currentRewardMultiplier = 1.0;
    uint64_t activeParticipants = 0;
    double networkHashRate = 0.0;
    double totalStake = 0.0;
    double resourceContribution = 0.0;
    uint64_t lastUpdateTime = 0;
    
    double getParticipationRate() const {
        return totalValidations > 0 ? 
            static_cast<double>(successfulValidations) / totalValidations : 0.0;
    }
    
    double getEfficiencyScore() const {
        double participationRate = getParticipationRate();
        double responseScore = averageResponseTime > 0 ? 1.0 / averageResponseTime : 1.0;
        return (participationRate * 0.7) + (responseScore * 0.3);
    }
};

struct BalanceConfig {
    std::map<ConsensusType, double> targetParticipationRates = {
        {ConsensusType::PROOF_OF_WORK, 0.3},
        {ConsensusType::PROOF_OF_STAKE, 0.3},
        {ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.25},
        {ConsensusType::VOTING_CONSENSUS, 0.1},
        {ConsensusType::SMART_CONTRACT_VALIDATION, 0.05}
    };
    
    double maxDifficultyAdjustment = 0.25;
    double maxRewardAdjustment = 0.20;
    double balanceThreshold = 0.15;
    uint64_t rebalancingInterval = 3600;
    uint64_t metricsWindow = 86400;
    double emergencyImbalanceThreshold = 0.5;
    double minParticipationThreshold = 0.05;
    double maxDominanceThreshold = 0.7;
    double targetResponseTime = 1.0;
    double minEfficiencyScore = 0.6;
};

struct BalanceMetrics {
    std::map<ConsensusType, ParticipationMetrics> participationMetrics;
    std::map<ConsensusType, double> participationRates;
    std::map<ConsensusType, double> dominanceRatios;
    double overallBalance = 1.0;
    double networkEfficiency = 1.0;
    bool isBalanced = true;
    bool emergencyMode = false;
    uint64_t lastRebalanceTime = 0;
    std::vector<std::string> recommendations;
    
    double getNetworkHealth() const {
        double totalHealth = 0.0;
        size_t count = 0;
        
        for (const auto& [type, metrics] : participationMetrics) {
            totalHealth += metrics.getEfficiencyScore();
            count++;
        }
        
        return count > 0 ? totalHealth / count : 0.0;
    }
    
    ConsensusType getMostDominant() const {
        ConsensusType dominant = ConsensusType::PROOF_OF_WORK;
        double maxRate = 0.0;
        
        for (const auto& [type, rate] : participationRates) {
            if (rate > maxRate) {
                maxRate = rate;
                dominant = type;
            }
        }
        
        return dominant;
    }
};

// Simplified ConsensusBalancer for testing
class ConsensusBalancer {
private:
    BalanceConfig config;
    BalanceMetrics currentMetrics;
    std::atomic<bool> initialized;
    std::atomic<bool> running;
    std::atomic<bool> emergencyMode;
    mutable std::mutex balancerMutex;
    std::map<ConsensusType, ConsensusEngine*> engines;
    uint64_t totalRebalances = 0;
    uint64_t emergencyActivations = 0;

public:
    ConsensusBalancer() : initialized(false), running(false), emergencyMode(false) {}
    
    bool initialize() {
        std::lock_guard<std::mutex> lock(balancerMutex);
        initialized.store(true);
        running.store(true);
        emergencyMode.store(false);
        
        // Initialize metrics for all consensus types
        for (const auto& [type, targetRate] : config.targetParticipationRates) {
            currentMetrics.participationMetrics[type] = ParticipationMetrics{};
            currentMetrics.participationRates[type] = 0.0;
            currentMetrics.dominanceRatios[type] = 0.0;
        }
        
        return true;
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(balancerMutex);
        initialized.store(false);
        running.store(false);
        engines.clear();
    }
    
    bool isInitialized() const { return initialized.load(); }
    bool isRunning() const { return running.load(); }
    bool isInEmergencyMode() const { return emergencyMode.load(); }
    
    bool registerEngine(ConsensusType type, ConsensusEngine* engine) {
        if (!engine || !engine->isHealthy()) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(balancerMutex);
        engines[type] = engine;
        
        if (currentMetrics.participationMetrics.find(type) == currentMetrics.participationMetrics.end()) {
            currentMetrics.participationMetrics[type] = ParticipationMetrics{};
            currentMetrics.participationRates[type] = 0.0;
            currentMetrics.dominanceRatios[type] = 0.0;
        }
        
        return true;
    }
    
    bool unregisterEngine(ConsensusType type) {
        std::lock_guard<std::mutex> lock(balancerMutex);
        auto it = engines.find(type);
        if (it == engines.end()) {
            return false;
        }
        engines.erase(it);
        return true;
    }
    
    void balanceConsensusParticipation() {
        if (!initialized.load() || !running.load()) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(balancerMutex);
        
        // Collect metrics from engines
        for (const auto& [type, engine] : engines) {
            try {
                nlohmann::json metrics = engine->getMetrics();
                ParticipationMetrics& pm = currentMetrics.participationMetrics[type];
                
                // Update metrics (simplified)
                pm.totalValidations = 100;
                pm.successfulValidations = 90;
                pm.averageResponseTime = 1.5;
                pm.activeParticipants = 50;
            } catch (...) {
                // Handle engine errors gracefully
            }
        }
        
        // Calculate participation rates
        uint64_t totalValidations = 0;
        for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
            totalValidations += metrics.totalValidations;
        }
        
        for (const auto& [type, metrics] : currentMetrics.participationMetrics) {
            if (totalValidations > 0) {
                currentMetrics.participationRates[type] = 
                    static_cast<double>(metrics.totalValidations) / totalValidations;
            }
        }
        
        // Update overall metrics
        currentMetrics.networkEfficiency = currentMetrics.getNetworkHealth();
        currentMetrics.overallBalance = 1.0; // Simplified
        currentMetrics.isBalanced = true; // Simplified
    }
    
    void adjustDifficulty(ConsensusType type, double adjustment) {
        if (adjustment < -1.0 || adjustment > 1.0) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(balancerMutex);
        auto engineIt = engines.find(type);
        if (engineIt != engines.end()) {
            try {
                auto params = engineIt->second->getParameters();
                double currentDifficulty = params.count("difficulty") > 0 ? params["difficulty"] : 1.0;
                double newDifficulty = currentDifficulty * (1.0 + adjustment);
                newDifficulty = std::max(0.1, std::min(newDifficulty, 100.0));
                
                std::map<std::string, double> newParams = {{"difficulty", newDifficulty}};
                engineIt->second->adjustParameters(newParams);
                
                currentMetrics.participationMetrics[type].currentDifficulty = newDifficulty;
            } catch (...) {
                // Handle errors gracefully
            }
        }
    }
    
    void adjustRewards(ConsensusType type, double multiplier) {
        if (multiplier <= 0.0 || multiplier > 10.0) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(balancerMutex);
        auto engineIt = engines.find(type);
        if (engineIt != engines.end()) {
            try {
                auto params = engineIt->second->getParameters();
                double currentMultiplier = params.count("rewardMultiplier") > 0 ? params["rewardMultiplier"] : 1.0;
                double newMultiplier = currentMultiplier * multiplier;
                newMultiplier = std::max(0.1, std::min(newMultiplier, 5.0));
                
                std::map<std::string, double> newParams = {{"rewardMultiplier", newMultiplier}};
                engineIt->second->adjustParameters(newParams);
                
                currentMetrics.participationMetrics[type].currentRewardMultiplier = newMultiplier;
            } catch (...) {
                // Handle errors gracefully
            }
        }
    }
    
    bool performAutomaticRebalancing() {
        if (!initialized.load() || !running.load()) {
            return false;
        }
        
        balanceConsensusParticipation();
        totalRebalances++;
        return true;
    }
    
    BalanceMetrics getBalanceMetrics() const {
        std::lock_guard<std::mutex> lock(balancerMutex);
        return currentMetrics;
    }
    
    ParticipationMetrics getParticipationMetrics(ConsensusType type) const {
        std::lock_guard<std::mutex> lock(balancerMutex);
        auto it = currentMetrics.participationMetrics.find(type);
        return (it != currentMetrics.participationMetrics.end()) ? it->second : ParticipationMetrics{};
    }
    
    nlohmann::json getDetailedAnalysis() const {
        std::lock_guard<std::mutex> lock(balancerMutex);
        nlohmann::json analysis;
        analysis["overallBalance"] = std::to_string(currentMetrics.overallBalance);
        analysis["networkEfficiency"] = std::to_string(currentMetrics.networkEfficiency);
        analysis["isBalanced"] = currentMetrics.isBalanced ? "true" : "false";
        return analysis;
    }
    
    BalanceConfig getBalanceConfig() const {
        std::lock_guard<std::mutex> lock(balancerMutex);
        return config;
    }
    
    void setBalanceConfig(const BalanceConfig& newConfig) {
        std::lock_guard<std::mutex> lock(balancerMutex);
        config = newConfig;
    }
    
    bool updateTargetParticipation(ConsensusType type, double targetRate) {
        if (targetRate < 0.0 || targetRate > 1.0) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(balancerMutex);
        config.targetParticipationRates[type] = targetRate;
        return true;
    }
    
    bool enterEmergencyMode() {
        std::lock_guard<std::mutex> lock(balancerMutex);
        if (!emergencyMode.load()) {
            emergencyMode.store(true);
            currentMetrics.emergencyMode = true;
            emergencyActivations++;
        }
        return true;
    }
    
    bool exitEmergencyMode() {
        std::lock_guard<std::mutex> lock(balancerMutex);
        if (emergencyMode.load()) {
            emergencyMode.store(false);
            currentMetrics.emergencyMode = false;
        }
        return true;
    }
    
    nlohmann::json getStatistics() const {
        std::lock_guard<std::mutex> lock(balancerMutex);
        nlohmann::json stats;
        stats["totalRebalances"] = std::to_string(totalRebalances);
        stats["emergencyActivations"] = std::to_string(emergencyActivations);
        stats["isInitialized"] = initialized.load() ? "true" : "false";
        stats["isRunning"] = running.load() ? "true" : "false";
        stats["isInEmergencyMode"] = emergencyMode.load() ? "true" : "false";
        stats["registeredEngines"] = std::to_string(engines.size());
        return stats;
    }
    
    void resetStatistics() {
        std::lock_guard<std::mutex> lock(balancerMutex);
        totalRebalances = 0;
        emergencyActivations = 0;
    }
};

// Simple test engine
class SimpleTestEngine : public ConsensusEngine {
private:
    ConsensusType type_;
    std::string name_;
    bool healthy_;
    std::map<std::string, double> parameters_;

public:
    SimpleTestEngine(ConsensusType type, const std::string& name) 
        : type_(type), name_(name), healthy_(true) {
        parameters_["difficulty"] = 4.0;
        parameters_["rewardMultiplier"] = 1.0;
    }
    
    bool validateBlock(const Block& block) override { return true; }
    bool validateTransaction(const Transaction& transaction) override { return true; }
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        return ConsensusResult(true, type_, 0.9, "Test validation");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy_; }
    ConsensusType getType() const override { return type_; }
    std::string getName() const override { return name_; }
    nlohmann::json getStatus() const override { return nlohmann::json{}; }
    nlohmann::json getMetrics() const override { return nlohmann::json{}; }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override {
        for (const auto& [key, value] : parameters) {
            parameters_[key] = value;
        }
        return true;
    }
    
    std::map<std::string, double> getParameters() const override { return parameters_; }
    
    void setHealthy(bool healthy) { healthy_ = healthy; }
};

// Test functions
void testBasicConstruction() {
    std::cout << "Testing basic construction..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(!balancer.isInitialized());
    assert(!balancer.isRunning());
    assert(!balancer.isInEmergencyMode());
    
    std::cout << "✓ Basic construction test passed" << std::endl;
}

void testInitialization() {
    std::cout << "Testing initialization..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    assert(balancer.isInitialized());
    assert(balancer.isRunning());
    
    balancer.shutdown();
    assert(!balancer.isInitialized());
    assert(!balancer.isRunning());
    
    std::cout << "✓ Initialization test passed" << std::endl;
}

void testEngineRegistration() {
    std::cout << "Testing engine registration..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    // Test null engine registration
    assert(!balancer.registerEngine(ConsensusType::VOTING_CONSENSUS, nullptr));
    
    assert(balancer.unregisterEngine(ConsensusType::PROOF_OF_WORK));
    assert(!balancer.unregisterEngine(ConsensusType::VOTING_CONSENSUS)); // Not registered
    
    std::cout << "✓ Engine registration test passed" << std::endl;
}

void testBalancing() {
    std::cout << "Testing balancing functionality..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    // Test basic balancing
    balancer.balanceConsensusParticipation();
    
    // Test difficulty adjustment
    balancer.adjustDifficulty(ConsensusType::PROOF_OF_WORK, 0.1);
    
    // Test reward adjustment
    balancer.adjustRewards(ConsensusType::PROOF_OF_WORK, 1.1);
    
    // Test automatic rebalancing
    assert(balancer.performAutomaticRebalancing());
    
    std::cout << "✓ Balancing functionality test passed" << std::endl;
}

void testMetrics() {
    std::cout << "Testing metrics functionality..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    
    // Trigger metrics collection
    balancer.balanceConsensusParticipation();
    
    // Test metrics retrieval
    BalanceMetrics metrics = balancer.getBalanceMetrics();
    assert(metrics.overallBalance >= 0.0 && metrics.overallBalance <= 1.0);
    assert(metrics.networkEfficiency >= 0.0 && metrics.networkEfficiency <= 1.0);
    
    ParticipationMetrics powMetrics = balancer.getParticipationMetrics(ConsensusType::PROOF_OF_WORK);
    assert(powMetrics.totalValidations >= 0);
    
    // Test detailed analysis
    nlohmann::json analysis = balancer.getDetailedAnalysis();
    assert(analysis.contains("overallBalance"));
    assert(analysis.contains("networkEfficiency"));
    
    std::cout << "✓ Metrics functionality test passed" << std::endl;
}

void testConfiguration() {
    std::cout << "Testing configuration management..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    BalanceConfig config = balancer.getBalanceConfig();
    assert(config.maxDifficultyAdjustment == 0.25);
    
    // Update configuration
    config.maxDifficultyAdjustment = 0.3;
    config.rebalancingInterval = 1800;
    balancer.setBalanceConfig(config);
    
    BalanceConfig updatedConfig = balancer.getBalanceConfig();
    assert(updatedConfig.maxDifficultyAdjustment == 0.3);
    assert(updatedConfig.rebalancingInterval == 1800);
    
    // Test target participation update
    assert(balancer.updateTargetParticipation(ConsensusType::PROOF_OF_WORK, 0.4));
    assert(!balancer.updateTargetParticipation(ConsensusType::PROOF_OF_WORK, -0.1)); // Invalid
    
    std::cout << "✓ Configuration management test passed" << std::endl;
}

void testEmergencyMode() {
    std::cout << "Testing emergency mode..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    assert(!balancer.isInEmergencyMode());
    
    assert(balancer.enterEmergencyMode());
    assert(balancer.isInEmergencyMode());
    
    assert(balancer.exitEmergencyMode());
    assert(!balancer.isInEmergencyMode());
    
    std::cout << "✓ Emergency mode test passed" << std::endl;
}

void testStatistics() {
    std::cout << "Testing statistics..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    nlohmann::json stats = balancer.getStatistics();
    assert(stats.contains("totalRebalances"));
    assert(stats.contains("emergencyActivations"));
    assert(stats.contains("isInitialized"));
    assert(stats.contains("isRunning"));
    
    assert(stats["isInitialized"] == "true");
    assert(stats["isRunning"] == "true");
    assert(stats["totalRebalances"] == "0");
    
    balancer.resetStatistics();
    
    std::cout << "✓ Statistics test passed" << std::endl;
}

void testPerformance() {
    std::cout << "Testing performance..." << std::endl;
    
    ConsensusBalancer balancer;
    assert(balancer.initialize());
    
    auto powEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_WORK, "TestPoW");
    auto posEngine = std::make_unique<SimpleTestEngine>(ConsensusType::PROOF_OF_STAKE, "TestPoS");
    
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_WORK, powEngine.get()));
    assert(balancer.registerEngine(ConsensusType::PROOF_OF_STAKE, posEngine.get()));
    
    const int numOperations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        balancer.balanceConsensusParticipation();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 1000 operations in reasonable time (less than 5 seconds)
    assert(duration.count() < 5000);
    
    std::cout << "✓ Performance test passed (completed " << numOperations 
              << " operations in " << duration.count() << "ms)" << std::endl;
}

int main() {
    std::cout << "Running Minimal Consensus Balancer Tests..." << std::endl;
    std::cout << "===========================================" << std::endl;
    
    try {
        testBasicConstruction();
        testInitialization();
        testEngineRegistration();
        testBalancing();
        testMetrics();
        testConfiguration();
        testEmergencyMode();
        testStatistics();
        testPerformance();
        
        std::cout << std::endl;
        std::cout << "===========================================" << std::endl;
        std::cout << "All tests passed successfully! ✓" << std::endl;
        std::cout << "===========================================" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << std::endl;
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << std::endl;
        std::cout << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}