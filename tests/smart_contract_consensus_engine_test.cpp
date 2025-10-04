#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/core/smart_contract_consensus_engine.h"
#include "../include/core/blockchain.h"
#include "../include/core/voting_consensus_engine.h"
#include "../include/core/logger.h"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Mock consensus engine for testing multi-consensus validation
class MockConsensusEngine : public ConsensusEngine {
private:
    ConsensusType type;
    bool healthy;
    bool validationResult;

public:
    MockConsensusEngine(ConsensusType t, bool h = true, bool valid = true) 
        : type(t), healthy(h), validationResult(valid) {}

    MOCK_METHOD(bool, validateBlock, (const Block& block), (override));
    MOCK_METHOD(bool, validateTransaction, (const Transaction& transaction), (override));
    MOCK_METHOD(ConsensusResult, processRequest, (const ConsensusRequest& request), (override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(bool, adjustParameters, (const std::map<std::string, double>& parameters), (override));
    MOCK_METHOD(std::map<std::string, double>, getParameters, (), (const, override));
    MOCK_METHOD(nlohmann::json, getStatus, (), (const, override));
    MOCK_METHOD(nlohmann::json, getMetrics, (), (const, override));

    bool isHealthy() const override { return healthy; }
    ConsensusType getType() const override { return type; }
    std::string getName() const override { return "MockConsensusEngine"; }
    
    void setHealthy(bool h) { healthy = h; }
    void setValidationResult(bool result) { validationResult = result; }
    
    // Default implementation for processRequest
    ConsensusResult defaultProcessRequest(const ConsensusRequest& request) {
        return ConsensusResult(validationResult, type, validationResult ? 1.0 : 0.0, 
                             validationResult ? "Mock validation passed" : "Mock validation failed");
    }
};

class SmartContractConsensusEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for testing
        Logger::setLogLevel(Logger::LogLevel::INFO);
        
        // Create blockchain instance
        blockchain = std::make_unique<Blockchain>();
        
        // Create voting engine
        votingEngine = std::make_unique<VotingConsensusEngine>(blockchain.get());
        votingEngine->initialize();
        
        // Create smart contract consensus engine
        scEngine = std::make_unique<SmartContractConsensusEngine>(blockchain.get(), votingEngine.get());
        
        // Create mock consensus engines
        mockPoWEngine = std::make_unique<NiceMock<MockConsensusEngine>>(ConsensusType::PROOF_OF_WORK);
        mockPoSEngine = std::make_unique<NiceMock<MockConsensusEngine>>(ConsensusType::PROOF_OF_STAKE);
        mockPoRCEngine = std::make_unique<NiceMock<MockConsensusEngine>>(ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION);
        
        // Set up default mock behaviors
        ON_CALL(*mockPoWEngine, processRequest(_))
            .WillByDefault([this](const ConsensusRequest& request) {
                return mockPoWEngine->defaultProcessRequest(request);
            });
        ON_CALL(*mockPoSEngine, processRequest(_))
            .WillByDefault([this](const ConsensusRequest& request) {
                return mockPoSEngine->defaultProcessRequest(request);
            });
        ON_CALL(*mockPoRCEngine, processRequest(_))
            .WillByDefault([this](const ConsensusRequest& request) {
                return mockPoRCEngine->defaultProcessRequest(request);
            });
        
        ON_CALL(*mockPoWEngine, initialize()).WillByDefault(Return(true));
        ON_CALL(*mockPoSEngine, initialize()).WillByDefault(Return(true));
        ON_CALL(*mockPoRCEngine, initialize()).WillByDefault(Return(true));
        
        ON_CALL(*mockPoWEngine, validateTransaction(_)).WillByDefault(Return(true));
        ON_CALL(*mockPoSEngine, validateTransaction(_)).WillByDefault(Return(true));
        ON_CALL(*mockPoRCEngine, validateTransaction(_)).WillByDefault(Return(true));
        
        ON_CALL(*mockPoWEngine, validateBlock(_)).WillByDefault(Return(true));
        ON_CALL(*mockPoSEngine, validateBlock(_)).WillByDefault(Return(true));
        ON_CALL(*mockPoRCEngine, validateBlock(_)).WillByDefault(Return(true));
    }

