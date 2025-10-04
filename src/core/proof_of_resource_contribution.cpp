#include "../../include/core/proof_of_resource_contribution.h"
#include "../../include/core/logger.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

// ResourceContribution implementation
std::string ResourceContribution::calculateHash() const {
    std::stringstream ss;
    ss << contributorAddress << static_cast<int>(type) << amount << duration 
       << timestamp << quality.availability << quality.reliability 
       << quality.latency << quality.throughput << quality.energyEfficiency
       << (quality.isRenewableEnergy ? "1" : "0") << quality.location;
    return Utils::calculateSHA256(ss.str());
}

nlohmann::json ResourceContribution::toJson() const {
    nlohmann::json j;
    j["contributorAddress"] = contributorAddress;
    j["type"] = resourceTypeToString(type);
    j["amount"] = amount;
    j["duration"] = duration;
    j["timestamp"] = timestamp;
    j["proof"] = proof;
    j["signature"] = signature;
    
    // Quality metrics
    nlohmann::json qualityJson;
    qualityJson["availability"] = quality.availability;
    qualityJson["reliability"] = quality.reliability;
    qualityJson["latency"] = quality.latency;
    qualityJson["throughput"] = quality.throughput;
    qualityJson["energyEfficiency"] = quality.energyEfficiency;
    qualityJson["isRenewableEnergy"] = quality.isRenewableEnergy;
    qualityJson["location"] = quality.location;
    j["quality"] = qualityJson;
    
    return j;
}

ResourceContribution ResourceContribution::fromJson(const nlohmann::json& j) {
    ResourceContribution contribution;
    
    contribution.contributorAddress = j["contributorAddress"];
    contribution.type = stringToResourceType(j["type"]);
    contribution.amount = j["amount"];
    contribution.duration = j["duration"];
    contribution.timestamp = j["timestamp"];
    contribution.proof = j.value("proof", "");
    contribution.signature = j.value("signature", "");
    
    // Quality metrics
    if (j.contains("quality")) {
        const auto& qualityJson = j["quality"];
        contribution.quality.availability = qualityJson.value("availability", 0.0);
        contribution.quality.reliability = qualityJson.value("reliability", 0.0);
        contribution.quality.latency = qualityJson.value("latency", 1000.0);
        contribution.quality.throughput = qualityJson.value("throughput", 0.0);
        contribution.quality.energyEfficiency = qualityJson.value("energyEfficiency", 0.0);
        contribution.quality.isRenewableEnergy = qualityJson.value("isRenewableEnergy", false);
        contribution.quality.location = qualityJson.value("location", "");
    }
    
    return contribution;
}

// ResourceValidator implementation
ResourceValidator::ResourceValidator() {
    Utils::logInfo("ResourceValidator initialized");
}

bool ResourceValidator::validateContribution(const ResourceContribution& contribution) {
    std::lock_guard<std::mutex> lock(validationMutex);
    
    // Basic validation
    if (contribution.contributorAddress.empty() || contribution.amount <= 0) {
        return false;
    }
    
    // Check if contribution is too recent (prevent spam)
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    auto it = lastValidationTime.find(contribution.contributorAddress);
    if (it != lastValidationTime.end() && (now - it->second) < 60) { // 1 minute cooldown
        return false;
    }
    
    // Validate resource amount
    if (!isValidResourceAmount(contribution.type, contribution.amount)) {
        return false;
    }
    
    // Validate quality metrics
    if (!isValidQualityMetrics(contribution.quality)) {
        return false;
    }
    
    // Check contributor reputation
    if (!checkContributorReputation(contribution.contributorAddress)) {
        return false;
    }
    
    // Verify cryptographic proof
    if (!verifyContributionProof(contribution)) {
        return false;
    }
    
    // Type-specific validation
    bool isValid = false;
    switch (contribution.type) {
        case ResourceType::COMPUTE:
            isValid = validateComputeContribution(contribution);
            break;
        case ResourceType::STORAGE:
            isValid = validateStorageContribution(contribution);
            break;
        case ResourceType::BANDWIDTH:
            isValid = validateBandwidthContribution(contribution);
            break;
        case ResourceType::ENERGY_EFFICIENT:
            isValid = validateEnergyEfficientContribution(contribution);
            break;
        case ResourceType::MOBILE_RELAY:
            isValid = validateMobileRelayContribution(contribution);
            break;
    }
    
    if (isValid) {
        lastValidationTime[contribution.contributorAddress] = now;
    }
    
    return isValid;
}

