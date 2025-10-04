#include "../include/core/proof_of_resource_contribution.h"
#include "../include/core/block.h"
#include "../include/core/transaction.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

// Simple test framework
class SimpleTest {
public:
    static void assertTrue(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << std::endl;
            exit(1);
        } else {
            std::cout << "PASS: " << message << std::endl;
        }
    }
    
    static void assertEqual(const std::string& expected, const std::string& actual, const std::string& message) {
        if (expected != actual) {
            std::cerr << "FAIL: " << message << " - Expected: " << expected << ", Actual: " << actual << std::endl;
            exit(1);
        } else {
            std::cout << "PASS: " << message << std::endl;
        }
    }
    
    static void assertGreaterThan(double value, double threshold, const std::string& message) {
        if (value <= threshold) {
            std::cerr << "FAIL: " << message << " - Value: " << value << " <= " << threshold << std::endl;
            exit(1);
        } else {
            std::cout << "PASS: " << message << std::endl;
        }
    }
};

// Helper function to create a valid resource contribution
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

// Helper function to create a renewable energy contribution
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

void testResourceContributionBasics() {
    std::cout << "\n=== Testing ResourceContribution Basics ===" << std::endl;
    
    QualityMetrics quality;
    quality.availability = 0.9;
    quality.reliability = 0.8;
    
    ResourceContribution contribution("test_address", ResourceType::COMPUTE, 100.0, 3600, quality);
    
    SimpleTest::assertEqual("test_address", contribution.contributorAddress, "Contributor address");
    SimpleTest::assertTrue(contribution.type == ResourceType::COMPUTE, "Resource type");
    SimpleTest::assertTrue(contribution.amount == 100.0, "Resource amount");
    SimpleTest::assertTrue(contribution.duration == 3600, "Duration");
    SimpleTest::assertTrue(contribution.quality.availability == 0.9, "Quality availability");
    SimpleTest::assertTrue(contribution.quality.reliability == 0.8, "Quality reliability");
    SimpleTest::assertTrue(contribution.timestamp > 0, "Timestamp set");
}

void testResourceContributionHash() {
    std::cout << "\n=== Testing ResourceContribution Hash ===" << std::endl;
    
    ResourceContribution contribution = createValidContribution();
    
    std::string hash1 = contribution.calculateHash();
    std::string hash2 = contribution.calculateHash();
    
    SimpleTest::assertEqual(hash1, hash2, "Hash consistency");
    SimpleTest::assertTrue(!hash1.empty(), "Hash not empty");
    SimpleTest::assertTrue(hash1.length() == 64, "Hash length (SHA256)");
}

void testResourceContributionJsonSerialization() {
    std::cout << "\n=== Testing ResourceContribution JSON Serialization ===" << std::endl;
    
    ResourceContribution original = createValidContribution();
    
    nlohmann::json j = original.toJson();
    ResourceContribution deserialized = ResourceContribution::fromJson(j);
    
    SimpleTest::assertEqual(original.contributorAddress, deserialized.contributorAddress, "Address serialization");
    SimpleTest::assertTrue(original.type == deserialized.type, "Type serialization");
    SimpleTest::assertTrue(original.amount == deserialized.amount, "Amount serialization");
    SimpleTest::assertTrue(original.duration == deserialized.duration, "Duration serialization");
    SimpleTest::assertTrue(original.quality.availability == deserialized.quality.availability, "Quality availability serialization");
}

