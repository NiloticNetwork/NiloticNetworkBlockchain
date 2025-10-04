#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "proof_of_resource_contribution.h"
#include "block.h"
#include "transaction.h"
#include <chrono>
#include <thread>

class ProofOfResourceContributionTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<ProofOfResourceContributionEngine>();
        ASSERT_TRUE(engine->initialize());
    }
    
    void TearDown() override {
        if (engine) {
            engine->shutdown();
        }
    }
    
    std::unique_ptr<ProofOfResourceContributionEngine> engine;
    
    // Helper method to create a valid resource contribution
    ResourceContribution createValidContribution(ResourceType type = ResourceType::COMPUTE,
                                                double amount = 100.0,
                                                const std::string& address = "test_address") {
        QualityMetrics quality;
        quality.availability = 0.9;
        quality.reliability = 0.85;
        quality.latency = 50.0;
        quality.throughput = 10.0;
        quality.energyEfficiency = 0.7;
        quality.isRenewableEnergy = false;
        quality.location = "urban";
        
        ResourceContribution contribution(address, type, amount, 3600, quality);
        contribution.proof = "test_proof_" + contribution.calculateHash().substr(0, 8);
        contribution.signature = "test_signature";
        
        return contribution;
    }
    
    // Helper method to create a renewable energy contribution
    ResourceContribution createRenewableContribution(const std::string& address = "renewable_address") {
        QualityMetrics quality;
        quality.availability = 0.95;
        quality.reliability = 0.9;
        quality.latency = 30.0;
        quality.throughput = 15.0;
        quality.energyEfficiency = 0.9;
        quality.isRenewableEnergy = true;
        quality.location = "rural";
        
        ResourceContribution contribution(address, ResourceType::ENERGY_EFFICIENT, 50.0, 7200, quality);
        contribution.proof = "renewable_proof_" + contribution.calculateHash().substr(0, 8);
        contribution.signature = "renewable_signature";
        
        return contribution;
    }
    
    // Helper method to create a mobile relay contribution
    ResourceContribution createMobileContribution(const std::string& address = "mobile_address") {
        QualityMetrics quality;
        quality.availability = 0.8;
        quality.reliability = 0.75;
        quality.latency = 1500.0; // Higher latency for mobile
        quality.throughput = 2.0;
        quality.energyEfficiency = 0.6;
        quality.isRenewableEnergy = false;
        quality.location = "remote";
        
        ResourceContribution contribution(address, ResourceType::MOBILE_RELAY, 25.0, 1800, quality);
        contribution.proof = "mobile_proof_" + contribution.calculateHash().substr(0, 8);
        contribution.signature = "mobile_signature";
        
        return contribution;
    }
};

// Test ResourceContribution structure
TEST_F(ProofOfResourceContributionTest, ResourceContributionBasicConstruction) {
    QualityMetrics quality;
    quality.availability = 0.9;
    quality.reliability = 0.8;
    
    ResourceContribution contribution("test_address", ResourceType::COMPUTE, 100.0, 3600, quality);
    
    EXPECT_EQ(contribution.contributorAddress, "test_address");
    EXPECT_EQ(contribution.type, ResourceType::COMPUTE);
    EXPECT_DOUBLE_EQ(contribution.amount, 100.0);
    EXPECT_EQ(contribution.duration, 3600);
    EXPECT_DOUBLE_EQ(contribution.quality.availability, 0.9);
    EXPECT_DOUBLE_EQ(contribution.quality.reliability, 0.8);
    EXPECT_GT(contribution.timestamp, 0);
}

TEST_F(ProofOfResourceContributionTest, ResourceContributionHashCalculation) {
    ResourceContribution contribution = createValidContribution();
    
    std::string hash1 = contribution.calculateHash();
    std::string hash2 = contribution.calculateHash();
    
    EXPECT_EQ(hash1, hash2); // Hash should be consistent
    EXPECT_FALSE(hash1.empty());
    EXPECT_EQ(hash1.length(), 64); // SHA256 hash length
}

TEST_F(ProofOfResourceContributionTest, ResourceContributionJsonSerialization) {
    ResourceContribution contribution = createValidContribution();
    
    nlohmann::json j = contribution.toJson();
    
    EXPECT_EQ(j["contributorAddress"], "test_address");
    EXPECT_EQ(j["type"], "COMPUTE");
    EXPECT_DOUBLE_EQ(j["amount"], 100.0);
    EXPECT_EQ(j["duration"], 3600);
    EXPECT_DOUBLE_EQ(j["quality"]["availability"], 0.9);
    EXPECT_DOUBLE_EQ(j["quality"]["reliability"], 0.85);
}

