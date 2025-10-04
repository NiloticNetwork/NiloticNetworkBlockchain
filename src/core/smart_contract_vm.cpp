#include "../../include/core/smart_contract_vm.h"
#include "../../include/core/logger.h"
#include <stdexcept>
#include <algorithm>

// Implementation is mostly in the header file as inline methods
// This file contains any additional implementations if needed

// Additional security validation for smart contracts
bool SmartContractVM::validateContract(const std::string& sourceCode) {
    // Security checks for contract source code
    
    // Check for dangerous keywords
    std::vector<std::string> dangerousKeywords = {
        "eval", "exec", "system", "popen", "fork", "execve",
        "file", "fopen", "fwrite", "fread", "remove", "unlink"
    };
    
    std::string lowerCode = sourceCode;
    std::transform(lowerCode.begin(), lowerCode.end(), lowerCode.begin(), ::tolower);
    
    for (const auto& keyword : dangerousKeywords) {
        if (lowerCode.find(keyword) != std::string::npos) {
            Logger::error("Contract contains dangerous keyword: " + keyword);
            return false;
        }
    }
    
    // Check contract size limit
    if (sourceCode.length() > 24576) { // 24KB limit like Ethereum
        Logger::error("Contract size exceeds limit");
        return false;
    }
    
    // Check for infinite loops (basic check)
    size_t whileCount = 0;
    size_t forCount = 0;
    size_t pos = 0;
    
    while ((pos = lowerCode.find("while", pos)) != std::string::npos) {
        whileCount++;
        pos += 5;
    }
    
    pos = 0;
    while ((pos = lowerCode.find("for", pos)) != std::string::npos) {
        forCount++;
        pos += 3;
    }
    
    if (whileCount > 10 || forCount > 10) {
        Logger::warning("Contract contains many loops, potential DoS risk");
    }
    
    return true;
}

// Secure contract execution with sandboxing
bool SmartContractVM::executeSecure(SmartContractContext& context, const std::vector<uint8_t>& code) {
    try {
        // Set execution limits
        context.gasLimit = std::min(context.gasLimit, static_cast<uint64_t>(1000000)); // Max 1M gas
        
        // Load and validate bytecode
        if (code.size() > 24576) {
            Logger::error("Bytecode size exceeds limit");
            return false;
        }
        
        loadBytecode(code);
        
        // Execute with timeout protection
        auto startTime = std::chrono::steady_clock::now();
        const auto maxExecutionTime = std::chrono::seconds(30);
        
        while (programCounter < bytecode.size() && !executionHalted) {
            // Check execution time
            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime - startTime > maxExecutionTime) {
                Logger::error("Contract execution timeout");
                return false;
            }
            
            // Check gas limit
            if (context.gasUsed >= context.gasLimit) {
                Logger::error("Contract out of gas");
                return false;
            }
            
            // Execute next instruction
            uint8_t opcode = bytecode[programCounter++];
            std::string opcodeName = getOpcodeName(opcode);
            
            auto handler = opcodeHandlers.find(opcodeName);
            if (handler != opcodeHandlers.end()) {
                handler->second(context);
            } else {
                Logger::error("Unknown opcode: " + opcodeName);
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        Logger::error("Contract execution error: " + std::string(e.what()));
        return false;
    }
}