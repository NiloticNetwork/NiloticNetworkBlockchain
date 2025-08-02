#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <mutex>
#include <deque>
#include "json.hpp"
#include "block.h"
#include "transaction.h"
#include "logger.h"

class Blockchain {
private:
    std::vector<Block> chain;
    std::deque<Transaction> pendingTransactions;
    uint64_t difficulty;
    double miningReward;
    
    // Account balances (address -> balance)
    std::map<std::string, double> balances;
    
    // Smart contracts (contract address -> code)
    std::map<std::string, std::string> contracts;
    
    // Mutex for thread-safety
    std::mutex chainMutex;
    std::mutex txMutex;
    
    // Validators for PoS (address -> stake amount)
    std::map<std::string, double> validators;

public:
    // Constructor
    Blockchain() : difficulty(4), miningReward(100.0) {
        // Create the genesis block
        createGenesisBlock();
    }
    
    // Create the first block in the chain
    void createGenesisBlock() {
        Logger::info("Creating genesis block");
        Block genesis(0, "0");
        
        // Create a coinbase transaction
        Transaction coinbase("COINBASE", "GENESIS", 1000.0);
        genesis.addTransaction(coinbase);
        
        // Add genesis block to the chain
        chain.push_back(genesis);
        
        // Update the balance for the genesis account
        balances["GENESIS"] = 1000.0;
        
        Logger::info("Genesis block created with hash: " + genesis.getHash());
    }
    
    // Get the latest block in the chain
    Block getLatestBlock() const {
        return chain.back();
    }
    
    // Add a block to the chain
    bool addBlock(Block newBlock) {
        std::lock_guard<std::mutex> lock(chainMutex);
        
        // Verify that the previous hash matches the hash of the latest block
        if (newBlock.getPreviousHash() != getLatestBlock().getHash()) {
            Logger::error("Block rejected: Invalid previous hash");
            return false;
        }
        
        // Verify that the index is sequential
        if (newBlock.getIndex() != getLatestBlock().getIndex() + 1) {
            Logger::error("Block rejected: Invalid block index");
            return false;
        }
        
        // Verify that the block's hash is valid based on our current difficulty
        std::string target(difficulty, '0');
        if (newBlock.getHash().substr(0, difficulty) != target) {
            Logger::error("Block rejected: Proof of work or stake verification failed");
            return false;
        }
        
        // Process transactions in the block
        for (const Transaction& tx : newBlock.getTransactions()) {
            processTransaction(tx);
        }
        
        // Add the block to the chain
        chain.push_back(newBlock);
        Logger::info("Block added to chain at height: " + std::to_string(newBlock.getIndex()));
        
        return true;
    }
    
    // Process a transaction and update balances
    bool processTransaction(const Transaction& tx) {
        if (!tx.isValid()) {
            Logger::error("Invalid transaction: " + tx.getHash());
            return false;
        }
        
        const std::string& sender = tx.getSender();
        const std::string& recipient = tx.getRecipient();
        double amount = tx.getAmount();
        
        // Handle coinbase transactions
        if (sender == "COINBASE") {
            balances[recipient] += amount;
            Logger::info("Coinbase transaction: " + tx.getHash() + " - " + std::to_string(amount) + " coins to " + recipient);
            return true;
        }
        
        // Check if sender has enough balance
        if (balances.find(sender) == balances.end() || balances[sender] < amount) {
            Logger::error("Transaction failed: Insufficient balance for " + sender);
            return false;
        }
        
        // Handle smart contract deployment
        if (recipient == "CONTRACT" && !tx.getContractCode().empty()) {
            // Generate a contract address
            std::string contractAddress = "CONTRACT-" + tx.getHash().substr(0, 10);
            
            // Store the contract code
            contracts[contractAddress] = tx.getContractCode();
            
            Logger::info("Smart contract deployed: " + contractAddress);
            return true;
        }
        
        // Handle offline transactions (Odero SLW tokens)
        if (tx.getIsOffline()) {
            // In a real implementation, this would use a more complex mechanism
            // for offline payments using the Odero SLW system
            Logger::info("Offline transaction: " + tx.getHash());
        }
        
        // Update balances
        balances[sender] -= amount;
        
        if (balances.find(recipient) == balances.end()) {
            balances[recipient] = 0;
        }
        
        balances[recipient] += amount;
        
        Logger::info("Transaction processed: " + tx.getHash() + " - " + std::to_string(amount) + 
                    " from " + sender + " to " + recipient);
        
        return true;
    }
    