bool ResourceValidator::validateComputeContribution(const ResourceContribution& contribution) {
    // Validate compute resource contribution
    // Amount should represent CPU cycles or computational units
    if (contribution.amount < 1.0 || contribution.amount > 1000000.0) {
        return false;
    }
    
    // Check quality metrics specific to compute
    if (contribution.quality.availability < 0.5) { // At least 50% uptime
        return false;
    }
    
    // Duration should be reasonable (at least 1 minute, max 24 hours)
    if (contribution.duration < 60 || contribution.duration > 86400) {
        return false;
    }
    
    return true;
}

bool ResourceValidator::validateStorageContribution(const ResourceContribution& contribution) {
    // Validate storage resource contribution
    // Amount should represent storage space in GB
    if (contribution.amount < 0.1 || contribution.amount > 10000.0) {
        return false;
    }
    
    // Storage requires high availability
    if (contribution.quality.availability < 0.8) { // At least 80% uptime
        return false;
    }
    
    // Storage contributions should be for longer periods
    if (contribution.duration < 3600) { // At least 1 hour
        return false;
    }
    
    return true;
}

bool ResourceValidator::validateBandwidthContribution(const ResourceContribution& contribution) {
    // Validate bandwidth resource contribution
    // Amount should represent bandwidth in Mbps
    if (contribution.amount < 0.1 || contribution.amount > 1000.0) {
        return false;
    }
    
    // Bandwidth requires good throughput
    if (contribution.quality.throughput < contribution.amount * 0.8) {
        return false; // Actual throughput should be at least 80% of claimed
    }
    
    // Latency should be reasonable
    if (contribution.quality.latency > 500.0) { // Max 500ms latency
        return false;
    }
    
    return true;
}

bool ResourceValidator::validateEnergyEfficientContribution(const ResourceContribution& contribution) {
    // Validate energy-efficient contribution (renewable energy sources)
    if (!contribution.quality.isRenewableEnergy) {
        return false; // Must use renewable energy
    }
    
    // Energy efficiency score should be high
    if (contribution.quality.energyEfficiency < 0.7) {
        return false;
    }
    
    // Any resource type can be energy-efficient, so validate the base resource
    ResourceContribution baseContribution = contribution;
    baseContribution.type = ResourceType::COMPUTE; // Default to compute for validation
    
    return validateComputeContribution(baseContribution);
}

bool ResourceValidator::validateMobileRelayContribution(const ResourceContribution& contribution) {
    // Validate mobile-based contribution (SMS/USSD relay)
    // Amount represents number of transactions relayed
    if (contribution.amount < 1.0 || contribution.amount > 1000.0) {
        return false;
    }
    
    // Mobile contributions can have higher latency
    if (contribution.quality.latency > 2000.0) { // Max 2 seconds for mobile
        return false;
    }
    
    // Reliability is important for mobile relay
    if (contribution.quality.reliability < 0.6) {
        return false;
    }
    
    return true;
}

bool ResourceValidator::verifyContributionProof(const ResourceContribution& contribution) {
    // In a real implementation, this would verify cryptographic proof
    // For now, we'll do basic validation
    if (contribution.proof.empty()) {
        return false;
    }
    
    // Verify the proof matches the contribution hash
    std::string expectedHash = contribution.calculateHash();
    return contribution.proof.find(expectedHash.substr(0, 8)) != std::string::npos;
}

