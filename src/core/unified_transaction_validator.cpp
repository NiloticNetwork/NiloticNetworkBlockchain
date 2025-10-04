#include "../../include/core/unified_transaction_validator.h"
#include "../../include/core/consensus_harmony_manager.h"
#include "../../include/core/consensus_router.h"
#include "../../include/core/logger.h"
#include <algorithm>
#include <numeric>
#include <thread>
#include <future>

// TransactionFeeStructure implementation
nlohmann::json TransactionFeeStructure::toJson() const {
    nlohmann::json j;
    j["baseFee"] = baseFee;
    j["powFee"] = powFee;
    j["posFee"] = posFee;
    j["porcFee"] = porcFee;
    j["votingFee"] = votingFee;
    j["smartContractFee"] = smartContractFee;
    j["networkLoadMultiplier"] = networkLoadMultiplier;
    j["priorityMultiplier"] = priorityMultiplier;
    j["complexityMultiplier"] = complexityMultiplier;
    return j;
}

void TransactionFeeStructure::fromJson(const nlohmann::json& j) {
    if (j.contains("baseFee")) baseFee = j["baseFee"];
    if (j.contains("powFee")) powFee = j["powFee"];
    if (j.contains("posFee")) posFee = j["posFee"];
    if (j.contains("porcFee")) porcFee = j["porcFee"];
    if (j.contains("votingFee")) votingFee = j["votingFee"];
    if (j.contains("smartContractFee")) smartContractFee = j["smartContractFee"];
    if (j.contains("networkLoadMultiplier")) networkLoadMultiplier = j["networkLoadMultiplier"];
    if (j.contains("priorityMultiplier")) priorityMultiplier = j["priorityMultiplier"];
    if (j.contains("complexityMultiplier")) complexityMultiplier = j["complexityMultiplier"];
}

// TransactionPriorityFactors implementation
nlohmann::json TransactionPriorityFactors::toJson() const {
    nlohmann::json j;
    j["feeWeight"] = feeWeight;
    j["ageWeight"] = ageWeight;
    j["senderReputationWeight"] = senderReputationWeight;
    j["networkImportanceWeight"] = networkImportanceWeight;
    j["consensusRequirementWeight"] = consensusRequirementWeight;
    return j;
}

void TransactionPriorityFactors::fromJson(const nlohmann::json& j) {
    if (j.contains("feeWeight")) feeWeight = j["feeWeight"];
    if (j.contains("ageWeight")) ageWeight = j["ageWeight"];
    if (j.contains("senderReputationWeight")) senderReputationWeight = j["senderReputationWeight"];
    if (j.contains("networkImportanceWeight")) networkImportanceWeight = j["networkImportanceWeight"];
    if (j.contains("consensusRequirementWeight")) consensusRequirementWeight = j["consensusRequirementWeight"];
}

// UnifiedTransactionValidator implementation
UnifiedTransactionValidator::UnifiedTransactionValidator(ConsensusHarmonyManager* manager)
    : harmonyManager(manager), router(nullptr), totalValidations(0), 
      successfulValidations(0), cachedValidations(0), initialized(false) {
    Logger::info("UnifiedTransactionValidator created");
}

UnifiedTransactionValidator::~UnifiedTransactionValidator() {
    shutdown();
    Logger::info("UnifiedTransactionValidator destroyed");
}

bool UnifiedTransactionValidator::initialize() {
    return initialize(TransactionFeeStructure{}, TransactionPriorityFactors{});
}

