#include "../../include/core/smart_contract_consensus_engine.h"
#include "../../include/core/blockchain.h"
#include "../../include/core/voting_consensus_engine.h"
#include "../../include/core/logger.h"
#include "../../include/core/utils.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>

SmartContractConsensusEngine::SmartContractConsensusEngine(Blockchain* bc, VotingConsensusEngine* voting)
    : blockchain(bc), votingEngine(voting), maxGasLimit(10000000), maxContractSize(24576),
      minValidationConfidence(0.67), requireMultiConsensus(true), enableGovernanceValidation(true),
      initialized(false), lastUpdate(std::chrono::steady_clock::now()),
      totalExecutions(0), successfulExecutions(0), failedExecutions(0) {
    
    vm = std::make_unique<SmartContractVM>();
}

bool SmartContractConsensusEngine::initialize() {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    if (initialized) {
        return true;
    }
    
    try {
        // Initialize the smart contract VM
        if (!vm) {
            vm = std::make_unique<SmartContractVM>();
        }
        
        // Set default configuration
        maxGasLimit = 10000000;  // 10M gas limit
        maxContractSize = 24576; // 24KB contract size limit
        minValidationConfidence = 0.67;
        requireMultiConsensus = true;
        enableGovernanceValidation = true;
        
        // Clear any existing state
        contractStates.clear();
        contractBytecodes.clear();
        contractSources.clear();
        stateValidations.clear();
        governanceInteractions.clear();
        governanceContracts.clear();
        
        // Reset statistics
        totalExecutions = 0;
        successfulExecutions = 0;
        failedExecutions = 0;
        
        initialized = true;
        lastUpdate = std::chrono::steady_clock::now();
        
        Logger::info("SmartContractConsensusEngine initialized successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize SmartContractConsensusEngine: " + std::string(e.what()));
        return false;
    }
}

void SmartContractConsensusEngine::shutdown() {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    if (!initialized) {
        return;
    }
    
    // Clear all state
    contractStates.clear();
    contractBytecodes.clear();
    contractSources.clear();
    stateValidations.clear();
    governanceInteractions.clear();
    governanceContracts.clear();
    validationEngines.clear();
    
    // Reset VM
    vm.reset();
    
    initialized = false;
    Logger::info("SmartContractConsensusEngine shutdown completed");
}

bool SmartContractConsensusEngine::isHealthy() const {
    if (!initialized || !vm) {
        return false;
    }
    
    // Check if we have recent activity (within last hour)
    auto now = std::chrono::steady_clock::now();
    auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count();
    
    // Consider healthy if initialized and no critical errors
    return timeSinceUpdate < 3600; // 1 hour
}

