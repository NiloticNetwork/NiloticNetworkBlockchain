#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>
#include "../include/core/unified_transaction_validator.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/consensus_router.h"
#include "../include/core/transaction.h"
#include "../include/core/logger.h"

class UnifiedTransactionValidatorTest {
private:
    std::unique_ptr<UnifiedTransactionValidator> validator;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
    std::unique_ptr<ConsensusRouter> router;

public:
    UnifiedTransactionValidatorTest() {
        Logger::info("Starting UnifiedTransactionValidator tests");
        
        // Initialize components
        harmonyManager = std::make_unique<ConsensusHarmonyManager>();
        router = std::make_unique<ConsensusRouter>();
        validator = std::make_unique<UnifiedTransactionValidator>(harmonyManager.get());
        
        // Initialize router
        router->initialize();
        
        // Set router in validator
        validator->setRouter(router.get());
    }
    
    ~UnifiedTransactionValidatorTest() {
        Logger::info("UnifiedTransactionValidator tests completed");
    }
    
    void runAllTests() {
        testInitialization();
        testBasicValidation();
        testFeeCalculation();
        testPriorityCalculation();
        testBatchValidation();
        testParallelValidation();
        testTransactionSorting();
        testTransactionFiltering();
        testTransactionSelection();
        testCacheManagement();
        testConfigurationManagement();
        testStatistics();
        testComplexTransactions();
        testErrorHandling();
        
        Logger::info("All UnifiedTransactionValidator tests passed!");
    }
    
private:
    void testInitialization() {
        Logger::info("Testing UnifiedTransactionValidator initialization");
        
        // Test basic initialization
        assert(validator->initialize());
        assert(validator->isInitialized());
        
        // Test initialization with custom configuration
        TransactionFeeStructure customFees;
        customFees.baseFee = 0.002;
        customFees.powFee = 0.001;
        
        TransactionPriorityFactors customPriority;
        customPriority.feeWeight = 0.5;
        customPriority.ageWeight = 0.3;
        
        auto validator2 = std::make_unique<UnifiedTransactionValidator>();
        assert(validator2->initialize(customFees, customPriority));
        
        TransactionFeeStructure retrievedFees = validator2->getFeeStructure();
        assert(retrievedFees.baseFee == 0.002);
        assert(retrievedFees.powFee == 0.001);
        
        Logger::info("✓ Initialization tests passed");
    }
    
    void testBasicValidation() {
        Logger::info("Testing basic transaction validation");
        
        // Test valid transaction
        Transaction validTx("Alice", "Bob", 10.0);
        TransactionValidationResult result = validator->validateTransaction(validTx);
        assert(result.isValid);
        assert(result.confidence > 0.0);
        assert(result.calculatedFee > 0.0);
        assert(result.priority > 0);
        
        // Test coinbase transaction
        Transaction coinbaseTx("COINBASE", "Alice", 100.0);
        TransactionValidationResult coinbaseResult = validator->validateTransaction(coinbaseTx);
        assert(coinbaseResult.isValid);
        assert(coinbaseResult.confidence > 0.0);
        
        // Test transaction with negative amount
        Transaction invalidTx("Alice", "Bob", -10.0);
        TransactionValidationResult invalidResult = validator->validateTransaction(invalidTx);
        assert(!invalidResult.isValid);
        
        Logger::info("✓ Basic validation tests passed");
    }
    
    void testFeeCalculation() {
        Logger::info("Testing transaction fee calculation");
        
        // Test basic fee calculation
        Transaction basicTx("Alice", "Bob", 10.0);
        double basicFee = validator->calculateTransactionFee(basicTx);
        assert(basicFee > 0.0);
        
        // Test large transaction fee
        Transaction largeTx("Alice", "Bob", 1500.0);
        double largeFee = validator->calculateTransactionFee(largeTx);
        assert(largeFee > basicFee); // Large transactions should cost more
        
        // Test smart contract deployment fee
        Transaction contractTx("Alice", "contract_code_here");
        double contractFee = validator->calculateTransactionFee(contractTx);
        assert(contractFee > basicFee); // Contract deployment should cost more
        
        // Test fee calculation with specific mechanisms
        std::vector<ConsensusType> mechanisms = {
            ConsensusType::PROOF_OF_WORK,
            ConsensusType::PROOF_OF_STAKE,
            ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION
        };
        double mechanismFee = validator->calculateTransactionFeeWithMechanisms(basicTx, mechanisms);
        assert(mechanismFee > basicFee); // More mechanisms should cost more
        
        Logger::info("✓ Fee calculation tests passed");
    }
    