    void TearDown() override {
        if (scEngine) {
            scEngine->shutdown();
        }
        if (votingEngine) {
            votingEngine->shutdown();
        }
    }

    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<VotingConsensusEngine> votingEngine;
    std::unique_ptr<SmartContractConsensusEngine> scEngine;
    std::unique_ptr<NiceMock<MockConsensusEngine>> mockPoWEngine;
    std::unique_ptr<NiceMock<MockConsensusEngine>> mockPoSEngine;
    std::unique_ptr<NiceMock<MockConsensusEngine>> mockPoRCEngine;
};

// Test 1: Basic engine initialization and configuration
TEST_F(SmartContractConsensusEngineTest, InitializationAndConfiguration) {
    EXPECT_TRUE(scEngine->initialize());
    EXPECT_TRUE(scEngine->isHealthy());
    EXPECT_EQ(scEngine->getType(), ConsensusType::SMART_CONTRACT_VALIDATION);
    EXPECT_EQ(scEngine->getName(), "SmartContractConsensusEngine");
    
    // Test parameter adjustment
    std::map<std::string, double> params = {
        {"maxGasLimit", 5000000.0},
        {"maxContractSize", 12288.0},
        {"minValidationConfidence", 0.8}
    };
    
    EXPECT_TRUE(scEngine->adjustParameters(params));
    
    auto retrievedParams = scEngine->getParameters();
    EXPECT_EQ(retrievedParams["maxGasLimit"], 5000000.0);
    EXPECT_EQ(retrievedParams["maxContractSize"], 12288.0);
    EXPECT_EQ(retrievedParams["minValidationConfidence"], 0.8);
}

// Test 2: Contract deployment validation
TEST_F(SmartContractConsensusEngineTest, ContractDeploymentValidation) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Test valid contract deployment
    std::string validContract = "contract SimpleStorage { uint256 value; function set(uint256 v) { value = v; } }";
    Transaction deployTx("deployer1", validContract);
    
    EXPECT_TRUE(scEngine->validateTransaction(deployTx));
    
    // Test contract deployment through engine
    ContractExecutionResult result = scEngine->deployContract(validContract, "deployer1");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.contractAddress.empty());
    
    // Test invalid contract (too large)
    std::string largeContract(50000, 'x'); // 50KB contract
    Transaction largeTx("deployer2", largeContract);
    
    ContractExecutionResult largeResult = scEngine->deployContract(largeContract, "deployer2");
    EXPECT_FALSE(largeResult.success);
    EXPECT_THAT(largeResult.errorMessage, ::testing::HasSubstr("size"));
}

// Test 3: Multi-consensus validation for contract state changes
TEST_F(SmartContractConsensusEngineTest, MultiConsensusStateValidation) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Add validation engines
    scEngine->addValidationEngine(mockPoWEngine.get());
    scEngine->addValidationEngine(mockPoSEngine.get());
    scEngine->addValidationEngine(mockPoRCEngine.get());
    
    // Deploy a contract first
    std::string contractCode = "contract TestContract { uint256 value; }";
    ContractExecutionResult deployResult = scEngine->deployContract(contractCode, "deployer1");
    EXPECT_TRUE(deployResult.success);
    
    std::string contractAddress = deployResult.contractAddress;
    
    // Test state change validation with all engines agreeing
    Transaction stateTx("user1", contractAddress, 0.0);
    StateChangeValidation validation = scEngine->validateStateChange(
        contractAddress, "{}", "{\"value\": 42}", stateTx);
    
    EXPECT_TRUE(validation.isValid);
    EXPECT_EQ(validation.validatedBy.size(), 3); // All three engines should validate
    EXPECT_GE(validation.confidence, 0.5);
    
    // Test state change validation with one engine disagreeing
    mockPoWEngine->setValidationResult(false);
    
    StateChangeValidation validation2 = scEngine->validateStateChange(
        contractAddress, "{\"value\": 42}", "{\"value\": 100}", stateTx);
    
    EXPECT_TRUE(validation2.isValid); // Should still pass with majority consensus
    EXPECT_EQ(validation2.validatedBy.size(), 2); // Only two engines should validate
}