bool SmartContractConsensusEngine::validateBlock(const Block& block) {
    if (!initialized) {
        return false;
    }
    
    try {
        // Validate all smart contract transactions in the block
        auto transactions = block.getTransactions();
        for (const auto& tx : transactions) {
            if (isContractTransaction(tx) && !validateTransaction(tx)) {
                Logger::warning("SmartContractConsensusEngine: Invalid contract transaction in block");
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine block validation error: " + std::string(e.what()));
        return false;
    }
}

bool SmartContractConsensusEngine::validateTransaction(const Transaction& transaction) {
    if (!initialized) {
        return false;
    }
    
    try {
        // Check if this is a contract-related transaction
        if (!isContractTransaction(transaction)) {
            return true; // Let other engines handle non-contract transactions
        }
        
        // Validate contract deployment
        if (isContractDeployment(transaction)) {
            return processContractDeployment(transaction);
        }
        
        // Validate contract execution
        return processContractTransaction(transaction);
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine transaction validation error: " + std::string(e.what()));
        return false;
    }
}

ConsensusResult SmartContractConsensusEngine::processRequest(const ConsensusRequest& request) {
    if (!initialized) {
        return ConsensusResult(false, ConsensusType::SMART_CONTRACT_VALIDATION, 0.0, "Engine not initialized");
    }
    
    try {
        switch (request.type) {
            case RequestType::BLOCK_VALIDATION: {
                Block block = Block::deserialize(request.data);
                bool isValid = validateBlock(block);
                return ConsensusResult(isValid, ConsensusType::SMART_CONTRACT_VALIDATION,
                                     isValid ? 1.0 : 0.0,
                                     isValid ? "Block validation passed" : "Block validation failed");
            }
            
            case RequestType::TRANSACTION_VALIDATION: {
                Transaction tx = Transaction::deserialize(request.data);
                bool isValid = validateTransaction(tx);
                return ConsensusResult(isValid, ConsensusType::SMART_CONTRACT_VALIDATION,
                                     isValid ? 1.0 : 0.0,
                                     isValid ? "Transaction validation passed" : "Transaction validation failed");
            }
            
            case RequestType::SMART_CONTRACT_EXECUTION: {
                nlohmann::json contractData = nlohmann::json::parse(request.data);
                std::string contractAddress = contractData["contractAddress"];
                std::string input = contractData["input"];
                std::string sender = contractData["sender"];
                uint64_t gasLimit = contractData.value("gasLimit", maxGasLimit);
                
                ContractExecutionResult result = executeContract(contractAddress, input, sender, gasLimit);
                return ConsensusResult(result.success, ConsensusType::SMART_CONTRACT_VALIDATION,
                                     result.success ? 1.0 : 0.0,
                                     result.success ? "Contract execution successful" : result.errorMessage);
            }
            
            default:
                return ConsensusResult(true, ConsensusType::SMART_CONTRACT_VALIDATION, 1.0, 
                                     "Request type not handled by smart contract consensus");
        }
    } catch (const std::exception& e) {
        return ConsensusResult(false, ConsensusType::SMART_CONTRACT_VALIDATION, 0.0,
                             "Error processing request: " + std::string(e.what()));
    }
}

nlohmann::json SmartContractConsensusEngine::getStatus() const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    nlohmann::json status;
    status["initialized"] = initialized;
    status["healthy"] = initialized && vm != nullptr;
    status["type"] = "SMART_CONTRACT_VALIDATION";
    status["name"] = getName();
    
    status["configuration"] = {
        {"maxGasLimit", maxGasLimit},
        {"maxContractSize", maxContractSize},
        {"minValidationConfidence", minValidationConfidence},
        {"requireMultiConsensus", requireMultiConsensus},
        {"enableGovernanceValidation", enableGovernanceValidation}
    };
    
    status["statistics"] = {
        {"totalContracts", contractStates.size()},
        {"governanceContracts", governanceContracts.size()},
        {"validationEngines", validationEngines.size()},
        {"totalExecutions", totalExecutions},
        {"successfulExecutions", successfulExecutions},
        {"failedExecutions", failedExecutions},
        {"successRate", getSuccessRate()}
    };
    
    return status;
}

nlohmann::json SmartContractConsensusEngine::getMetrics() const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    nlohmann::json metrics;
    
    metrics["contracts"] = {
        {"total", contractStates.size()},
        {"governance", governanceContracts.size()},
        {"active", contractStates.size()} // All stored contracts are considered active
    };
    
    metrics["executions"] = {
        {"total", totalExecutions},
        {"successful", successfulExecutions},
        {"failed", failedExecutions},
        {"success_rate", getSuccessRate()}
    };
    
    metrics["validation"] = {
        {"engines", validationEngines.size()},
        {"multi_consensus_required", requireMultiConsensus},
        {"governance_validation_enabled", enableGovernanceValidation},
        {"min_confidence", minValidationConfidence}
    };
    
    return metrics;
}

bool SmartContractConsensusEngine::adjustParameters(const std::map<std::string, double>& parameters) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    try {
        for (const auto& [param, value] : parameters) {
            if (param == "maxGasLimit") {
                if (value > 0 && value <= 100000000) { // Max 100M gas
                    maxGasLimit = static_cast<uint64_t>(value);
                    Logger::info("SmartContractConsensusEngine: Updated maxGasLimit to " + std::to_string(value));
                }
            } else if (param == "maxContractSize") {
                if (value > 0 && value <= 1048576) { // Max 1MB
                    maxContractSize = static_cast<uint64_t>(value);
                    Logger::info("SmartContractConsensusEngine: Updated maxContractSize to " + std::to_string(value));
                }
            } else if (param == "minValidationConfidence") {
                if (value >= 0.0 && value <= 1.0) {
                    minValidationConfidence = value;
                    Logger::info("SmartContractConsensusEngine: Updated minValidationConfidence to " + std::to_string(value));
                }
            } else if (param == "requireMultiConsensus") {
                requireMultiConsensus = (value > 0.5);
                Logger::info("SmartContractConsensusEngine: Updated requireMultiConsensus to " + std::to_string(requireMultiConsensus));
            } else if (param == "enableGovernanceValidation") {
                enableGovernanceValidation = (value > 0.5);
                Logger::info("SmartContractConsensusEngine: Updated enableGovernanceValidation to " + std::to_string(enableGovernanceValidation));
            }
        }
        
        updateLastActivity();
        return true;
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine parameter adjustment error: " + std::string(e.what()));
        return false;
    }
}

