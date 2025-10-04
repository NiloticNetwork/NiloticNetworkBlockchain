#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>
#include "../include/core/enhanced_blockchain.h"
#include "../include/core/transaction.h"
#include "../include/core/logger.h"

class EnhancedBlockchainIntegrationTest {
private:
    std::unique_ptr<EnhancedBlockchain> blockchain;

public:
    EnhancedBlockchainIntegrationTest() {
        Logger::info("Starting EnhancedBlockchain integration tests");
        blockchain = std::make_unique<EnhancedBlockchain>();
    }
    
    ~EnhancedBlockchainIntegrationTest() {
        Logger::info("EnhancedBlockchain integration tests completed");
    }
    
    void runAllTests() {
        testInitialization();
        testEnhancedTransactionValidation();
        testEnhancedTransactionAddition();
        testBatchTransactionProcessing();
        testEnhancedBlockMining();
        testFeeCalculationAndOptimization();
        testTransactionPrioritization();
        testConsensusIntegration();
        testConfigurationManagement();
        testStatisticsAndMonitoring();
        testPerformanceOptimizations();
        testErrorHandlingAndRecovery();
        
        Logger::info("All EnhancedBlockchain integration tests passed!");
    }
    
private:
    void testInitialization() {
        Logger::info("Testing EnhancedBlockchain initialization");
        
        // Test basic initialization
        assert(blockchain->initializeEnhancedFeatures());
        
        // Verify components are initialized
        assert(blockchain->getHarmonyManager() != nullptr);
        assert(blockchain->getTransactionValidator() != nullptr);
        
        // Test initialization with custom configuration
        TransactionFeeStructure customFees;
        customFees.baseFee = 0.005;
        customFees.powFee = 0.002;
        
        TransactionPriorityFactors customPriority;
        customPriority.feeWeight = 0.6;
        customPriority.ageWeight = 0.4;
        
        auto blockchain2 = std::make_unique<EnhancedBlockchain>();
        assert(blockchain2->initializeEnhancedFeatures(customFees, customPriority));
        
        Logger::info("✓ Initialization tests passed");
    }
    
    void testEnhancedTransactionValidation() {
        Logger::info("Testing enhanced transaction validation");
        
        // Test valid transaction
        Transaction validTx("Alice", "Bob", 10.0);
        TransactionValidationResult result = blockchain->validateTransactionEnhanced(validTx);
        assert(result.isValid);
        assert(result.confidence > 0.0);
        assert(result.calculatedFee > 0.0);
        assert(result.priority > 0);
        
        // Test invalid transaction
        Transaction invalidTx("", "Bob", -10.0);
        TransactionValidationResult invalidResult = blockchain->validateTransactionEnhanced(invalidTx);
        assert(!invalidResult.isValid);
        
        // Test coinbase transaction
        Transaction coinbaseTx("COINBASE", "Miner", 100.0);
        TransactionValidationResult coinbaseResult = blockchain->validateTransactionEnhanced(coinbaseTx);
        assert(coinbaseResult.isValid);
        assert(coinbaseResult.confidence > 0.0);
        
        // Test smart contract transaction
        Transaction contractTx("Alice", "contract_code_example");
        TransactionValidationResult contractResult = blockchain->validateTransactionEnhanced(contractTx);
        assert(contractResult.isValid);
        assert(contractResult.calculatedFee > result.calculatedFee); // Should cost more
        
        Logger::info("✓ Enhanced transaction validation tests passed");
    }
    
    void testEnhancedTransactionAddition() {
        Logger::info("Testing enhanced transaction addition");
        
        // Test adding valid transaction
        Transaction validTx("Alice", "Bob", 10.0);
        assert(blockchain->addTransactionEnhanced(validTx));
        
        // Test adding invalid transaction
        Transaction invalidTx("", "Bob", -10.0);
        assert(!blockchain->addTransactionEnhanced(invalidTx));
        
        // Test batch addition
        std::vector<Transaction> transactions;
        transactions.emplace_back("Bob", "Charlie", 20.0);
        transactions.emplace_back("Charlie", "Dave", 15.0);
        transactions.emplace_back("Dave", "Eve", 25.0);
        transactions.emplace_back("", "Invalid", 5.0); // Invalid transaction
        
        assert(blockchain->addTransactionBatchEnhanced(transactions));
        
        // Verify pending transaction count increased
        assert(blockchain->getPendingTransactionCount() > 0);
        
        Logger::info("✓ Enhanced transaction addition tests passed");
    }
    