// Test 4: Contract-governance interaction validation
TEST_F(SmartContractConsensusEngineTest, GovernanceInteractionValidation) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Register a governance contract
    std::string govContractAddress = "GOVERNANCE_CONTRACT_001";
    scEngine->registerGovernanceContract(govContractAddress);
    EXPECT_TRUE(scEngine->isGovernanceContract(govContractAddress));
    
    // Test governance proposal transaction
    Transaction proposalTx("proposer1", "PROPOSAL_PARAM_CHANGE", 0.0);
    ContractGovernanceInteraction interaction = scEngine->analyzeGovernanceInteraction(proposalTx);
    
    EXPECT_EQ(interaction.type, GovernanceInteractionType::PARAMETER_PROPOSAL);
    EXPECT_TRUE(scEngine->validateGovernanceInteraction(interaction));
    
    // Test voting participation transaction
    Transaction voteTx("voter1", "VOTE_PROPOSAL_001", 1.0);
    ContractGovernanceInteraction voteInteraction = scEngine->analyzeGovernanceInteraction(voteTx);
    
    EXPECT_EQ(voteInteraction.type, GovernanceInteractionType::VOTING_PARTICIPATION);
    EXPECT_TRUE(scEngine->validateGovernanceInteraction(voteInteraction));
    
    // Test consensus rule change (should require supermajority)
    Transaction consensusTx("admin1", "CONSENSUS_RULE_CHANGE", 0.0);
    ContractGovernanceInteraction consensusInteraction = scEngine->analyzeGovernanceInteraction(consensusTx);
    
    EXPECT_EQ(consensusInteraction.type, GovernanceInteractionType::CONSENSUS_RULE_CHANGE);
    EXPECT_TRUE(consensusInteraction.requiresSupermajority);
}

// Test 5: Contract execution with gas limits and security
TEST_F(SmartContractConsensusEngineTest, ContractExecutionSecurity) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Deploy a simple contract
    std::string contractCode = "contract Counter { uint256 count; function increment() { count++; } }";
    ContractExecutionResult deployResult = scEngine->deployContract(contractCode, "deployer1");
    EXPECT_TRUE(deployResult.success);
    
    std::string contractAddress = deployResult.contractAddress;
    
    // Test normal execution
    ContractExecutionResult execResult = scEngine->executeContract(contractAddress, "increment()", "user1", 100000);
    EXPECT_TRUE(execResult.success);
    EXPECT_LE(execResult.gasUsed, 100000);
    
    // Test execution with insufficient gas
    ContractExecutionResult lowGasResult = scEngine->executeContract(contractAddress, "increment()", "user1", 10);
    // This might succeed or fail depending on the actual gas usage, but should not crash
    
    // Test execution of non-existent contract
    ContractExecutionResult nonExistentResult = scEngine->executeContract("NON_EXISTENT", "test()", "user1");
    EXPECT_FALSE(nonExistentResult.success);
    EXPECT_THAT(nonExistentResult.errorMessage, ::testing::HasSubstr("not found"));
}

// Test 6: Block validation with contract transactions
TEST_F(SmartContractConsensusEngineTest, BlockValidationWithContracts) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Create a block with contract transactions
    Block testBlock(1, "previous_hash");
    
    // Add contract deployment transaction
    Transaction deployTx("deployer1", "contract Test { uint256 value; }");
    testBlock.addTransaction(deployTx);
    
    // Add regular transaction (should be ignored by smart contract engine)
    Transaction regularTx("user1", "user2", 10.0);
    testBlock.addTransaction(regularTx);
    
    // Add contract execution transaction
    Transaction contractTx("user1", "CONTRACT_123", 0.0);
    testBlock.addTransaction(contractTx);
    
    // Validate the block
    EXPECT_TRUE(scEngine->validateBlock(testBlock));
}

// Test 7: Security warnings and contract safety
TEST_F(SmartContractConsensusEngineTest, SecurityWarningsAndSafety) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Test contract with security warnings
    std::string unsafeContract = R"(
        contract UnsafeContract {
            function dangerousFunction() {
                eval("malicious code");
                while(true) { /* infinite loop */ }
            }
        }
    )";
    
    std::vector<std::string> warnings = scEngine->getSecurityWarnings(unsafeContract);
    EXPECT_GT(warnings.size(), 0);
    
    // Should contain warnings about eval and infinite loop
    bool hasEvalWarning = false;
    bool hasLoopWarning = false;
    for (const auto& warning : warnings) {
        if (warning.find("eval") != std::string::npos) hasEvalWarning = true;
        if (warning.find("loop") != std::string::npos) hasLoopWarning = true;
    }
    EXPECT_TRUE(hasEvalWarning);
    EXPECT_TRUE(hasLoopWarning);
    
    // Test safe contract
    std::string safeContract = "contract SafeContract { uint256 value; function setValue(uint256 v) { value = v; } }";
    std::vector<std::string> safeWarnings = scEngine->getSecurityWarnings(safeContract);
    EXPECT_EQ(safeWarnings.size(), 0);
}