bool ResourceValidator::isValidResourceAmount(ResourceType type, double amount) {
    if (amount <= 0) return false;
    
    switch (type) {
        case ResourceType::COMPUTE:
            return amount >= 1.0 && amount <= 1000000.0;
        case ResourceType::STORAGE:
            return amount >= 0.1 && amount <= 10000.0;
        case ResourceType::BANDWIDTH:
            return amount >= 0.1 && amount <= 1000.0;
        case ResourceType::ENERGY_EFFICIENT:
            return amount >= 0.1 && amount <= 1000000.0;
        case ResourceType::MOBILE_RELAY:
            return amount >= 1.0 && amount <= 1000.0;
    }
    return false;
}

bool ResourceValidator::isValidQualityMetrics(const QualityMetrics& quality) {
    // Validate quality metrics ranges
    if (quality.availability < 0.0 || quality.availability > 1.0) return false;
    if (quality.reliability < 0.0 || quality.reliability > 1.0) return false;
    if (quality.latency < 0.0 || quality.latency > 10000.0) return false;
    if (quality.throughput < 0.0) return false;
    if (quality.energyEfficiency < 0.0 || quality.energyEfficiency > 1.0) return false;
    
    return true;
}

bool ResourceValidator::checkContributorReputation(const std::string& address) {
    // TODO: In a real implementation, this would check contributor's reputation
    // For now, we'll accept all non-empty addresses
    return !address.empty() && address.length() >= 10;
}

// RewardCalculator implementation
RewardCalculator::RewardCalculator() : energyEfficiencyBonusRate(0.2) {
    initializeRewardRates();
    Utils::logInfo("RewardCalculator initialized");
}

void RewardCalculator::initializeRewardRates() {
    // Base reward rates per unit of resource
    baseRewardRates[ResourceType::COMPUTE] = 0.1;        // 0.1 tokens per compute unit
    baseRewardRates[ResourceType::STORAGE] = 0.05;       // 0.05 tokens per GB
    baseRewardRates[ResourceType::BANDWIDTH] = 0.2;      // 0.2 tokens per Mbps
    baseRewardRates[ResourceType::ENERGY_EFFICIENT] = 0.15; // Bonus for renewable energy
    baseRewardRates[ResourceType::MOBILE_RELAY] = 0.3;   // Higher reward for mobile inclusion
    
    // Regional multipliers to incentivize underserved areas
    regionalMultipliers["rural"] = 1.5;
    regionalMultipliers["urban"] = 1.0;
    regionalMultipliers["remote"] = 2.0;
    regionalMultipliers["default"] = 1.0;
}

double RewardCalculator::calculateResourceReward(const ResourceContribution& contribution, 
                                               const ResourceMetrics& networkMetrics) {
    // Calculate base reward
    double baseReward = calculateBaseReward(contribution.type, contribution.amount);
    
    // Apply quality multiplier
    double qualityMultiplier = calculateQualityMultiplier(contribution.quality);
    
    // Apply scarcity multiplier based on network needs
    double scarcityMultiplier = calculateScarcityMultiplier(contribution.type, networkMetrics);
    
    // Apply regional incentive
    double regionalMultiplier = calculateRegionalMultiplier(contribution.quality.location);
    
    // Apply energy efficiency bonus
    double energyBonus = calculateEnergyEfficiencyBonus(contribution.quality);
    
    // Calculate duration factor (longer contributions get slight bonus)
    double durationFactor = 1.0 + std::min(0.5, contribution.duration / 86400.0 * 0.1);
    
    // Final reward calculation
    double totalReward = baseReward * qualityMultiplier * scarcityMultiplier * 
                        regionalMultiplier * durationFactor + energyBonus;
    
    return std::max(0.0, totalReward);
}

double RewardCalculator::calculateBaseReward(ResourceType type, double amount) {
    auto it = baseRewardRates.find(type);
    if (it == baseRewardRates.end()) {
        return 0.0;
    }
    
    return it->second * amount;
}