bool UnifiedTransactionValidator::initialize(const TransactionFeeStructure& feeConfig,
                                           const TransactionPriorityFactors& priorityConfig) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    if (initialized) {
        Logger::warning("UnifiedTransactionValidator already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing UnifiedTransactionValidator");
        
        // Set configuration
        feeStructure = feeConfig;
        priorityFactors = priorityConfig;
        
        // Validate configuration
        if (!validateConfiguration()) {
            Logger::error("Invalid configuration provided");
            return false;
        }
        
        // Get router from harmony manager if available
        if (harmonyManager) {
            // Note: In a real implementation, we'd need a way to get the router
            // from the harmony manager. For now, we'll assume it's set separately.
        }
        
        // Initialize statistics
        totalValidations = 0;
        successfulValidations = 0;
        cachedValidations = 0;
        mechanismUsageCount.clear();
        
        // Clear validation cache
        validationCache.clear();
        
        initialized = true;
        Logger::info("UnifiedTransactionValidator initialized successfully");
        
        logValidationEvent("validator_initialized", {
            {"feeStructure", feeStructure.toJson()},
            {"priorityFactors", priorityFactors.toJson()}
        });
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize UnifiedTransactionValidator: " + std::string(e.what()));
        return false;
    }
}

void UnifiedTransactionValidator::shutdown() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    if (!initialized) {
        return;
    }
    
    Logger::info("Shutting down UnifiedTransactionValidator");
    
    // Clear cache
    validationCache.clear();
    
    initialized = false;
    Logger::info("UnifiedTransactionValidator shut down successfully");
    
    logValidationEvent("validator_shutdown");
}

TransactionValidationResult UnifiedTransactionValidator::validateTransaction(const Transaction& transaction) {
    if (!initialized) {
        Logger::error("UnifiedTransactionValidator not initialized");
        return TransactionValidationResult(false, 0.0, "Validator not initialized");
    }
    
    totalValidations++;
    
    try {
        std::string txHash = transaction.getHash();
        Logger::debug("Validating transaction: " + txHash);
        
        // Check cache first
        if (isCacheValid(txHash)) {
            cachedValidations++;
            TransactionValidationResult cachedResult = getFromCache(txHash);
            Logger::debug("Using cached validation result for transaction: " + txHash);
            return cachedResult;
        }
        
        // Determine required consensus mechanisms
        std::vector<ConsensusType> requiredMechanisms = determineRequiredMechanisms(transaction);
        
        // Perform validation
        TransactionValidationResult result = performValidation(transaction, requiredMechanisms);
        
        // Calculate fee
        result.calculatedFee = calculateTransactionFeeWithMechanisms(transaction, requiredMechanisms);
        
        // Calculate priority
        result.priority = calculateTransactionPriority(transaction);
        
        // Add to cache
        addToCache(txHash, result);
        
        // Update statistics
        updateValidationStatistics(result);
        
        if (result.isValid) {
            successfulValidations++;
        }
        
        Logger::debug("Transaction validation completed: " + txHash + 
                     " - Result: " + (result.isValid ? "VALID" : "INVALID") + 
                     " (confidence: " + std::to_string(result.confidence) + 
                     ", fee: " + std::to_string(result.calculatedFee) + 
                     ", priority: " + std::to_string(result.priority) + ")");
        
        return result;
        
    } catch (const std::exception& e) {
        Logger::error("Transaction validation failed: " + std::string(e.what()));
        return TransactionValidationResult(false, 0.0, "Validation failed: " + std::string(e.what()));
    }
}

TransactionValidationResult UnifiedTransactionValidator::validateTransactionWithMechanisms(
    const Transaction& transaction,
    const std::vector<ConsensusType>& requiredMechanisms) {
    
    if (!initialized) {
        Logger::error("UnifiedTransactionValidator not initialized");
        return TransactionValidationResult(false, 0.0, "Validator not initialized");
    }
    
    totalValidations++;
    
    try {
        std::string txHash = transaction.getHash();
        Logger::debug("Validating transaction with specific mechanisms: " + txHash);
        
        // Perform validation with specified mechanisms
        TransactionValidationResult result = performValidation(transaction, requiredMechanisms);
        
        // Calculate fee with specified mechanisms
        result.calculatedFee = calculateTransactionFeeWithMechanisms(transaction, requiredMechanisms);
        
        // Calculate priority
        result.priority = calculateTransactionPriority(transaction);
        
        // Update statistics
        updateValidationStatistics(result);
        incrementMechanismUsage(requiredMechanisms);
        
        if (result.isValid) {
            successfulValidations++;
        }
        
        Logger::debug("Transaction validation with mechanisms completed: " + txHash + 
                     " - Result: " + (result.isValid ? "VALID" : "INVALID"));
        
        return result;
        
    } catch (const std::exception& e) {
        Logger::error("Transaction validation with mechanisms failed: " + std::string(e.what()));
        return TransactionValidationResult(false, 0.0, "Validation failed: " + std::string(e.what()));
    }
}