    // Add a transaction to the pending pool
    bool addTransaction(const Transaction& tx) {
        std::lock_guard<std::mutex> lock(txMutex);
        
        if (!tx.isValid()) {
            Logger::error("Invalid transaction rejected: " + tx.getHash());
            return false;
        }
        
        // For non-coinbase transactions, check if sender has enough balance
        if (tx.getSender() != "COINBASE") {
            const std::string& sender = tx.getSender();
            
            if (balances.find(sender) == balances.end() || balances[sender] < tx.getAmount()) {
                Logger::error("Transaction rejected: Insufficient balance for " + sender);
                return false;
            }
        }
        
        pendingTransactions.push_back(tx);
        Logger::info("Transaction added to pending pool: " + tx.getHash());
        
        return true;
    }
    
    // Mine pending transactions (reward goes to the provided address)
    Block minePendingTransactions(const std::string& miningRewardAddress) {
        std::lock_guard<std::mutex> lockChain(chainMutex);
        std::lock_guard<std::mutex> lockTx(txMutex);
        
        // Create a coinbase transaction
        Transaction coinbaseTx("COINBASE", miningRewardAddress, miningReward);
        
        // Create a new block
        uint64_t newIndex = getLatestBlock().getIndex() + 1;
        Block newBlock(newIndex, getLatestBlock().getHash());
        
        // Add the coinbase transaction first
        newBlock.addTransaction(coinbaseTx);
        
        // Add pending transactions to the block (up to a limit)
        const size_t MAX_TRANSACTIONS_PER_BLOCK = 10;
        size_t count = 0;
        
        while (!pendingTransactions.empty() && count < MAX_TRANSACTIONS_PER_BLOCK) {
            Transaction tx = pendingTransactions.front();
            pendingTransactions.pop_front();
            
            if (newBlock.addTransaction(tx)) {
                count++;
            }
        }
        
        // Mine the block
        Logger::info("Mining block " + std::to_string(newIndex) + " with " + 
                    std::to_string(count + 1) + " transactions");
        newBlock.mineBlock(difficulty);
        
        // Add the block to the chain
        chain.push_back(newBlock);
        
        // Process transactions
        for (const Transaction& tx : newBlock.getTransactions()) {
            processTransaction(tx);
        }
        
        Logger::info("Block mined successfully: " + newBlock.getHash());
        
        return newBlock;
    }
    
    // Stake tokens for PoS validation
    bool stakeTokens(const std::string& address, double amount) {
        std::lock_guard<std::mutex> lock(chainMutex);
        
        if (balances.find(address) == balances.end() || balances[address] < amount) {
            Logger::error("Staking failed: Insufficient balance for " + address);
            return false;
        }
        
        // Move tokens from balance to stake
        balances[address] -= amount;
        
        if (validators.find(address) == validators.end()) {
            validators[address] = 0;
        }
        
        validators[address] += amount;
        
        Logger::info("Tokens staked: " + std::to_string(amount) + " by " + address);
        
        return true;
    }
    
    // Choose a validator for the next block (PoS)
    std::string selectValidator() const {
        if (validators.empty()) {
            return "";
        }
        
        // In a real implementation, this would use a more sophisticated
        // selection algorithm based on stake amount and randomization
        
        // For now, simply return the validator with the highest stake
        std::string selectedValidator;
        double maxStake = 0;
        
        for (const auto& pair : validators) {
            if (pair.second > maxStake) {
                maxStake = pair.second;
                selectedValidator = pair.first;
            }
        }
        
        return selectedValidator;
    }
    
    // Validate a block using PoS
    bool validateBlockPoS(Block& block, const std::string& validatorAddress, const std::string& signature) {
        if (validators.find(validatorAddress) == validators.end()) {
            Logger::error("Block validation failed: Not a validator - " + validatorAddress);
            return false;
        }
        
        // Set the validator and signature on the block
        block.setValidator(validatorAddress);
        block.setSignature(signature);
        
        // In a real implementation, this would verify the cryptographic signature
        // For now, we'll just accept any signature as valid
        
        // Adjust the mining reward based on the validator's stake
        double stake = validators[validatorAddress];
        double reward = miningReward * (stake / 1000.0); // Example calculation
        
        // Add a reward transaction
        Transaction rewardTx("COINBASE", validatorAddress, reward);
        block.addTransaction(rewardTx);
        
        Logger::info("Block validated by " + validatorAddress + " with stake " + 
                    std::to_string(stake) + " and reward " + std::to_string(reward));
        
        return true;
    }
    
    // Get account balance
    double getBalance(const std::string& address) const {
        if (balances.find(address) != balances.end()) {
            return balances.at(address);
        }
        return 0.0;
    }
    
