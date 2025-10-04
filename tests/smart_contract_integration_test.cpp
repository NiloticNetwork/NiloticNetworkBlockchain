#include <iostream>
#include <memory>
#include <vector>
#include <cassert>
#include "../include/core/smart_contract_consensus_engine.h"

#include "../include/core/blockchain.h"
#include "../include/core/logger.h"

// Simple integration test for Smart Contract Consensus Engine
class SmartContractIntegrationTest {
private:
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<SmartContractConsensusEngine> scEngine;
    
public:
    SmartContractIntegrationTest() {
        Logger::setLevel(LogLevel::INFO);
        blockchain = std::make_unique<Blockchain>();
        scEngine = std::make_unique<SmartContractConsensusEngine>(blockchain.get(), nullptr);
    }
    
    bool runBasicTests() {
        std::cout << "=== Smart Contract Consensus Engine Integration Test ===" << std::endl;
        
        // Test 1: Engine initialization
        std::cout << "Test 1: Engine Initialization... ";
        if (!scEngine->initialize()) {
            std::cout << "FAILED" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 2: Engine health check
        std::cout << "Test 2: Health Check... ";
        if (!scEngine->isHealthy()) {
            std::cout << "FAILED" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 3: Contract deployment
        std::cout << "Test 3: Contract Deployment... ";
        std::string contractCode = "contract SimpleStorage { uint256 value; function set(uint256 v) { value = v; } }";
        ContractExecutionResult deployResult = scEngine->deployContract(contractCode, "deployer1");
        if (!deployResult.success) {
            std::cout << "FAILED: " << deployResult.errorMessage << std::endl;
            return false;
        }
        std::cout << "PASSED (Address: " << deployResult.contractAddress << ")" << std::endl;
        
        // Test 4: Contract execution
        std::cout << "Test 4: Contract Execution... ";
        ContractExecutionResult execResult = scEngine->executeContract(
            deployResult.contractAddress, "set(42)", "user1", 100000);
        if (!execResult.success) {
            std::cout << "FAILED: " << execResult.errorMessage << std::endl;
            return false;
        }
        std::cout << "PASSED (Gas used: " << execResult.gasUsed << ")" << std::endl;
        
        // Test 5: Transaction validation
        std::cout << "Test 5: Transaction Validation... ";
        Transaction contractTx("user1", deployResult.contractAddress, 0.0);
        if (!scEngine->validateTransaction(contractTx)) {
            std::cout << "FAILED" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 6: Block validation
        std::cout << "Test 6: Block Validation... ";
        Block testBlock(1, "previous_hash");
        testBlock.addTransaction(contractTx);
        if (!scEngine->validateBlock(testBlock)) {
            std::cout << "FAILED" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 7: Governance contract registration
        std::cout << "Test 7: Governance Contract Registration... ";
        scEngine->registerGovernanceContract(deployResult.contractAddress);
        if (!scEngine->isGovernanceContract(deployResult.contractAddress)) {
            std::cout << "FAILED" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 8: Parameter adjustment
        std::cout << "Test 8: Parameter Adjustment... ";
        std::map<std::string, double> params = {
            {"maxGasLimit", 5000000.0},
            {"minValidationConfidence", 0.8}
        };
        if (!scEngine->adjustParameters(params)) {
            std::cout << "FAILED" << std::endl;
            return false;
        }
        auto retrievedParams = scEngine->getParameters();
        if (retrievedParams["maxGasLimit"] != 5000000.0) {
            std::cout << "FAILED (Parameter not updated)" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 9: Status and metrics
        std::cout << "Test 9: Status and Metrics... ";
        nlohmann::json status = scEngine->getStatus();
        nlohmann::json metrics = scEngine->getMetrics();
        if (!status["initialized"].get<bool>() || !status["healthy"].get<bool>()) {
            std::cout << "FAILED (Invalid status)" << std::endl;
            return false;
        }
        if (metrics["contracts"]["total"].get<int>() < 1) {
            std::cout << "FAILED (Invalid metrics)" << std::endl;
            return false;
        }
        std::cout << "PASSED" << std::endl;
        
        // Test 10: Security validation
        std::cout << "Test 10: Security Validation... ";
        std::string unsafeContract = "contract Unsafe { function bad() { eval('malicious'); } }";
        std::vector<std::string> warnings = scEngine->getSecurityWarnings(unsafeContract);
        if (warnings.empty()) {
            std::cout << "FAILED (No security warnings for unsafe contract)" << std::endl;
            return false;
        }
        std::cout << "PASSED (Found " << warnings.size() << " warnings)" << std::endl;
        
        return true;
    }
    
    void printSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Engine Type: " << scEngine->getName() << std::endl;
        std::cout << "Total Executions: " << scEngine->getTotalExecutions() << std::endl;
        std::cout << "Successful Executions: " << scEngine->getSuccessfulExecutions() << std::endl;
        std::cout << "Success Rate: " << (scEngine->getSuccessRate() * 100) << "%" << std::endl;
        
        nlohmann::json status = scEngine->getStatus();
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Max Gas Limit: " << status["configuration"]["maxGasLimit"] << std::endl;
        std::cout << "  Max Contract Size: " << status["configuration"]["maxContractSize"] << std::endl;
        std::cout << "  Min Validation Confidence: " << status["configuration"]["minValidationConfidence"] << std::endl;
        std::cout << "  Multi-Consensus Required: " << status["configuration"]["requireMultiConsensus"] << std::endl;
        std::cout << "  Governance Validation: " << status["configuration"]["enableGovernanceValidation"] << std::endl;
    }
    
    ~SmartContractIntegrationTest() {
        if (scEngine) {
            scEngine->shutdown();
        }
    }
};

int main() {
    try {
        SmartContractIntegrationTest test;
        
        bool success = test.runBasicTests();
        
        test.printSummary();
        
        if (success) {
            std::cout << "\n✅ All tests PASSED!" << std::endl;
            return 0;
        } else {
            std::cout << "\n❌ Some tests FAILED!" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
        return 1;
    }
}