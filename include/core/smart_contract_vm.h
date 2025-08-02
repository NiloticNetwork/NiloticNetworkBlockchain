#ifndef SMART_CONTRACT_VM_H
#define SMART_CONTRACT_VM_H

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <functional>
#include <memory>
#include <variant>
#include "utils.h"

// Value types for the VM
using Value = std::variant<int64_t, double, std::string, bool>;

// Smart contract context
struct SmartContractContext {
    std::string sender;
    std::string contractAddress;
    uint64_t gasLimit;
    uint64_t gasUsed;
    std::map<std::string, Value> storage;
    std::vector<Value> stack;
    std::map<std::string, Value> memory;
    std::vector<std::string> logs;
};

// Opcode definitions
enum class Opcode {
    // Stack operations
    PUSH, POP, DUP, SWAP,
    
    // Arithmetic
    ADD, SUB, MUL, DIV, MOD,
    
    // Logic
    AND, OR, NOT, EQ, LT, GT,
    
    // Storage
    SSTORE, SLOAD,
    
    // Memory
    MSTORE, MLOAD,
    
    // Control flow
    JUMP, JUMPI, JUMPDEST,
    
    // Contract operations
    CALL, CALLCODE, DELEGATECALL, STATICCALL,
    CREATE, CREATE2, RETURN, REVERT,
    
    // Gas and environment
    GAS, ADDRESS, BALANCE, CALLER, CALLVALUE,
    
    // Logging
    LOG0, LOG1, LOG2, LOG3, LOG4
};

class SmartContractVM {
private:
    std::map<std::string, std::function<void(SmartContractContext&)>> opcodeHandlers;
    std::vector<uint8_t> bytecode;
    size_t programCounter;
    bool executionHalted;
    
    // Gas costs for operations
    std::map<Opcode, uint64_t> gasCosts;
    
    // Initialize gas costs
    void initializeGasCosts() {
        gasCosts[Opcode::PUSH] = 3;
        gasCosts[Opcode::POP] = 2;
        gasCosts[Opcode::ADD] = 3;
        gasCosts[Opcode::SUB] = 3;
        gasCosts[Opcode::MUL] = 5;
        gasCosts[Opcode::DIV] = 5;
        gasCosts[Opcode::SSTORE] = 20000;
        gasCosts[Opcode::SLOAD] = 200;
        gasCosts[Opcode::CALL] = 2600;
    }
    
    // Initialize opcode handlers
    void initializeOpcodeHandlers() {
        // Stack operations
        opcodeHandlers["PUSH"] = [this](SmartContractContext& ctx) {
            if (ctx.gasUsed + gasCosts[Opcode::PUSH] > ctx.gasLimit) {
                throw std::runtime_error("Out of gas");
            }
            ctx.gasUsed += gasCosts[Opcode::PUSH];
            
            // Read next bytes as value
            uint8_t size = bytecode[programCounter++];
            std::string value;
            for (int i = 0; i < size; i++) {
                value += static_cast<char>(bytecode[programCounter++]);
            }
            ctx.stack.push_back(Value(value));
        };
        
        opcodeHandlers["POP"] = [this](SmartContractContext& ctx) {
            if (ctx.gasUsed + gasCosts[Opcode::POP] > ctx.gasLimit) {
                throw std::runtime_error("Out of gas");
            }
            ctx.gasUsed += gasCosts[Opcode::POP];
            
            if (!ctx.stack.empty()) {
                ctx.stack.pop_back();
            }
        };
        
        // Arithmetic operations
        opcodeHandlers["ADD"] = [this](SmartContractContext& ctx) {
            if (ctx.gasUsed + gasCosts[Opcode::ADD] > ctx.gasLimit) {
                throw std::runtime_error("Out of gas");
            }
            ctx.gasUsed += gasCosts[Opcode::ADD];
            
            if (ctx.stack.size() < 2) {
                throw std::runtime_error("Stack underflow");
            }
            
            Value b = ctx.stack.back(); ctx.stack.pop_back();
            Value a = ctx.stack.back(); ctx.stack.pop_back();
            
            if (std::holds_alternative<int64_t>(a) && std::holds_alternative<int64_t>(b)) {
                int64_t result = std::get<int64_t>(a) + std::get<int64_t>(b);
                ctx.stack.push_back(Value(result));
            } else if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
                double result = std::get<double>(a) + std::get<double>(b);
                ctx.stack.push_back(Value(result));
            } else {
                throw std::runtime_error("Type mismatch for ADD");
            }
        };
        
