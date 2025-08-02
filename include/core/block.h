#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "transaction.h"
#include "utils.h"

class Block {
private:
    uint64_t index;
    std::string previousHash;
    time_t timestamp;
    std::vector<Transaction> transactions;
    std::string merkleRoot;
    uint64_t nonce;
    std::string hash;
    
    // PoS fields
    std::string validator;
    std::string signature;

public:
    // Constructor
    Block(uint64_t indexIn, const std::string& previousHashIn) 
        : index(indexIn), previousHash(previousHashIn), timestamp(time(nullptr)), 
          nonce(0), validator(""), signature("") {
        calculateMerkleRoot();
        hash = calculateHash();
    }

    // Calculate hash of the block
    std::string calculateHash() const {
        std::stringstream ss;
        ss << index << previousHash << timestamp << merkleRoot << nonce;
        
        // For PoS
        if (!validator.empty()) {
            ss << validator;
        }
        
        return Utils::calculateSHA256(ss.str());
    }

    // For mining (PoW) or validation (PoS)
    void mineBlock(uint64_t difficulty) {
        // TODO: Implement proper PoS validation
        merkleRoot = calculateMerkleRoot();
        nonce = 0;
        std::string target(difficulty, '0');
        
        hash = calculateHash();
        while (hash.substr(0, difficulty) != target) {
            nonce++;
            hash = calculateHash();
        }
    }

    // Calculate Merkle Root
    std::string calculateMerkleRoot() {
        if (transactions.empty()) {
            return "0";
        }
        
        std::vector<std::string> txHashes;
        for (const Transaction& tx : transactions) {
            txHashes.push_back(tx.calculateHash());
        }
        
        while (txHashes.size() > 1) {
            if (txHashes.size() % 2 != 0) {
                txHashes.push_back(txHashes.back());
            }
            
            std::vector<std::string> newHashes;
            for (size_t i = 0; i < txHashes.size(); i += 2) {
                std::string combinedHash = Utils::calculateSHA256(txHashes[i] + txHashes[i + 1]);
                newHashes.push_back(combinedHash);
            }
            
            txHashes = newHashes;
        }
        
        merkleRoot = txHashes[0];
        return merkleRoot;
    }

    // Add a transaction to the block
    bool addTransaction(const Transaction& transaction) {
        // Check if the transaction is valid before adding
        if (!transaction.isValid()) {
            return false;
        }
        
        transactions.push_back(transaction);
        return true;
    }

    // Getters
    uint64_t getIndex() const { return index; }
    std::string getPreviousHash() const { return previousHash; }
    time_t getTimestamp() const { return timestamp; }
    std::string getHash() const { return hash; }
    std::vector<Transaction> getTransactions() const { return transactions; }
    std::string getMerkleRoot() const { return merkleRoot; }
    
    // PoS related methods
    void setValidator(const std::string& validatorAddress) { validator = validatorAddress; }
    std::string getValidator() const { return validator; }
    void setSignature(const std::string& validatorSignature) { signature = validatorSignature; }
    
    // Set nonce for mining
    void setNonce(uint64_t nonceValue) { nonce = nonceValue; }
    std::string getSignature() const { return signature; }
    
    // Format timestamp as string
    std::string getFormattedTimestamp() const {
        char buffer[26];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    
    // Serialize block to JSON
    std::string serialize() const {
        nlohmann::json j;
        j["index"] = index;
        j["timestamp"] = timestamp;
        j["previousHash"] = previousHash;
        j["hash"] = hash;
        j["nonce"] = nonce;
        j["merkleRoot"] = merkleRoot;
        
        // PoS fields
        j["validator"] = validator;
        j["signature"] = signature;
        
        // Transactions
        nlohmann::json tx_array = nlohmann::json::array();
        for (const auto& tx : transactions) {
            tx_array.push_back(tx.serialize());
        }
        j["transactions"] = tx_array;
        
        return j.dump(4);
    }
    
    // Deserialize block from JSON
    static Block deserialize(const std::string& json_str) {
        nlohmann::json j = nlohmann::json::parse(json_str);
        
        uint64_t idx = j["index"].get<uint64_t>();
        std::string prev_hash = j["previousHash"].get<std::string>();
        
        Block block(idx, prev_hash);
        block.timestamp = j["timestamp"].get<time_t>();
        block.hash = j["hash"].get<std::string>();
        block.nonce = j["nonce"].get<uint64_t>();
        block.merkleRoot = j["merkleRoot"].get<std::string>();
        
        // PoS fields
        if (j.contains("validator")) {
            block.validator = j["validator"].get<std::string>();
        }
        if (j.contains("signature")) {
            block.signature = j["signature"].get<std::string>();
        }
        
        // Transactions
        for (const auto& tx_json : j["transactions"]) {
            block.transactions.push_back(Transaction::deserialize(tx_json.dump()));
        }
        
        return block;
    }
    
    // Add toJson method for compatibility
    std::string toJson() const {
        return serialize();
    }
    
    // Add fromJson method for compatibility
    static Block fromJson(const std::string& jsonStr) {
        return deserialize(jsonStr);
    }
};

#endif // BLOCK_H