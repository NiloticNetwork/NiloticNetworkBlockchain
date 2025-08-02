#include "persistence.h"
#include "utils.h"
#include <fstream>
#include <filesystem>
#include "json.hpp"

// Constructor
Persistence::Persistence() {
    // Create data directory if it doesn't exist
    std::filesystem::create_directories("data");
}

// Check if blockchain exists in storage
bool Persistence::blockchainExists() const {
    // Check if blockchain.json file exists
    return std::filesystem::exists("data/blockchain.json");
}

// Save blockchain to storage
void Persistence::saveBlockchain(const std::vector<Block>& blocks) {
    try {
        nlohmann::json blockchainJson = nlohmann::json::array();
        
        // Serialize each block to JSON
        for (const auto& block : blocks) {
            blockchainJson.push_back(Utils::safeParseJson(block.toJson()));
        }
        
        // Write to file
        std::ofstream outFile("data/blockchain.json");
        outFile << blockchainJson.dump(4) << std::endl;
        outFile.close();
        
        Utils::logInfo("Blockchain saved to storage");
    } catch (const std::exception& e) {
        Utils::logError("Error saving blockchain: " + std::string(e.what()));
    }
}

// Load blockchain from storage
std::vector<Block> Persistence::loadBlockchain() const {
    std::vector<Block> blocks;
    
    try {
        // Read blockchain file
        std::ifstream inFile("data/blockchain.json");
        std::string json((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        // Parse JSON
        nlohmann::json blockchainJson = Utils::safeParseJson(json);
        
        // Deserialize each block
        for (const auto& blockJson : blockchainJson) {
            blocks.push_back(Block::fromJson(blockJson.dump()));
        }
        
        Utils::logInfo("Blockchain loaded from storage");
    } catch (const std::exception& e) {
        Utils::logError("Error loading blockchain: " + std::string(e.what()));
    }
    
    // If we couldn't load any blocks, create an empty vector
    if (blocks.empty()) {
        Utils::logInfo("No blocks loaded, returning empty vector");
    }
    
    return blocks;
}

// Save pending transactions to storage
void Persistence::savePendingTransactions(const std::vector<Transaction>& transactions) {
    try {
        nlohmann::json transactionsJson = nlohmann::json::array();
        
        // Serialize each transaction to JSON
        for (const auto& tx : transactions) {
            transactionsJson.push_back(Utils::safeParseJson(tx.toJson()));
        }
        
        // Write to file
        std::ofstream outFile("data/pending_transactions.json");
        outFile << transactionsJson.dump(4) << std::endl;
        outFile.close();
        
        Utils::logInfo("Pending transactions saved to storage");
    } catch (const std::exception& e) {
        Utils::logError("Error saving pending transactions: " + std::string(e.what()));
    }
}

// Load pending transactions from storage
std::vector<Transaction> Persistence::loadPendingTransactions() const {
    std::vector<Transaction> transactions;
    
    try {
        // Check if the file exists
        if (!std::filesystem::exists("data/pending_transactions.json")) {
            Utils::logInfo("No pending transactions file found");
            return transactions;
        }
        
        // Read transactions file
        std::ifstream inFile("data/pending_transactions.json");
        std::string json((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        // Parse JSON
        nlohmann::json transactionsJson = Utils::safeParseJson(json);
        
        // Deserialize each transaction
        for (const auto& txJson : transactionsJson) {
            transactions.push_back(Transaction::fromJson(txJson.dump()));
        }
        
        Utils::logInfo("Pending transactions loaded from storage");
    } catch (const std::exception& e) {
        Utils::logError("Error loading pending transactions: " + std::string(e.what()));
    }
    
    return transactions;
}

// Save wallets to storage
void Persistence::saveWallets(const std::map<std::string, Wallet>& wallets) {
    try {
        nlohmann::json walletsJson;
        
        // Serialize each wallet to JSON
        for (const auto& [address, wallet] : wallets) {
            walletsJson[address] = Utils::safeParseJson(wallet.toJson());
        }
        
        // Write to file
        std::ofstream outFile("data/wallets.json");
        outFile << walletsJson.dump(4) << std::endl;
        outFile.close();
        
        Utils::logInfo("Wallets saved to storage");
    } catch (const std::exception& e) {
        Utils::logError("Error saving wallets: " + std::string(e.what()));
    }
}

// Load wallets from storage
std::map<std::string, Wallet> Persistence::loadWallets() const {
    std::map<std::string, Wallet> wallets;
    
    try {
        // Check if the file exists
        if (!std::filesystem::exists("data/wallets.json")) {
            Utils::logInfo("No wallets file found");
            return wallets;
        }
        
        // Read wallets file
        std::ifstream inFile("data/wallets.json");
        std::string json((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        // Parse JSON
        nlohmann::json walletsJson = Utils::safeParseJson(json);
        
        // Deserialize each wallet
        for (auto it = walletsJson.begin(); it != walletsJson.end(); ++it) {
            std::string address = it.key();
            wallets[address] = Wallet::fromJson(it.value().dump());
        }
        
        Utils::logInfo("Wallets loaded from storage");
    } catch (const std::exception& e) {
        Utils::logError("Error loading wallets: " + std::string(e.what()));
    }
    
    return wallets;
}

// Save stakes to storage
void Persistence::saveStakes(const std::map<std::string, double>& stakes) {
    try {
        nlohmann::json stakesJson;
        
        // Serialize each stake to JSON
        for (const auto& [address, amount] : stakes) {
            stakesJson[address] = amount;
        }
        
        // Write to file
        std::ofstream outFile("data/stakes.json");
        outFile << stakesJson.dump(4) << std::endl;
        outFile.close();
        
        Utils::logInfo("Stakes saved to storage");
    } catch (const std::exception& e) {
        Utils::logError("Error saving stakes: " + std::string(e.what()));
    }
}

// Load stakes from storage
std::map<std::string, double> Persistence::loadStakes() const {
    std::map<std::string, double> stakes;
    
    try {
        // Check if the file exists
        if (!std::filesystem::exists("data/stakes.json")) {
            Utils::logInfo("No stakes file found");
            return stakes;
        }
        
        // Read stakes file
        std::ifstream inFile("data/stakes.json");
        std::string json((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        // Parse JSON
        nlohmann::json stakesJson = Utils::safeParseJson(json);
        
        // Deserialize each stake
        for (auto it = stakesJson.begin(); it != stakesJson.end(); ++it) {
            std::string address = it.key();
            stakes[address] = it.value();
        }
        
        Utils::logInfo("Stakes loaded from storage");
    } catch (const std::exception& e) {
        Utils::logError("Error loading stakes: " + std::string(e.what()));
    }
    
    return stakes;
}

// Save Odero SLW tokens to storage
void Persistence::saveOderoTokens(const std::map<std::string, OderoSLW>& tokens) {
    try {
        nlohmann::json tokensJson;
        
        // Serialize each token to JSON
        for (const auto& [tokenId, token] : tokens) {
            tokensJson[tokenId] = Utils::safeParseJson(token.toJson());
        }
        
        // Write to file
        std::ofstream outFile("data/odero_tokens.json");
        outFile << tokensJson.dump(4) << std::endl;
        outFile.close();
        
        Utils::logInfo("Odero SLW tokens saved to storage");
    } catch (const std::exception& e) {
        Utils::logError("Error saving Odero SLW tokens: " + std::string(e.what()));
    }
}

// Load Odero SLW tokens from storage
std::map<std::string, OderoSLW> Persistence::loadOderoTokens() const {
    std::map<std::string, OderoSLW> tokens;
    
    try {
        // Check if the file exists
        if (!std::filesystem::exists("data/odero_tokens.json")) {
            Utils::logInfo("No Odero SLW tokens file found");
            return tokens;
        }
        
        // Read tokens file
        std::ifstream inFile("data/odero_tokens.json");
        std::string json((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        // Parse JSON
        nlohmann::json tokensJson = Utils::safeParseJson(json);
        
        // Deserialize each token
        for (auto it = tokensJson.begin(); it != tokensJson.end(); ++it) {
            std::string tokenId = it.key();
            tokens[tokenId] = OderoSLW::fromJson(it.value().dump());
        }
        
        Utils::logInfo("Odero SLW tokens loaded from storage");
    } catch (const std::exception& e) {
        Utils::logError("Error loading Odero SLW tokens: " + std::string(e.what()));
    }
    
    return tokens;
}