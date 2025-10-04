#ifndef UNIFIED_TRANSACTION_VALIDATOR_H
#define UNIFIED_TRANSACTION_VALIDATOR_H

#include "consensus_harmony.h"
#include "transaction.h"
#include "block.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>

// Forward declarations
class ConsensusHarmonyManager;
class ConsensusRouter;

/**
 * Transaction validation result with detailed information
 */
struct TransactionValidationResult {
    bool isValid;
    double confidence;
    std::string reason;
    std::map<ConsensusType, ConsensusResult> mechanismResults;
    double calculatedFee;
    uint32_t priority;
    uint64_t timestamp;
    
    TransactionValidationResult() 
        : isValid(false), confidence(0.0), calculatedFee(0.0), priority(0), timestamp(0) {}
    
    TransactionValidationResult(bool valid, double conf, const std::string& r, double fee = 0.0)
        : isValid(valid), confidence(conf), reason(r), calculatedFee(fee), priority(0),
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

/**
 * Transaction fee structure for different consensus mechanisms
 */
struct TransactionFeeStructure {
    double baseFee = 0.001;                    // Base transaction fee
    double powFee = 0.0005;                    // Additional fee for PoW validation
    double posFee = 0.0003;                    // Additional fee for PoS validation
    double porcFee = 0.0002;                   // Additional fee for PoRC validation
    double votingFee = 0.001;                  // Additional fee for voting consensus
    double smartContractFee = 0.002;           // Additional fee for smart contract validation
    
    // Dynamic fee multipliers
    double networkLoadMultiplier = 1.0;       // Multiplier based on network load
    double priorityMultiplier = 1.0;           // Multiplier for priority transactions
    double complexityMultiplier = 1.0;         // Multiplier based on transaction complexity
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

/**
 * Transaction priority factors
 */
struct TransactionPriorityFactors {
    double feeWeight = 0.4;                    // Weight of fee in priority calculation
    double ageWeight = 0.2;                    // Weight of transaction age
    double senderReputationWeight = 0.2;       // Weight of sender reputation
    double networkImportanceWeight = 0.1;      // Weight of network importance
    double consensusRequirementWeight = 0.1;   // Weight of consensus requirements
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

/**
 * Unified Transaction Validator
 * Provides unified validation, fee calculation, and prioritization across all consensus mechanisms
 */
class UnifiedTransactionValidator {
private:
    // Core components
    ConsensusHarmonyManager* harmonyManager;
    ConsensusRouter* router;
    
    // Configuration
    TransactionFeeStructure feeStructure;
    TransactionPriorityFactors priorityFactors;
    
    // Validation cache
    std::map<std::string, TransactionValidationResult> validationCache;
    std::chrono::seconds cacheTimeout{300}; // 5 minutes
    
    // Statistics
    uint64_t totalValidations;
    uint64_t successfulValidations;
    uint64_t cachedValidations;
    std::map<ConsensusType, uint64_t> mechanismUsageCount;
    
    // Thread safety
    mutable std::mutex validatorMutex;
    
    // Initialization state
    bool initialized;

public:
    explicit UnifiedTransactionValidator(ConsensusHarmonyManager* manager = nullptr);
    ~UnifiedTransactionValidator();
    
    // Lifecycle management
    bool initialize();
    bool initialize(const TransactionFeeStructure& feeConfig, 
                   const TransactionPriorityFactors& priorityConfig);
    void shutdown();
    bool isInitialized() const { return initialized; }
    
    // Core validation methods
    TransactionValidationResult validateTransaction(const Transaction& transaction);
    TransactionValidationResult validateTransactionWithMechanisms(
        const Transaction& transaction, 
        const std::vector<ConsensusType>& requiredMechanisms);
    
    // Batch validation
    std::vector<TransactionValidationResult> validateTransactions(
        const std::vector<Transaction>& transactions);
    std::vector<TransactionValidationResult> validateTransactionsParallel(
        const std::vector<Transaction>& transactions);
    
    // Fee calculation methods
    double calculateTransactionFee(const Transaction& transaction);
    double calculateTransactionFeeWithMechanisms(
        const Transaction& transaction,
        const std::vector<ConsensusType>& mechanisms);
    TransactionFeeStructure calculateDynamicFeeStructure();
    
    // Priority calculation methods
    uint32_t calculateTransactionPriority(const Transaction& transaction);
    uint32_t calculateTransactionPriorityWithFactors(
        const Transaction& transaction,
        const TransactionPriorityFactors& factors);
    
    // Transaction sorting and filtering
    std::vector<Transaction> sortTransactionsByPriority(
        const std::vector<Transaction>& transactions);
    std::vector<Transaction> filterValidTransactions(
        const std::vector<Transaction>& transactions);
    std::vector<Transaction> selectTransactionsForBlock(
        const std::vector<Transaction>& transactions,
        size_t maxTransactions,
        double maxTotalFees = 0.0);
    
    // Configuration management
    bool updateFeeStructure(const TransactionFeeStructure& newStructure);
    bool updatePriorityFactors(const TransactionPriorityFactors& newFactors);
    TransactionFeeStructure getFeeStructure() const;
    TransactionPriorityFactors getPriorityFactors() const;
    
    // Cache management
    void clearValidationCache();
    void cleanupExpiredCache();
    size_t getCacheSize() const;
    
    // Statistics and monitoring
    nlohmann::json getStatistics() const;
    nlohmann::json getValidationMetrics() const;
    void resetStatistics();
    
    // Component integration
    void setHarmonyManager(ConsensusHarmonyManager* manager) { harmonyManager = manager; }
    void setRouter(ConsensusRouter* r) { router = r; }
    ConsensusHarmonyManager* getHarmonyManager() const { return harmonyManager; }
    ConsensusRouter* getRouter() const { return router; }

private:
    // Internal validation logic
    TransactionValidationResult performValidation(const Transaction& transaction,
                                                 const std::vector<ConsensusType>& mechanisms);
    bool validateTransactionBasics(const Transaction& transaction);
    bool validateTransactionBalance(const Transaction& transaction);
    bool validateTransactionSignature(const Transaction& transaction);
    
    // Fee calculation helpers
    double calculateBaseFee(const Transaction& transaction);
    double calculateMechanismFees(const std::vector<ConsensusType>& mechanisms);
    double calculateComplexityFee(const Transaction& transaction);
    double calculateNetworkLoadMultiplier();
    
    // Priority calculation helpers
    double calculateFeeScore(const Transaction& transaction);
    double calculateAgeScore(const Transaction& transaction);
    double calculateSenderReputationScore(const Transaction& transaction);
    double calculateNetworkImportanceScore(const Transaction& transaction);
    double calculateConsensusRequirementScore(const Transaction& transaction);
    
    // Cache management helpers
    bool isCacheValid(const std::string& txHash) const;
    void addToCache(const std::string& txHash, const TransactionValidationResult& result);
    TransactionValidationResult getFromCache(const std::string& txHash) const;
    
    // Utility methods
    std::vector<ConsensusType> determineRequiredMechanisms(const Transaction& transaction);
    std::string getTransactionComplexityLevel(const Transaction& transaction);
    double getSenderReputation(const std::string& senderAddress);
    bool isHighPriorityTransaction(const Transaction& transaction);
    
    // Statistics helpers
    void updateValidationStatistics(const TransactionValidationResult& result);
    void incrementMechanismUsage(const std::vector<ConsensusType>& mechanisms);
    
    // Validation helpers
    bool validateConfiguration() const;
    void logValidationEvent(const std::string& event, const nlohmann::json& data = {}) const;
};

#endif // UNIFIED_TRANSACTION_VALIDATOR_H