std::vector<TransactionValidationResult> UnifiedTransactionValidator::validateTransactions(
    const std::vector<Transaction>& transactions) {
    
    std::vector<TransactionValidationResult> results;
    results.reserve(transactions.size());
    
    for (const Transaction& tx : transactions) {
        results.push_back(validateTransaction(tx));
    }
    
    Logger::info("Batch validation completed: " + std::to_string(transactions.size()) + " transactions");
    
    return results;
}

std::vector<TransactionValidationResult> UnifiedTransactionValidator::validateTransactionsParallel(
    const std::vector<Transaction>& transactions) {
    
    if (transactions.empty()) {
        return {};
    }
    
    const size_t numThreads = std::min(transactions.size(), 
                                      static_cast<size_t>(std::thread::hardware_concurrency()));
    const size_t chunkSize = transactions.size() / numThreads;
    
    std::vector<std::future<std::vector<TransactionValidationResult>>> futures;
    std::vector<TransactionValidationResult> results;
    results.reserve(transactions.size());
    
    // Launch parallel validation tasks
    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? transactions.size() : (i + 1) * chunkSize;
        
        futures.push_back(std::async(std::launch::async, [this, &transactions, start, end]() {
            std::vector<TransactionValidationResult> chunkResults;
            for (size_t j = start; j < end; ++j) {
                chunkResults.push_back(validateTransaction(transactions[j]));
            }
            return chunkResults;
        }));
    }
    
    // Collect results
    for (auto& future : futures) {
        auto chunkResults = future.get();
        results.insert(results.end(), chunkResults.begin(), chunkResults.end());
    }
    
    Logger::info("Parallel batch validation completed: " + std::to_string(transactions.size()) + 
                " transactions using " + std::to_string(numThreads) + " threads");
    
    return results;
}

double UnifiedTransactionValidator::calculateTransactionFee(const Transaction& transaction) {
    std::vector<ConsensusType> mechanisms = determineRequiredMechanisms(transaction);
    return calculateTransactionFeeWithMechanisms(transaction, mechanisms);
}

double UnifiedTransactionValidator::calculateTransactionFeeWithMechanisms(
    const Transaction& transaction,
    const std::vector<ConsensusType>& mechanisms) {
    
    try {
        // Calculate base fee
        double totalFee = calculateBaseFee(transaction);
        
        // Add mechanism-specific fees
        totalFee += calculateMechanismFees(mechanisms);
        
        // Add complexity fee
        totalFee += calculateComplexityFee(transaction);
        
        // Apply multipliers
        totalFee *= feeStructure.networkLoadMultiplier;
        totalFee *= feeStructure.complexityMultiplier;
        
        // Apply priority multiplier if this is a high-priority transaction
        if (isHighPriorityTransaction(transaction)) {
            totalFee *= feeStructure.priorityMultiplier;
        }
        
        // Ensure minimum fee
        totalFee = std::max(totalFee, feeStructure.baseFee);
        
        Logger::debug("Calculated transaction fee: " + std::to_string(totalFee) + 
                     " for transaction: " + transaction.getHash());
        
        return totalFee;
        
    } catch (const std::exception& e) {
        Logger::error("Fee calculation failed: " + std::string(e.what()));
        return feeStructure.baseFee; // Return minimum fee on error
    }
}

uint32_t UnifiedTransactionValidator::calculateTransactionPriority(const Transaction& transaction) {
    return calculateTransactionPriorityWithFactors(transaction, priorityFactors);
}