TEST_F(ProofOfResourceContributionTest, ResourceContributionJsonDeserialization) {
    ResourceContribution original = createValidContribution();
    nlohmann::json j = original.toJson();
    
    ResourceContribution deserialized = ResourceContribution::fromJson(j);
    
    EXPECT_EQ(deserialized.contributorAddress, original.contributorAddress);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_DOUBLE_EQ(deserialized.amount, original.amount);
    EXPECT_EQ(deserialized.duration, original.duration);
    EXPECT_DOUBLE_EQ(deserialized.quality.availability, original.quality.availability);
    EXPECT_DOUBLE_EQ(deserialized.quality.reliability, original.quality.reliability);
}

// Test ResourceValidator
TEST_F(ProofOfResourceContributionTest, ValidateValidContribution) {
    ResourceContribution contribution = createValidContribution();
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ValidateInvalidContribution) {
    ResourceContribution contribution = createValidContribution();
    
    // Test empty address
    contribution.contributorAddress = "";
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test zero amount
    contribution = createValidContribution();
    contribution.amount = 0.0;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test negative amount
    contribution = createValidContribution();
    contribution.amount = -10.0;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ValidateComputeContribution) {
    ResourceContribution contribution = createValidContribution(ResourceType::COMPUTE, 500.0);
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution));
    
    // Test invalid compute amount (too small)
    contribution.amount = 0.5;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test invalid compute amount (too large)
    contribution.amount = 2000000.0;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test low availability
    contribution = createValidContribution(ResourceType::COMPUTE, 500.0);
    contribution.quality.availability = 0.3;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ValidateStorageContribution) {
    ResourceContribution contribution = createValidContribution(ResourceType::STORAGE, 1000.0);
    contribution.quality.availability = 0.9; // Storage needs high availability
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution));
    
    // Test low availability for storage
    contribution.quality.availability = 0.7;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test short duration for storage
    contribution = createValidContribution(ResourceType::STORAGE, 1000.0);
    contribution.duration = 1800; // 30 minutes - too short
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ValidateBandwidthContribution) {
    ResourceContribution contribution = createValidContribution(ResourceType::BANDWIDTH, 50.0);
    contribution.quality.throughput = 45.0; // 90% of claimed bandwidth
    contribution.quality.latency = 100.0;
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution));
    
    // Test low throughput
    contribution.quality.throughput = 30.0; // Only 60% of claimed
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test high latency
    contribution = createValidContribution(ResourceType::BANDWIDTH, 50.0);
    contribution.quality.throughput = 45.0;
    contribution.quality.latency = 600.0; // Too high
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ValidateEnergyEfficientContribution) {
    ResourceContribution contribution = createRenewableContribution();
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution));
    
    // Test non-renewable energy
    contribution.quality.isRenewableEnergy = false;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test low energy efficiency
    contribution = createRenewableContribution();
    contribution.quality.energyEfficiency = 0.5;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ValidateMobileRelayContribution) {
    ResourceContribution contribution = createMobileContribution();
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution));
    
    // Test too high latency even for mobile
    contribution.quality.latency = 3000.0;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
    
    // Test low reliability
    contribution = createMobileContribution();
    contribution.quality.reliability = 0.4;
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

// Test RewardCalculator
TEST_F(ProofOfResourceContributionTest, CalculateBasicReward) {
    ResourceContribution contribution = createValidContribution(ResourceType::COMPUTE, 100.0);
    
    double reward = engine->calculateResourceReward(contribution);
    
    EXPECT_GT(reward, 0.0);
    EXPECT_LT(reward, 1000.0); // Reasonable upper bound
}

TEST_F(ProofOfResourceContributionTest, CalculateRenewableEnergyBonus) {
    ResourceContribution regularContribution = createValidContribution(ResourceType::COMPUTE, 100.0);
    ResourceContribution renewableContribution = createRenewableContribution();
    renewableContribution.type = ResourceType::COMPUTE;
    renewableContribution.amount = 100.0;
    
    double regularReward = engine->calculateResourceReward(regularContribution);
    double renewableReward = engine->calculateResourceReward(renewableContribution);
    
    EXPECT_GT(renewableReward, regularReward); // Renewable should get bonus
}

