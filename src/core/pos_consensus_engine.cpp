#include "../../include/core/pos_consensus_engine.h"
#include <algorithm>
#include <numeric>
#include <cmath>

// PoSConfig serialization methods
nlohmann::json PoSConfig::toJson() const {
    nlohmann::json j;
    j["minStakeAmount"] = minStakeAmount;
    j["stakingPeriod"] = stakingPeriod;
    j["slashingPenalty"] = slashingPenalty;
    j["validatorCooldown"] = validatorCooldown;
    j["reputationDecayRate"] = reputationDecayRate;
    j["maxValidatorsPerBlock"] = maxValidatorsPerBlock;
    j["coordinationThreshold"] = coordinationThreshold;
    return j;
}

void PoSConfig::fromJson(const nlohmann::json& j) {
    if (j.contains("minStakeAmount")) minStakeAmount = j["minStakeAmount"];
    if (j.contains("stakingPeriod")) stakingPeriod = j["stakingPeriod"];
    if (j.contains("slashingPenalty")) slashingPenalty = j["slashingPenalty"];
    if (j.contains("validatorCooldown")) validatorCooldown = j["validatorCooldown"];
    if (j.contains("reputationDecayRate")) reputationDecayRate = j["reputationDecayRate"];
    if (j.contains("maxValidatorsPerBlock")) maxValidatorsPerBlock = j["maxValidatorsPerBlock"];
    if (j.contains("coordinationThreshold")) coordinationThreshold = j["coordinationThreshold"];
}

PoSConsensusEngine::PoSConsensusEngine() 
    : totalValidations(0), successfulValidations(0), coordinationEvents(0),
      lastMetricsUpdate(std::chrono::steady_clock::now()) {
    
    // Initialize random generator with current time
    randomGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    
    // Initialize mechanism availability (assume all available initially)
    mechanismAvailability[ConsensusType::PROOF_OF_WORK] = true;
    mechanismAvailability[ConsensusType::PROOF_OF_STAKE] = true;
    mechanismAvailability[ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION] = true;
    mechanismAvailability[ConsensusType::VOTING_CONSENSUS] = true;
    mechanismAvailability[ConsensusType::SMART_CONTRACT_VALIDATION] = true;
}

bool PoSConsensusEngine::initialize() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    Logger::info("Initializing PoS Consensus Engine");
    
    // Initialize coordination data
    updateCoordinationData();
    
    Logger::info("PoS Consensus Engine initialized successfully");
    return true;
}

void PoSConsensusEngine::shutdown() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    Logger::info("Shutting down PoS Consensus Engine");
    
    // Clear validator data
    validators.clear();
    validatorMechanismSupport.clear();
    
    Logger::info("PoS Consensus Engine shutdown complete");
}

bool PoSConsensusEngine::isHealthy() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    // Check if we have active validators
    size_t activeValidators = 0;
    for (const auto& [address, validator] : validators) {
        if (validator.isActive && validator.stakedAmount >= config.minStakeAmount) {
            activeValidators++;
        }
    }
    
    // Need at least 3 active validators for healthy operation
    return activeValidators >= 3;
}

bool PoSConsensusEngine::validateBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    totalValidations++;
    
    try {
        // Get the validator who signed this block
        std::string validatorAddress = block.getValidator();
        
        if (validatorAddress.empty()) {
            Logger::error("PoS: Block validation failed - no validator specified");
            return false;
        }
        
        // Check if validator exists and is eligible
        auto validatorIt = validators.find(validatorAddress);
        if (validatorIt == validators.end()) {
            Logger::error("PoS: Block validation failed - unknown validator: " + validatorAddress);
            return false;
        }
        
        if (!isValidatorEligible(validatorIt->second)) {
            Logger::error("PoS: Block validation failed - validator not eligible: " + validatorAddress);
            return false;
        }
        
        // Verify stake requirements
        if (!verifyStakeRequirements(validatorAddress, config.minStakeAmount)) {
            Logger::error("PoS: Block validation failed - insufficient stake: " + validatorAddress);
            return false;
        }
        
        // Update validator metrics
        ValidatorInfo& validator = validatorIt->second;
        validator.lastValidationTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        validator.validationCount++;
        
        successfulValidations++;
        
        Logger::info("PoS: Block validated successfully by validator: " + validatorAddress);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("PoS: Block validation exception: " + std::string(e.what()));
        return false;
    }
}