    void testBatchTransactionProcessing() {
        Logger::info("Testing batch transaction processing");
        
        // Create batch of transactions
        std::vector<Transaction> transactions;
        for (int i = 0; i < 20; ++i) {
            transactions.emplace_back("Sender" + std::to_string(i), 
                                    "Recipient" + std::to_string(i), 
                                    10.0 + i);
        }
        
        // Test batch validation
        std::vector<TransactionValidationResult> results = blockchain->validateTransactionBatch(transactions);
        assert(results.size() == transactions.size());
        
        // Verify all valid transactions passed
        size_t validCount = 0;
        for (const TransactionValidationResult& result : results) {
            if (result.isValid) {
                validCount++;
                assert(result.confidence > 0.0);
                assert(result.calculatedFee > 0.0);
            }
        }
        assert(validCount == transactions.size()); // All should be valid
        
        // Test batch addition
        assert(blockchain->addTransactionBatchEnhanced(transactions));
        
        Logger::info("✓ Batch transaction processing tests passed");
    }
    
    void testEnhancedBlockMining() {
        Logger::info("Testing enhanced block mining");
        
        // Add some transactions first
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 100.0);
        transactions.emplace_back("Bob", "Charlie", 200.0);
        transactions.emplace_back("Charlie", "Dave", 150.0);
        
        blockchain->addTransactionBatchEnhanced(transactions);
        
        // Test enhanced mining
        Block minedBlock = blockchain->minePendingTransactionsEnhanced("Miner1");
        assert(minedBlock.getIndex() > 0);
        assert(minedBlock.getTransactions().size() > 0);
        
        // Verify block contains coinbase transaction
        bool hasCoinbase = false;
        for (const Transaction& tx : minedBlock.getTransactions()) {
            if (tx.getSender() == "COINBASE") {
                hasCoinbase = true;
                assert(tx.getAmount() > blockchain->getMiningReward()); // Should include fees
                break;
            }
        }
        assert(hasCoinbase);
        
        // Test mining with consensus mechanisms
        std::vector<ConsensusType> mechanisms = {
            ConsensusType::PROOF_OF_WORK,
            ConsensusType::PROOF_OF_STAKE
        };
        
        // Add more transactions
        blockchain->addTransactionEnhanced(Transaction("Eve", "Frank", 75.0));
        
        Block consensusBlock = blockchain->minePendingTransactionsWithConsensus("Miner2", mechanisms);
        assert(consensusBlock.getIndex() > minedBlock.getIndex());
        