    // Validate the entire blockchain
    bool isChainValid() const {
        // Start from index 1 (after genesis)
        for (size_t i = 1; i < chain.size(); i++) {
            const Block& currentBlock = chain[i];
            const Block& previousBlock = chain[i - 1];
            
            // Check if the block's hash is valid
            if (currentBlock.getHash() != currentBlock.calculateHash()) {
                Logger::error("Invalid block hash at height " + std::to_string(i));
                return false;
            }
            
            // Check if the previous hash matches
            if (currentBlock.getPreviousHash() != previousBlock.getHash()) {
                Logger::error("Invalid previous hash at height " + std::to_string(i));
                return false;
            }
        }
        
        return true;
    }
    
    // Save the blockchain to a file
    bool saveToFile(const std::string& filename) const {
        // Note: For const methods, we use a const_cast for the mutex
        // since locking a mutex is not a const operation
        std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&chainMutex));
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for saving: " + filename);
            return false;
        }
        
        nlohmann::json blockchain_json;
        
        // Save the chain
        nlohmann::json blocks_json = nlohmann::json::array();
        for (const Block& block : chain) {
            blocks_json.push_back(nlohmann::json::parse(block.serialize()));
        }
        blockchain_json["blocks"] = blocks_json;
        
        // Save the balances
        nlohmann::json balances_json;
        for (const auto& pair : balances) {
            balances_json[pair.first] = pair.second;
        }
        blockchain_json["balances"] = balances_json;
        
        // Save the pending transactions
        nlohmann::json pending_json = nlohmann::json::array();
        for (const Transaction& tx : pendingTransactions) {
            pending_json.push_back(nlohmann::json::parse(tx.serialize()));
        }
        blockchain_json["pendingTransactions"] = pending_json;
        
        // Save the validators
        nlohmann::json validators_json;
        for (const auto& pair : validators) {
            validators_json[pair.first] = pair.second;
        }
        blockchain_json["validators"] = validators_json;
        
        // Save blockchain metadata
        blockchain_json["difficulty"] = difficulty;
        blockchain_json["miningReward"] = miningReward;
        
        file << blockchain_json.dump(4);
        file.close();
        
        Logger::info("Blockchain saved to file: " + filename);
        
        return true;
    }
    
    // Load the blockchain from a file
    bool loadFromFile(const std::string& filename) {
        std::lock_guard<std::mutex> lockChain(chainMutex);
        std::lock_guard<std::mutex> lockTx(txMutex);
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for loading: " + filename);
            return false;
        }
        
        std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        try {
            nlohmann::json blockchain_json = nlohmann::json::parse(json_str);
            
            // Clear existing data
            chain.clear();
            pendingTransactions.clear();
            balances.clear();
            validators.clear();
            
            // Load the chain
            for (const auto& block_json : blockchain_json["blocks"]) {
                chain.push_back(Block::deserialize(block_json.dump()));
            }
            
            // Load the balances
            for (const auto& [address, balance] : blockchain_json["balances"].items()) {
                balances[address] = balance.get<double>();
            }
            
            // Load the pending transactions
            for (const auto& tx_json : blockchain_json["pendingTransactions"]) {
                pendingTransactions.push_back(Transaction::deserialize(tx_json.dump()));
            }
            
            // Load the validators
            for (const auto& [address, stake] : blockchain_json["validators"].items()) {
                validators[address] = stake.get<double>();
            }
            
            // Load blockchain metadata
            difficulty = blockchain_json["difficulty"];
            miningReward = blockchain_json["miningReward"];
            
            Logger::info("Blockchain loaded from file: " + filename);
            Logger::info("Chain height: " + std::to_string(chain.size()));
            
            return true;
        } catch (const std::exception& e) {
            Logger::error("Failed to load blockchain: " + std::string(e.what()));
            return false;
        }
    }
    
    // Get the chain data
    std::vector<Block> getChain() const {
        return chain;
    }
    
    // Get pending transactions
    std::deque<Transaction> getPendingTransactions() const {
        return pendingTransactions;
    }
    
    // Get all balances
    std::map<std::string, double> getAllBalances() const {
        return balances;
    }
    
    // Get chain height
    size_t getChainHeight() const {
        return chain.size();
    }
    
    // Set mining difficulty
    void setDifficulty(uint64_t newDifficulty) {
        difficulty = newDifficulty;
    }
    
    // Get mining difficulty
    uint64_t getDifficulty() const {
        return difficulty;
    }
    
    // Set mining reward
    void setMiningReward(double newReward) {
        miningReward = newReward;
    }
    
    // Get mining reward
    double getMiningReward() const {
        return miningReward;
    }
};

#endif // BLOCKCHAIN_H