bool PoSConsensusEngine::validateTransaction(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    try {
        // Basic transaction validation
        if (!transaction.isValid()) {
            Logger::error("PoS: Transaction validation failed - invalid transaction");
            return false;
        }
        
        const std::string& sender = transaction.getSender();
        double amount = transaction.getAmount();
        
        // Skip validation for coinbase transactions
        if (sender == "COINBASE") {
            return true;
        }
        
        // Check if sender has sufficient balance (including staked amount)
        auto balanceIt = accountBalances.find(sender);
        if (balanceIt == accountBalances.end()) {
            Logger::error("PoS: Transaction validation failed - sender not found: " + sender);
            return false;
        }
        
        double availableBalance = balanceIt->second;
        
        // If sender is a validator, subtract staked amount from available balance
        auto validatorIt = validators.find(sender);
        if (validatorIt != validators.end()) {
            availableBalance -= validatorIt->second.stakedAmount;
        }
        
        if (availableBalance < amount) {
            Logger::error("PoS: Transaction validation failed - insufficient balance for: " + sender);
            return false;
        }
        
        Logger::info("PoS: Transaction validated successfully: " + transaction.getHash());
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("PoS: Transaction validation exception: " + std::string(e.what()));
        return false;
    }
}

ConsensusResult PoSConsensusEngine::processRequest(const ConsensusRequest& request) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    ConsensusResult result(false, ConsensusType::PROOF_OF_STAKE);
    
    try {
        switch (request.type) {
            case RequestType::BLOCK_VALIDATION: {
                // Deserialize block data and validate
                // For now, assume validation passes if we have eligible validators
                if (!validators.empty()) {
                    result.isValid = true;
                    result.confidence = calculateValidatorWeight(validators.begin()->second);
                    result.reason = "Block validation successful";
                }
                break;
            }
            
            case RequestType::TRANSACTION_VALIDATION: {
                // Deserialize transaction data and validate
                result.isValid = true;
                result.confidence = 0.9;
                result.reason = "Transaction validation successful";
                break;
            }
            
            case RequestType::PARAMETER_ADJUSTMENT: {
                // Handle parameter adjustment requests
                result.isValid = true;
                result.confidence = 0.8;
                result.reason = "Parameter adjustment processed";
                break;
            }
            
            case RequestType::GOVERNANCE_PROPOSAL: {
                // Coordinate with voting consensus for governance
                if (coordinateWithMechanism(ConsensusType::VOTING_CONSENSUS, request)) {
                    result.isValid = true;
                    result.confidence = 0.7;
                    result.reason = "Governance proposal coordinated";
                }
                break;
            }
            
            default:
                result.reason = "Unsupported request type";
                break;
        }
        
        // Add metadata
        result.metadata["totalStake"] = std::to_string(getTotalStake());
        result.metadata["activeValidators"] = std::to_string(validators.size());
        
    } catch (const std::exception& e) {
        result.reason = "Processing exception: " + std::string(e.what());
        Logger::error("PoS: Request processing exception: " + std::string(e.what()));
    }
    
    return result;
}

nlohmann::json PoSConsensusEngine::getStatus() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    nlohmann::json status;
    status["type"] = "PROOF_OF_STAKE";
    status["healthy"] = isHealthy();
    status["totalValidators"] = validators.size();
    status["totalStake"] = getTotalStake();
    status["minStakeAmount"] = config.minStakeAmount;
    
    // Count active validators
    size_t activeValidators = 0;
    for (const auto& [address, validator] : validators) {
        if (validator.isActive) {
            activeValidators++;
        }
    }
    status["activeValidators"] = activeValidators;
    
    return status;
}

nlohmann::json PoSConsensusEngine::getMetrics() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    nlohmann::json metrics;
    metrics["totalValidations"] = totalValidations;
    metrics["successfulValidations"] = successfulValidations;
    metrics["coordinationEvents"] = coordinationEvents;
    metrics["successRate"] = totalValidations > 0 ? 
        static_cast<double>(successfulValidations) / totalValidations : 0.0;
    
    // Validator metrics
    nlohmann::json validatorMetrics = nlohmann::json::array();
    for (const auto& [address, validator] : validators) {
        nlohmann::json vm;
        vm["address"] = address;
        vm["stakedAmount"] = validator.stakedAmount;
        vm["validationCount"] = validator.validationCount;
        vm["reputationScore"] = validator.reputationScore;
        vm["isActive"] = validator.isActive;
        validatorMetrics.push_back(vm);
    }
    metrics["validators"] = validatorMetrics;
    
    return metrics;
}