    void testPriorityCalculation() {
        Logger::info("Testing transaction priority calculation");
        
        // Test basic priority calculation
        Transaction basicTx("Alice", "Bob", 10.0);
        uint32_t basicPriority = validator->calculateTransactionPriority(basicTx);
        assert(basicPriority > 0);
        
        // Test high-value transaction priority
        Transaction highValueTx("Alice", "Bob", 2000.0);
        uint32_t highValuePriority = validator->calculateTransactionPriority(highValueTx);
        assert(highValuePriority > basicPriority); // High-value transactions should have higher priority
        
        // Test coinbase transaction priority
        Transaction coinbaseTx("COINBASE", "Alice", 100.0);
        uint32_t coinbasePriority = validator->calculateTransactionPriority(coinbaseTx);
        assert(coinbasePriority > basicPriority); // Coinbase transactions should have high priority
        
        // Test priority with custom factors
        TransactionPriorityFactors customFactors;
        customFactors.feeWeight = 0.8;
        customFactors.ageWeight = 0.2;
        
        uint32_t customPriority = validator->calculateTransactionPriorityWithFactors(basicTx, customFactors);
        assert(customPriority > 0);
        
        Logger::info("✓ Priority calculation tests passed");
    }
    
    void testBatchValidation() {
        Logger::info("Testing batch transaction validation");
        
        // Create batch of transactions
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 10.0);
        transactions.emplace_back("Bob", "Charlie", 20.0);
        transactions.emplace_back("Charlie", "Alice", 15.0);
        transactions.emplace_back("COINBASE", "Miner", 100.0);
        
        // Test batch validation
        std::vector<TransactionValidationResult> results = validator->validateTransactions(transactions);
        assert(results.size() == transactions.size());
        
        // Check that all valid transactions passed
        for (size_t i = 0; i < results.size(); ++i) {
            assert(results[i].isValid);
            assert(results[i].confidence > 0.0);
            assert(results[i].calculatedFee > 0.0);
        }
        
