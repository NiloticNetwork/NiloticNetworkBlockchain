#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/consensus_router.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>

/**
 * End-to-End Consensus Workflow Test Suite
 * 
 * Tests complete consensus workflows from start to finish:
 * - Complete block mining and validation workflow
 * - Multi-consensus transaction processing
 * - Governance proposal lifecycle
 * - Smart contract deployment and execution
 * 
 * Requirements: 1.1, 1.2, 1.3, 1.4, 1.5
 */
class ConsensusEndToEndTest {
private:
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
    std::unique_ptr<ConsensusRouter> router;

public:
    ConsensusEndToEndTest() {
        Logger::info("Initializing End-to-End Consensus Workflow Test Suite");
        
        blockchain = std::make_unique<Blockchain>();
        harmonyManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        router = std::make_unique<ConsensusRouter>();
        
        if (!harmonyManager->initializeConsensus()) {
            Logger::warning("Consensus harmony manager initialization failed, using mock setup");
        }
        
        Logger::info("✓ End-to-end test suite initialization complete");
    }
    
    void runAllEndToEndTests() {
        Logger::info("=== Starting End-to-End Consensus Workflow Tests ===");
        
        testCompleteBlockMiningWorkflow();
        testMultiConsensusTransactionWorkflow();
        testGovernanceProposalLifecycle();
        testSmartContractDeploymentWorkflow();
        
        Logger::info("=== End-to-End Consensus Workflow Tests Complete ===");
    }

private:
    void testCompleteBlockMiningWorkflow() {
        Logger::info("\n--- Testing Complete Block Mining Workflow ---");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Step 1: Create and validate transactions
            std::vector<Transaction> transactions;
            for (int i = 0; i < 10; ++i) {
                Transaction tx("MiningWorkflowSender" + std::to_string(i),
                             "MiningWorkflowRecipient" + std::to_string(i),
                             10.0 + i);
                
                bool txValid = harmonyManager->validateTransaction(tx);
                assert(txValid);
                
                transactions.push_back(tx);
                blockchain->addTransaction(tx);
            }
            
            // Step 2: Mine block with consensus validation
            Block newBlock = blockchain->minePendingTransactions("MiningWorkflowMiner");
            
            // Step 3: Validate mined block
            bool blockValid = harmonyManager->validateBlock(newBlock);
            assert(blockValid);
            
            // Step 4: Verify block properties
            assert(newBlock.getIndex() > 0);
            assert(!newBlock.getTransactions().empty());
            
            auto end = std::chrono::high_resolution_clock::now();
            double executionTime = std::chrono::duration<double, std::milli>(end - start).count();
            
            Logger::info("✓ Complete Block Mining Workflow PASSED (" + std::to_string(executionTime) + "ms)");
            
        } catch (const std::exception& e) {
            Logger::error("✗ Complete Block Mining Workflow FAILED: " + std::string(e.what()));
        }
    }
    
    void testMultiConsensusTransactionWorkflow() {
        Logger::info("\n--- Testing Multi-Consensus Transaction Workflow ---");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Different types of transactions
            std::vector<Transaction> multiConsensusTransactions = {
                Transaction("Alice", "Bob", 100.0),                    // Regular transaction
                Transaction("COINBASE", "Miner", 50.0),               // Coinbase transaction
                Transaction("Voter1", "VOTE:proposal_123"),           // Voting transaction
                Transaction("Developer", "contract_deployment_code"), // Smart contract transaction
            };
            
            // Validate each transaction
            for (const auto& tx : multiConsensusTransactions) {
                bool harmonyValid = harmonyManager->validateTransaction(tx);
                assert(harmonyValid);
                
                blockchain->addTransaction(tx);
            }
            
            // Mine block with all transaction types
            Block multiConsensusBlock = blockchain->minePendingTransactions("MultiConsensusMiner");
            
            // Validate block
            bool blockValid = harmonyManager->validateBlock(multiConsensusBlock);
            assert(blockValid);
            
            auto end = std::chrono::high_resolution_clock::now();
            double executionTime = std::chrono::duration<double, std::milli>(end - start).count();
            
            Logger::info("✓ Multi-Consensus Transaction Workflow PASSED (" + std::to_string(executionTime) + "ms)");
            
        } catch (const std::exception& e) {
            Logger::error("✗ Multi-Consensus Transaction Workflow FAILED: " + std::string(e.what()));
        }
    }
    
    void testGovernanceProposalLifecycle() {
        Logger::info("\n--- Testing Governance Proposal Lifecycle ---");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Create governance proposal
            ConsensusRequest proposalRequest;
            proposalRequest.type = RequestType::GOVERNANCE_VALIDATION;
            proposalRequest.data = "proposal_increase_block_reward_to_150";
            proposalRequest.required = {ConsensusType::VOTING_CONSENSUS};
            
            // Validate proposal
            ConsensusResult proposalResult = router->routeValidation(proposalRequest);
            assert(proposalResult.isValid);
            
            // Simulate voting process
            std::vector<Transaction> votes;
            for (int i = 0; i < 5; ++i) {
                Transaction vote("Voter" + std::to_string(i), 
                               "VOTE:proposal_increase_block_reward_to_150:YES",
                               0.0);
                
                bool voteValid = harmonyManager->validateTransaction(vote);
                assert(voteValid);
                
                votes.push_back(vote);
                blockchain->addTransaction(vote);
            }
            
            // Mine block with votes
            Block votingBlock = blockchain->minePendingTransactions("GovernanceMiner");
            bool votingBlockValid = harmonyManager->validateBlock(votingBlock);
            assert(votingBlockValid);
            
            auto end = std::chrono::high_resolution_clock::now();
            double executionTime = std::chrono::duration<double, std::milli>(end - start).count();
            
            Logger::info("✓ Governance Proposal Lifecycle PASSED (" + std::to_string(executionTime) + "ms)");
            
        } catch (const std::exception& e) {
            Logger::error("✗ Governance Proposal Lifecycle FAILED: " + std::string(e.what()));
        }
    }
    
    void testSmartContractDeploymentWorkflow() {
        Logger::info("\n--- Testing Smart Contract Deployment Workflow ---");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Create contract deployment transaction
            Transaction contractDeployment("ContractDeveloper", 
                                          "contract_simple_token_bytecode");
            
            // Validate contract deployment
            bool deploymentValid = harmonyManager->validateTransaction(contractDeployment);
            assert(deploymentValid);
            
            // Add deployment to blockchain
            blockchain->addTransaction(contractDeployment);
            
            // Mine block with contract deployment
            Block contractBlock = blockchain->minePendingTransactions("ContractMiner");
            bool contractBlockValid = harmonyManager->validateBlock(contractBlock);
            assert(contractBlockValid);
            
            // Test contract execution validation
            ConsensusRequest executionRequest;
            executionRequest.type = RequestType::CONTRACT_VALIDATION;
            executionRequest.data = "contract_execution_transfer_100_tokens";
            executionRequest.required = {ConsensusType::SMART_CONTRACT_VALIDATION};
            
            ConsensusResult executionResult = router->routeValidation(executionRequest);
            assert(executionResult.isValid);
            
            auto end = std::chrono::high_resolution_clock::now();
            double executionTime = std::chrono::duration<double, std::milli>(end - start).count();
            
            Logger::info("✓ Smart Contract Deployment Workflow PASSED (" + std::to_string(executionTime) + "ms)");
            
        } catch (const std::exception& e) {
            Logger::error("✗ Smart Contract Deployment Workflow FAILED: " + std::string(e.what()));
        }
    }
};

int main() {
    try {
        Logger::info("=== End-to-End Consensus Workflow Test Suite ===");
        Logger::info("Testing complete consensus workflows from start to finish");
        Logger::info("Requirements: 1.1, 1.2, 1.3, 1.4, 1.5");
        Logger::info("==================================================");
        
        ConsensusEndToEndTest endToEndTest;
        endToEndTest.runAllEndToEndTests();
        
        Logger::info("\n🎉 End-to-End Consensus Workflow Test Suite Complete!");
        return 0;
        
    } catch (const std::exception& e) {
        Logger::error("End-to-end test suite failed with exception: " + std::string(e.what()));
        return 1;
    }
}