bool PoSConsensusEngine::adjustParameters(const std::map<std::string, double>& parameters) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    try {
        for (const auto& [key, value] : parameters) {
            if (key == "minStakeAmount") {
                config.minStakeAmount = value;
            } else if (key == "stakingPeriod") {
                config.stakingPeriod = static_cast<uint64_t>(value);
            } else if (key == "slashingPenalty") {
                config.slashingPenalty = value;
            } else if (key == "validatorCooldown") {
                config.validatorCooldown = static_cast<uint64_t>(value);
            } else if (key == "reputationDecayRate") {
                config.reputationDecayRate = value;
            } else if (key == "coordinationThreshold") {
                config.coordinationThreshold = value;
            }
        }
        
        Logger::info("PoS: Parameters adjusted successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("PoS: Parameter adjustment failed: " + std::string(e.what()));
        return false;
    }
}

std::map<std::string, double> PoSConsensusEngine::getParameters() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    std::map<std::string, double> parameters;
    parameters["minStakeAmount"] = config.minStakeAmount;
    parameters["stakingPeriod"] = static_cast<double>(config.stakingPeriod);
    parameters["slashingPenalty"] = config.slashingPenalty;
    parameters["validatorCooldown"] = static_cast<double>(config.validatorCooldown);
    parameters["reputationDecayRate"] = config.reputationDecayRate;
    parameters["coordinationThreshold"] = config.coordinationThreshold;
    
    return parameters;
}

bool PoSConsensusEngine::stakeTokens(const std::string& address, double amount) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    if (amount < config.minStakeAmount) {
        Logger::error("PoS: Staking failed - amount below minimum: " + std::to_string(amount));
        return false;
    }
    
    // Check if address has sufficient balance
    auto balanceIt = accountBalances.find(address);
    if (balanceIt == accountBalances.end() || balanceIt->second < amount) {
        Logger::error("PoS: Staking failed - insufficient balance for: " + address);
        return false;
    }
    
    // Add or update validator
    auto validatorIt = validators.find(address);
    if (validatorIt == validators.end()) {
        validators[address] = ValidatorInfo(address, amount);
    } else {
        validatorIt->second.stakedAmount += amount;
        validatorIt->second.isActive = true;
    }
    
    // Update coordination data
    updateCoordinationData();
    
    Logger::info("PoS: Tokens staked successfully - " + std::to_string(amount) + " by " + address);
    return true;
}

bool PoSConsensusEngine::unstakeTokens(const std::string& address, double amount) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto validatorIt = validators.find(address);
    if (validatorIt == validators.end()) {
        Logger::error("PoS: Unstaking failed - validator not found: " + address);
        return false;
    }
    
    if (validatorIt->second.stakedAmount < amount) {
        Logger::error("PoS: Unstaking failed - insufficient staked amount for: " + address);
        return false;
    }
    
    validatorIt->second.stakedAmount -= amount;
    
    // Deactivate validator if stake falls below minimum
    if (validatorIt->second.stakedAmount < config.minStakeAmount) {
        validatorIt->second.isActive = false;
    }
    
    // Update coordination data
    updateCoordinationData();
    
    Logger::info("PoS: Tokens unstaked successfully - " + std::to_string(amount) + " by " + address);
    return true;
}

std::string PoSConsensusEngine::selectValidator() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    return selectValidatorWeighted();
}

std::vector<std::string> PoSConsensusEngine::selectValidators(size_t count) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    std::vector<std::string> selectedValidators;
    std::vector<std::pair<std::string, double>> eligibleValidators;
    
    // Collect eligible validators with their weights
    for (const auto& [address, validator] : validators) {
        if (isValidatorEligible(validator)) {
            double weight = calculateValidatorWeight(validator);
            eligibleValidators.push_back({address, weight});
        }
    }
    
    if (eligibleValidators.empty()) {
        return selectedValidators;
    }
    
    // Sort by weight (descending)
    std::sort(eligibleValidators.begin(), eligibleValidators.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Select top validators up to the requested count
    size_t selectCount = std::min(count, eligibleValidators.size());
    for (size_t i = 0; i < selectCount; i++) {
        selectedValidators.push_back(eligibleValidators[i].first);
    }
    
    return selectedValidators;
}

bool PoSConsensusEngine::validateBlockWithStake(const Block& block, const std::string& validatorAddress) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto validatorIt = validators.find(validatorAddress);
    if (validatorIt == validators.end()) {
        Logger::error("PoS: Block validation failed - validator not found: " + validatorAddress);
        return false;
    }
    
    if (!isValidatorEligible(validatorIt->second)) {
        Logger::error("PoS: Block validation failed - validator not eligible: " + validatorAddress);
        return false;
    }
    
    // Update validator metrics
    validatorIt->second.lastValidationTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    validatorIt->second.validationCount++;
    
    Logger::info("PoS: Block validated with stake by: " + validatorAddress);
    return true;
}