uint32_t UnifiedTransactionValidator::calculateTransactionPriorityWithFactors(
    const Transaction& transaction,
    const TransactionPriorityFactors& factors) {
    
    try {
        double priority = 0.0;
        
        // Calculate individual scores
        double feeScore = calculateFeeScore(transaction);
        double ageScore = calculateAgeScore(transaction);
        double reputationScore = calculateSenderReputationScore(transaction);
        double importanceScore = calculateNetworkImportanceScore(transaction);
        double consensusScore = calculateConsensusRequirementScore(transaction);
        
        // Apply weights
        priority += feeScore * factors.feeWeight;
        priority += ageScore * factors.ageWeight;
        priority += reputationScore * factors.senderReputationWeight;
        priority += importanceScore * factors.networkImportanceWeight;
        priority += consensusScore * factors.consensusRequirementWeight;
        
        // Normalize to uint32_t range (0 to 4294967295)
        uint32_t finalPriority = static_cast<uint32_t>(priority * 1000000);
        
        Logger::debug("Calculated transaction priority: " + std::to_string(finalPriority) + 
                     " for transaction: " + transaction.getHash());
        
        return finalPriority;
        
    } catch (const std::exception& e) {
        Logger::error("Priority calculation failed: " + std::string(e.what()));
        return 0; // Return lowest priority on error
    }
}

std::vector<Transaction> UnifiedTransactionValidator::sortTransactionsByPriority(
    const std::vector<Transaction>& transactions) {
    
    std::vector<std::pair<Transaction, uint32_t>> txWithPriority;
    txWithPriority.reserve(transactions.size());
    
    // Calculate priorities
    for (const Transaction& tx : transactions) {
        uint32_t priority = calculateTransactionPriority(tx);
        txWithPriority.emplace_back(tx, priority);
    }
    
    // Sort by priority (higher priority first)
    std::sort(txWithPriority.begin(), txWithPriority.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    // Extract sorted transactions
    std::vector<Transaction> sortedTransactions;
    sortedTransactions.reserve(transactions.size());
    
    for (const auto& pair : txWithPriority) {
        sortedTransactions.push_back(pair.first);
    }
    
    Logger::debug("Sorted " + std::to_string(transactions.size()) + " transactions by priority");
    
    return sortedTransactions;
}

std::vector<Transaction> UnifiedTransactionValidator::filterValidTransactions(
    const std::vector<Transaction>& transactions) {
    
    std::vector<Transaction> validTransactions;
    validTransactions.reserve(transactions.size());
    
    for (const Transaction& tx : transactions) {
        TransactionValidationResult result = validateTransaction(tx);
        if (result.isValid) {
            validTransactions.push_back(tx);
        }
    }
    
    Logger::info("Filtered " + std::to_string(validTransactions.size()) + 
                " valid transactions from " + std::to_string(transactions.size()) + " total");
    
    return validTransactions;
}

std::vector<Transaction> UnifiedTransactionValidator::selectTransactionsForBlock(
    const std::vector<Transaction>& transactions,
    size_t maxTransactions,
    double maxTotalFees) {
    
    // First filter valid transactions
    std::vector<Transaction> validTransactions = filterValidTransactions(transactions);
    
    // Sort by priority
    std::vector<Transaction> sortedTransactions = sortTransactionsByPriority(validTransactions);
    
    // Select transactions up to limits
    std::vector<Transaction> selectedTransactions;
    selectedTransactions.reserve(std::min(maxTransactions, sortedTransactions.size()));
    
    double totalFees = 0.0;
    
    for (const Transaction& tx : sortedTransactions) {
        if (selectedTransactions.size() >= maxTransactions) {
            break;
        }
        
        double txFee = calculateTransactionFee(tx);
        
        if (maxTotalFees > 0.0 && (totalFees + txFee) > maxTotalFees) {
            break;
        }
        
        selectedTransactions.push_back(tx);
        totalFees += txFee;
    }
    
    Logger::info("Selected " + std::to_string(selectedTransactions.size()) + 
                " transactions for block (total fees: " + std::to_string(totalFees) + ")");
    
    return selectedTransactions;
}

// Configuration management
bool UnifiedTransactionValidator::updateFeeStructure(const TransactionFeeStructure& newStructure) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    feeStructure = newStructure;
    
    // Clear cache since fee calculations may have changed
    validationCache.clear();
    
    Logger::info("Fee structure updated");
    logValidationEvent("fee_structure_updated", newStructure.toJson());
    
    return true;
}

