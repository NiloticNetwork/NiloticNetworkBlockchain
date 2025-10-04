#include "../../include/core/enhanced_blockchain.h"
#include "../../include/core/logger.h"
#include <algorithm>
#include <numeric>
#include <chrono>

// TransactionProcessingConfig implementation
nlohmann::json EnhancedBlockchain::TransactionProcessingConfig::toJson() const {
    nlohmann::json j;
    j["maxTransactionsPerBlock"] = maxTransactionsPerBlock;
    j["maxTotalFeesPerBlock"] = maxTotalFeesPerBlock;
    j["enablePriorityProcessing"] = enablePriorityProcessing;
    j["enableParallelValidation"] = enableParallelValidation;
    j["enableFeeOptimization"] = enableFeeOptimization;
    return j;
}

void EnhancedBlockchain::TransactionProcessingConfig::fromJson(const nlohmann::json& j) {
    if (j.contains("maxTransactionsPerBlock")) maxTransactionsPerBlock = j["maxTransactionsPerBlock"];
    if (j.contains("maxTotalFeesPerBlock")) maxTotalFeesPerBlock = j["maxTotalFeesPerBlock"];
    if (j.contains("enablePriorityProcessing")) enablePriorityProcessing = j["enablePriorityProcessing"];
    if (j.contains("enableParallelValidation")) enableParallelValidation = j["enableParallelValidation"];
    if (j.contains("enableFeeOptimization")) enableFeeOptimization = j["enableFeeOptimization"];
}

// TransactionStats implementation
nlohmann::json EnhancedBlockchain::TransactionStats::toJson() const {
    nlohmann::json j;
    j["totalProcessed"] = totalProcessed;
    j["totalValidated"] = totalValidated;
    j["totalRejected"] = totalRejected;
    j["totalFeesCollected"] = totalFeesCollected;
    j["averageValidationTime"] = averageValidationTime;
    return j;
}

void EnhancedBlockchain::TransactionStats::reset() {
    totalProcessed = 0;
    totalValidated = 0;
    totalRejected = 0;
    totalFeesCollected = 0.0;
    averageValidationTime = 0;
}

// EnhancedBlockchain implementation
EnhancedBlockchain::EnhancedBlockchain() : Blockchain() {
    Logger::info("EnhancedBlockchain created");
}

EnhancedBlockchain::~EnhancedBlockchain() {
    shutdownEnhancedFeatures();
    Logger::info("EnhancedBlockchain destroyed");
}

bool EnhancedBlockchain::initializeEnhancedFeatures() {
    return initializeEnhancedFeatures(TransactionFeeStructure{}, TransactionPriorityFactors{});
}

bool EnhancedBlockchain::initializeEnhancedFeatures(const TransactionFeeStructure& feeConfig,
                                                   const TransactionPriorityFactors& priorityConfig) {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    try {
        Logger::info("Initializing EnhancedBlockchain features");
        
        if (!initializeComponents()) {
            Logger::error("Failed to initialize enhanced blockchain components");
            return false;
        }
        
        // Initialize transaction validator
        if (!transactionValidator->initialize(feeConfig, priorityConfig)) {
            Logger::error("Failed to initialize transaction validator");
            return false;
        }
        
        // Initialize harmony manager
        if (!harmonyManager->initializeConsensus()) {
            Logger::error("Failed to initialize consensus harmony manager");
            return false;
        }
        
        // Set up component connections
        transactionValidator->setHarmonyManager(harmonyManager.get());
        
        // Reset statistics
        transactionStats.reset();
        
        Logger::info("EnhancedBlockchain features initialized successfully");
        
        logEnhancedEvent("enhanced_blockchain_initialized", {
            {"feeStructure", feeConfig.toJson()},
            {"priorityFactors", priorityConfig.toJson()},
            {"processingConfig", processingConfig.toJson()}
        });
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize enhanced features: " + std::string(e.what()));
        return false;
    }
}