std::map<std::string, double> SmartContractConsensusEngine::getParameters() const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    return {
        {"maxGasLimit", static_cast<double>(maxGasLimit)},
        {"maxContractSize", static_cast<double>(maxContractSize)},
        {"minValidationConfidence", minValidationConfidence},
        {"requireMultiConsensus", requireMultiConsensus ? 1.0 : 0.0},
        {"enableGovernanceValidation", enableGovernanceValidation ? 1.0 : 0.0}
    };
}

ContractExecutionResult SmartContractConsensusEngine::executeContract(const std::string& contractAddress,
                                                                     const std::string& input,
                                                                     const std::string& sender,
                                                                     uint64_t gasLimit) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    if (!initialized || !vm) {
        return ContractExecutionResult(false, contractAddress, "", 0, {}, "Engine not initialized");
    }
    
    try {
        // Check if contract exists
        if (contractBytecodes.find(contractAddress) == contractBytecodes.end()) {
            updateExecutionStats(false);
            return ContractExecutionResult(false, contractAddress, "", 0, {}, "Contract not found");
        }
        
        // Get contract bytecode and current state
        std::vector<uint8_t> bytecode = contractBytecodes[contractAddress];
        std::string currentState = getContractState(contractAddress);
        
        // Validate gas limit
        gasLimit = std::min(gasLimit, maxGasLimit);
        
        // Create execution context
        SmartContractContext context;
        context.sender = sender;
        context.contractAddress = contractAddress;
        context.gasLimit = gasLimit;
        context.gasUsed = 0;
        
        // Parse current state into storage
        if (!currentState.empty()) {
            try {
                nlohmann::json stateJson = nlohmann::json::parse(currentState);
                for (auto& [key, value] : stateJson.items()) {
                    if (value.is_string()) {
                        context.storage[key] = Value(value.get<std::string>());
                    } else if (value.is_number_integer()) {
                        context.storage[key] = Value(value.get<int64_t>());
                    } else if (value.is_number_float()) {
                        context.storage[key] = Value(value.get<double>());
                    } else if (value.is_boolean()) {
                        context.storage[key] = Value(value.get<bool>());
                    }
                }
            } catch (const std::exception& e) {
                Logger::warning("SmartContractConsensusEngine: Failed to parse contract state: " + std::string(e.what()));
            }
        }
        
        // Execute the contract
        bool executionSuccess = vm->executeSecure(context, bytecode);
        
        if (!executionSuccess) {
            updateExecutionStats(false);
            return ContractExecutionResult(false, contractAddress, currentState, context.gasUsed, 
                                         context.logs, "Contract execution failed");
        }
        
        // Convert storage back to JSON state
        nlohmann::json newStateJson;
        for (const auto& [key, value] : context.storage) {
            if (std::holds_alternative<std::string>(value)) {
                newStateJson[key] = std::get<std::string>(value);
            } else if (std::holds_alternative<int64_t>(value)) {
                newStateJson[key] = std::get<int64_t>(value);
            } else if (std::holds_alternative<double>(value)) {
                newStateJson[key] = std::get<double>(value);
            } else if (std::holds_alternative<bool>(value)) {
                newStateJson[key] = std::get<bool>(value);
            }
        }
        
        std::string newState = newStateJson.dump();
        
        // Validate state change if multi-consensus is required
        if (requireMultiConsensus && !validationEngines.empty()) {
            Transaction dummyTx(sender, contractAddress, 0.0);
            StateChangeValidation validation = validateStateChange(contractAddress, currentState, newState, dummyTx);
            
            if (!validation.isValid || validation.confidence < minValidationConfidence) {
                updateExecutionStats(false);
                return ContractExecutionResult(false, contractAddress, currentState, context.gasUsed,
                                             context.logs, "State change validation failed: " + validation.reason);
            }
        }
        
        // Update contract state
        setContractState(contractAddress, newState);
        
        updateExecutionStats(true);
        logContractEvent("contract_executed", {
            {"contractAddress", contractAddress},
            {"sender", sender},
            {"gasUsed", context.gasUsed},
            {"success", true}
        });
        
        return ContractExecutionResult(true, contractAddress, newState, context.gasUsed, context.logs);
        
    } catch (const std::exception& e) {
        updateExecutionStats(false);
        Logger::error("SmartContractConsensusEngine contract execution error: " + std::string(e.what()));
        return ContractExecutionResult(false, contractAddress, "", 0, {}, 
                                     "Execution error: " + std::string(e.what()));
    }
}