void testProofOfResourceContributionEngine() {
    std::cout << "\n=== Testing ProofOfResourceContributionEngine ===" << std::endl;
    
    ProofOfResourceContributionEngine engine;
    
    // Test initialization
    SimpleTest::assertTrue(engine.initialize(), "Engine initialization");
    SimpleTest::assertTrue(engine.isHealthy(), "Engine health after initialization");
    SimpleTest::assertEqual("Proof of Resource Contribution", engine.getName(), "Engine name");
    SimpleTest::assertTrue(engine.getType() == ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, "Engine type");
    
    // Test basic validation
    ResourceContribution contribution = createValidContribution();
    SimpleTest::assertTrue(engine.validateResourceContribution(contribution), "Valid contribution validation");
    
    // Test invalid contribution (empty address)
    ResourceContribution invalidContribution = createValidContribution();
    invalidContribution.contributorAddress = "";
    SimpleTest::assertTrue(!engine.validateResourceContribution(invalidContribution), "Invalid contribution (empty address)");
    
    // Test invalid contribution (zero amount)
    invalidContribution = createValidContribution();
    invalidContribution.amount = 0.0;
    SimpleTest::assertTrue(!engine.validateResourceContribution(invalidContribution), "Invalid contribution (zero amount)");
    
    // Test reward calculation
    double reward = engine.calculateResourceReward(contribution);
    SimpleTest::assertGreaterThan(reward, 0.0, "Reward calculation positive");
    
    // Test renewable energy bonus
    ResourceContribution renewableContribution = createRenewableContribution();
    double renewableReward = engine.calculateResourceReward(renewableContribution);
    SimpleTest::assertGreaterThan(renewableReward, 0.0, "Renewable reward positive");
    
    // Test contribution registration (use different address to avoid cooldown)
    ResourceContribution registrationContrib = createValidContribution(ResourceType::COMPUTE, 100.0, "registration_address");
    SimpleTest::assertTrue(engine.registerResourceContribution(registrationContrib), "Contribution registration");
    
    // Test contributor history
    std::vector<ResourceContribution> history = engine.getContributorHistory("registration_address");
    SimpleTest::assertTrue(history.size() == 1, "Contributor history size");
    SimpleTest::assertEqual("registration_address", history[0].contributorAddress, "History contributor address");
    
    // Test resource quality assessment
    ResourceQuality quality = engine.assessResourceQuality("registration_address");
    SimpleTest::assertTrue(quality != ResourceQuality::INVALID, "Resource quality assessment");
    
    // Test unknown contributor
    ResourceQuality unknownQuality = engine.assessResourceQuality("unknown_address");
    SimpleTest::assertTrue(unknownQuality == ResourceQuality::INVALID, "Unknown contributor quality");
    
    // Test metrics
    ResourceMetrics metrics = engine.getResourceMetrics();
    SimpleTest::assertGreaterThan(metrics.totalContributions[ResourceType::COMPUTE], 0.0, "Metrics total contributions");
    SimpleTest::assertTrue(metrics.contributorCounts[ResourceType::COMPUTE] > 0, "Metrics contributor counts");
    
    // Test status and metrics JSON
    nlohmann::json status = engine.getStatus();
    SimpleTest::assertEqual("Proof of Resource Contribution", status["name"], "Status name");
    SimpleTest::assertTrue(status["initialized"], "Status initialized");
    SimpleTest::assertTrue(status["healthy"], "Status healthy");
    
    nlohmann::json metricsJson = engine.getMetrics();
    SimpleTest::assertTrue(metricsJson.contains("total_contributions"), "Metrics JSON contains total_contributions");
    SimpleTest::assertTrue(metricsJson.contains("contributor_counts"), "Metrics JSON contains contributor_counts");
    
    // Test parameter adjustment
    std::map<std::string, double> newParams;
    newParams["minResourceContribution"] = 50.0;
    newParams["qualityThreshold"] = 0.6;
    
    SimpleTest::assertTrue(engine.adjustParameters(newParams), "Parameter adjustment");
    
    std::map<std::string, double> updatedParams = engine.getParameters();
    SimpleTest::assertTrue(updatedParams["minResourceContribution"] == 50.0, "Updated min resource contribution");
    SimpleTest::assertTrue(updatedParams["qualityThreshold"] == 0.6, "Updated quality threshold");
    
    // Test block validation
    Block testBlock(1, "previous_hash");
    Transaction tx("sender", "recipient", 100.0);
    testBlock.addTransaction(tx);
    SimpleTest::assertTrue(engine.validateBlock(testBlock), "Block validation");
    
    // Test transaction validation
    Transaction validTx("sender", "recipient", 100.0);
    SimpleTest::assertTrue(engine.validateTransaction(validTx), "Transaction validation");
    
    // Test consensus request processing (parameter adjustment - simpler test)
    ConsensusRequest paramRequest(RequestType::PARAMETER_ADJUSTMENT, "test_param_data");
    ConsensusResult paramResult = engine.processRequest(paramRequest);
    SimpleTest::assertTrue(paramResult.isValid, "Parameter adjustment request");
    SimpleTest::assertTrue(paramResult.mechanism == ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, "Parameter adjustment mechanism");
    SimpleTest::assertGreaterThan(paramResult.confidence, 0.0, "Parameter adjustment confidence");
    
    // Test shutdown
    engine.shutdown();
    SimpleTest::assertTrue(!engine.isHealthy(), "Engine health after shutdown");
}