double RewardCalculator::calculateQualityMultiplier(const QualityMetrics& quality) {
    // Weighted average of quality metrics
    double availabilityWeight = 0.3;
    double reliabilityWeight = 0.3;
    double latencyWeight = 0.2;
    double throughputWeight = 0.2;
    
    // Normalize latency (lower is better)
    double normalizedLatency = std::max(0.0, 1.0 - quality.latency / 1000.0);
    
    // Normalize throughput (higher is better, cap at reasonable value)
    double normalizedThroughput = std::min(1.0, quality.throughput / 100.0);
    
    double qualityScore = availabilityWeight * quality.availability +
                         reliabilityWeight * quality.reliability +
                         latencyWeight * normalizedLatency +
                         throughputWeight * normalizedThroughput;
    
    // Quality multiplier ranges from 0.5 to 2.0
    return 0.5 + 1.5 * qualityScore;
}

double RewardCalculator::calculateScarcityMultiplier(ResourceType type, 
                                                   const ResourceMetrics& networkMetrics) {
    // Find the total contribution for this resource type
    auto it = networkMetrics.totalContributions.find(type);
    if (it == networkMetrics.totalContributions.end()) {
        return 2.0; // High multiplier for new resource types
    }
    
    double totalContribution = it->second;
    
    // Calculate scarcity based on network demand vs supply
    // This is a simplified model - in reality, you'd have more sophisticated demand prediction
    double demandThreshold = 1000.0; // Arbitrary threshold for demonstration
    
    if (totalContribution < demandThreshold * 0.5) {
        return 2.0; // High scarcity
    } else if (totalContribution < demandThreshold) {
        return 1.5; // Medium scarcity
    } else {
        return 1.0; // Normal supply
    }
}

double RewardCalculator::calculateRegionalMultiplier(const std::string& location) {
    if (location.empty()) {
        return regionalMultipliers["default"];
    }
    
    // Simple location-based multiplier
    std::string lowerLocation = location;
    std::transform(lowerLocation.begin(), lowerLocation.end(), lowerLocation.begin(), ::tolower);
    
    for (const auto& [region, multiplier] : regionalMultipliers) {
        if (lowerLocation.find(region) != std::string::npos) {
            return multiplier;
        }
    }
    
    return regionalMultipliers["default"];
}

double RewardCalculator::calculateEnergyEfficiencyBonus(const QualityMetrics& quality) {
    if (!quality.isRenewableEnergy) {
        return 0.0;
    }
    
    // Bonus based on energy efficiency score
    return energyEfficiencyBonusRate * quality.energyEfficiency;
}

// ProofOfResourceContributionEngine implementation
ProofOfResourceContributionEngine::ProofOfResourceContributionEngine() 
    : validator(std::make_unique<ResourceValidator>()),
      rewardCalculator(std::make_unique<RewardCalculator>()),
      minResourceContribution(1.0),
      contributionValidityPeriod(86400), // 24 hours
      qualityThreshold(0.5) {
    
    // Initialize accepted resource types
    acceptedResourceTypes = {
        ResourceType::COMPUTE,
        ResourceType::STORAGE,
        ResourceType::BANDWIDTH,
        ResourceType::ENERGY_EFFICIENT,
        ResourceType::MOBILE_RELAY
    };
    
    // Initialize network demand (would be updated based on actual network conditions)
    networkDemand[ResourceType::COMPUTE] = 100.0;
    networkDemand[ResourceType::STORAGE] = 50.0;
    networkDemand[ResourceType::BANDWIDTH] = 75.0;
    networkDemand[ResourceType::ENERGY_EFFICIENT] = 25.0;
    networkDemand[ResourceType::MOBILE_RELAY] = 10.0;
}

bool ProofOfResourceContributionEngine::initialize() {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    if (isInitialized.load()) {
        return true;
    }
    
    try {
        Utils::logInfo("Initializing Proof of Resource Contribution Engine");
        
        // Initialize metrics
        metrics = ResourceMetrics();
        
        // Mark as initialized
        isInitialized.store(true);
        isShuttingDown.store(false);
        
        Utils::logInfo("PoRC Engine initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Utils::logError("Failed to initialize PoRC Engine: " + std::string(e.what()));
        return false;
    }
}

void ProofOfResourceContributionEngine::shutdown() {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    if (!isInitialized.load() || isShuttingDown.load()) {
        return;
    }
    
    Utils::logInfo("Shutting down Proof of Resource Contribution Engine");
    
    isShuttingDown.store(true);
    
    // Clean up resources
    contributorHistory.clear();
    
    isInitialized.store(false);
    
    Utils::logInfo("PoRC Engine shutdown complete");
}

bool ProofOfResourceContributionEngine::isHealthy() const {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return false;
    }
    
    // If no validations have been performed yet, consider healthy if initialized
    if (metrics.totalValidations == 0) {
        return true;
    }
    
    // Consider healthy if we have a reasonable success rate
    return (metrics.successfulValidations * 100 / metrics.totalValidations >= 50);
}

