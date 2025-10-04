#ifndef PROOF_OF_RESOURCE_CONTRIBUTION_H
#define PROOF_OF_RESOURCE_CONTRIBUTION_H

#include "consensus_harmony.h"
#include "block.h"
#include "transaction.h"
#include "utils.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cmath>

// Resource types that can be contributed
enum class ResourceType {
    COMPUTE,
    STORAGE,
    BANDWIDTH,
    ENERGY_EFFICIENT,  // For renewable energy sources
    MOBILE_RELAY       // For mobile-based contributions
};

// Resource quality assessment result
enum class ResourceQuality {
    EXCELLENT,
    GOOD,
    FAIR,
    POOR,
    INVALID
};

// Resource quality metrics
struct QualityMetrics {
    double availability;        // Uptime percentage (0.0 - 1.0)
    double reliability;         // Consistency of contribution (0.0 - 1.0)
    double latency;            // Network latency in milliseconds
    double throughput;         // Data throughput in MB/s
    double energyEfficiency;   // Energy efficiency score (0.0 - 1.0)
    bool isRenewableEnergy;    // Whether using renewable energy
    std::string location;      // Geographic location for regional balancing
    
    QualityMetrics() : availability(0.0), reliability(0.0), latency(1000.0), 
                      throughput(0.0), energyEfficiency(0.0), 
                      isRenewableEnergy(false), location("") {}
};

// Resource contribution data
struct ResourceContribution {
    std::string contributorAddress;
    ResourceType type;
    double amount;                     // Amount of resource contributed
    uint64_t duration;                 // Duration of contribution in seconds
    QualityMetrics quality;            // Resource quality metrics
    uint64_t timestamp;
    std::string proof;                 // Cryptographic proof of contribution
    std::string signature;             // Contributor's signature
    
    ResourceContribution() : type(ResourceType::COMPUTE), amount(0.0), 
                           duration(0), timestamp(0) {}
    
    ResourceContribution(const std::string& addr, ResourceType t, double amt, 
                        uint64_t dur, const QualityMetrics& qual)
        : contributorAddress(addr), type(t), amount(amt), duration(dur), 
          quality(qual), timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
    
    // Calculate hash of the contribution
    std::string calculateHash() const;
    
    // Serialize to JSON
    nlohmann::json toJson() const;
    
    // Deserialize from JSON
    static ResourceContribution fromJson(const nlohmann::json& j);
};

// Resource metrics for monitoring
struct ResourceMetrics {
    std::map<ResourceType, double> totalContributions;
    std::map<ResourceType, uint64_t> contributorCounts;
    std::map<ResourceType, double> averageQuality;
    std::map<std::string, double> regionalDistribution;
    uint64_t totalValidations;
    uint64_t successfulValidations;
    double networkEfficiency;
    
    ResourceMetrics() : totalValidations(0), successfulValidations(0), 
                       networkEfficiency(0.0) {}
};

// Resource validator for validating contributions
class ResourceValidator {
public:
    ResourceValidator();
    ~ResourceValidator() = default;
    
    // Validate a resource contribution
    bool validateContribution(const ResourceContribution& contribution);
    
    // Validate compute resource contribution
    bool validateComputeContribution(const ResourceContribution& contribution);
    
    // Validate storage resource contribution
    bool validateStorageContribution(const ResourceContribution& contribution);
    
    // Validate bandwidth resource contribution
    bool validateBandwidthContribution(const ResourceContribution& contribution);
    
    // Validate energy-efficient contribution
    bool validateEnergyEfficientContribution(const ResourceContribution& contribution);
    
    // Validate mobile relay contribution
    bool validateMobileRelayContribution(const ResourceContribution& contribution);
    
    // Verify cryptographic proof of contribution
    bool verifyContributionProof(const ResourceContribution& contribution);
    
private:
    std::mutex validationMutex;
    std::map<std::string, uint64_t> lastValidationTime;
    
    // Helper methods for validation
    bool isValidResourceAmount(ResourceType type, double amount);
    bool isValidQualityMetrics(const QualityMetrics& quality);
    bool checkContributorReputation(const std::string& address);
};