ContractExecutionResult SmartContractConsensusEngine::deployContract(const std::string& sourceCode,
                                                                    const std::string& deployer,
                                                                    uint64_t gasLimit) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    if (!initialized || !vm) {
        return ContractExecutionResult(false, "", "", 0, {}, "Engine not initialized");
    }
    
    try {
        // Validate contract code
        if (!validateContractCode(sourceCode)) {
            updateExecutionStats(false);
            return ContractExecutionResult(false, "", "", 0, {}, "Contract code validation failed");
        }
        
        // Check contract size
        if (!checkContractSize(sourceCode)) {
            updateExecutionStats(false);
            return ContractExecutionResult(false, "", "", 0, {}, "Contract size exceeds limit");
        }
        
        // Generate contract address
        static uint64_t deploymentNonce = 0;
        std::string contractAddress = generateContractAddress(deployer, ++deploymentNonce);
        
        // Compile contract to bytecode
        std::vector<uint8_t> bytecode = vm->compileContract(sourceCode);
        if (bytecode.empty()) {
            updateExecutionStats(false);
            return ContractExecutionResult(false, "", "", 0, {}, "Contract compilation failed");
        }
        
        // Store contract
        contractBytecodes[contractAddress] = bytecode;
        contractSources[contractAddress] = sourceCode;
        contractStates[contractAddress] = "{}"; // Empty initial state
        
        updateExecutionStats(true);
        logContractEvent("contract_deployed", {
            {"contractAddress", contractAddress},
            {"deployer", deployer},
            {"codeSize", sourceCode.length()}
        });
        
        return ContractExecutionResult(true, contractAddress, "{}", 0, {});
        
    } catch (const std::exception& e) {
        updateExecutionStats(false);
        Logger::error("SmartContractConsensusEngine contract deployment error: " + std::string(e.what()));
        return ContractExecutionResult(false, "", "", 0, {}, 
                                     "Deployment error: " + std::string(e.what()));
    }
}

StateChangeValidation SmartContractConsensusEngine::validateStateChange(const std::string& contractAddress,
                                                                       const std::string& oldState,
                                                                       const std::string& newState,
                                                                       const Transaction& transaction) {
    if (!requireMultiConsensus || validationEngines.empty()) {
        return StateChangeValidation(true, "Multi-consensus validation disabled", {}, 1.0);
    }
    
    try {
        std::vector<ConsensusResult> validationResults;
        std::vector<ConsensusType> validatedBy;
        
        // Create consensus request for state change validation
        ConsensusRequest request(RequestType::SMART_CONTRACT_EXECUTION, transaction.serialize());
        request.metadata["contractAddress"] = contractAddress;
        request.metadata["oldState"] = oldState;
        request.metadata["newState"] = newState;
        
        // Validate with each consensus engine
        for (ConsensusEngine* engine : validationEngines) {
            if (engine && engine->isHealthy()) {
                ConsensusResult result = engine->processRequest(request);
                validationResults.push_back(result);
                if (result.isValid) {
                    validatedBy.push_back(result.mechanism);
                }
            }
        }
        
        // Calculate overall confidence and validity
        double confidence = calculateValidationConfidence(validationResults);
        bool isValid = aggregateValidationResults(validationResults);
        
        std::string reason = isValid ? "State change validated by consensus engines" : 
                           "State change rejected by consensus engines";
        
        StateChangeValidation validation(isValid, reason, validatedBy, confidence);
        
        // Store validation result
        stateValidations[contractAddress + "_" + std::to_string(std::time(nullptr))] = validation;
        
        return validation;
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine state change validation error: " + std::string(e.what()));
        return StateChangeValidation(false, "Validation error: " + std::string(e.what()), {}, 0.0);
    }
}