bool UnifiedTransactionValidator::updatePriorityFactors(const TransactionPriorityFactors& newFactors) {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    priorityFactors = newFactors;
    
    Logger::info("Priority factors updated");
    logValidationEvent("priority_factors_updated", newFactors.toJson());
    
    return true;
}

TransactionFeeStructure UnifiedTransactionValidator::getFeeStructure() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    return feeStructure;
}

TransactionPriorityFactors UnifiedTransactionValidator::getPriorityFactors() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    return priorityFactors;
}

// Cache management
void UnifiedTransactionValidator::clearValidationCache() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    validationCache.clear();
    Logger::info("Validation cache cleared");
}

void UnifiedTransactionValidator::cleanupExpiredCache() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto expireTime = now - cacheTimeout;
    
    auto it = validationCache.begin();
    size_t removedCount = 0;
    
    while (it != validationCache.end()) {
        auto cacheTime = std::chrono::steady_clock::time_point(
            std::chrono::seconds(it->second.timestamp));
        
        if (cacheTime < expireTime) {
            it = validationCache.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    
    if (removedCount > 0) {
        Logger::debug("Cleaned up " + std::to_string(removedCount) + " expired cache entries");
    }
}

size_t UnifiedTransactionValidator::getCacheSize() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    return validationCache.size();
}

// Statistics and monitoring
nlohmann::json UnifiedTransactionValidator::getStatistics() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    nlohmann::json stats;
    stats["totalValidations"] = totalValidations;
    stats["successfulValidations"] = successfulValidations;
    stats["cachedValidations"] = cachedValidations;
    stats["successRate"] = totalValidations > 0 ? 
        static_cast<double>(successfulValidations) / totalValidations : 0.0;
    stats["cacheHitRate"] = totalValidations > 0 ? 
        static_cast<double>(cachedValidations) / totalValidations : 0.0;
    stats["cacheSize"] = validationCache.size();
    
    // Mechanism usage statistics
    nlohmann::json mechanismUsage;
    for (const auto& [type, count] : mechanismUsageCount) {
        std::string typeStr;
        switch (type) {
            case ConsensusType::PROOF_OF_WORK:
                typeStr = "PROOF_OF_WORK";
                break;
            case ConsensusType::PROOF_OF_STAKE:
                typeStr = "PROOF_OF_STAKE";
                break;
            case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
                typeStr = "PROOF_OF_RESOURCE_CONTRIBUTION";
                break;
            case ConsensusType::VOTING_CONSENSUS:
                typeStr = "VOTING_CONSENSUS";
                break;
            case ConsensusType::SMART_CONTRACT_VALIDATION:
                typeStr = "SMART_CONTRACT_VALIDATION";
                break;
        }
        mechanismUsage[typeStr] = count;
    }
    stats["mechanismUsage"] = mechanismUsage;
    
    return stats;
}

nlohmann::json UnifiedTransactionValidator::getValidationMetrics() const {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    nlohmann::json metrics;
    metrics["initialized"] = initialized;
    metrics["feeStructure"] = feeStructure.toJson();
    metrics["priorityFactors"] = priorityFactors.toJson();
    metrics["cacheTimeout"] = cacheTimeout.count();
    
    return metrics;
}

void UnifiedTransactionValidator::resetStatistics() {
    std::lock_guard<std::mutex> lock(validatorMutex);
    
    totalValidations = 0;
    successfulValidations = 0;
    cachedValidations = 0;
    mechanismUsageCount.clear();
    
    Logger::info("UnifiedTransactionValidator statistics reset");
}

// Private methods implementation continues in next part...
// Private methods implementation