bool ProofOfResourceContributionEngine::validateBlock(const Block& block) {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    try {
        // For PoRC, we validate that the block contains valid resource contributions
        // This is a simplified implementation
        
        const auto& transactions = block.getTransactions();
        bool hasValidContributions = false;
        
        for (const auto& tx : transactions) {
            // Check if transaction contains resource contribution data
            // In a real implementation, this would be encoded in transaction metadata
            if (tx.getSender() != "COINBASE" && tx.getAmount() > 0) {
                hasValidContributions = true;
                break;
            }
        }
        
        // Block is valid if it has at least one valid transaction
        // In a full implementation, we'd validate specific PoRC requirements
        return hasValidContributions || transactions.empty();
        
    } catch (const std::exception& e) {
        Utils::logError("PoRC block validation error: " + std::string(e.what()));
        return false;
    }
}

bool ProofOfResourceContributionEngine::validateTransaction(const Transaction& transaction) {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    try {
        // Basic transaction validation
        if (!transaction.isValid()) {
            return false;
        }
        
        // For PoRC, we accept all valid transactions
        // In a full implementation, we'd validate resource contribution transactions
        return true;
        
    } catch (const std::exception& e) {
        Utils::logError("PoRC transaction validation error: " + std::string(e.what()));
        return false;
    }
}

ConsensusResult ProofOfResourceContributionEngine::processRequest(const ConsensusRequest& request) {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.0, 
                             "Engine not initialized");
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    try {
        bool isValid = false;
        double confidence = 0.0;
        std::string reason = "";
        
        switch (request.type) {
            case RequestType::BLOCK_VALIDATION: {
                // Parse block data and validate
                try {
                    Block block = Block::deserialize(request.data);
                    isValid = validateBlock(block);
                    confidence = isValid ? 0.9 : 0.0;
                    reason = isValid ? "Block validated successfully" : "Block validation failed";
                } catch (const std::exception& e) {
                    isValid = false;
                    confidence = 0.0;
                    reason = "Failed to parse block data";
                }
                break;
            }
            
            case RequestType::TRANSACTION_VALIDATION: {
                // Parse transaction data and validate
                try {
                    Transaction tx = Transaction::deserialize(request.data);
                    isValid = validateTransaction(tx);
                    confidence = isValid ? 0.9 : 0.0;
                    reason = isValid ? "Transaction validated successfully" : "Transaction validation failed";
                } catch (const std::exception& e) {
                    isValid = false;
                    confidence = 0.0;
                    reason = "Failed to parse transaction data";
                }
                break;
            }
            
            case RequestType::PARAMETER_ADJUSTMENT: {
                // Handle parameter adjustment requests
                isValid = true;
                confidence = 0.8;
                reason = "Parameter adjustment acknowledged";
                break;
            }
            
            default:
                isValid = false;
                confidence = 0.0;
                reason = "Unsupported request type";
                break;
        }
        
        // Update metrics
        metrics.totalValidations++;
        if (isValid) {
            metrics.successfulValidations++;
        }
        
        return ConsensusResult(isValid, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 
                             confidence, reason);
        
    } catch (const std::exception& e) {
        Utils::logError("PoRC request processing error: " + std::string(e.what()));
        return ConsensusResult(false, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.0, 
                             "Request processing failed");
    }
}