bool SmartContractConsensusEngine::requiresMultiConsensusValidation(const std::string& contractAddress) const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    // Always require multi-consensus for governance contracts
    if (isGovernanceContract(contractAddress)) {
        return true;
    }
    
    // Check global setting
    return requireMultiConsensus;
}

void SmartContractConsensusEngine::addValidationEngine(ConsensusEngine* engine) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    if (engine && std::find(validationEngines.begin(), validationEngines.end(), engine) == validationEngines.end()) {
        validationEngines.push_back(engine);
        Logger::info("SmartContractConsensusEngine: Added validation engine: " + engine->getName());
    }
}

void SmartContractConsensusEngine::removeValidationEngine(ConsensusEngine* engine) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    auto it = std::find(validationEngines.begin(), validationEngines.end(), engine);
    if (it != validationEngines.end()) {
        validationEngines.erase(it);
        Logger::info("SmartContractConsensusEngine: Removed validation engine");
    }
}

bool SmartContractConsensusEngine::validateGovernanceInteraction(const ContractGovernanceInteraction& interaction) {
    if (!enableGovernanceValidation) {
        return true;
    }
    
    try {
        switch (interaction.type) {
            case GovernanceInteractionType::PARAMETER_PROPOSAL:
                return validateGovernanceProposal(interaction);
                
            case GovernanceInteractionType::VOTING_PARTICIPATION:
                return validateVotingParticipation(interaction);
                
            case GovernanceInteractionType::CONSENSUS_RULE_CHANGE:
                return validateConsensusRuleChange(interaction);
                
            case GovernanceInteractionType::EMERGENCY_ACTION:
                // Emergency actions require special validation
                return interaction.requiresSupermajority;
                
            case GovernanceInteractionType::NONE:
                return true;
                
            default:
                Logger::warning("SmartContractConsensusEngine: Unknown governance interaction type");
                return false;
        }
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine governance validation error: " + std::string(e.what()));
        return false;
    }
}

ContractGovernanceInteraction SmartContractConsensusEngine::analyzeGovernanceInteraction(const Transaction& transaction) {
    ContractGovernanceInteraction interaction;
    
    try {
        // Check if this is a governance contract transaction
        if (!isGovernanceContract(transaction.getRecipient())) {
            return interaction; // type remains NONE
        }
        
        interaction.contractAddress = transaction.getRecipient();
        interaction.type = detectGovernanceInteractionType(transaction);
        
        // Extract proposal ID if present
        std::string recipient = transaction.getRecipient();
        if (recipient.find("PROPOSAL_") != std::string::npos) {
            interaction.proposalId = recipient.substr(recipient.find("PROPOSAL_"));
        }
        
        // Determine if supermajority is required
        interaction.requiresSupermajority = (interaction.type == GovernanceInteractionType::CONSENSUS_RULE_CHANGE ||
                                           interaction.type == GovernanceInteractionType::EMERGENCY_ACTION);
        
        return interaction;
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine governance analysis error: " + std::string(e.what()));
        return interaction;
    }
}

bool SmartContractConsensusEngine::isGovernanceContract(const std::string& contractAddress) const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    return std::find(governanceContracts.begin(), governanceContracts.end(), contractAddress) != 
           governanceContracts.end();
}

void SmartContractConsensusEngine::registerGovernanceContract(const std::string& contractAddress) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    if (!isGovernanceContract(contractAddress)) {
        governanceContracts.push_back(contractAddress);
        Logger::info("SmartContractConsensusEngine: Registered governance contract: " + contractAddress);
    }
}