        Logger::info("✓ Batch validation tests passed");
    }
    
    void testParallelValidation() {
        Logger::info("Testing parallel transaction validation");
        
        // Create large batch of transactions
        std::vector<Transaction> transactions;
        for (int i = 0; i < 100; ++i) {
            transactions.emplace_back("Sender" + std::to_string(i), 
                                    "Recipient" + std::to_string(i), 
                                    10.0 + i);
        }
        
        // Test parallel validation
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<TransactionValidationResult> parallelResults = 
            validator->validateTransactionsParallel(transactions);
        auto parallelTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test sequential validation for comparison
        start = std::chrono::high_resolution_clock::now();
        std::vector<TransactionValidationResult> sequentialResults = 
            validator->validateTransactions(transactions);
        auto sequentialTime = std::chrono::high_resolution_clock::now() - start;
        
        // Verify results are the same
        assert(parallelResults.size() == sequentialResults.size());
        assert(parallelResults.size() == transactions.size());
        
        // Parallel should be faster (or at least not significantly slower)
        Logger::info("Parallel time: " + std::to_string(parallelTime.count()) + 
                    "ns, Sequential time: " + std::to_string(sequentialTime.count()) + "ns");
        
        Logger::info("✓ Parallel validation tests passed");
    }
    
    void testTransactionSorting() {
        Logger::info("Testing transaction sorting by priority");
        
        // Create transactions with different priorities
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 10.0);        // Low priority
        transactions.emplace_back("COINBASE", "Miner", 100.0);  // High priority
        transactions.emplace_back("Charlie", "Dave", 1500.0);   // High value = high priority
        transactions.emplace_back("Eve", "Frank", 5.0);         // Low priority
        
        // Sort by priority
        std::vector<Transaction> sortedTx = validator->sortTransactionsByPriority(transactions);
        assert(sortedTx.size() == transactions.size());
        
        // Verify sorting (coinbase and high-value transactions should be first)
        uint32_t prevPriority = UINT32_MAX;
        for (const Transaction& tx : sortedTx) {
            uint32_t priority = validator->calculateTransactionPriority(tx);
            assert(priority <= prevPriority); // Should be in descending order
            prevPriority = priority;
        }
        
        Logger::info("✓ Transaction sorting tests passed");
    }
    
    void testTransactionFiltering() {
        Logger::info("Testing transaction filtering");
        
        // Create mix of valid and invalid transactions
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 10.0);     // Valid
        transactions.emplace_back("Bob", "Charlie", -5.0);   // Invalid (negative amount)
        transactions.emplace_back("COINBASE", "Miner", 100.0); // Valid
        transactions.emplace_back("", "Dave", 20.0);         // Invalid (empty sender)
        
        // Filter valid transactions
        std::vector<Transaction> validTx = validator->filterValidTransactions(transactions);
        
        // Should have 2 valid transactions
        assert(validTx.size() == 2);
        
        // Verify all filtered transactions are valid
        for (const Transaction& tx : validTx) {
            TransactionValidationResult result = validator->validateTransaction(tx);
            assert(result.isValid);
        }
        
        Logger::info("✓ Transaction filtering tests passed");
    }
    
    void testTransactionSelection() {
        Logger::info("Testing transaction selection for block");
        
        // Create many transactions
        std::vector<Transaction> transactions;
        for (int i = 0; i < 20; ++i) {
            transactions.emplace_back("Sender" + std::to_string(i), 
                                    "Recipient" + std::to_string(i), 
                                    10.0 + i * 10);
        }
        
        // Select transactions for block with limits
        size_t maxTransactions = 10;
        double maxTotalFees = 0.1;
        
        std::vector<Transaction> selectedTx = validator->selectTransactionsForBlock(
            transactions, maxTransactions, maxTotalFees);
        
        // Should not exceed limits
        assert(selectedTx.size() <= maxTransactions);
        
        // Calculate total fees
        double totalFees = 0.0;
        for (const Transaction& tx : selectedTx) {
            totalFees += validator->calculateTransactionFee(tx);
        }
        assert(totalFees <= maxTotalFees);
        
        // Test without fee limit
        std::vector<Transaction> selectedTx2 = validator->selectTransactionsForBlock(
            transactions, maxTransactions, 0.0);
        assert(selectedTx2.size() <= maxTransactions);
        
        Logger::info("✓ Transaction selection tests passed");
    }
    
    void testCacheManagement() {
        Logger::info("Testing validation cache management");
        
        // Test cache functionality
        Transaction tx("Alice", "Bob", 10.0);
        
        // First validation should not be cached
        size_t initialCacheSize = validator->getCacheSize();
        TransactionValidationResult result1 = validator->validateTransaction(tx);
        assert(validator->getCacheSize() > initialCacheSize);
        
        // Second validation should use cache
        TransactionValidationResult result2 = validator->validateTransaction(tx);
        assert(result1.isValid == result2.isValid);
        assert(result1.confidence == result2.confidence);
        
        // Test cache clearing
        validator->clearValidationCache();
        assert(validator->getCacheSize() == 0);
        
        // Test cache cleanup
        validator->validateTransaction(tx);
        assert(validator->getCacheSize() > 0);
        validator->cleanupExpiredCache(); // Should not remove recent entries
        assert(validator->getCacheSize() > 0);
        
        Logger::info("✓ Cache management tests passed");
    }
    
    void testConfigurationManagement() {
        Logger::info("Testing configuration management");
        
        // Test fee structure update
        TransactionFeeStructure newFees;
        newFees.baseFee = 0.005;
        newFees.powFee = 0.002;
        
        assert(validator->updateFeeStructure(newFees));
        
        TransactionFeeStructure retrievedFees = validator->getFeeStructure();
        assert(retrievedFees.baseFee == 0.005);
        assert(retrievedFees.powFee == 0.002);
        
        // Test priority factors update
        TransactionPriorityFactors newFactors;
        newFactors.feeWeight = 0.6;
        newFactors.ageWeight = 0.4;
        
        assert(validator->updatePriorityFactors(newFactors));
        
        TransactionPriorityFactors retrievedFactors = validator->getPriorityFactors();
        assert(retrievedFactors.feeWeight == 0.6);
        assert(retrievedFactors.ageWeight == 0.4);
        
        Logger::info("✓ Configuration management tests passed");
    }
    
    void testStatistics() {
        Logger::info("Testing statistics and monitoring");
        
        // Reset statistics
        validator->resetStatistics();
        
        // Perform some validations
        Transaction tx1("Alice", "Bob", 10.0);
        Transaction tx2("Bob", "Charlie", 20.0);
        Transaction invalidTx("", "Dave", 30.0); // Invalid
        
        validator->validateTransaction(tx1);
        validator->validateTransaction(tx2);
        validator->validateTransaction(invalidTx);
        
        // Check statistics
        nlohmann::json stats = validator->getStatistics();
        assert(stats["totalValidations"].get<uint64_t>() >= 3);
        assert(stats["successfulValidations"].get<uint64_t>() >= 2);
        assert(stats.contains("successRate"));
        assert(stats.contains("mechanismUsage"));
        
        // Check validation metrics
        nlohmann::json metrics = validator->getValidationMetrics();
        assert(metrics["initialized"].get<bool>() == true);
        assert(metrics.contains("feeStructure"));
        assert(metrics.contains("priorityFactors"));
        
        Logger::info("✓ Statistics tests passed");
    }
    
    void testComplexTransactions() {
        Logger::info("Testing complex transaction scenarios");
        
        // Test smart contract deployment
        Transaction contractTx("Alice", "contract_code_example");
        TransactionValidationResult contractResult = validator->validateTransaction(contractTx);
        assert(contractResult.isValid);
        assert(contractResult.calculatedFee > validator->getFeeStructure().baseFee);
        
        // Test offline transaction
        Transaction offlineTx("Alice", "Bob", 10.0, true);
        TransactionValidationResult offlineResult = validator->validateTransaction(offlineTx);
        assert(offlineResult.isValid);
        
        // Test high-value transaction
        Transaction highValueTx("Alice", "Bob", 5000.0);
        TransactionValidationResult highValueResult = validator->validateTransaction(highValueTx);
        assert(highValueResult.isValid);
        assert(highValueResult.priority > validator->calculateTransactionPriority(Transaction("Alice", "Bob", 10.0)));
        
        Logger::info("✓ Complex transaction tests passed");
    }
    
    void testErrorHandling() {
        Logger::info("Testing error handling");
        
        // Test validation without initialization
        auto uninitializedValidator = std::make_unique<UnifiedTransactionValidator>();
        Transaction tx("Alice", "Bob", 10.0);
        TransactionValidationResult result = uninitializedValidator->validateTransaction(tx);
        assert(!result.isValid);
        assert(result.reason.find("not initialized") != std::string::npos);
        
        // Test invalid transaction validation
        Transaction invalidTx("", "", -10.0);
        TransactionValidationResult invalidResult = validator->validateTransaction(invalidTx);
        assert(!invalidResult.isValid);
        
        // Test empty batch validation
        std::vector<Transaction> emptyBatch;
        std::vector<TransactionValidationResult> emptyResults = validator->validateTransactions(emptyBatch);
        assert(emptyResults.empty());
        
        Logger::info("✓ Error handling tests passed");
    }
};

int main() {
    try {
        UnifiedTransactionValidatorTest test;
        test.runAllTests();
        
        std::cout << "All UnifiedTransactionValidator tests passed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}