// Reward calculator for PoRC rewards
class RewardCalculator {
public:
    RewardCalculator();
    ~RewardCalculator() = default;
    
    // Calculate reward for a resource contribution
    double calculateResourceReward(const ResourceContribution& contribution, 
                                  const ResourceMetrics& networkMetrics);
    
    // Calculate base reward for resource type
    double calculateBaseReward(ResourceType type, double amount);
    
    // Calculate quality multiplier based on metrics
    double calculateQualityMultiplier(const QualityMetrics& quality);
    
    // Calculate scarcity multiplier based on network needs
    double calculateScarcityMultiplier(ResourceType type, 
                                     const ResourceMetrics& networkMetrics);
    
    // Calculate regional incentive multiplier
    double calculateRegionalMultiplier(const std::string& location);
    
    // Calculate energy efficiency bonus
    double calculateEnergyEfficiencyBonus(const QualityMetrics& quality);
    
private:
    std::map<ResourceType, double> baseRewardRates;
    std::map<std::string, double> regionalMultipliers;
    double energyEfficiencyBonusRate;
    
    void initializeRewardRates();
};

// Main Proof of Resource Contribution Engine
class ProofOfResourceContributionEngine : public ConsensusEngine {
public:
    ProofOfResourceContributionEngine();
    ~ProofOfResourceContributionEngine() override = default;
    
    // ConsensusEngine interface implementation
    bool validateBlock(const Block& block) override;
    bool validateTransaction(const Transaction& transaction) override;
    ConsensusResult processRequest(const ConsensusRequest& request) override;
    
    bool initialize() override;
    void shutdown() override;
    bool isHealthy() const override;
    
    ConsensusType getType() const override { return ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION; }
    std::string getName() const override { return "Proof of Resource Contribution"; }
    nlohmann::json getStatus() const override;
    nlohmann::json getMetrics() const override;
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override;
    std::map<std::string, double> getParameters() const override;
    
    // PoRC-specific methods
    bool validateResourceContribution(const ResourceContribution& contribution);
    double calculateResourceReward(const ResourceContribution& contribution);
    ResourceQuality assessResourceQuality(const std::string& contributor);
    
    // Resource management
    bool registerResourceContribution(const ResourceContribution& contribution);
    std::vector<ResourceContribution> getContributorHistory(const std::string& address);
    ResourceMetrics getResourceMetrics() const;
    
    // Network optimization
    void optimizeResourceAllocation();
    void updateNetworkDemand();
    std::map<ResourceType, double> getResourceDemand() const;
    
private:
    std::unique_ptr<ResourceValidator> validator;
    std::unique_ptr<RewardCalculator> rewardCalculator;
    
    // Resource tracking
    std::map<std::string, std::vector<ResourceContribution>> contributorHistory;
    std::map<ResourceType, double> networkDemand;
    ResourceMetrics metrics;
    
    // Configuration
    double minResourceContribution;
    std::vector<ResourceType> acceptedResourceTypes;
    uint64_t contributionValidityPeriod;
    double qualityThreshold;
    
    // Thread safety
    mutable std::mutex engineMutex;
    std::atomic<bool> isInitialized{false};
    std::atomic<bool> isShuttingDown{false};
    
    // Helper methods
    bool isValidContributionForBlock(const ResourceContribution& contribution, 
                                   const Block& block);
    bool isValidContributionForTransaction(const ResourceContribution& contribution, 
                                         const Transaction& transaction);
    void updateMetrics(const ResourceContribution& contribution, bool isValid);
    void cleanupExpiredContributions();
    
    // Mathematical formulas for resource assessment
    double calculateResourceScore(const ResourceContribution& contribution);
    double calculateNetworkImpact(const ResourceContribution& contribution);
    double calculateSustainabilityScore(const QualityMetrics& quality);
};



// Helper functions
std::string resourceTypeToString(ResourceType type);
ResourceType stringToResourceType(const std::string& typeStr);
std::string resourceQualityToString(ResourceQuality quality);

#endif // PROOF_OF_RESOURCE_CONTRIBUTION_H