void testResourceTypeValidation() {
    std::cout << "\n=== Testing Resource Type Validation ===" << std::endl;
    
    ProofOfResourceContributionEngine engine;
    engine.initialize();
    
    // Test compute contribution
    ResourceContribution computeContrib = createValidContribution(ResourceType::COMPUTE, 500.0);
    SimpleTest::assertTrue(engine.validateResourceContribution(computeContrib), "Valid compute contribution");
    
    // Test storage contribution
    ResourceContribution storageContrib = createValidContribution(ResourceType::STORAGE, 1000.0, "storage_addr");
    storageContrib.quality.availability = 0.9; // Storage needs high availability
    storageContrib.duration = 7200; // Longer duration for storage
    storageContrib.proof = "storage_proof_" + storageContrib.calculateHash().substr(0, 8);
    
    // Add delay to avoid cooldown
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SimpleTest::assertTrue(engine.validateResourceContribution(storageContrib), "Valid storage contribution");
    
    // Test bandwidth contribution
    ResourceContribution bandwidthContrib = createValidContribution(ResourceType::BANDWIDTH, 50.0, "bandwidth_addr");
    bandwidthContrib.quality.throughput = 45.0; // 90% of claimed bandwidth
    bandwidthContrib.quality.latency = 100.0;
    bandwidthContrib.proof = "bandwidth_proof_" + bandwidthContrib.calculateHash().substr(0, 8);
    
    // Add delay to avoid cooldown
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SimpleTest::assertTrue(engine.validateResourceContribution(bandwidthContrib), "Valid bandwidth contribution");
    
    // Test energy efficient contribution
    ResourceContribution energyContrib = createRenewableContribution("energy_addr");
    
    // Add delay to avoid cooldown
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SimpleTest::assertTrue(engine.validateResourceContribution(energyContrib), "Valid energy efficient contribution");
    
    // Test mobile relay contribution
    QualityMetrics mobileQuality;
    mobileQuality.availability = 0.8;
    mobileQuality.reliability = 0.75;
    mobileQuality.latency = 1500.0; // Higher latency for mobile
    mobileQuality.throughput = 2.0;
    mobileQuality.energyEfficiency = 0.6;
    mobileQuality.location = "remote";
    
    ResourceContribution mobileContrib("mobile_address", ResourceType::MOBILE_RELAY, 25.0, 1800, mobileQuality);
    mobileContrib.proof = "mobile_proof_" + mobileContrib.calculateHash().substr(0, 8);
    mobileContrib.signature = "mobile_signature";
    
    // Add delay to avoid cooldown
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SimpleTest::assertTrue(engine.validateResourceContribution(mobileContrib), "Valid mobile relay contribution");
    
    engine.shutdown();
}

void testRewardCalculation() {
    std::cout << "\n=== Testing Reward Calculation ===" << std::endl;
    
    ProofOfResourceContributionEngine engine;
    engine.initialize();
    
    // Test basic reward calculation
    ResourceContribution basicContrib = createValidContribution(ResourceType::COMPUTE, 100.0);
    double basicReward = engine.calculateResourceReward(basicContrib);
    SimpleTest::assertGreaterThan(basicReward, 0.0, "Basic reward positive");
    
    // Test renewable energy bonus
    ResourceContribution renewableContrib = createRenewableContribution();
    renewableContrib.type = ResourceType::COMPUTE;
    renewableContrib.amount = 100.0;
    double renewableReward = engine.calculateResourceReward(renewableContrib);
    SimpleTest::assertGreaterThan(renewableReward, basicReward, "Renewable energy bonus");
    
    // Test regional multiplier
    ResourceContribution urbanContrib = createValidContribution();
    urbanContrib.quality.location = "urban";
    
    ResourceContribution ruralContrib = createValidContribution();
    ruralContrib.quality.location = "rural";
    
    ResourceContribution remoteContrib = createValidContribution();
    remoteContrib.quality.location = "remote";
    
    double urbanReward = engine.calculateResourceReward(urbanContrib);
    double ruralReward = engine.calculateResourceReward(ruralContrib);
    double remoteReward = engine.calculateResourceReward(remoteContrib);
    
    SimpleTest::assertGreaterThan(ruralReward, urbanReward, "Rural reward bonus");
    SimpleTest::assertGreaterThan(remoteReward, ruralReward, "Remote reward bonus");
    
    // Test quality multiplier
    ResourceContribution highQualityContrib = createValidContribution();
    highQualityContrib.quality.availability = 0.99;
    highQualityContrib.quality.reliability = 0.95;
    highQualityContrib.quality.latency = 10.0;
    highQualityContrib.quality.throughput = 100.0;
    
    ResourceContribution lowQualityContrib = createValidContribution();
    lowQualityContrib.quality.availability = 0.6;
    lowQualityContrib.quality.reliability = 0.5;
    lowQualityContrib.quality.latency = 800.0;
    lowQualityContrib.quality.throughput = 1.0;
    
    double highQualityReward = engine.calculateResourceReward(highQualityContrib);
    double lowQualityReward = engine.calculateResourceReward(lowQualityContrib);
    
    SimpleTest::assertGreaterThan(highQualityReward, lowQualityReward, "Quality multiplier effect");
    
    engine.shutdown();
}