        Logger::info("✓ Enhanced block mining tests passed");
    }
    
    void testFeeCalculationAndOptimization() {
        Logger::info("Testing fee calculation and optimization");
        
        // Test basic fee calculation
        Transaction basicTx("Alice", "Bob", 10.0);
        double basicFee = blockchain->calculateTransactionFeeEnhanced(basicTx);
        assert(basicFee > 0.0);
        
        // Test large transaction fee
        Transaction largeTx("Alice", "Bob", 1500.0);
        double largeFee = blockchain->calculateTransactionFeeEnhanced(largeTx);
        assert(largeFee >= basicFee); // Large transactions may cost more
        
        // Test smart contract fee
        Transaction contractTx("Alice", "contract_code_here");
        double contractFee = blockchain->calculateTransactionFeeEnhanced(contractTx);
        assert(contractFee > basicFee); // Contract deployment should cost more
        
        // Test optimal fee calculation for priority
        uint32_t targetPriority = 1000000;
        double optimalFee = blockchain->calculateOptimalFeeForPriority(basicTx, targetPriority);
        assert(optimalFee >= basicFee);
        
        // Test fee estimation
        double estimatedFee = blockchain->estimateTransactionFee(basicTx);
        assert(estimatedFee > 0.0);
        
        // Test fee prediction
        double predictedFee = blockchain->predictOptimalFee(60.0); // 1 minute target
        assert(predictedFee > 0.0);
        
        Logger::info("✓ Fee calculation and optimization tests passed");
    }
    
    void testTransactionPrioritization() {
        Logger::info("Testing transaction prioritization");
        
        // Create transactions with different priorities
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 10.0);        // Low priority
        transactions.emplace_back("COINBASE", "Miner", 100.0);  // High priority
        transactions.emplace_back("Charlie", "Dave", 1500.0);   // High value = high priority
        transactions.emplace_back("Eve", "Frank", 5.0);         // Low priority
        
        // Add transactions to blockchain
        blockchain->addTransactionBatchEnhanced(transactions);
        
        // Get transactions sorted by priority
        std::vector<Transaction> prioritizedTxs = blockchain->getPendingTransactionsByPriority();
        assert(prioritizedTxs.size() >= transactions.size());
        
        // Test transaction selection with constraints
        std::vector<Transaction> selectedTxs = blockchain->selectTransactionsWithConstraints(2, 0.1);
        assert(selectedTxs.size() <= 2);
        
        // Test optimal transaction selection
        std::vector<Transaction> optimalTxs = blockchain->selectOptimalTransactionsForBlock();
        assert(!optimalTxs.empty());
        
        Logger::info("✓ Transaction prioritization tests passed");
    }
    
    void testConsensusIntegration() {
        Logger::info("Testing consensus integration");
        
        // Create a block for validation
        Transaction tx("Alice", "Bob", 50.0);
        blockchain->addTransactionEnhanced(tx);
        
        Block testBlock = blockchain->minePendingTransactionsEnhanced("TestMiner");
        
        // Test enhanced block validation
        assert(blockchain->validateBlockEnhanced(testBlock));
        
        // Test validation with specific consensus mechanisms
        std::vector<ConsensusType> mechanisms = {
            ConsensusType::PROOF_OF_WORK,
            ConsensusType::PROOF_OF_STAKE
        };
        
        assert(blockchain->validateBlockWithUnifiedConsensus(testBlock, mechanisms));
        
        // Test consensus harmony status
        nlohmann::json harmonyStatus = blockchain->getConsensusHarmonyStatus();
        assert(harmonyStatus.contains("initialized"));
        
        Logger::info("✓ Consensus integration tests passed");
    }
    
    void testConfigurationManagement() {
        Logger::info("Testing configuration management");
        
        // Test processing configuration update
        EnhancedBlockchain::TransactionProcessingConfig newConfig;
        newConfig.maxTransactionsPerBlock = 500;
        newConfig.maxTotalFeesPerBlock = 5.0;
        newConfig.enablePriorityProcessing = true;
        
        assert(blockchain->updateProcessingConfig(newConfig));
        
        EnhancedBlockchain::TransactionProcessingConfig retrievedConfig = blockchain->getProcessingConfig();
        assert(retrievedConfig.maxTransactionsPerBlock == 500);
        assert(retrievedConfig.maxTotalFeesPerBlock == 5.0);
        
        // Test fee structure update
        TransactionFeeStructure newFees;
        newFees.baseFee = 0.003;
        newFees.powFee = 0.0015;
        
        assert(blockchain->updateFeeStructure(newFees));
        
        // Test priority factors update
        TransactionPriorityFactors newFactors;
        newFactors.feeWeight = 0.7;
        newFactors.ageWeight = 0.3;
        
        assert(blockchain->updatePriorityFactors(newFactors));
        
        Logger::info("✓ Configuration management tests passed");
    }
    
    void testStatisticsAndMonitoring() {
        Logger::info("Testing statistics and monitoring");
        
        // Process some transactions to generate statistics
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 10.0);
        transactions.emplace_back("Bob", "Charlie", 20.0);
        transactions.emplace_back("", "Invalid", 30.0); // Invalid
        
        blockchain->validateTransactionBatch(transactions);
        blockchain->addTransactionBatchEnhanced(transactions);
        
        // Test enhanced statistics
        nlohmann::json stats = blockchain->getEnhancedStatistics();
        assert(stats.contains("transactionStats"));
        assert(stats.contains("processingConfig"));
        
        // Test validation metrics
        nlohmann::json validationMetrics = blockchain->getTransactionValidationMetrics();
        assert(validationMetrics.contains("initialized"));
        
        // Test total fees collected
        double totalFees = blockchain->getTotalFeesCollected();
        assert(totalFees >= 0.0);
        
        // Test pending transaction count
        size_t pendingCount = blockchain->getPendingTransactionCount();
        assert(pendingCount >= 0);
        
        // Test network load
        double networkLoad = blockchain->getCurrentNetworkLoad();
        assert(networkLoad >= 0.0 && networkLoad <= 1.0);
        
        // Test fee histogram
        std::vector<double> feeHistogram = blockchain->getFeeHistogram();
        // Histogram can be empty if no pending transactions
        
        // Reset statistics
        blockchain->resetEnhancedStatistics();
        
        Logger::info("✓ Statistics and monitoring tests passed");
    }
    
    void testPerformanceOptimizations() {
        Logger::info("Testing performance optimizations");
        
        // Test transaction cleanup
        blockchain->cleanupInvalidTransactions();
        
        // Test transaction revalidation
        blockchain->revalidatePendingTransactions();
        
        // Test network congestion detection
        bool isCongested = blockchain->isNetworkCongested();
        // Can be true or false depending on current state
        
        // Test parameter adjustment
        blockchain->adjustProcessingParameters();
        
        // Test fee optimization
        std::vector<Transaction> transactions;
        transactions.emplace_back("Alice", "Bob", 10.0);
        transactions.emplace_back("Bob", "Charlie", 20.0);
        transactions.emplace_back("Charlie", "Dave", 15.0);
        
        std::vector<Transaction> optimizedTxs = blockchain->optimizeTransactionFees(transactions);
        assert(optimizedTxs.size() == transactions.size());
        
        // Test transaction reordering
        std::vector<Transaction> reorderedTxs = blockchain->reorderTransactionsByPriority(transactions);
        assert(reorderedTxs.size() == transactions.size());
        
        Logger::info("✓ Performance optimization tests passed");
    }
    
    void testErrorHandlingAndRecovery() {
        Logger::info("Testing error handling and recovery");
        
        // Test validation without initialization
        auto uninitializedBlockchain = std::make_unique<EnhancedBlockchain>();
        Transaction tx("Alice", "Bob", 10.0);
        
        TransactionValidationResult result = uninitializedBlockchain->validateTransactionEnhanced(tx);
        assert(!result.isValid);
        assert(result.reason.find("not initialized") != std::string::npos);
        
        // Test adding invalid transaction
        Transaction invalidTx("", "", -10.0);
        assert(!blockchain->addTransactionEnhanced(invalidTx));
        
        // Test empty batch processing
        std::vector<Transaction> emptyBatch;
        std::vector<TransactionValidationResult> emptyResults = blockchain->validateTransactionBatch(emptyBatch);
        assert(emptyResults.empty());
        
        // Test invalid configuration
        EnhancedBlockchain::TransactionProcessingConfig invalidConfig;
        invalidConfig.maxTransactionsPerBlock = 0; // Invalid
        assert(!blockchain->updateProcessingConfig(invalidConfig));
        
        // Test graceful shutdown and restart
        blockchain->shutdownEnhancedFeatures();
        assert(blockchain->initializeEnhancedFeatures());
        
        Logger::info("✓ Error handling and recovery tests passed");
    }
};

int main() {
    try {
        EnhancedBlockchainIntegrationTest test;
        test.runAllTests();
        
        std::cout << "All EnhancedBlockchain integration tests passed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}