// Test 8: Engine status and metrics
TEST_F(SmartContractConsensusEngineTest, StatusAndMetrics) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Get initial status
    nlohmann::json status = scEngine->getStatus();
    EXPECT_TRUE(status["initialized"].get<bool>());
    EXPECT_TRUE(status["healthy"].get<bool>());
    EXPECT_EQ(status["type"].get<std::string>(), "SMART_CONTRACT_VALIDATION");
    
    // Deploy some contracts to change metrics
    scEngine->deployContract("contract Test1 { }", "deployer1");
    scEngine->deployContract("contract Test2 { }", "deployer2");
    
    // Register a governance contract
    scEngine->registerGovernanceContract("GOV_CONTRACT_001");
    
    // Get updated metrics
    nlohmann::json metrics = scEngine->getMetrics();
    EXPECT_EQ(metrics["contracts"]["total"].get<int>(), 2);
    EXPECT_EQ(metrics["contracts"]["governance"].get<int>(), 1);
    EXPECT_GE(metrics["executions"]["total"].get<uint64_t>(), 2);
}

// Test 9: Multi-consensus requirement scenarios
TEST_F(SmartContractConsensusEngineTest, MultiConsensusRequirements) {
    EXPECT_TRUE(scEngine->initialize());
    
    // Add validation engines
    scEngine->addValidationEngine(mockPoWEngine.get());
    scEngine->addValidationEngine(mockPoSEngine.get());
    
    // Deploy a regular contract
    ContractExecutionResult deployResult = scEngine->deployContract("contract Regular { }", "deployer1");
    EXPECT_TRUE(deployResult.success);
    std::string regularContract = deployResult.contractAddress;
    
    // Deploy and register a governance contract
    ContractExecutionResult govDeployResult = scEngine->deployContract("contract Governance { }", "deployer2");
    EXPECT_TRUE(govDeployResult.success);
    std::string govContract = govDeployResult.contractAddress;
    scEngine->registerGovernanceContract(govContract);
    
    // Regular contract should follow global multi-consensus setting
    EXPECT_TRUE(scEngine->requiresMultiConsensusValidation(regularContract));
    
    // Governance contract should always require multi-consensus
    EXPECT_TRUE(scEngine->requiresMultiConsensusValidation(govContract));
    
    // Test removing validation engines
    scEngine->removeValidationEngine(mockPoWEngine.get());
    scEngine->removeValidationEngine(mockPoSEngine.get());
    
    // Should still require validation for governance contracts
    EXPECT_TRUE(scEngine->requiresMultiConsensusValidation(govContract));
}

// Test 10: Error handling and edge cases
TEST_F(SmartContractConsensusEngineTest, ErrorHandlingAndEdgeCases) {
    // Test operations before initialization
    EXPECT_FALSE(scEngine->validateTransaction(Transaction("user1", "user2", 10.0)));
    EXPECT_FALSE(scEngine->validateBlock(Block(1, "hash")));
    
    // Initialize engine
    EXPECT_TRUE(scEngine->initialize());
    
    // Test with null/empty inputs
    ContractExecutionResult emptyResult = scEngine->deployContract("", "deployer1");
    EXPECT_FALSE(emptyResult.success);
    
    ContractExecutionResult nonExistentExec = scEngine->executeContract("", "input", "sender");
    EXPECT_FALSE(nonExistentExec.success);
    
    // Test state operations with invalid addresses
    EXPECT_EQ(scEngine->getContractState("INVALID_ADDRESS"), "");
    EXPECT_TRUE(scEngine->setContractState("TEST_ADDRESS", "{\"test\": true}"));
    EXPECT_EQ(scEngine->getContractState("TEST_ADDRESS"), "{\"test\": true}");
    
    // Test governance operations with invalid data
    ContractGovernanceInteraction invalidInteraction;
    invalidInteraction.type = GovernanceInteractionType::PARAMETER_PROPOSAL;
    // Empty parameters should make validation fail
    EXPECT_FALSE(scEngine->validateGovernanceInteraction(invalidInteraction));
    
    // Test shutdown and operations after shutdown
    scEngine->shutdown();
    EXPECT_FALSE(scEngine->isHealthy());
    EXPECT_FALSE(scEngine->validateTransaction(Transaction("user1", "user2", 10.0)));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}