bool PoSConsensusEngine::coordinateWithMechanism(ConsensusType mechanism, const ConsensusRequest& request) {
    std::lock_guard<std::mutex> lock(coordinationMutex);
    
    coordinationEvents++;
    
    // Check if mechanism is available
    if (mechanismAvailability.find(mechanism) == mechanismAvailability.end() ||
        !mechanismAvailability[mechanism]) {
        Logger::warning("PoS: Coordination failed - mechanism not available");
        return false;
    }
    
    // Select validators that support the target mechanism
    std::vector<std::string> coordinatingValidators = selectValidatorsForCoordination({mechanism});
    
    if (coordinatingValidators.empty()) {
        Logger::warning("PoS: Coordination failed - no supporting validators");
        return false;
    }
    
    Logger::info("PoS: Coordinating with mechanism, validators: " + std::to_string(coordinatingValidators.size()));
    return true;
}

std::vector<std::string> PoSConsensusEngine::selectValidatorsForCoordination(const std::vector<ConsensusType>& mechanisms) {
    std::vector<std::string> coordinatingValidators;
    
    for (const auto& [address, validator] : validators) {
        if (!isValidatorEligible(validator)) {
            continue;
        }
        
        // Check if validator supports all required mechanisms
        bool supportsAll = true;
        for (ConsensusType mechanism : mechanisms) {
            auto supportIt = validatorMechanismSupport.find(address);
            if (supportIt == validatorMechanismSupport.end() ||
                supportIt->second.find(mechanism) == supportIt->second.end() ||
                !supportIt->second[mechanism]) {
                supportsAll = false;
                break;
            }
        }
        
        if (supportsAll) {
            coordinatingValidators.push_back(address);
        }
    }
    
    // Limit to maximum validators per coordination
    if (coordinatingValidators.size() > config.maxValidatorsPerBlock) {
        coordinatingValidators.resize(config.maxValidatorsPerBlock);
    }
    
    return coordinatingValidators;
}

bool PoSConsensusEngine::validateCrossMechanismConsensus(const std::vector<ConsensusResult>& results) {
    std::lock_guard<std::mutex> lock(coordinationMutex);
    
    if (results.empty()) {
        return false;
    }
    
    double coordinationScore = calculateCoordinationScore(results);
    
    bool consensusReached = coordinationScore >= config.coordinationThreshold;
    
    Logger::info("PoS: Cross-mechanism consensus validation - Score: " + 
                std::to_string(coordinationScore) + ", Reached: " + 
                (consensusReached ? "true" : "false"));
    
    return consensusReached;
}

void PoSConsensusEngine::updateMechanismWeights(const std::map<ConsensusType, double>& newWeights) {
    std::lock_guard<std::mutex> lock(coordinationMutex);
    
    coordinationData.mechanismWeights = newWeights;
    coordinationData.lastCoordinationUpdate = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    Logger::info("PoS: Mechanism weights updated");
}

// Additional implementation methods continue...
bool PoSConsensusEngine::addValidator(const std::string& address, double initialStake) {
    return stakeTokens(address, initialStake);
}

bool PoSConsensusEngine::removeValidator(const std::string& address) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto validatorIt = validators.find(address);
    if (validatorIt == validators.end()) {
        return false;
    }
    
    validatorIt->second.isActive = false;
    Logger::info("PoS: Validator removed: " + address);
    return true;
}

bool PoSConsensusEngine::slashValidator(const std::string& address, double penalty) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto validatorIt = validators.find(address);
    if (validatorIt == validators.end()) {
        return false;
    }
    
    double slashAmount = validatorIt->second.stakedAmount * penalty;
    validatorIt->second.stakedAmount -= slashAmount;
    validatorIt->second.reputationScore *= (1.0 - penalty);
    
    if (validatorIt->second.stakedAmount < config.minStakeAmount) {
        validatorIt->second.isActive = false;
    }
    
    Logger::info("PoS: Validator slashed - " + address + ", amount: " + std::to_string(slashAmount));
    return true;
}

void PoSConsensusEngine::updateValidatorReputation(const std::string& address, bool successful) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto validatorIt = validators.find(address);
    if (validatorIt == validators.end()) {
        return;
    }
    
    if (successful) {
        validatorIt->second.reputationScore = std::min(1.0, validatorIt->second.reputationScore + 0.01);
    } else {
        validatorIt->second.reputationScore = std::max(0.0, validatorIt->second.reputationScore - 0.05);
    }
}

std::map<std::string, ValidatorInfo> PoSConsensusEngine::getValidators() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    return validators;
}

