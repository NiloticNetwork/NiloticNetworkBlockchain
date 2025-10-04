#include <iostream>
#include <cassert>
#include "../include/core/mining.h"
#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony.h"
#include "../include/core/logger.h"

// Simple test without gtest dependency
int main() {
    std::cout << "Starting Mining Harmony Integration Tests..." << std::endl;
    
    try {
        // Initialize logger
        Logger::setLevel(LogLevel::INFO);
        
        // Create blockchain instance
        Blockchain blockchain;
        
        // Create mining configuration
        MiningConfig config;
        config.targetDifficulty = 2;  // Low difficulty for fast testing
        config.targetBlockTime = 10;  // 10 seconds for testing
        config.miningReward = 50.0;
        config.transactionFee = 0.01;
        config.maxBlockSize = 1024;
        config.maxTransactionsPerBlock = 10;
        
        // Create mining engine
        MiningEngine miningEngine(blockchain, config);
        
        std::cout << "Test 1: ConsensusEngine Interface Implementation" << std::endl;
        
        // Test initialization
        bool initialized = miningEngine.initialize();
        assert(initialized);
        std::cout << "✓ Mining engine initialized successfully" << std::endl;
        
        // Test health check
        bool healthy = miningEngine.isHealthy();
        assert(healthy);
        std::cout << "✓ Mining engine is healthy" << std::endl;
        
        // Test consensus type
        ConsensusType type = miningEngine.getType();
        assert(type == ConsensusType::PROOF_OF_WORK);
        std::cout << "✓ Consensus type is PROOF_OF_WORK" << std::endl;
        
        // Test engine name
        std::string name = miningEngine.getName();
        assert(name == "ProofOfWorkEngine");
        std::cout << "✓ Engine name is correct: " << name << std::endl;
        
        std::cout << "Test 2: Parameter Adjustment" << std::endl;
        
        // Get initial parameters
        auto initialParams = miningEngine.getParameters();
        assert(initialParams.size() > 0);
        std::cout << "✓ Retrieved initial parameters (" << initialParams.size() << " parameters)" << std::endl;
        
        // Adjust parameters
        std::map<std::string, double> newParams;
        newParams["difficulty"] = 3.0;
        newParams["targetBlockTime"] = 15.0;
        newParams["miningReward"] = 25.0;
        
        bool adjusted = miningEngine.adjustParameters(newParams);
        assert(adjusted);
        std::cout << "✓ Parameters adjusted successfully" << std::endl;
        
        // Verify parameters were adjusted
        auto adjustedParams = miningEngine.getParameters();
        assert(adjustedParams["difficulty"] == 3.0);
        assert(adjustedParams["targetBlockTime"] == 15.0);
        assert(adjustedParams["miningReward"] == 25.0);
        std::cout << "✓ Parameter values verified" << std::endl;
        
        std::cout << "Test 3: Transaction Validation Request" << std::endl;
        
        // Create a test transaction
        Transaction testTx("sender_address", "recipient_address", 10.0);
        
        // Create consensus request for transaction validation
        ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, testTx.serialize());
        
        // Process the request
        ConsensusResult result = miningEngine.processRequest(request);
        
        // Verify result
        assert(result.mechanism == ConsensusType::PROOF_OF_WORK);
        assert(result.confidence >= 0.0 && result.confidence <= 1.0);
        assert(!result.reason.empty());
        std::cout << "✓ Transaction validation request processed successfully" << std::endl;
        std::cout << "  Result: " << (result.isValid ? "VALID" : "INVALID") << std::endl;
        std::cout << "  Confidence: " << result.confidence << std::endl;
        std::cout << "  Reason: " << result.reason << std::endl;
        
        std::cout << "Test 4: Block Validation Request" << std::endl;
        
        // Create a test block
        Block testBlock(1, "previous_hash");
        Transaction coinbaseTx("COINBASE", "miner_address", 50.0);
        testBlock.addTransaction(coinbaseTx);
        testBlock.setNonce(12345);
        testBlock.updateHash();
        
        // Create consensus request for block validation
        ConsensusRequest blockRequest(RequestType::BLOCK_VALIDATION, testBlock.serialize());
        
        // Process the request
        ConsensusResult blockResult = miningEngine.processRequest(blockRequest);
        
        // Verify result
        assert(blockResult.mechanism == ConsensusType::PROOF_OF_WORK);
        assert(blockResult.confidence >= 0.0 && blockResult.confidence <= 1.0);
        assert(!blockResult.reason.empty());
        std::cout << "✓ Block validation request processed successfully" << std::endl;
        std::cout << "  Result: " << (blockResult.isValid ? "VALID" : "INVALID") << std::endl;
        std::cout << "  Confidence: " << blockResult.confidence << std::endl;
        std::cout << "  Reason: " << blockResult.reason << std::endl;
        
        std::cout << "Test 5: Harmony Metrics Collection" << std::endl;
        
        // Process multiple requests to generate metrics
        for (int i = 0; i < 3; ++i) {
            Transaction tx("sender" + std::to_string(i), "recipient" + std::to_string(i), 1.0 + i);
            ConsensusRequest req(RequestType::TRANSACTION_VALIDATION, tx.serialize());
            miningEngine.processRequest(req);
        }
        
        // Collect harmony metrics
        miningEngine.collectHarmonyMetrics();
        
        // Get harmony metrics
        nlohmann::json harmonyMetrics = miningEngine.getHarmonyMetrics();
        assert(harmonyMetrics.contains("totalHarmonyValidations"));
        assert(harmonyMetrics.contains("successfulHarmonyValidations"));
        assert(harmonyMetrics.contains("averageConfidence"));
        assert(harmonyMetrics.contains("harmonySuccessRate"));
        std::cout << "✓ Harmony metrics collected successfully" << std::endl;
        std::cout << "  Total validations: " << harmonyMetrics["totalHarmonyValidations"] << std::endl;
        std::cout << "  Successful validations: " << harmonyMetrics["successfulHarmonyValidations"] << std::endl;
        std::cout << "  Average confidence: " << harmonyMetrics["averageConfidence"] << std::endl;
        
        std::cout << "Test 6: Status and Metrics" << std::endl;
        
        // Test status
        nlohmann::json status = miningEngine.getStatus();
        assert(status.contains("harmonyInitialized"));
        assert(status.contains("harmonyHealthy"));
        assert(status.contains("consensusType"));
        std::cout << "✓ Status retrieved successfully" << std::endl;
        
        // Test metrics
        nlohmann::json metrics = miningEngine.getMetrics();
        assert(metrics.contains("mining"));
        assert(metrics.contains("harmony"));
        std::cout << "✓ Metrics retrieved successfully" << std::endl;
        
        std::cout << "Test 7: Error Handling" << std::endl;
        
        // Test unsupported request type
        ConsensusRequest governanceRequest(RequestType::GOVERNANCE_PROPOSAL, "test_proposal_data");
        ConsensusResult govResult = miningEngine.processRequest(governanceRequest);
        
        assert(!govResult.isValid);
        assert(govResult.mechanism == ConsensusType::PROOF_OF_WORK);
        assert(govResult.confidence == 0.0);
        assert(govResult.reason.find("Unsupported request type") != std::string::npos);
        std::cout << "✓ Unsupported request type handled correctly" << std::endl;
        
        std::cout << "Test 8: Shutdown Behavior" << std::endl;
        
        // Test shutdown
        miningEngine.shutdown();
        assert(!miningEngine.isHealthy());
        std::cout << "✓ Mining engine shut down successfully" << std::endl;
        
        // Test that requests fail after shutdown
        ConsensusRequest shutdownRequest(RequestType::TRANSACTION_VALIDATION, testTx.serialize());
        ConsensusResult shutdownResult = miningEngine.processRequest(shutdownRequest);
        assert(!shutdownResult.isValid);
        std::cout << "✓ Requests properly rejected after shutdown" << std::endl;
        
        std::cout << std::endl << "🎉 All Mining Harmony Integration Tests Passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}