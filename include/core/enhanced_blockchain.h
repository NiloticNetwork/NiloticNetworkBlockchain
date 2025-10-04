#ifndef ENHANCED_BLOCKCHAIN_H
#define ENHANCED_BLOCKCHAIN_H

#include "blockchain.h"
#include "unified_transaction_validator.h"
#include "consensus_harmony_manager.h"
#include <memory>

/**
 * Enhanced Blockchain with Unified Transaction Validation
 * Extends the base Blockchain class to integrate with the unified transaction validator
 * and consensus harmony system for improved transaction processing
 */
class EnhancedBlockchain : public Blockchain {
private:
    // Unified transaction validation system
    std::unique_ptr<UnifiedTransactionValidator> transactionValidator;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
    
    // Enhanced transaction processing configuration
    struct TransactionProcessingConfig {
        size_t maxTransactionsPerBlock = 1000;
        double maxTotalFeesPerBlock = 10.0;
        bool enablePriorityProcessing = true;
        bool enableParallelValidation = true;
        bool enableFeeOptimization = true;
        
        nlohmann::json toJson() const;
        void fromJson(const nlohmann::json& j);
    } processingConfig;
    
    // Transaction processing statistics
    struct TransactionStats {
        uint64_t totalProcessed = 0;
        uint64_t totalValidated = 0;
        uint64_t totalRejected = 0;
        double totalFeesCollected = 0.0;
        uint64_t averageValidationTime = 0;
        
        nlohmann::json toJson() const;
        void reset();
    } transactionStats;
    
    // Thread safety for enhanced features
    mutable std::mutex enhancedMutex;

public:
    // Constructor
    EnhancedBlockchain();
    ~EnhancedBlockchain();
    
    // Initialization
    bool initializeEnhancedFeatures();
    bool initializeEnhancedFeatures(const TransactionFeeStructure& feeConfig,
                                   const TransactionPriorityFactors& priorityConfig);
    void shutdownEnhancedFeatures();
    
    // Enhanced transaction validation
    TransactionValidationResult validateTransactionEnhanced(const Transaction& transaction);
    std::vector<TransactionValidationResult> validateTransactionBatch(
        const std::vector<Transaction>& transactions);
    
    // Enhanced transaction addition with validation
    bool addTransactionEnhanced(const Transaction& transaction);
    bool addTransactionBatchEnhanced(const std::vector<Transaction>& transactions);
    
    // Enhanced block mining with optimized transaction selection
    Block minePendingTransactionsEnhanced(const std::string& miningRewardAddress);
    Block minePendingTransactionsWithConsensus(const std::string& miningRewardAddress,
                                              const std::vector<ConsensusType>& mechanisms);
    
    // Transaction fee calculation and optimization
    double calculateTransactionFeeEnhanced(const Transaction& transaction);
    double calculateOptimalFeeForPriority(const Transaction& transaction, uint32_t targetPriority);
    std::vector<Transaction> optimizeTransactionFees(const std::vector<Transaction>& transactions);
    
    // Transaction prioritization and selection
    std::vector<Transaction> selectOptimalTransactionsForBlock();
    std::vector<Transaction> selectTransactionsWithConstraints(size_t maxCount, double maxFees);
    std::vector<Transaction> reorderTransactionsByPriority(const std::vector<Transaction>& transactions);
    
    // Enhanced balance and fee management
    double getTransactionFeeBalance(const std::string& address) const;
    double getTotalFeesCollected() const;
    std::map<std::string, double> getFeeDistribution() const;
    
    // Configuration management
    bool updateProcessingConfig(const TransactionProcessingConfig& newConfig);
    TransactionProcessingConfig getProcessingConfig() const;
    bool updateFeeStructure(const TransactionFeeStructure& newStructure);
    bool updatePriorityFactors(const TransactionPriorityFactors& newFactors);
    
    // Statistics and monitoring
    nlohmann::json getEnhancedStatistics() const;
    nlohmann::json getTransactionValidationMetrics() const;
    nlohmann::json getConsensusHarmonyStatus() const;
    void resetEnhancedStatistics();
    
    // Consensus harmony integration
    ConsensusHarmonyManager* getHarmonyManager() const { return harmonyManager.get(); }
    UnifiedTransactionValidator* getTransactionValidator() const { return transactionValidator.get(); }
    
    // Enhanced validation with multiple consensus mechanisms
    bool validateBlockEnhanced(const Block& block);
    bool validateBlockWithUnifiedConsensus(const Block& block, 
                                          const std::vector<ConsensusType>& mechanisms);
    
    // Transaction pool management
    void cleanupInvalidTransactions();
    void revalidatePendingTransactions();
    size_t getPendingTransactionCount() const;
    std::vector<Transaction> getPendingTransactionsByPriority() const;
    
    // Fee estimation and prediction
    double estimateTransactionFee(const Transaction& transaction) const;
    double predictOptimalFee(double targetConfirmationTime) const;
    std::vector<double> getFeeHistogram() const;
    
    // Network load and performance optimization
    double getCurrentNetworkLoad() const;
    void adjustProcessingParameters();
    bool isNetworkCongested() const;
    
    // Enhanced serialization with validation data
    std::string serializeEnhanced() const;
    bool deserializeEnhanced(const std::string& data);
    bool saveEnhancedToFile(const std::string& filename) const;
    bool loadEnhancedFromFile(const std::string& filename);

private:
    // Internal enhanced processing methods
    bool initializeComponents();
    void shutdownComponents();
    
    // Transaction processing helpers
    std::vector<Transaction> filterAndValidateTransactions(const std::vector<Transaction>& transactions);
    std::vector<Transaction> selectTransactionsForBlock(const std::vector<Transaction>& candidates);
    void updateTransactionStatistics(const std::vector<TransactionValidationResult>& results);
    
    // Fee calculation helpers
    double calculateBlockRewardWithFees(const std::vector<Transaction>& transactions);
    void distributeFees(const std::vector<Transaction>& transactions, const std::string& minerAddress);
    
    // Performance optimization helpers
    void optimizeValidationCache();
    void adjustValidationParameters();
    bool shouldUseParallelValidation(size_t transactionCount) const;
    
    // Configuration validation
    bool validateProcessingConfig(const TransactionProcessingConfig& config) const;
    
    // Logging and monitoring helpers
    void logEnhancedEvent(const std::string& event, const nlohmann::json& data = {}) const;
    void updatePerformanceMetrics();
};

#endif // ENHANCED_BLOCKCHAIN_H