void SmartContractConsensusEngine::unregisterGovernanceContract(const std::string& contractAddress) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    auto it = std::find(governanceContracts.begin(), governanceContracts.end(), contractAddress);
    if (it != governanceContracts.end()) {
        governanceContracts.erase(it);
        Logger::info("SmartContractConsensusEngine: Unregistered governance contract: " + contractAddress);
    }
}

std::string SmartContractConsensusEngine::getContractState(const std::string& contractAddress) const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    auto it = contractStates.find(contractAddress);
    return (it != contractStates.end()) ? it->second : "";
}

bool SmartContractConsensusEngine::setContractState(const std::string& contractAddress, const std::string& state) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    try {
        contractStates[contractAddress] = state;
        updateLastActivity();
        return true;
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine set state error: " + std::string(e.what()));
        return false;
    }
}

std::vector<uint8_t> SmartContractConsensusEngine::getContractBytecode(const std::string& contractAddress) const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    auto it = contractBytecodes.find(contractAddress);
    return (it != contractBytecodes.end()) ? it->second : std::vector<uint8_t>();
}

bool SmartContractConsensusEngine::setContractBytecode(const std::string& contractAddress, 
                                                      const std::vector<uint8_t>& bytecode) {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    try {
        contractBytecodes[contractAddress] = bytecode;
        updateLastActivity();
        return true;
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine set bytecode error: " + std::string(e.what()));
        return false;
    }
}

bool SmartContractConsensusEngine::validateContractCode(const std::string& sourceCode) const {
    if (!vm) {
        return false;
    }
    
    return vm->validateContract(sourceCode);
}

bool SmartContractConsensusEngine::validateContractExecution(const std::string& contractAddress,
                                                           const SmartContractContext& context) const {
    try {
        // Check gas limits
        if (!checkGasLimits(context.gasUsed, context.gasLimit)) {
            return false;
        }
        
        // Check if contract exists
        if (contractBytecodes.find(contractAddress) == contractBytecodes.end()) {
            return false;
        }
        
        // Additional security checks
        return isContractSafe(contractAddress);
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine execution validation error: " + std::string(e.what()));
        return false;
    }
}