nlohmann::json ProofOfResourceContributionEngine::getStatus() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    nlohmann::json status;
    status["name"] = getName();
    status["type"] = "PROOF_OF_RESOURCE_CONTRIBUTION";
    status["initialized"] = isInitialized.load();
    status["healthy"] = isHealthy();
    status["shutting_down"] = isShuttingDown.load();
    
    // Resource statistics
    status["total_contributors"] = contributorHistory.size();
    status["total_validations"] = metrics.totalValidations;
    status["successful_validations"] = metrics.successfulValidations;
    status["success_rate"] = metrics.totalValidations > 0 ? 
        (double)metrics.successfulValidations / metrics.totalValidations : 0.0;
    
    // Network demand
    nlohmann::json demandJson;
    for (const auto& [type, demand] : networkDemand) {
        demandJson[resourceTypeToString(type)] = demand;
    }
    status["network_demand"] = demandJson;
    
    return status;
}

nlohmann::json ProofOfResourceContributionEngine::getMetrics() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    nlohmann::json metricsJson;
    
    // Total contributions by type
    nlohmann::json contributionsJson;
    for (const auto& [type, total] : metrics.totalContributions) {
        contributionsJson[resourceTypeToString(type)] = total;
    }
    metricsJson["total_contributions"] = contributionsJson;
    
    // Contributor counts by type
    nlohmann::json countsJson;
    for (const auto& [type, count] : metrics.contributorCounts) {
        countsJson[resourceTypeToString(type)] = count;
    }
    metricsJson["contributor_counts"] = countsJson;
    
    // Average quality by type
    nlohmann::json qualityJson;
    for (const auto& [type, quality] : metrics.averageQuality) {
        qualityJson[resourceTypeToString(type)] = quality;
    }
    metricsJson["average_quality"] = qualityJson;
    
    // Regional distribution
    metricsJson["regional_distribution"] = metrics.regionalDistribution;
    
    // Performance metrics
    metricsJson["network_efficiency"] = metrics.networkEfficiency;
    metricsJson["total_validations"] = metrics.totalValidations;
    metricsJson["successful_validations"] = metrics.successfulValidations;
    
    return metricsJson;
}

bool ProofOfResourceContributionEngine::adjustParameters(const std::map<std::string, double>& parameters) {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    try {
        for (const auto& [key, value] : parameters) {
            if (key == "minResourceContribution") {
                minResourceContribution = std::max(0.1, value);
            } else if (key == "contributionValidityPeriod") {
                contributionValidityPeriod = static_cast<uint64_t>(std::max(3600.0, value)); // Min 1 hour
            } else if (key == "qualityThreshold") {
                qualityThreshold = std::max(0.0, std::min(1.0, value));
            }
        }
        
        Utils::logInfo("PoRC parameters adjusted successfully");
        return true;
        
    } catch (const std::exception& e) {
        Utils::logError("Failed to adjust PoRC parameters: " + std::string(e.what()));
        return false;
    }
}

std::map<std::string, double> ProofOfResourceContributionEngine::getParameters() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    std::map<std::string, double> params;
    params["minResourceContribution"] = minResourceContribution;
    params["contributionValidityPeriod"] = static_cast<double>(contributionValidityPeriod);
    params["qualityThreshold"] = qualityThreshold;
    
    return params;
}

bool ProofOfResourceContributionEngine::validateResourceContribution(const ResourceContribution& contribution) {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    bool isValid = validator->validateContribution(contribution);
    updateMetrics(contribution, isValid);
    
    return isValid;
}

double ProofOfResourceContributionEngine::calculateResourceReward(const ResourceContribution& contribution) {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return 0.0;
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    return rewardCalculator->calculateResourceReward(contribution, metrics);
}