TransactionValidationResult UnifiedTransactionValidator::performValidation(
    const Transaction& transaction,
    const std::vector<ConsensusType>& mechanisms) {
    
    TransactionValidationResult result;
    result.mechanismResults.clear();
    
    // Basic validation first
    if (!validateTransactionBasics(transaction)) {
        result.isValid = false;
        result.confidence = 0.0;
        result.reason = "Basic validation failed";
        return result;
    }
    
    // Balance validation
    if (!validateTransactionBalance(transaction)) {
        result.isValid = false;
        result.confidence = 0.0;
        result.reason = "Insufficient balance";
        return result;
    }
    
    // Signature validation
    if (!validateTransactionSignature(transaction)) {
        result.isValid = false;
        result.confidence = 0.0;
        result.reason = "Invalid signature";
        return result;
    }
    
    // Consensus mechanism validation
    if (router) {
        try {
            ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, transaction.serialize());
            request.requiredMechanisms = mechanisms;
            
            ConsensusResult consensusResult = router->routeValidation(request);
            
            result.isValid = consensusResult.isValid;
            result.confidence = consensusResult.confidence;
            result.reason = consensusResult.reason;
            
            // Store individual mechanism results if available
            result.mechanismResults[consensusResult.mechanism] = consensusResult;
            
        } catch (const std::exception& e) {
            Logger::error("Consensus validation failed: " + std::string(e.what()));
            result.isValid = false;
            result.confidence = 0.0;
            result.reason = "Consensus validation failed: " + std::string(e.what());
        }
    } else {
        // Fallback validation without consensus router
        result.isValid = true;
        result.confidence = 0.8; // Lower confidence without full consensus validation
        result.reason = "Basic validation passed (no consensus router)";
    }
    
    return result;
}

bool UnifiedTransactionValidator::validateTransactionBasics(const Transaction& transaction) {
    // Check basic transaction validity
    if (!transaction.isValid()) {
        Logger::debug("Transaction failed basic validity check: " + transaction.getHash());
        return false;
    }
    
    // Check transaction amount
    if (transaction.getAmount() < 0) {
        Logger::debug("Transaction has negative amount: " + transaction.getHash());
        return false;
    }
    
    // Check sender and recipient
    if (transaction.getSender().empty()) {
        Logger::debug("Transaction has empty sender: " + transaction.getHash());
        return false;
    }
    
    // For non-coinbase transactions, recipient should not be empty (unless it's a contract deployment)
    if (transaction.getSender() != "COINBASE" && 
        transaction.getRecipient().empty() && 
        transaction.getContractCode().empty()) {
        Logger::debug("Transaction has empty recipient: " + transaction.getHash());
        return false;
    }
    
    return true;
}

bool UnifiedTransactionValidator::validateTransactionBalance(const Transaction& transaction) {
    // Skip balance check for coinbase transactions
    if (transaction.getSender() == "COINBASE") {
        return true;
    }
    
    // In a real implementation, this would check the actual blockchain state
    // For now, we'll assume the balance check is handled elsewhere
    return true;
}

bool UnifiedTransactionValidator::validateTransactionSignature(const Transaction& transaction) {
    // Skip signature check for coinbase transactions
    if (transaction.getSender() == "COINBASE") {
        return true;
    }
    
    // In a real implementation, this would verify the cryptographic signature
    // For now, we'll use the transaction's built-in validation
    return transaction.isValid();
}

double UnifiedTransactionValidator::calculateBaseFee(const Transaction& transaction) {
    double baseFee = feeStructure.baseFee;
    
    // Adjust base fee based on transaction amount
    if (transaction.getAmount() > 1000.0) {
        baseFee *= 1.1; // 10% increase for large transactions
    } else if (transaction.getAmount() < 1.0) {
        baseFee *= 0.9; // 10% decrease for small transactions
    }
    
    return baseFee;
}