TEST_F(ProofOfResourceContributionTest, CalculateRegionalMultiplier) {
    ResourceContribution urbanContribution = createValidContribution();
    urbanContribution.quality.location = "urban";
    
    ResourceContribution ruralContribution = createValidContribution();
    ruralContribution.quality.location = "rural";
    
    ResourceContribution remoteContribution = createValidContribution();
    remoteContribution.quality.location = "remote";
    
    double urbanReward = engine->calculateResourceReward(urbanContribution);
    double ruralReward = engine->calculateResourceReward(ruralContribution);
    double remoteReward = engine->calculateResourceReward(remoteContribution);
    
    EXPECT_GT(ruralReward, urbanReward); // Rural should get higher reward
    EXPECT_GT(remoteReward, ruralReward); // Remote should get highest reward
}

TEST_F(ProofOfResourceContributionTest, CalculateQualityMultiplier) {
    ResourceContribution highQuality = createValidContribution();
    highQuality.quality.availability = 0.99;
    highQuality.quality.reliability = 0.95;
    highQuality.quality.latency = 10.0;
    highQuality.quality.throughput = 100.0;
    
    ResourceContribution lowQuality = createValidContribution();
    lowQuality.quality.availability = 0.6;
    lowQuality.quality.reliability = 0.5;
    lowQuality.quality.latency = 800.0;
    lowQuality.quality.throughput = 1.0;
    
    double highQualityReward = engine->calculateResourceReward(highQuality);
    double lowQualityReward = engine->calculateResourceReward(lowQuality);
    
    EXPECT_GT(highQualityReward, lowQualityReward);
}

// Test ProofOfResourceContributionEngine
TEST_F(ProofOfResourceContributionTest, EngineInitialization) {
    EXPECT_TRUE(engine->isHealthy());
    EXPECT_EQ(engine->getType(), ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
    EXPECT_EQ(engine->getName(), "Proof of Resource Contribution");
}

TEST_F(ProofOfResourceContributionTest, EngineShutdown) {
    EXPECT_TRUE(engine->isHealthy());
    
    engine->shutdown();
    
    EXPECT_FALSE(engine->isHealthy());
}

TEST_F(ProofOfResourceContributionTest, BlockValidation) {
    Block testBlock(1, "previous_hash");
    Transaction tx("sender", "recipient", 100.0);
    testBlock.addTransaction(tx);
    
    EXPECT_TRUE(engine->validateBlock(testBlock));
    
    // Test empty block
    Block emptyBlock(2, "previous_hash");
    EXPECT_TRUE(engine->validateBlock(emptyBlock)); // Empty blocks are valid
}

TEST_F(ProofOfResourceContributionTest, TransactionValidation) {
    Transaction validTx("sender", "recipient", 100.0);
    EXPECT_TRUE(engine->validateTransaction(validTx));
    
    // Test invalid transaction
    Transaction invalidTx("", "recipient", -100.0);
    EXPECT_FALSE(engine->validateTransaction(invalidTx));
}

TEST_F(ProofOfResourceContributionTest, ConsensusRequestProcessing) {
    // Test block validation request
    Block testBlock(1, "hash");
    std::string blockData = testBlock.serialize();
    ConsensusRequest blockRequest(RequestType::BLOCK_VALIDATION, blockData);
    
    ConsensusResult blockResult = engine->processRequest(blockRequest);
    EXPECT_TRUE(blockResult.isValid);
    EXPECT_EQ(blockResult.mechanism, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
    EXPECT_GT(blockResult.confidence, 0.0);
    
    // Test transaction validation request
    Transaction testTx("sender", "recipient", 100.0);
    std::string txData = testTx.serialize();
    ConsensusRequest txRequest(RequestType::TRANSACTION_VALIDATION, txData);
    
    ConsensusResult txResult = engine->processRequest(txRequest);
    EXPECT_TRUE(txResult.isValid);
    EXPECT_GT(txResult.confidence, 0.0);
}

TEST_F(ProofOfResourceContributionTest, ResourceContributionRegistration) {
    ResourceContribution contribution = createValidContribution();
    
    EXPECT_TRUE(engine->registerResourceContribution(contribution));
    
    // Check contributor history
    std::vector<ResourceContribution> history = engine->getContributorHistory("test_address");
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].contributorAddress, "test_address");
    EXPECT_EQ(history[0].type, ResourceType::COMPUTE);
}