ResourceQuality ProofOfResourceContributionEngine::assessResourceQuality(const std::string& contributor) {
    if (!isInitialized.load() || isShuttingDown.load()) {
        return ResourceQuality::INVALID;
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    auto it = contributorHistory.find(contributor);
    if (it == contributorHistory.end() || it->second.empty()) {
        return ResourceQuality::INVALID;
    }
    
    // Calculate average quality score from recent contributions
    double totalQualityScore = 0.0;
    int validContributions = 0;
    
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (const auto& contribution : it->second) {
        // Only consider contributions from the last 7 days
        if (now - contribution.timestamp <= 604800) {
            double score = calculateResourceScore(contribution);
            totalQualityScore += score;
            validContributions++;
        }
    }
    
    if (validContributions == 0) {
        return ResourceQuality::INVALID;
    }
    
    double averageScore = totalQualityScore / validContributions;
    
    if (averageScore >= 0.9) return ResourceQuality::EXCELLENT;
    if (averageScore >= 0.7) return ResourceQuality::GOOD;
    if (averageScore >= 0.5) return ResourceQuality::FAIR;
    if (averageScore >= 0.3) return ResourceQuality::POOR;
    
    return ResourceQuality::INVALID;
}

bool ProofOfResourceContributionEngine::registerResourceContribution(const ResourceContribution& contribution) {
    if (!validateResourceContribution(contribution)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(engineMutex);
    
    // Add to contributor history
    contributorHistory[contribution.contributorAddress].push_back(contribution);
    
    // Update metrics
    metrics.totalContributions[contribution.type] += contribution.amount;
    metrics.contributorCounts[contribution.type]++;
    
    // Update average quality
    double qualityScore = calculateResourceScore(contribution);
    auto& avgQuality = metrics.averageQuality[contribution.type];
    auto count = metrics.contributorCounts[contribution.type];
    avgQuality = (avgQuality * (count - 1) + qualityScore) / count;
    
    // Update regional distribution
    if (!contribution.quality.location.empty()) {
        metrics.regionalDistribution[contribution.quality.location] += contribution.amount;
    }
    
    // Clean up old contributions periodically
    if (contributorHistory.size() % 100 == 0) {
        cleanupExpiredContributions();
    }
    
    Utils::logInfo("Resource contribution registered for " + contribution.contributorAddress);
    return true;
}

std::vector<ResourceContribution> ProofOfResourceContributionEngine::getContributorHistory(const std::string& address) {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    auto it = contributorHistory.find(address);
    if (it != contributorHistory.end()) {
        return it->second;
    }
    
    return {};
}

ResourceMetrics ProofOfResourceContributionEngine::getResourceMetrics() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    return metrics;
}

void ProofOfResourceContributionEngine::optimizeResourceAllocation() {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    // Update network demand based on current resource usage
    updateNetworkDemand();
    
    // Calculate network efficiency
    double totalDemand = 0.0;
    double totalSupply = 0.0;
    
    for (const auto& [type, demand] : networkDemand) {
        totalDemand += demand;
        auto it = metrics.totalContributions.find(type);
        if (it != metrics.totalContributions.end()) {
            totalSupply += it->second;
        }
    }
    
    metrics.networkEfficiency = totalSupply > 0 ? std::min(1.0, totalDemand / totalSupply) : 0.0;
    
    Utils::logInfo("Resource allocation optimized - efficiency: " + 
                  std::to_string(metrics.networkEfficiency));
}

void ProofOfResourceContributionEngine::updateNetworkDemand() {
    // This would be updated based on actual network conditions
    // For now, we'll simulate demand fluctuations
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.8, 1.2);
    
    for (auto& [type, demand] : networkDemand) {
        demand *= dis(gen); // Simulate demand fluctuation
        demand = std::max(1.0, demand); // Ensure minimum demand
    }
}

std::map<ResourceType, double> ProofOfResourceContributionEngine::getResourceDemand() const {
    std::lock_guard<std::mutex> lock(engineMutex);
    return networkDemand;
}

double ProofOfResourceContributionEngine::calculateResourceScore(const ResourceContribution& contribution) {
    // Calculate a composite score based on quality metrics
    const auto& quality = contribution.quality;
    
    double availabilityScore = quality.availability;
    double reliabilityScore = quality.reliability;
    double latencyScore = std::max(0.0, 1.0 - quality.latency / 1000.0);
    double throughputScore = std::min(1.0, quality.throughput / 100.0);
    double efficiencyScore = quality.energyEfficiency;
    
    // Weighted average
    double score = 0.25 * availabilityScore + 
                  0.25 * reliabilityScore + 
                  0.2 * latencyScore + 
                  0.2 * throughputScore + 
                  0.1 * efficiencyScore;
    
    // Bonus for renewable energy
    if (quality.isRenewableEnergy) {
        score *= 1.1; // 10% bonus
    }
    
    return std::min(1.0, score);
}