double UnifiedTransactionValidator::calculateMechanismFees(const std::vector<ConsensusType>& mechanisms) {
    double totalMechanismFee = 0.0;
    
    for (ConsensusType mechanism : mechanisms) {
        switch (mechanism) {
            case ConsensusType::PROOF_OF_WORK:
                totalMechanismFee += feeStructure.powFee;
                break;
            case ConsensusType::PROOF_OF_STAKE:
                totalMechanismFee += feeStructure.posFee;
                break;
            case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
                totalMechanismFee += feeStructure.porcFee;
                break;
            case ConsensusType::VOTING_CONSENSUS:
                totalMechanismFee += feeStructure.votingFee;
                break;
            case ConsensusType::SMART_CONTRACT_VALIDATION:
                totalMechanismFee += feeStructure.smartContractFee;
                break;
        }
    }
    
    return totalMechanismFee;
}

double UnifiedTransactionValidator::calculateComplexityFee(const Transaction& transaction) {
    double complexityFee = 0.0;
    
    // Smart contract deployment has higher complexity
    if (!transaction.getContractCode().empty()) {
        complexityFee += feeStructure.smartContractFee * 2.0;
    }
    
    // Offline transactions have additional complexity
    if (transaction.getIsOffline()) {
        complexityFee += feeStructure.baseFee * 0.5;
    }
    
    return complexityFee;
}

double UnifiedTransactionValidator::calculateNetworkLoadMultiplier() {
    // In a real implementation, this would check actual network load
    // For now, return the configured multiplier
    return feeStructure.networkLoadMultiplier;
}

double UnifiedTransactionValidator::calculateFeeScore(const Transaction& transaction) {
    double fee = calculateTransactionFee(transaction);
    
    // Normalize fee score to 0-1 range
    // Higher fees get higher scores
    double maxExpectedFee = feeStructure.baseFee * 10.0; // Assume max fee is 10x base fee
    return std::min(fee / maxExpectedFee, 1.0);
}

double UnifiedTransactionValidator::calculateAgeScore(const Transaction& transaction) {
    auto now = std::chrono::system_clock::now();
    auto txTime = std::chrono::system_clock::from_time_t(transaction.getTimestamp());
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - txTime);
    
    // Older transactions get higher priority (up to a limit)
    double maxAge = 3600.0; // 1 hour
    double ageScore = std::min(age.count() / maxAge, 1.0);
    
    return ageScore;
}

double UnifiedTransactionValidator::calculateSenderReputationScore(const Transaction& transaction) {
    // Get sender reputation (simplified implementation)
    double reputation = getSenderReputation(transaction.getSender());
    
    // Normalize to 0-1 range
    return std::min(reputation / 100.0, 1.0);
}

double UnifiedTransactionValidator::calculateNetworkImportanceScore(const Transaction& transaction) {
    double importance = 0.5; // Default importance
    
    // Coinbase transactions are important
    if (transaction.getSender() == "COINBASE") {
        importance = 1.0;
    }
    
    // Smart contract deployments are important
    if (!transaction.getContractCode().empty()) {
        importance = 0.8;
    }
    
    // Large transactions are more important
    if (transaction.getAmount() > 1000.0) {
        importance = std::min(importance + 0.2, 1.0);
    }
    
    return importance;
}

double UnifiedTransactionValidator::calculateConsensusRequirementScore(const Transaction& transaction) {
    std::vector<ConsensusType> mechanisms = determineRequiredMechanisms(transaction);
    
    // More consensus mechanisms required = higher priority
    double maxMechanisms = 5.0; // Maximum possible mechanisms
    return mechanisms.size() / maxMechanisms;
}

bool UnifiedTransactionValidator::isCacheValid(const std::string& txHash) const {
    auto it = validationCache.find(txHash);
    if (it == validationCache.end()) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto cacheTime = std::chrono::steady_clock::time_point(
        std::chrono::seconds(it->second.timestamp));
    
    return (now - cacheTime) < cacheTimeout;
}

void UnifiedTransactionValidator::addToCache(const std::string& txHash, 
                                           const TransactionValidationResult& result) {
    validationCache[txHash] = result;
    
    // Cleanup expired entries periodically
    if (validationCache.size() % 100 == 0) {
        cleanupExpiredCache();
    }
}

TransactionValidationResult UnifiedTransactionValidator::getFromCache(const std::string& txHash) const {
    auto it = validationCache.find(txHash);
    if (it != validationCache.end()) {
        return it->second;
    }
    
    return TransactionValidationResult(false, 0.0, "Not found in cache");
}