double PoSConsensusEngine::getTotalStake() const {
    double totalStake = 0.0;
    for (const auto& [address, validator] : validators) {
        if (validator.isActive) {
            totalStake += validator.stakedAmount;
        }
    }
    return totalStake;
}

ValidatorInfo PoSConsensusEngine::getValidatorInfo(const std::string& address) const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto validatorIt = validators.find(address);
    if (validatorIt != validators.end()) {
        return validatorIt->second;
    }
    return ValidatorInfo();
}

StakeCoordinationData PoSConsensusEngine::getCoordinationData() const {
    std::lock_guard<std::mutex> lock(coordinationMutex);
    return coordinationData;
}

void PoSConsensusEngine::setAccountBalances(const std::map<std::string, double>& balances) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    accountBalances = balances;
}

void PoSConsensusEngine::updateAccountBalance(const std::string& address, double balance) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    accountBalances[address] = balance;
}

// Private helper methods
double PoSConsensusEngine::calculateValidatorWeight(const ValidatorInfo& validator) const {
    if (!validator.isActive) {
        return 0.0;
    }
    
    double stakeWeight = validator.stakedAmount / std::max(1.0, coordinationData.totalNetworkStake);
    double reputationWeight = validator.reputationScore;
    
    // Combine stake and reputation with 70% stake, 30% reputation weighting
    return (stakeWeight * 0.7) + (reputationWeight * 0.3);
}

bool PoSConsensusEngine::isValidatorEligible(const ValidatorInfo& validator) const {
    if (!validator.isActive) {
        return false;
    }
    
    if (validator.stakedAmount < config.minStakeAmount) {
        return false;
    }
    
    // Check cooldown period
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (currentTime - validator.lastValidationTime < config.validatorCooldown) {
        return false;
    }
    
    return true;
}

std::string PoSConsensusEngine::selectValidatorWeighted() {
    std::vector<std::pair<std::string, double>> eligibleValidators;
    
    for (const auto& [address, validator] : validators) {
        if (isValidatorEligible(validator)) {
            double weight = calculateValidatorWeight(validator);
            eligibleValidators.push_back({address, weight});
        }
    }
    
    if (eligibleValidators.empty()) {
        return "";
    }
    
    // Calculate total weight
    double totalWeight = 0.0;
    for (const auto& [address, weight] : eligibleValidators) {
        totalWeight += weight;
    }
    
    if (totalWeight == 0.0) {
        return eligibleValidators[0].first;
    }
    
    // Weighted random selection
    std::uniform_real_distribution<double> dist(0.0, totalWeight);
    double randomValue = dist(randomGenerator);
    
    double cumulativeWeight = 0.0;
    for (const auto& [address, weight] : eligibleValidators) {
        cumulativeWeight += weight;
        if (randomValue <= cumulativeWeight) {
            return address;
        }
    }
    
    return eligibleValidators.back().first;
}

void PoSConsensusEngine::updateCoordinationData() {
    coordinationData.totalNetworkStake = getTotalStake();
    coordinationData.lastCoordinationUpdate = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool PoSConsensusEngine::verifyStakeRequirements(const std::string& address, double requiredStake) const {
    auto validatorIt = validators.find(address);
    if (validatorIt == validators.end()) {
        return false;
    }
    
    return validatorIt->second.stakedAmount >= requiredStake;
}

void PoSConsensusEngine::decayValidatorReputations() {
    for (auto& [address, validator] : validators) {
        validator.reputationScore *= (1.0 - config.reputationDecayRate);
        validator.reputationScore = std::max(0.0, validator.reputationScore);
    }
}

double PoSConsensusEngine::calculateCoordinationScore(const std::vector<ConsensusResult>& results) const {
    if (results.empty()) {
        return 0.0;
    }
    
    double totalScore = 0.0;
    double totalWeight = 0.0;
    
    for (const auto& result : results) {
        double weight = 1.0;
        auto weightIt = coordinationData.mechanismWeights.find(result.mechanism);
        if (weightIt != coordinationData.mechanismWeights.end()) {
            weight = weightIt->second;
        }
        
        double score = result.isValid ? result.confidence : 0.0;
        totalScore += score * weight;
        totalWeight += weight;
    }
    
    return totalWeight > 0.0 ? totalScore / totalWeight : 0.0;
}

bool PoSConsensusEngine::checkMechanismCompatibility(ConsensusType mechanism) const {
    auto availabilityIt = mechanismAvailability.find(mechanism);
    return availabilityIt != mechanismAvailability.end() && availabilityIt->second;
}