double ProofOfResourceContributionEngine::calculateNetworkImpact(const ResourceContribution& contribution) {
    // Calculate the impact of this contribution on the network
    auto it = metrics.totalContributions.find(contribution.type);
    double currentTotal = (it != metrics.totalContributions.end()) ? it->second : 0.0;
    
    // Impact is higher when the resource is scarce
    double scarcityFactor = 1.0 / (1.0 + currentTotal / 100.0);
    
    return contribution.amount * scarcityFactor;
}

double ProofOfResourceContributionEngine::calculateSustainabilityScore(const QualityMetrics& quality) {
    double score = 0.0;
    
    // Energy efficiency contributes to sustainability
    score += quality.energyEfficiency * 0.4;
    
    // Renewable energy gets high sustainability score
    if (quality.isRenewableEnergy) {
        score += 0.6;
    }
    
    return std::min(1.0, score);
}

void ProofOfResourceContributionEngine::updateMetrics(const ResourceContribution& contribution, bool isValid) {
    metrics.totalValidations++;
    if (isValid) {
        metrics.successfulValidations++;
    }
}

void ProofOfResourceContributionEngine::cleanupExpiredContributions() {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (auto& [address, contributions] : contributorHistory) {
        contributions.erase(
            std::remove_if(contributions.begin(), contributions.end(),
                [now, this](const ResourceContribution& contrib) {
                    return (now - contrib.timestamp) > contributionValidityPeriod;
                }),
            contributions.end()
        );
    }
    
    // Remove empty entries
    for (auto it = contributorHistory.begin(); it != contributorHistory.end();) {
        if (it->second.empty()) {
            it = contributorHistory.erase(it);
        } else {
            ++it;
        }
    }
}

bool ProofOfResourceContributionEngine::isValidContributionForBlock(const ResourceContribution& contribution, 
                                                                   const Block& block) {
    // Check if contribution is relevant for this block
    // This is a simplified implementation
    return contribution.timestamp <= static_cast<uint64_t>(block.getTimestamp());
}

bool ProofOfResourceContributionEngine::isValidContributionForTransaction(const ResourceContribution& contribution, 
                                                                         const Transaction& transaction) {
    // Check if contribution is relevant for this transaction
    // This is a simplified implementation
    return contribution.timestamp <= static_cast<uint64_t>(transaction.getTimestamp());
}

// Helper functions implementation
std::string resourceTypeToString(ResourceType type) {
    switch (type) {
        case ResourceType::COMPUTE: return "COMPUTE";
        case ResourceType::STORAGE: return "STORAGE";
        case ResourceType::BANDWIDTH: return "BANDWIDTH";
        case ResourceType::ENERGY_EFFICIENT: return "ENERGY_EFFICIENT";
        case ResourceType::MOBILE_RELAY: return "MOBILE_RELAY";
    }
    return "UNKNOWN";
}

ResourceType stringToResourceType(const std::string& typeStr) {
    if (typeStr == "COMPUTE") return ResourceType::COMPUTE;
    if (typeStr == "STORAGE") return ResourceType::STORAGE;
    if (typeStr == "BANDWIDTH") return ResourceType::BANDWIDTH;
    if (typeStr == "ENERGY_EFFICIENT") return ResourceType::ENERGY_EFFICIENT;
    if (typeStr == "MOBILE_RELAY") return ResourceType::MOBILE_RELAY;
    
    return ResourceType::COMPUTE; // Default
}

std::string resourceQualityToString(ResourceQuality quality) {
    switch (quality) {
        case ResourceQuality::EXCELLENT: return "EXCELLENT";
        case ResourceQuality::GOOD: return "GOOD";
        case ResourceQuality::FAIR: return "FAIR";
        case ResourceQuality::POOR: return "POOR";
        case ResourceQuality::INVALID: return "INVALID";
    }
    return "UNKNOWN";
}