void EnhancedBlockchain::shutdownEnhancedFeatures() {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    Logger::info("Shutting down EnhancedBlockchain features");
    
    shutdownComponents();
    
    Logger::info("EnhancedBlockchain features shut down successfully");
    
    logEnhancedEvent("enhanced_blockchain_shutdown");
}

TransactionValidationResult EnhancedBlockchain::validateTransactionEnhanced(const Transaction& transaction) {
    if (!transactionValidator) {
        Logger::error("Transaction validator not initialized");
        return TransactionValidationResult(false, 0.0, "Validator not initialized");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    TransactionValidationResult result = transactionValidator->validateTransaction(transaction);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Update statistics
    transactionStats.totalProcessed++;
    if (result.isValid) {
        transactionStats.totalValidated++;
    } else {
        transactionStats.totalRejected++;
    }
    
    // Update average validation time
    transactionStats.averageValidationTime = 
        (transactionStats.averageValidationTime * (transactionStats.totalProcessed - 1) + duration.count()) / 
        transactionStats.totalProcessed;
    
    Logger::debug("Enhanced transaction validation completed: " + transaction.getHash() + 
                 " - Result: " + (result.isValid ? "VALID" : "INVALID") + 
                 " (time: " + std::to_string(duration.count()) + "μs)");
    
    return result;
}

std::vector<TransactionValidationResult> EnhancedBlockchain::validateTransactionBatch(
    const std::vector<Transaction>& transactions) {
    
    if (!transactionValidator) {
        Logger::error("Transaction validator not initialized");
        std::vector<TransactionValidationResult> results;
        for (size_t i = 0; i < transactions.size(); ++i) {
            results.emplace_back(false, 0.0, "Validator not initialized");
        }
        return results;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<TransactionValidationResult> results;
    
    if (processingConfig.enableParallelValidation && shouldUseParallelValidation(transactions.size())) {
        results = transactionValidator->validateTransactionsParallel(transactions);
    } else {
        results = transactionValidator->validateTransactions(transactions);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Update statistics
    updateTransactionStatistics(results);
    
    Logger::info("Enhanced batch validation completed: " + std::to_string(transactions.size()) + 
                " transactions in " + std::to_string(duration.count()) + "ms");
    
    return results;
}

bool EnhancedBlockchain::addTransactionEnhanced(const Transaction& transaction) {
    // Validate transaction using enhanced validator
    TransactionValidationResult result = validateTransactionEnhanced(transaction);
    
    if (!result.isValid) {
        Logger::warning("Transaction rejected by enhanced validation: " + transaction.getHash() + 
                       " - Reason: " + result.reason);
        return false;
    }
    
    // Add to pending transactions using base class method
    bool added = addTransaction(transaction);
    
    if (added) {
        transactionStats.totalFeesCollected += result.calculatedFee;
        Logger::info("Transaction added with enhanced validation: " + transaction.getHash() + 
                    " (fee: " + std::to_string(result.calculatedFee) + 
                    ", priority: " + std::to_string(result.priority) + ")");
    }
    
    return added;
}

bool EnhancedBlockchain::addTransactionBatchEnhanced(const std::vector<Transaction>& transactions) {
    if (transactions.empty()) {
        return true;
    }
    
    // Validate all transactions
    std::vector<TransactionValidationResult> results = validateTransactionBatch(transactions);
    
    // Add valid transactions
    size_t addedCount = 0;
    double totalFees = 0.0;
    
    for (size_t i = 0; i < transactions.size(); ++i) {
        if (results[i].isValid) {
            if (addTransaction(transactions[i])) {
                addedCount++;
                totalFees += results[i].calculatedFee;
            }
        }
    }
    
    transactionStats.totalFeesCollected += totalFees;
    
    Logger::info("Enhanced batch transaction addition completed: " + 
                std::to_string(addedCount) + "/" + std::to_string(transactions.size()) + 
                " transactions added (total fees: " + std::to_string(totalFees) + ")");
    
    return addedCount > 0;
}

Block EnhancedBlockchain::minePendingTransactionsEnhanced(const std::string& miningRewardAddress) {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    Logger::info("Mining block with enhanced transaction selection");
    
    // Get optimal transactions for the block
    std::vector<Transaction> selectedTransactions = selectOptimalTransactionsForBlock();
    
    // Create new block
    uint64_t newIndex = getLatestBlock().getIndex() + 1;
    Block newBlock(newIndex, getLatestBlock().getHash());
    
    // Calculate total fees
    double totalFees = 0.0;
    for (const Transaction& tx : selectedTransactions) {
        if (transactionValidator) {
            totalFees += transactionValidator->calculateTransactionFee(tx);
        }
    }
    
    // Create coinbase transaction with mining reward + fees
    double totalReward = getMiningReward() + totalFees;
    Transaction coinbaseTx("COINBASE", miningRewardAddress, totalReward);
    newBlock.addTransaction(coinbaseTx);
    
    // Add selected transactions
    for (const Transaction& tx : selectedTransactions) {
        newBlock.addTransaction(tx);
    }
    
    // Mine the block
    Logger::info("Mining block " + std::to_string(newIndex) + " with " + 
                std::to_string(selectedTransactions.size()) + " transactions (total fees: " + 
                std::to_string(totalFees) + ")");
    
    newBlock.mineBlock(getDifficulty());
    
    // Add block to chain using base class method
    if (addBlock(newBlock)) {
        // Remove processed transactions from pending pool
        auto pendingTxs = getPendingTransactions();
        std::deque<Transaction> remainingTxs;
        
        for (const Transaction& pendingTx : pendingTxs) {
            bool found = false;
            for (const Transaction& selectedTx : selectedTransactions) {
                if (pendingTx.getHash() == selectedTx.getHash()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                remainingTxs.push_back(pendingTx);
            }
        }
        
        // Update pending transactions (this would need to be implemented in base class)
        // For now, we'll log the operation
        Logger::info("Block mined successfully with enhanced features: " + newBlock.getHash());
        
        logEnhancedEvent("enhanced_block_mined", {
            {"blockHash", newBlock.getHash()},
            {"blockIndex", newIndex},
            {"transactionCount", selectedTransactions.size()},
            {"totalFees", totalFees},
            {"totalReward", totalReward}
        });
    }
    
    return newBlock;
}

Block EnhancedBlockchain::minePendingTransactionsWithConsensus(
    const std::string& miningRewardAddress,
    const std::vector<ConsensusType>& mechanisms) {
    
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    Logger::info("Mining block with enhanced transaction selection and consensus mechanisms");
    
    // Get transactions that require specific consensus mechanisms
    std::vector<Transaction> candidateTransactions;
    auto pendingTxs = getPendingTransactions();
    
    for (const Transaction& tx : pendingTxs) {
        if (transactionValidator) {
            TransactionValidationResult result = transactionValidator->validateTransactionWithMechanisms(tx, mechanisms);
            if (result.isValid) {
                candidateTransactions.push_back(tx);
            }
        }
    }
    
    // Select optimal transactions
    std::vector<Transaction> selectedTransactions;
    if (transactionValidator) {
        selectedTransactions = transactionValidator->selectTransactionsForBlock(
            candidateTransactions, 
            processingConfig.maxTransactionsPerBlock,
            processingConfig.maxTotalFeesPerBlock
        );
    } else {
        selectedTransactions = candidateTransactions;
    }
    
    // Create and mine block
    uint64_t newIndex = getLatestBlock().getIndex() + 1;
    Block newBlock(newIndex, getLatestBlock().getHash());
    
    // Calculate total fees
    double totalFees = 0.0;
    for (const Transaction& tx : selectedTransactions) {
        if (transactionValidator) {
            totalFees += transactionValidator->calculateTransactionFeeWithMechanisms(tx, mechanisms);
        }
    }
    
    // Create coinbase transaction
    double totalReward = getMiningReward() + totalFees;
    Transaction coinbaseTx("COINBASE", miningRewardAddress, totalReward);
    newBlock.addTransaction(coinbaseTx);
    
    // Add selected transactions
    for (const Transaction& tx : selectedTransactions) {
        newBlock.addTransaction(tx);
    }
    
    // Mine the block
    newBlock.mineBlock(getDifficulty());
    
    // Validate with multiple consensus mechanisms
    if (validateBlockWithUnifiedConsensus(newBlock, mechanisms)) {
        addBlock(newBlock);
        
        Logger::info("Block mined successfully with consensus mechanisms: " + newBlock.getHash());
        
        logEnhancedEvent("consensus_block_mined", {
            {"blockHash", newBlock.getHash()},
            {"blockIndex", newIndex},
            {"transactionCount", selectedTransactions.size()},
            {"totalFees", totalFees},
            {"consensusMechanisms", mechanisms.size()}
        });
    } else {
        Logger::error("Block failed consensus validation");
    }
    
    return newBlock;
}

double EnhancedBlockchain::calculateTransactionFeeEnhanced(const Transaction& transaction) {
    if (!transactionValidator) {
        Logger::warning("Transaction validator not available, using default fee");
        return 0.001; // Default fee
    }
    
    return transactionValidator->calculateTransactionFee(transaction);
}

double EnhancedBlockchain::calculateOptimalFeeForPriority(const Transaction& transaction, uint32_t targetPriority) {
    if (!transactionValidator) {
        return 0.001; // Default fee
    }
    
    // Get current fee structure
    TransactionFeeStructure feeStructure = transactionValidator->getFeeStructure();
    
    // Calculate current priority
    uint32_t currentPriority = transactionValidator->calculateTransactionPriority(transaction);
    
    if (currentPriority >= targetPriority) {
        return transactionValidator->calculateTransactionFee(transaction);
    }
    
    // Estimate fee multiplier needed to reach target priority
    double priorityRatio = static_cast<double>(targetPriority) / std::max(currentPriority, 1u);
    double feeMultiplier = std::sqrt(priorityRatio); // Square root relationship
    
    // Apply multiplier to current fee structure
    TransactionFeeStructure adjustedFees = feeStructure;
    adjustedFees.priorityMultiplier *= feeMultiplier;
    
    // Calculate fee with adjusted structure
    return transactionValidator->calculateTransactionFee(transaction) * feeMultiplier;
}

std::vector<Transaction> EnhancedBlockchain::selectOptimalTransactionsForBlock() {
    if (!transactionValidator) {
        // Fallback to simple selection
        auto pendingTxs = getPendingTransactions();
        std::vector<Transaction> transactions(pendingTxs.begin(), pendingTxs.end());
        
        size_t maxCount = std::min(transactions.size(), processingConfig.maxTransactionsPerBlock);
        transactions.resize(maxCount);
        
        return transactions;
    }
    
    // Get all pending transactions
    auto pendingTxs = getPendingTransactions();
    std::vector<Transaction> candidateTransactions(pendingTxs.begin(), pendingTxs.end());
    
    // Use transaction validator for optimal selection
    return transactionValidator->selectTransactionsForBlock(
        candidateTransactions,
        processingConfig.maxTransactionsPerBlock,
        processingConfig.enableFeeOptimization ? processingConfig.maxTotalFeesPerBlock : 0.0
    );
}

std::vector<Transaction> EnhancedBlockchain::selectTransactionsWithConstraints(size_t maxCount, double maxFees) {
    if (!transactionValidator) {
        auto pendingTxs = getPendingTransactions();
        std::vector<Transaction> transactions(pendingTxs.begin(), pendingTxs.end());
        
        size_t count = std::min(transactions.size(), maxCount);
        transactions.resize(count);
        
        return transactions;
    }
    
    auto pendingTxs = getPendingTransactions();
    std::vector<Transaction> candidateTransactions(pendingTxs.begin(), pendingTxs.end());
    
    return transactionValidator->selectTransactionsForBlock(candidateTransactions, maxCount, maxFees);
}

bool EnhancedBlockchain::validateBlockEnhanced(const Block& block) {
    if (!transactionValidator || !harmonyManager) {
        Logger::warning("Enhanced validation components not available, using base validation");
        return isChainValid(); // Fallback to base validation
    }
    
    // Validate all transactions in the block
    std::vector<Transaction> transactions = block.getTransactions();
    std::vector<TransactionValidationResult> results = validateTransactionBatch(transactions);
    
    // Check if all transactions are valid
    for (const TransactionValidationResult& result : results) {
        if (!result.isValid) {
            Logger::error("Block validation failed: Invalid transaction found - " + result.reason);
            return false;
        }
    }
    
    // Validate block through harmony manager
    if (!harmonyManager->validateBlock(block)) {
        Logger::error("Block validation failed: Consensus harmony validation failed");
        return false;
    }
    
    Logger::info("Block validation passed with enhanced features: " + block.getHash());
    return true;
}

bool EnhancedBlockchain::validateBlockWithUnifiedConsensus(const Block& block,
                                                          const std::vector<ConsensusType>& mechanisms) {
    if (!harmonyManager) {
        Logger::warning("Harmony manager not available, using base validation");
        return validateBlockEnhanced(block);
    }
    
    // Create consensus request for block validation
    ConsensusRequest request(RequestType::BLOCK_VALIDATION, block.serialize());
    request.requiredMechanisms = mechanisms;
    
    // Process through harmony manager
    ConsensusResult result = harmonyManager->processConsensusRequest(request);
    
    if (!result.isValid) {
        Logger::error("Block validation failed with unified consensus: " + result.reason);
        return false;
    }
    
    Logger::info("Block validation passed with unified consensus: " + block.getHash() + 
                " (confidence: " + std::to_string(result.confidence) + ")");
    
    return true;
}

// Configuration and statistics methods
bool EnhancedBlockchain::updateProcessingConfig(const TransactionProcessingConfig& newConfig) {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    if (!validateProcessingConfig(newConfig)) {
        Logger::error("Invalid processing configuration provided");
        return false;
    }
    
    processingConfig = newConfig;
    
    Logger::info("Processing configuration updated");
    logEnhancedEvent("processing_config_updated", newConfig.toJson());
    
    return true;
}

EnhancedBlockchain::TransactionProcessingConfig EnhancedBlockchain::getProcessingConfig() const {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    return processingConfig;
}

nlohmann::json EnhancedBlockchain::getEnhancedStatistics() const {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    nlohmann::json stats;
    stats["transactionStats"] = transactionStats.toJson();
    stats["processingConfig"] = processingConfig.toJson();
    
    if (transactionValidator) {
        stats["validatorStats"] = transactionValidator->getStatistics();
    }
    
    if (harmonyManager) {
        stats["harmonyStats"] = harmonyManager->getMetrics();
    }
    
    return stats;
}

nlohmann::json EnhancedBlockchain::getTransactionValidationMetrics() const {
    if (transactionValidator) {
        return transactionValidator->getValidationMetrics();
    }
    
    return nlohmann::json::object();
}

nlohmann::json EnhancedBlockchain::getConsensusHarmonyStatus() const {
    if (harmonyManager) {
        return harmonyManager->getDetailedStatus();
    }
    
    return nlohmann::json::object();
}

// Private methods implementation continues in next part...
// Private methods implementation

bool EnhancedBlockchain::initializeComponents() {
    try {
        // Initialize harmony manager
        harmonyManager = std::make_unique<ConsensusHarmonyManager>(this);
        
        // Initialize transaction validator
        transactionValidator = std::make_unique<UnifiedTransactionValidator>(harmonyManager.get());
        
        Logger::info("Enhanced blockchain components initialized");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize enhanced blockchain components: " + std::string(e.what()));
        return false;
    }
}

void EnhancedBlockchain::shutdownComponents() {
    try {
        if (transactionValidator) {
            transactionValidator->shutdown();
            transactionValidator.reset();
        }
        
        if (harmonyManager) {
            harmonyManager->shutdown();
            harmonyManager.reset();
        }
        
        Logger::info("Enhanced blockchain components shut down");
        
    } catch (const std::exception& e) {
        Logger::error("Error during enhanced blockchain component shutdown: " + std::string(e.what()));
    }
}

std::vector<Transaction> EnhancedBlockchain::filterAndValidateTransactions(
    const std::vector<Transaction>& transactions) {
    
    if (!transactionValidator) {
        return transactions; // Return all if validator not available
    }
    
    return transactionValidator->filterValidTransactions(transactions);
}

std::vector<Transaction> EnhancedBlockchain::selectTransactionsForBlock(
    const std::vector<Transaction>& candidates) {
    
    if (!transactionValidator) {
        // Simple selection without validator
        size_t maxCount = std::min(candidates.size(), processingConfig.maxTransactionsPerBlock);
        std::vector<Transaction> selected(candidates.begin(), candidates.begin() + maxCount);
        return selected;
    }
    
    return transactionValidator->selectTransactionsForBlock(
        candidates,
        processingConfig.maxTransactionsPerBlock,
        processingConfig.maxTotalFeesPerBlock
    );
}

void EnhancedBlockchain::updateTransactionStatistics(
    const std::vector<TransactionValidationResult>& results) {
    
    for (const TransactionValidationResult& result : results) {
        transactionStats.totalProcessed++;
        
        if (result.isValid) {
            transactionStats.totalValidated++;
            transactionStats.totalFeesCollected += result.calculatedFee;
        } else {
            transactionStats.totalRejected++;
        }
    }
}

double EnhancedBlockchain::calculateBlockRewardWithFees(const std::vector<Transaction>& transactions) {
    double totalFees = 0.0;
    
    if (transactionValidator) {
        for (const Transaction& tx : transactions) {
            totalFees += transactionValidator->calculateTransactionFee(tx);
        }
    }
    
    return getMiningReward() + totalFees;
}

void EnhancedBlockchain::distributeFees(const std::vector<Transaction>& transactions,
                                       const std::string& minerAddress) {
    // In a real implementation, this would distribute fees according to consensus rules
    // For now, all fees go to the miner (handled in coinbase transaction)
    
    double totalFees = 0.0;
    if (transactionValidator) {
        for (const Transaction& tx : transactions) {
            totalFees += transactionValidator->calculateTransactionFee(tx);
        }
    }
    
    Logger::info("Distributed " + std::to_string(totalFees) + " in fees to miner: " + minerAddress);
}

bool EnhancedBlockchain::shouldUseParallelValidation(size_t transactionCount) const {
    // Use parallel validation for batches larger than 10 transactions
    return processingConfig.enableParallelValidation && transactionCount > 10;
}

bool EnhancedBlockchain::validateProcessingConfig(const TransactionProcessingConfig& config) const {
    if (config.maxTransactionsPerBlock == 0 || config.maxTransactionsPerBlock > 10000) {
        return false;
    }
    
    if (config.maxTotalFeesPerBlock < 0) {
        return false;
    }
    
    return true;
}

void EnhancedBlockchain::logEnhancedEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logEntry;
    logEntry["event"] = event;
    logEntry["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    logEntry["data"] = data;
    
    Logger::info("EnhancedBlockchain: " + logEntry.dump());
}

// Additional utility methods

void EnhancedBlockchain::cleanupInvalidTransactions() {
    if (!transactionValidator) {
        return;
    }
    
    auto pendingTxs = getPendingTransactions();
    std::vector<Transaction> validTransactions;
    
    for (const Transaction& tx : pendingTxs) {
        TransactionValidationResult result = transactionValidator->validateTransaction(tx);
        if (result.isValid) {
            validTransactions.push_back(tx);
        }
    }
    
    Logger::info("Cleaned up invalid transactions: " + 
                std::to_string(pendingTxs.size() - validTransactions.size()) + 
                " removed, " + std::to_string(validTransactions.size()) + " remaining");
}

void EnhancedBlockchain::revalidatePendingTransactions() {
    if (!transactionValidator) {
        return;
    }
    
    auto pendingTxs = getPendingTransactions();
    std::vector<Transaction> transactions(pendingTxs.begin(), pendingTxs.end());
    
    // Clear validation cache to force revalidation
    transactionValidator->clearValidationCache();
    
    // Revalidate all transactions
    std::vector<TransactionValidationResult> results = validateTransactionBatch(transactions);
    
    size_t validCount = 0;
    for (const TransactionValidationResult& result : results) {
        if (result.isValid) {
            validCount++;
        }
    }
    
    Logger::info("Revalidated " + std::to_string(transactions.size()) + 
                " pending transactions: " + std::to_string(validCount) + " valid");
}

size_t EnhancedBlockchain::getPendingTransactionCount() const {
    return getPendingTransactions().size();
}

std::vector<Transaction> EnhancedBlockchain::getPendingTransactionsByPriority() const {
    if (!transactionValidator) {
        auto pendingTxs = getPendingTransactions();
        return std::vector<Transaction>(pendingTxs.begin(), pendingTxs.end());
    }
    
    auto pendingTxs = getPendingTransactions();
    std::vector<Transaction> transactions(pendingTxs.begin(), pendingTxs.end());
    
    return transactionValidator->sortTransactionsByPriority(transactions);
}

double EnhancedBlockchain::estimateTransactionFee(const Transaction& transaction) const {
    if (!transactionValidator) {
        return 0.001; // Default estimate
    }
    
    return transactionValidator->calculateTransactionFee(transaction);
}

double EnhancedBlockchain::predictOptimalFee(double targetConfirmationTime) const {
    // Simplified fee prediction based on network load
    double baseOptimalFee = 0.002;
    double networkLoad = getCurrentNetworkLoad();
    
    // Higher network load requires higher fees for faster confirmation
    double loadMultiplier = 1.0 + (networkLoad * 2.0);
    
    // Time factor: shorter target time requires higher fees
    double timeMultiplier = std::max(1.0, 60.0 / std::max(targetConfirmationTime, 1.0));
    
    return baseOptimalFee * loadMultiplier * timeMultiplier;
}

std::vector<double> EnhancedBlockchain::getFeeHistogram() const {
    std::vector<double> histogram;
    
    if (!transactionValidator) {
        return histogram;
    }
    
    auto pendingTxs = getPendingTransactions();
    
    for (const Transaction& tx : pendingTxs) {
        double fee = transactionValidator->calculateTransactionFee(tx);
        histogram.push_back(fee);
    }
    
    // Sort fees for histogram
    std::sort(histogram.begin(), histogram.end());
    
    return histogram;
}

double EnhancedBlockchain::getCurrentNetworkLoad() const {
    size_t pendingCount = getPendingTransactionCount();
    size_t maxCapacity = processingConfig.maxTransactionsPerBlock * 10; // Assume 10 blocks worth
    
    return static_cast<double>(pendingCount) / std::max(maxCapacity, 1ul);
}

void EnhancedBlockchain::adjustProcessingParameters() {
    double networkLoad = getCurrentNetworkLoad();
    
    // Adjust processing parameters based on network load
    if (networkLoad > 0.8) {
        // High load: enable all optimizations
        processingConfig.enableParallelValidation = true;
        processingConfig.enableFeeOptimization = true;
        processingConfig.enablePriorityProcessing = true;
    } else if (networkLoad < 0.2) {
        // Low load: can afford to be less aggressive
        processingConfig.enableParallelValidation = false;
    }
    
    Logger::debug("Adjusted processing parameters for network load: " + std::to_string(networkLoad));
}

bool EnhancedBlockchain::isNetworkCongested() const {
    return getCurrentNetworkLoad() > 0.7;
}

double EnhancedBlockchain::getTransactionFeeBalance(const std::string& address) const {
    // In a real implementation, this would track fees earned by validators/miners
    // For now, return 0 as fees are handled through coinbase transactions
    return 0.0;
}

double EnhancedBlockchain::getTotalFeesCollected() const {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    return transactionStats.totalFeesCollected;
}

std::map<std::string, double> EnhancedBlockchain::getFeeDistribution() const {
    std::map<std::string, double> distribution;
    
    // In a real implementation, this would track fee distribution among validators
    // For now, return empty map
    
    return distribution;
}

bool EnhancedBlockchain::updateFeeStructure(const TransactionFeeStructure& newStructure) {
    if (!transactionValidator) {
        Logger::error("Transaction validator not available");
        return false;
    }
    
    return transactionValidator->updateFeeStructure(newStructure);
}

bool EnhancedBlockchain::updatePriorityFactors(const TransactionPriorityFactors& newFactors) {
    if (!transactionValidator) {
        Logger::error("Transaction validator not available");
        return false;
    }
    
    return transactionValidator->updatePriorityFactors(newFactors);
}

void EnhancedBlockchain::resetEnhancedStatistics() {
    std::lock_guard<std::mutex> lock(enhancedMutex);
    
    transactionStats.reset();
    
    if (transactionValidator) {
        transactionValidator->resetStatistics();
    }
    
    Logger::info("Enhanced blockchain statistics reset");
}

std::vector<Transaction> EnhancedBlockchain::optimizeTransactionFees(
    const std::vector<Transaction>& transactions) {
    
    if (!transactionValidator || !processingConfig.enableFeeOptimization) {
        return transactions;
    }
    
    // Sort transactions by fee efficiency (fee per byte or complexity)
    std::vector<std::pair<Transaction, double>> txWithEfficiency;
    
    for (const Transaction& tx : transactions) {
        double fee = transactionValidator->calculateTransactionFee(tx);
        double efficiency = fee / std::max(1.0, static_cast<double>(tx.serialize().length()));
        txWithEfficiency.emplace_back(tx, efficiency);
    }
    
    // Sort by efficiency (higher efficiency first)
    std::sort(txWithEfficiency.begin(), txWithEfficiency.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    // Extract optimized transaction order
    std::vector<Transaction> optimizedTransactions;
    for (const auto& pair : txWithEfficiency) {
        optimizedTransactions.push_back(pair.first);
    }
    
    Logger::debug("Optimized transaction order by fee efficiency");
    
    return optimizedTransactions;
}

std::vector<Transaction> EnhancedBlockchain::reorderTransactionsByPriority(
    const std::vector<Transaction>& transactions) {
    
    if (!transactionValidator || !processingConfig.enablePriorityProcessing) {
        return transactions;
    }
    
    return transactionValidator->sortTransactionsByPriority(transactions);
}

// Enhanced serialization methods
std::string EnhancedBlockchain::serializeEnhanced() const {
    nlohmann::json enhancedData;
    
    // Include base blockchain data (would need to be implemented in base class)
    // enhancedData["baseBlockchain"] = serializeBase();
    
    // Add enhanced features data
    enhancedData["processingConfig"] = processingConfig.toJson();
    enhancedData["transactionStats"] = transactionStats.toJson();
    
    if (transactionValidator) {
        enhancedData["validatorMetrics"] = transactionValidator->getValidationMetrics();
    }
    
    if (harmonyManager) {
        enhancedData["harmonyStatus"] = harmonyManager->getDetailedStatus();
    }
    
    return enhancedData.dump(4);
}

bool EnhancedBlockchain::saveEnhancedToFile(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for enhanced save: " + filename);
            return false;
        }
        
        file << serializeEnhanced();
        file.close();
        
        Logger::info("Enhanced blockchain saved to file: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to save enhanced blockchain: " + std::string(e.what()));
        return false;
    }
}

void EnhancedBlockchain::updatePerformanceMetrics() {
    // Update various performance metrics
    if (transactionValidator) {
        transactionValidator->cleanupExpiredCache();
    }
    
    adjustProcessingParameters();
    
    Logger::debug("Performance metrics updated");
}