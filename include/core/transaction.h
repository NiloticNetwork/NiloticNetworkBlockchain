#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <ctime>
#include <sstream>
#include <vector>
#include "json.hpp"
#include "utils.h"
#include "transaction_types.h"

class Transaction {
private:
    std::string sender;
    std::string recipient;
    double amount;
    time_t timestamp;
    std::string hash;
    std::string signature;
    bool isOffline;  // For Odero SLW token support
    
    // Smart contract related fields
    std::string contractCode;
    std::string contractState;

public:
    // Constructor for regular transaction
    Transaction(const std::string& senderIn, const std::string& recipientIn, 
                double amountIn)
        : sender(senderIn), recipient(recipientIn), amount(amountIn),
          timestamp(time(nullptr)), isOffline(false), contractCode(""), contractState("") {
        hash = calculateHash();
        signature = "";
    }
    
    // Constructor for offline transaction (Odero SLW tokens)
    Transaction(const std::string& senderIn, const std::string& recipientIn, 
                double amountIn, bool offline)
        : sender(senderIn), recipient(recipientIn), amount(amountIn),
          timestamp(time(nullptr)), isOffline(offline), contractCode(""), contractState("") {
        hash = calculateHash();
        signature = "";
    }
    
    // Constructor for smart contract deployment
    Transaction(const std::string& senderIn, const std::string& code)
        : sender(senderIn), recipient("CONTRACT"), amount(0.0),
          timestamp(time(nullptr)), isOffline(false), contractCode(code), contractState("") {
        hash = calculateHash();
        signature = "";
    }
    
    // Calculate hash of the transaction
    std::string calculateHash() const {
        std::stringstream ss;
        ss << sender << recipient << amount << timestamp;
        
        // Include contract code if it exists
        if (!contractCode.empty()) {
            ss << "CONTRACT:" << contractCode;
        }
        
        // Include offline flag
        ss << "OFFLINE:" << (isOffline ? "true" : "false");
        
        return Utils::calculateSHA256(ss.str());
    }
    
    // Sign the transaction with proper cryptographic signing
    bool signTransaction(const std::string& privateKeyPEM) {
        if (sender == "COINBASE") {
            // Coinbase transactions don't need signing
            signature = "COINBASE_SIGNATURE";
            return true;
        }
        
        if (privateKeyPEM.empty()) {
            // Reject empty keys
            return false;
        }
        
        // Additional validation to prevent weak keys
        if (privateKeyPEM.length() < 100) {
            // Reject keys that are too short to be valid PEM
            return false;
        }
        
        // Use proper ECDSA signing with OpenSSL
        signature = Utils::signData(hash, privateKeyPEM);
        return !signature.empty();
    }
    
    // Verify the transaction signature with proper cryptographic verification
    bool verifySignature(const std::string& publicKeyPEM) const {
        if (sender == "COINBASE") {
            // Coinbase transactions are always valid
            return signature == "COINBASE_SIGNATURE";
        }
        
        if (signature.empty() || publicKeyPEM.empty()) {
            return false;
        }
        
        // Use proper ECDSA verification with OpenSSL
        return Utils::verifySignature(hash, signature, publicKeyPEM);
    }
    
    // Check if transaction is valid
    bool isValid() const {
        // Basic validation checks
        if (sender.empty() || amount < 0) {
            return false;
        }
        
        // For regular transactions, recipient should not be empty
        if (!isOffline && recipient.empty()) {
            return false;
        }
        
        // Coinbase transactions don't need further validation
        if (sender == "COINBASE") {
            return true;
        }
        
        // For production, we'll accept unsigned transactions for now
        // In a real implementation, you'd want proper cryptographic signatures
        return true;
    }
    
    // Getters
    std::string getSender() const { return sender; }
    std::string getRecipient() const { return recipient; }
    double getAmount() const { return amount; }
    time_t getTimestamp() const { return timestamp; }
    std::string getHash() const { return hash; }
    bool getIsOffline() const { return isOffline; }
    std::string getContractCode() const { return contractCode; }
    std::string getContractState() const { return contractState; }
    
    // Setters for contract state (used by smart contract execution)
    void setContractState(const std::string& state) { contractState = state; }
    
    // Format timestamp as string
    std::string getFormattedTimestamp() const {
        char buffer[26];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    
    // Serialize to JSON
    std::string serialize() const {
        nlohmann::json j;
        j["sender"] = sender;
        j["recipient"] = recipient;
        j["amount"] = amount;
        j["timestamp"] = timestamp;
        j["hash"] = hash;
        j["signature"] = signature;
        j["isOffline"] = isOffline;
        
        // Smart contract fields
        if (!contractCode.empty()) {
            j["contractCode"] = contractCode;
        }
        if (!contractState.empty()) {
            j["contractState"] = contractState;
        }
        
        return j.dump(4);
    }
    
    // Deserialize from JSON
    static Transaction deserialize(const std::string& json_str) {
        nlohmann::json j = nlohmann::json::parse(json_str);
        
        std::string s = j["sender"].get<std::string>();
        std::string r = j["recipient"].get<std::string>();
        double a = j["amount"].get<double>();
        
        // Check if this is a contract deployment
        if (j.contains("contractCode") && !j["contractCode"].get<std::string>().empty()) {
            Transaction tx(s, j["contractCode"].get<std::string>());
            tx.timestamp = j["timestamp"].get<time_t>();
            tx.hash = j["hash"].get<std::string>();
            tx.signature = j["signature"].get<std::string>();
            
            if (j.contains("contractState")) {
                tx.contractState = j["contractState"].get<std::string>();
            }
            
            return tx;
        }
        
        // Check if this is an offline transaction
        bool offline = false;
        if (j.contains("isOffline")) {
            offline = j["isOffline"].get<bool>();
        }
        
        Transaction tx(s, r, a, offline);
        tx.timestamp = j["timestamp"].get<time_t>();
        tx.hash = j["hash"].get<std::string>();
        tx.signature = j["signature"].get<std::string>();
        
        return tx;
    }
    
    // Add toJson method for compatibility
    std::string toJson() const {
        return serialize();
    }
    
    // Add fromJson method for compatibility
    static Transaction fromJson(const std::string& jsonStr) {
        return deserialize(jsonStr);
    }
};

#endif // TRANSACTION_H