TEST_F(ProofOfResourceContributionTest, MultipleContributionsFromSameAddress) {
    ResourceContribution contribution1 = createValidContribution(ResourceType::COMPUTE, 100.0);
    ResourceContribution contribution2 = createValidContribution(ResourceType::STORAGE, 500.0);
    
    // Add small delay to avoid validation cooldown
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    EXPECT_TRUE(engine->registerResourceContribution(contribution1));
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    EXPECT_TRUE(engine->registerResourceContribution(contribution2));
    
    std::vector<ResourceContribution> history = engine->getContributorHistory("test_address");
    EXPECT_EQ(history.size(), 2);
}

TEST_F(ProofOfResourceContributionTest, ResourceQualityAssessment) {
    // Register some contributions
    ResourceContribution highQuality = createValidContribution();
    highQuality.quality.availability = 0.99;
    highQuality.quality.reliability = 0.95;
    
    EXPECT_TRUE(engine->registerResourceContribution(highQuality));
    
    ResourceQuality quality = engine->assessResourceQuality("test_address");
    EXPECT_NE(quality, ResourceQuality::INVALID);
    
    // Test unknown contributor
    ResourceQuality unknownQuality = engine->assessResourceQuality("unknown_address");
    EXPECT_EQ(unknownQuality, ResourceQuality::INVALID);
}

TEST_F(ProofOfResourceContributionTest, ResourceMetrics) {
    // Register various contributions
    ResourceContribution compute = createValidContribution(ResourceType::COMPUTE, 100.0);
    ResourceContribution storage = createValidContribution(ResourceType::STORAGE, 500.0, "storage_address");
    ResourceContribution bandwidth = createValidContribution(ResourceType::BANDWIDTH, 50.0, "bandwidth_address");
    
    engine->registerResourceContribution(compute);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    engine->registerResourceContribution(storage);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    engine->registerResourceContribution(bandwidth);
    
    ResourceMetrics metrics = engine->getResourceMetrics();
    
    EXPECT_GT(metrics.totalContributions[ResourceType::COMPUTE], 0.0);
    EXPECT_GT(metrics.totalContributions[ResourceType::STORAGE], 0.0);
    EXPECT_GT(metrics.totalContributions[ResourceType::BANDWIDTH], 0.0);
    
    EXPECT_GT(metrics.contributorCounts[ResourceType::COMPUTE], 0);
    EXPECT_GT(metrics.contributorCounts[ResourceType::STORAGE], 0);
    EXPECT_GT(metrics.contributorCounts[ResourceType::BANDWIDTH], 0);
}

TEST_F(ProofOfResourceContributionTest, NetworkDemandAndOptimization) {
    std::map<ResourceType, double> initialDemand = engine->getResourceDemand();
    EXPECT_GT(initialDemand.size(), 0);
    
    // Optimize resource allocation
    engine->optimizeResourceAllocation();
    
    std::map<ResourceType, double> updatedDemand = engine->getResourceDemand();
    EXPECT_GT(updatedDemand.size(), 0);
}

TEST_F(ProofOfResourceContributionTest, ParameterAdjustment) {
    std::map<std::string, double> originalParams = engine->getParameters();
    
    std::map<std::string, double> newParams;
    newParams["minResourceContribution"] = 50.0;
    newParams["qualityThreshold"] = 0.6;
    newParams["contributionValidityPeriod"] = 7200.0;
    
    EXPECT_TRUE(engine->adjustParameters(newParams));
    
    std::map<std::string, double> updatedParams = engine->getParameters();
    EXPECT_DOUBLE_EQ(updatedParams["minResourceContribution"], 50.0);
    EXPECT_DOUBLE_EQ(updatedParams["qualityThreshold"], 0.6);
    EXPECT_DOUBLE_EQ(updatedParams["contributionValidityPeriod"], 7200.0);
}

TEST_F(ProofOfResourceContributionTest, StatusAndMetricsReporting) {
    nlohmann::json status = engine->getStatus();
    
    EXPECT_EQ(status["name"], "Proof of Resource Contribution");
    EXPECT_EQ(status["type"], "PROOF_OF_RESOURCE_CONTRIBUTION");
    EXPECT_TRUE(status["initialized"]);
    EXPECT_TRUE(status["healthy"]);
    EXPECT_FALSE(status["shutting_down"]);
    
    nlohmann::json metrics = engine->getMetrics();
    EXPECT_TRUE(metrics.contains("total_contributions"));
    EXPECT_TRUE(metrics.contains("contributor_counts"));
    EXPECT_TRUE(metrics.contains("average_quality"));
}