        // Storage operations
        opcodeHandlers["SSTORE"] = [this](SmartContractContext& ctx) {
            if (ctx.gasUsed + gasCosts[Opcode::SSTORE] > ctx.gasLimit) {
                throw std::runtime_error("Out of gas");
            }
            ctx.gasUsed += gasCosts[Opcode::SSTORE];
            
            if (ctx.stack.size() < 2) {
                throw std::runtime_error("Stack underflow");
            }
            
            Value value = ctx.stack.back(); ctx.stack.pop_back();
            Value key = ctx.stack.back(); ctx.stack.pop_back();
            
            if (std::holds_alternative<std::string>(key)) {
                ctx.storage[std::get<std::string>(key)] = value;
            } else {
                throw std::runtime_error("Invalid key type for SSTORE");
            }
        };
        
        opcodeHandlers["SLOAD"] = [this](SmartContractContext& ctx) {
            if (ctx.gasUsed + gasCosts[Opcode::SLOAD] > ctx.gasLimit) {
                throw std::runtime_error("Out of gas");
            }
            ctx.gasUsed += gasCosts[Opcode::SLOAD];
            
            if (ctx.stack.empty()) {
                throw std::runtime_error("Stack underflow");
            }
            
            Value key = ctx.stack.back(); ctx.stack.pop_back();
            
            if (std::holds_alternative<std::string>(key)) {
                std::string keyStr = std::get<std::string>(key);
                auto it = ctx.storage.find(keyStr);
                if (it != ctx.storage.end()) {
                    ctx.stack.push_back(it->second);
                } else {
                    ctx.stack.push_back(Value(int64_t(0))); // Default value
                }
            } else {
                throw std::runtime_error("Invalid key type for SLOAD");
            }
        };
        
        // Logging operations
        opcodeHandlers["LOG0"] = [this](SmartContractContext& ctx) {
            if (ctx.stack.empty()) {
                throw std::runtime_error("Stack underflow");
            }
            
            Value data = ctx.stack.back(); ctx.stack.pop_back();
            if (std::holds_alternative<std::string>(data)) {
                ctx.logs.push_back(std::get<std::string>(data));
            }
        };
    }

public:
    SmartContractVM() : programCounter(0), executionHalted(false) {
        initializeGasCosts();
        initializeOpcodeHandlers();
    }
    
    // Load bytecode
    void loadBytecode(const std::vector<uint8_t>& code) {
        bytecode = code;
        programCounter = 0;
        executionHalted = false;
    }
    
    // Execute contract
    void execute(SmartContractContext& context) {
        while (programCounter < bytecode.size() && !executionHalted) {
            if (context.gasUsed >= context.gasLimit) {
                throw std::runtime_error("Out of gas");
            }
            
            // Read opcode
            uint8_t opcode = bytecode[programCounter++];
            std::string opcodeName = getOpcodeName(opcode);
            
            // Execute opcode
            auto handler = opcodeHandlers.find(opcodeName);
            if (handler != opcodeHandlers.end()) {
                handler->second(context);
            } else {
                throw std::runtime_error("Unknown opcode: " + opcodeName);
            }
        }
    }
    
    // Get opcode name (simplified)
    std::string getOpcodeName(uint8_t opcode) {
        switch (opcode) {
            case 0x60: return "PUSH";
            case 0x50: return "POP";
            case 0x01: return "ADD";
            case 0x55: return "SSTORE";
            case 0x54: return "SLOAD";
            case 0xa0: return "LOG0";
            default: return "UNKNOWN";
        }
    }
    
    // Compile simple contract (placeholder)
    std::vector<uint8_t> compileContract(const std::string& sourceCode) {
        // This is a simplified compiler
        // In a real implementation, this would parse the source code
        // and generate proper bytecode
        
        std::vector<uint8_t> bytecode;
        
        // Example: compile a simple storage contract
        if (sourceCode.find("store") != std::string::npos) {
            // PUSH value, SSTORE
            bytecode = {0x60, 0x01, 0x55}; // PUSH 1, SSTORE
        } else if (sourceCode.find("load") != std::string::npos) {
            // SLOAD
            bytecode = {0x54}; // SLOAD
        }
        
        return bytecode;
    }
};

#endif // SMART_CONTRACT_VM_H 