bool SmartContractConsensusEngine::isContractSafe(const std::string& contractAddress) const {
    std::lock_guard<std::mutex> lock(contractMutex);
    
    try {
        // Check if we have the source code for security analysis
        auto sourceIt = contractSources.find(contractAddress);
        if (sourceIt == contractSources.end()) {
            Logger::warning("SmartContractConsensusEngine: No source code available for security check: " + contractAddress);
            return true; // Assume safe if we can't check
        }
        
        return checkContractSecurity(sourceIt->second);
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine safety check error: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> SmartContractConsensusEngine::getSecurityWarnings(const std::string& sourceCode) const {
    std::vector<std::string> warnings;
    
    try {
        // Check for dangerous patterns
        std::vector<std::pair<std::string, std::string>> dangerousPatterns = {
            {"eval\\(", "Use of eval() function"},
            {"exec\\(", "Use of exec() function"},
            {"system\\(", "System call detected"},
            {"while\\s*\\(\\s*true\\s*\\)", "Potential infinite loop"},
            {"for\\s*\\(\\s*;;\\s*\\)", "Potential infinite loop"},
            {"delete\\s+", "Use of delete operator"},
            {"new\\s+", "Dynamic memory allocation"}
        };
        
        for (const auto& [pattern, warning] : dangerousPatterns) {
            std::regex regex(pattern, std::regex_constants::icase);
            if (std::regex_search(sourceCode, regex)) {
                warnings.push_back(warning);
            }
        }
        
        // Check contract size
        if (sourceCode.length() > maxContractSize) {
            warnings.push_back("Contract size exceeds recommended limit");
        }
        
        // Check for excessive complexity
        size_t functionCount = std::count(sourceCode.begin(), sourceCode.end(), '{');
        if (functionCount > 50) {
            warnings.push_back("Contract has high complexity (many functions)");
        }
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine security analysis error: " + std::string(e.what()));
        warnings.push_back("Security analysis failed");
    }
    
    return warnings;
}

bool SmartContractConsensusEngine::enforceSecurityPolicies(const std::string& contractAddress) const {
    try {
        // Check if contract is safe
        if (!isContractSafe(contractAddress)) {
            Logger::warning("SmartContractConsensusEngine: Contract failed security policy check: " + contractAddress);
            return false;
        }
        
        // Check if governance validation is required
        if (isGovernanceContract(contractAddress) && !enableGovernanceValidation) {
            Logger::warning("SmartContractConsensusEngine: Governance contract requires validation: " + contractAddress);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine security policy enforcement error: " + std::string(e.what()));
        return false;
    }
}

double SmartContractConsensusEngine::getSuccessRate() const {
    if (totalExecutions == 0) {
        return 0.0;
    }
    
    return static_cast<double>(successfulExecutions) / static_cast<double>(totalExecutions);
}

std::string SmartContractConsensusEngine::generateContractAddress(const std::string& deployer, uint64_t nonce) const {
    std::stringstream ss;
    ss << "CONTRACT_" << deployer << "_" << nonce << "_" << std::time(nullptr);
    return Utils::calculateSHA256(ss.str()).substr(0, 40); // Take first 40 characters
}

bool SmartContractConsensusEngine::isContractTransaction(const Transaction& transaction) const {
    // Check if recipient is a contract address or if it's a contract deployment
    std::string recipient = transaction.getRecipient();
    return (recipient == "CONTRACT" || 
            recipient.find("CONTRACT_") == 0 || 
            !transaction.getContractCode().empty() ||
            contractStates.find(recipient) != contractStates.end());
}

bool SmartContractConsensusEngine::isContractDeployment(const Transaction& transaction) const {
    return (transaction.getRecipient() == "CONTRACT" || !transaction.getContractCode().empty());
}

// Private helper methods

bool SmartContractConsensusEngine::processContractTransaction(const Transaction& transaction) {
    try {
        // Analyze governance interaction if enabled
        if (enableGovernanceValidation) {
            ContractGovernanceInteraction interaction = analyzeGovernanceInteraction(transaction);
            if (interaction.type != GovernanceInteractionType::NONE) {
                if (!validateGovernanceInteraction(interaction)) {
                    Logger::warning("SmartContractConsensusEngine: Governance interaction validation failed");
                    return false;
                }
                
                // Store governance interaction
                governanceInteractions[transaction.getHash()] = interaction;
            }
        }
        
        // Validate with multi-consensus if required
        if (requiresMultiConsensusValidation(transaction.getRecipient())) {
            return validateContractWithMultiConsensus(transaction.getRecipient(), transaction);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine contract transaction processing error: " + std::string(e.what()));
        return false;
    }
}

bool SmartContractConsensusEngine::processContractDeployment(const Transaction& transaction) {
    try {
        std::string sourceCode = transaction.getContractCode();
        
        // Validate contract code
        if (!validateContractCode(sourceCode)) {
            Logger::warning("SmartContractConsensusEngine: Contract code validation failed");
            return false;
        }
        
        // Check security
        if (!checkContractSecurity(sourceCode)) {
            Logger::warning("SmartContractConsensusEngine: Contract security check failed");
            return false;
        }
        
        // Check size
        if (!checkContractSize(sourceCode)) {
            Logger::warning("SmartContractConsensusEngine: Contract size check failed");
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine contract deployment processing error: " + std::string(e.what()));
        return false;
    }
}

bool SmartContractConsensusEngine::validateContractWithMultiConsensus(const std::string& contractAddress,
                                                                     const Transaction& transaction) {
    if (validationEngines.empty()) {
        return true; // No engines to validate with
    }
    
    try {
        ConsensusRequest request(RequestType::TRANSACTION_VALIDATION, transaction.serialize());
        request.metadata["contractAddress"] = contractAddress;
        
        std::vector<ConsensusResult> results;
        for (ConsensusEngine* engine : validationEngines) {
            if (engine && engine->isHealthy()) {
                ConsensusResult result = engine->processRequest(request);
                results.push_back(result);
            }
        }
        
        return aggregateValidationResults(results);
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine multi-consensus validation error: " + std::string(e.what()));
        return false;
    }
}

GovernanceInteractionType SmartContractConsensusEngine::detectGovernanceInteractionType(const Transaction& transaction) const {
    try {
        std::string recipient = transaction.getRecipient();
        double amount = transaction.getAmount();
        
        // Simple heuristics for detecting governance interaction types
        if (recipient.find("PROPOSAL_") != std::string::npos) {
            return GovernanceInteractionType::PARAMETER_PROPOSAL;
        } else if (recipient.find("VOTE_") != std::string::npos) {
            return GovernanceInteractionType::VOTING_PARTICIPATION;
        } else if (amount == 0.0 && recipient.find("CONSENSUS_") != std::string::npos) {
            return GovernanceInteractionType::CONSENSUS_RULE_CHANGE;
        } else if (recipient.find("EMERGENCY_") != std::string::npos) {
            return GovernanceInteractionType::EMERGENCY_ACTION;
        }
        
        return GovernanceInteractionType::NONE;
        
    } catch (const std::exception& e) {
        Logger::error("SmartContractConsensusEngine governance type detection error: " + std::string(e.what()));
        return GovernanceInteractionType::NONE;
    }
}

bool SmartContractConsensusEngine::validateGovernanceProposal(const ContractGovernanceInteraction& interaction) {
    // Validate that the proposal has required parameters
    if (interaction.parameters.empty()) {
        Logger::warning("SmartContractConsensusEngine: Governance proposal has no parameters");
        return false;
    }
    
    // Check if voting engine is available for validation
    if (votingEngine && !votingEngine->isHealthy()) {
        Logger::warning("SmartContractConsensusEngine: Voting engine not available for governance validation");
        return false;
    }
    
    return true;
}

bool SmartContractConsensusEngine::validateVotingParticipation(const ContractGovernanceInteraction& interaction) {
    // Check if proposal exists
    if (interaction.proposalId.empty()) {
        Logger::warning("SmartContractConsensusEngine: Voting participation without proposal ID");
        return false;
    }
    
    // Additional validation can be added here
    return true;
}

bool SmartContractConsensusEngine::validateConsensusRuleChange(const ContractGovernanceInteraction& interaction) {
    // Consensus rule changes always require supermajority
    if (!interaction.requiresSupermajority) {
        Logger::warning("SmartContractConsensusEngine: Consensus rule change must require supermajority");
        return false;
    }
    
    return true;
}

bool SmartContractConsensusEngine::checkContractSecurity(const std::string& sourceCode) const {
    if (!vm) {
        return false;
    }
    
    // Use VM's security validation
    return vm->validateContract(sourceCode);
}

bool SmartContractConsensusEngine::checkGasLimits(uint64_t gasUsed, uint64_t gasLimit) const {
    return gasUsed <= gasLimit && gasLimit <= maxGasLimit;
}

bool SmartContractConsensusEngine::checkContractSize(const std::string& sourceCode) const {
    return sourceCode.length() <= maxContractSize;
}

double SmartContractConsensusEngine::calculateValidationConfidence(const std::vector<ConsensusResult>& results) const {
    if (results.empty()) {
        return 0.0;
    }
    
    double totalConfidence = 0.0;
    int validResults = 0;
    
    for (const auto& result : results) {
        if (result.isValid) {
            totalConfidence += result.confidence;
            validResults++;
        }
    }
    
    return validResults > 0 ? totalConfidence / validResults : 0.0;
}

bool SmartContractConsensusEngine::aggregateValidationResults(const std::vector<ConsensusResult>& results) const {
    if (results.empty()) {
        return false;
    }
    
    int validCount = 0;
    for (const auto& result : results) {
        if (result.isValid) {
            validCount++;
        }
    }
    
    // Require majority consensus
    return validCount > (results.size() / 2);
}

void SmartContractConsensusEngine::logContractEvent(const std::string& event, const nlohmann::json& data) const {
    nlohmann::json logData;
    logData["engine"] = "SmartContractConsensusEngine";
    logData["event"] = event;
    logData["timestamp"] = std::time(nullptr);
    logData["data"] = data;
    
    Logger::info("ContractEvent: " + logData.dump());
}

void SmartContractConsensusEngine::updateExecutionStats(bool success) {
    totalExecutions++;
    if (success) {
        successfulExecutions++;
    } else {
        failedExecutions++;
    }
}

void SmartContractConsensusEngine::updateLastActivity() {
    lastUpdate = std::chrono::steady_clock::now();
}