std::vector<ConsensusType> UnifiedTransactionValidator::determineRequiredMechanisms(
    const Transaction& transaction) {
    
    std::vector<ConsensusType> mechanisms;
    
    // All transactions require basic PoW validation
    mechanisms.push_back(ConsensusType::PROOF_OF_WORK);
    
    // Large transactions also require PoS validation
    if (transaction.getAmount() > 100.0) {
        mechanisms.push_back(ConsensusType::PROOF_OF_STAKE);
    }
    
    // Smart contract transactions require smart contract validation
    if (!transaction.getContractCode().empty()) {
        mechanisms.push_back(ConsensusType::SMART_CONTRACT_VALIDATION);
    }
    
    // High-value transactions may require PoRC validation
    if (transaction.getAmount() > 1000.0) {
        mechanisms.push_back(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
    }
    
    return mechanisms;
}

std::string UnifiedTransactionValidator::getTransactionComplexityLevel(const Transaction& transaction) {
    if (!transaction.getContractCode().empty()) {
        return "HIGH";
    } else if (transaction.getIsOffline()) {
        return "MEDIUM";
    } else if (transaction.getAmount() > 1000.0) {
        return "MEDIUM";
    } else {
        return "LOW";
    }
}

double UnifiedTransactionValidator::getSenderReputation(const std::string& senderAddress) {
    // Simplified reputation system
    // In a real implementation, this would track sender history
    
    if (senderAddress == "COINBASE") {
        return 100.0; // Maximum reputation
    }
    
    // Default reputation for unknown senders
    return 50.0;
}

bool UnifiedTransactionValidator::isHighPriorityTransaction(const Transaction& transaction) {
    // Coinbase transactions are always high priority
    if (transaction.getSender() == "COINBASE") {
        return true;
    }
    
    // Large transactions are high priority
    if (transaction.getAmount() > 1000.0) {
        return true;
    }
    
    // Smart contract deployments are high priority
    if (!transaction.getContractCode().empty()) {
        return true;
    }
    
    return false;
}

void UnifiedTransactionValidator::updateValidationStatistics(const TransactionValidationResult& result) {
    // Update mechanism usage statistics
    for (const auto& [mechanism, mechanismResult] : result.mechanismResults) {
        mechanismUsageCount[mechanism]++;
    }
}

void UnifiedTransactionValidator::incrementMechanismUsage(const std::vector<ConsensusType>& mechanisms) {
    for (ConsensusType mechanism : mechanisms) {
        mechanismUsageCount[mechanism]++;
    }
}

bool UnifiedTransactionValidator::validateConfiguration() const {
    // Validate fee structure
    if (feeStructure.baseFee < 0 || feeStructure.powFee < 0 || feeStructure.posFee < 0 ||
        feeStructure.porcFee < 0 || feeStructure.votingFee < 0 || feeStructure.smartContractFee < 0) {
        return false;
    }
    
    if (feeStructure.networkLoadMultiplier <= 0 || feeStructure.priorityMultiplier <= 0 ||
        feeStructure.complexityMultiplier <= 0) {
        return false;
    }
    
    // Validate priority factors (should sum to approximately 1.0)
    double totalWeight = priorityFactors.feeWeight + priorityFactors.ageWeight +
                        priorityFactors.senderReputationWeight + priorityFactors.networkImportanceWeight +
                        priorityFactors.consensusRequirementWeight;
    
    if (totalWeight < 0.9 || totalWeight > 1.1) {
        Logger::warning("Priority factor weights sum to " + std::to_string(totalWeight) + 
                       " (should be close to 1.0)");
    }
    
    return true;
}

void UnifiedTransactionValidator::logValidationEvent(const std::string& event, 
                                                   const nlohmann::json& data) const {
    nlohmann::json logEntry;
    logEntry["event"] = event;
    logEntry["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    logEntry["data"] = data;
    
    Logger::info("UnifiedTransactionValidator: " + logEntry.dump());
}