// Test edge cases and error conditions
TEST_F(ProofOfResourceContributionTest, ValidationCooldown) {
    ResourceContribution contribution1 = createValidContribution();
    ResourceContribution contribution2 = createValidContribution();
    
    EXPECT_TRUE(engine->validateResourceContribution(contribution1));
    
    // Second validation should fail due to cooldown
    EXPECT_FALSE(engine->validateResourceContribution(contribution2));
    
    // Wait for cooldown to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Now it should succeed
    EXPECT_TRUE(engine->validateResourceContribution(contribution2));
}

TEST_F(ProofOfResourceContributionTest, InvalidProofValidation) {
    ResourceContribution contribution = createValidContribution();
    contribution.proof = "invalid_proof";
    
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, EmptyProofValidation) {
    ResourceContribution contribution = createValidContribution();
    contribution.proof = "";
    
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

TEST_F(ProofOfResourceContributionTest, ShortAddressValidation) {
    ResourceContribution contribution = createValidContribution();
    contribution.contributorAddress = "short";
    
    EXPECT_FALSE(engine->validateResourceContribution(contribution));
}

// Test helper functions
TEST_F(ProofOfResourceContributionTest, ResourceTypeStringConversion) {
    EXPECT_EQ(resourceTypeToString(ResourceType::COMPUTE), "COMPUTE");
    EXPECT_EQ(resourceTypeToString(ResourceType::STORAGE), "STORAGE");
    EXPECT_EQ(resourceTypeToString(ResourceType::BANDWIDTH), "BANDWIDTH");
    EXPECT_EQ(resourceTypeToString(ResourceType::ENERGY_EFFICIENT), "ENERGY_EFFICIENT");
    EXPECT_EQ(resourceTypeToString(ResourceType::MOBILE_RELAY), "MOBILE_RELAY");
    
    EXPECT_EQ(stringToResourceType("COMPUTE"), ResourceType::COMPUTE);
    EXPECT_EQ(stringToResourceType("STORAGE"), ResourceType::STORAGE);
    EXPECT_EQ(stringToResourceType("BANDWIDTH"), ResourceType::BANDWIDTH);
    EXPECT_EQ(stringToResourceType("ENERGY_EFFICIENT"), ResourceType::ENERGY_EFFICIENT);
    EXPECT_EQ(stringToResourceType("MOBILE_RELAY"), ResourceType::MOBILE_RELAY);
    EXPECT_EQ(stringToResourceType("UNKNOWN"), ResourceType::COMPUTE); // Default
}

TEST_F(ProofOfResourceContributionTest, ResourceQualityStringConversion) {
    EXPECT_EQ(resourceQualityToString(ResourceQuality::EXCELLENT), "EXCELLENT");
    EXPECT_EQ(resourceQualityToString(ResourceQuality::GOOD), "GOOD");
    EXPECT_EQ(resourceQualityToString(ResourceQuality::FAIR), "FAIR");
    EXPECT_EQ(resourceQualityToString(ResourceQuality::POOR), "POOR");
    EXPECT_EQ(resourceQualityToString(ResourceQuality::INVALID), "INVALID");
}

// Performance test
TEST_F(ProofOfResourceContributionTest, PerformanceTest) {
    const int numContributions = 100;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numContributions; ++i) {
        ResourceContribution contribution = createValidContribution(
            ResourceType::COMPUTE, 100.0, "address_" + std::to_string(i));
        engine->validateResourceContribution(contribution);
        
        // Small delay to avoid cooldown
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete validations in reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
    
    ResourceMetrics metrics = engine->getResourceMetrics();
    EXPECT_GT(metrics.totalValidations, 0);
}

// Test concurrent access
TEST_F(ProofOfResourceContributionTest, ConcurrentValidation) {
    const int numThreads = 4;
    const int contributionsPerThread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, contributionsPerThread, &successCount]() {
            for (int i = 0; i < contributionsPerThread; ++i) {
                ResourceContribution contribution = createValidContribution(
                    ResourceType::COMPUTE, 100.0, 
                    "thread_" + std::to_string(t) + "_addr_" + std::to_string(i));
                
                if (engine->validateResourceContribution(contribution)) {
                    successCount++;
                }
                
                // Small delay to avoid overwhelming the system
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(successCount.load(), 0);
    EXPECT_LE(successCount.load(), numThreads * contributionsPerThread);
}