void testHelperFunctions() {
    std::cout << "\n=== Testing Helper Functions ===" << std::endl;
    
    // Test resource type string conversion
    SimpleTest::assertEqual("COMPUTE", resourceTypeToString(ResourceType::COMPUTE), "COMPUTE type to string");
    SimpleTest::assertEqual("STORAGE", resourceTypeToString(ResourceType::STORAGE), "STORAGE type to string");
    SimpleTest::assertEqual("BANDWIDTH", resourceTypeToString(ResourceType::BANDWIDTH), "BANDWIDTH type to string");
    SimpleTest::assertEqual("ENERGY_EFFICIENT", resourceTypeToString(ResourceType::ENERGY_EFFICIENT), "ENERGY_EFFICIENT type to string");
    SimpleTest::assertEqual("MOBILE_RELAY", resourceTypeToString(ResourceType::MOBILE_RELAY), "MOBILE_RELAY type to string");
    
    SimpleTest::assertTrue(stringToResourceType("COMPUTE") == ResourceType::COMPUTE, "String to COMPUTE type");
    SimpleTest::assertTrue(stringToResourceType("STORAGE") == ResourceType::STORAGE, "String to STORAGE type");
    SimpleTest::assertTrue(stringToResourceType("BANDWIDTH") == ResourceType::BANDWIDTH, "String to BANDWIDTH type");
    SimpleTest::assertTrue(stringToResourceType("ENERGY_EFFICIENT") == ResourceType::ENERGY_EFFICIENT, "String to ENERGY_EFFICIENT type");
    SimpleTest::assertTrue(stringToResourceType("MOBILE_RELAY") == ResourceType::MOBILE_RELAY, "String to MOBILE_RELAY type");
    SimpleTest::assertTrue(stringToResourceType("UNKNOWN") == ResourceType::COMPUTE, "String to unknown type (default)");
    
    // Test resource quality string conversion
    SimpleTest::assertEqual("EXCELLENT", resourceQualityToString(ResourceQuality::EXCELLENT), "EXCELLENT quality to string");
    SimpleTest::assertEqual("GOOD", resourceQualityToString(ResourceQuality::GOOD), "GOOD quality to string");
    SimpleTest::assertEqual("FAIR", resourceQualityToString(ResourceQuality::FAIR), "FAIR quality to string");
    SimpleTest::assertEqual("POOR", resourceQualityToString(ResourceQuality::POOR), "POOR quality to string");
    SimpleTest::assertEqual("INVALID", resourceQualityToString(ResourceQuality::INVALID), "INVALID quality to string");
}

void testValidationCooldown() {
    std::cout << "\n=== Testing Validation Cooldown ===" << std::endl;
    
    ProofOfResourceContributionEngine engine;
    engine.initialize();
    
    ResourceContribution contribution1 = createValidContribution(ResourceType::COMPUTE, 100.0, "cooldown_test_addr");
    ResourceContribution contribution2 = createValidContribution(ResourceType::COMPUTE, 200.0, "cooldown_test_addr"); // Same address
    
    SimpleTest::assertTrue(engine.validateResourceContribution(contribution1), "First validation");
    
    // Second validation should fail due to cooldown (same address)
    SimpleTest::assertTrue(!engine.validateResourceContribution(contribution2), "Second validation (cooldown)");
    
    // Test with different address should work immediately
    ResourceContribution contribution3 = createValidContribution(ResourceType::COMPUTE, 300.0, "different_addr");
    SimpleTest::assertTrue(engine.validateResourceContribution(contribution3), "Validation with different address");
    
    engine.shutdown();
}

int main() {
    std::cout << "Starting Proof of Resource Contribution Engine Tests" << std::endl;
    
    try {
        testResourceContributionBasics();
        testResourceContributionHash();
        testResourceContributionJsonSerialization();
        testProofOfResourceContributionEngine();
        testResourceTypeValidation();
        testRewardCalculation();
        testHelperFunctions();
        testValidationCooldown();
        
        std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
        std::cout << "Proof of Resource Contribution Engine implementation is working correctly!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}