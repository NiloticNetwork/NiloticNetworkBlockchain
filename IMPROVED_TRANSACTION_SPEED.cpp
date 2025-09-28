// Improved Transaction Speed Implementation
// This file contains modifications to reduce transaction confirmation time

#include "blockchain.h"
#include "mining.h"
#include "api.h"
#include <algorithm>
#include <chrono>

// ============================================================================
// IMPROVED MINING CONFIGURATION
// ============================================================================

struct ImprovedMiningConfig {
    // Faster block times
    uint64_t targetDifficulty = 2;              // Reduced from 4 (faster mining)
    uint64_t targetBlockTime = 30;              // Reduced from 600 (30 seconds)
    uint64_t maxTransactionsPerBlock = 50;      // Increased from 10
    double miningReward = 100.0;
    double transactionFee = 0.001;
    
    // New features
    bool enableInstantConfirmation = true;       // For small amounts
    double instantConfirmationLimit = 10.0;     // Amounts under 10 NIL
    bool enableFeePrioritization = true;        // Higher fees = higher priority
    bool enableTransactionStatus = true;         // Real-time status updates
};

// ============================================================================
// INSTANT CONFIRMATION FOR SMALL AMOUNTS
// ============================================================================

class InstantConfirmation {
private:
    static const double INSTANT_LIMIT = 10.0;
    
public:
    static bool canProcessInstantly(const Transaction& tx) {
        return tx.getAmount() <= INSTANT_LIMIT && 
               tx.getSender() != "COINBASE" &&
               !tx.getIsOffline();
    }
    
    static bool processInstantTransaction(Blockchain& blockchain, const Transaction& tx) {
        if (!canProcessInstantly(tx)) {
            return false;
        }
        
        // Update balances immediately
        const std::string& sender = tx.getSender();
        const std::string& recipient = tx.getRecipient();
        double amount = tx.getAmount();
        
        // Check sender balance
        if (blockchain.getBalance(sender) < amount) {
            return false;
        }
        
        // Update balances instantly
        blockchain.updateBalance(sender, -amount);
        blockchain.updateBalance(recipient, amount);
        
        Logger::info("Instant confirmation: " + std::to_string(amount) + 
                    " NIL from " + sender + " to " + recipient);
        
        return true;
    }
};

// ============================================================================
// FEE-BASED TRANSACTION PRIORITIZATION
// ============================================================================

class TransactionPrioritizer {
public:
    static void sortByFee(std::deque<Transaction>& pendingTransactions) {
        std::sort(pendingTransactions.begin(), pendingTransactions.end(),
                  [](const Transaction& a, const Transaction& b) {
                      // Higher fees get priority
                      double feeA = a.getFee();
                      double feeB = b.getFee();
                      
                      if (feeA != feeB) {
                          return feeA > feeB;
                      }
                      
                      // If fees are equal, prioritize by timestamp (FIFO)
                      return a.getTimestamp() < b.getTimestamp();
                  });
    }
    
    static double calculateDynamicFee(const Transaction& tx) {
        double baseFee = 0.001;
        double amountFee = tx.getAmount() * 0.0001; // 0.01% of amount
        return baseFee + amountFee;
    }
};

// ============================================================================
// IMPROVED MINING ENGINE
// ============================================================================

class ImprovedMiningEngine {
private:
    Blockchain& blockchain;
    ImprovedMiningConfig config;
    
public:
    ImprovedMiningEngine(Blockchain& bc) : blockchain(bc) {}
    
    Block minePendingTransactions(const std::string& miningRewardAddress) {
        std::lock_guard<std::mutex> lockChain(blockchain.getChainMutex());
        std::lock_guard<std::mutex> lockTx(blockchain.getTxMutex());
        
        // Create coinbase transaction
        Transaction coinbaseTx("COINBASE", miningRewardAddress, config.miningReward);
        
        // Create new block
        uint64_t newIndex = blockchain.getLatestBlock().getIndex() + 1;
        Block newBlock(newIndex, blockchain.getLatestBlock().getHash());
        
        // Add coinbase transaction first
        newBlock.addTransaction(coinbaseTx);
        
        // Get pending transactions and prioritize by fee
        auto pendingTxs = blockchain.getPendingTransactions();
        TransactionPrioritizer::sortByFee(pendingTxs);
        
        // Add pending transactions to block (up to limit)
        size_t count = 0;
        for (const auto& tx : pendingTxs) {
            if (count >= config.maxTransactionsPerBlock) {
                break;
            }
            
            // Try instant confirmation for small amounts
            if (config.enableInstantConfirmation && 
                InstantConfirmation::canProcessInstantly(tx)) {
                if (InstantConfirmation::processInstantTransaction(blockchain, tx)) {
                    // Transaction processed instantly, skip adding to block
                    continue;
                }
            }
            
            // Add to block for mining
            if (newBlock.addTransaction(tx)) {
                count++;
            }
        }
        
        // Mine the block with reduced difficulty
        Logger::info("Mining block " + std::to_string(newIndex) + 
                    " with " + std::to_string(count + 1) + " transactions");
        
        auto startTime = std::chrono::steady_clock::now();
        newBlock.mineBlock(config.targetDifficulty);
        auto endTime = std::chrono::steady_clock::now();
        
        auto miningTime = std::chrono::duration_cast<std::chrono::milliseconds>
                         (endTime - startTime).count();
        
        Logger::info("Block mined in " + std::to_string(miningTime) + "ms");
        
        // Add block to chain
        if (blockchain.addBlock(newBlock)) {
            Logger::info("Block added successfully: " + newBlock.getHash());
        } else {
            Logger::error("Failed to add block to chain");
        }
        
        return newBlock;
    }
};

// ============================================================================
// TRANSACTION STATUS API
// ============================================================================

class TransactionStatusAPI {
private:
    Blockchain& blockchain;
    
public:
    TransactionStatusAPI(Blockchain& bc) : blockchain(bc) {}
    
    nlohmann::json getTransactionStatus(const std::string& txHash) {
        nlohmann::json response;
        
        // Check if transaction is in pending pool
        auto pendingTxs = blockchain.getPendingTransactions();
        auto it = std::find_if(pendingTxs.begin(), pendingTxs.end(),
                              [&txHash](const Transaction& tx) {
                                  return tx.getHash() == txHash;
                              });
        
        if (it != pendingTxs.end()) {
            // Transaction is pending
            size_t position = std::distance(pendingTxs.begin(), it);
            double estimatedTime = calculateEstimatedTime(position);
            
            response["transaction_hash"] = txHash;
            response["status"] = "pending";
            response["position_in_queue"] = position;
            response["estimated_confirmation_time"] = estimatedTime;
            response["fee"] = it->getFee();
            response["priority"] = getPriority(it->getFee());
        } else {
            // Check if transaction is in a block
            auto chain = blockchain.getChain();
            bool found = false;
            
            for (const auto& block : chain) {
                for (const auto& tx : block.getTransactions()) {
                    if (tx.getHash() == txHash) {
                        response["transaction_hash"] = txHash;
                        response["status"] = "confirmed";
                        response["block_index"] = block.getIndex();
                        response["block_hash"] = block.getHash();
                        response["confirmation_time"] = block.getTimestamp();
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            
            if (!found) {
                response["transaction_hash"] = txHash;
                response["status"] = "not_found";
                response["error"] = "Transaction not found in blockchain";
            }
        }
        
        return response;
    }
    
private:
    double calculateEstimatedTime(size_t position) {
        // Estimate time based on position in queue
        // Assuming 2.4 seconds per block and 50 transactions per block
        double blocksNeeded = (double)position / 50.0;
        return blocksNeeded * 2.4; // seconds
    }
    
    std::string getPriority(double fee) {
        if (fee > 0.01) return "high";
        if (fee > 0.005) return "medium";
        return "normal";
    }
};

// ============================================================================
// IMPROVED API ENDPOINTS
// ============================================================================

class ImprovedAPI {
private:
    Blockchain& blockchain;
    TransactionStatusAPI statusAPI;
    
public:
    ImprovedAPI(Blockchain& bc) : blockchain(bc), statusAPI(bc) {}
    
    std::string handleTransactionStatus(const std::string& path, const std::string& method) {
        // Handle /transaction/{hash}/status endpoint
        if (path.substr(0, 12) == "/transaction/" && path.length() > 12) {
            std::string txHash = path.substr(12);
            
            // Remove "/status" if present
            size_t statusPos = txHash.find("/status");
            if (statusPos != std::string::npos) {
                txHash = txHash.substr(0, statusPos);
            }
            
            nlohmann::json response = statusAPI.getTransactionStatus(txHash);
            return Utils::createJsonResponse(200, response);
        }
        
        return ""; // Not handled by this class
    }
    
    std::string handleImprovedTransaction(const std::string& body) {
        try {
            nlohmann::json tx_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!tx_data.contains("sender") || !tx_data.contains("recipient") || 
                !tx_data.contains("amount")) {
                return Utils::createJsonErrorResponse(400, "Missing required fields");
            }
            
            std::string sender = tx_data["sender"].get<std::string>();
            std::string recipient = tx_data["recipient"].get<std::string>();
            double amount = tx_data["amount"].get<double>();
            
            // Create transaction
            Transaction tx(sender, recipient, amount);
            tx.signTransaction("demo-key");
            
            // Try instant confirmation for small amounts
            if (InstantConfirmation::canProcessInstantly(tx)) {
                if (InstantConfirmation::processInstantTransaction(blockchain, tx)) {
                    nlohmann::json response;
                    response["success"] = true;
                    response["message"] = "Transaction confirmed instantly";
                    response["transaction_hash"] = tx.getHash();
                    response["confirmation_type"] = "instant";
                    response["amount"] = amount;
                    
                    return Utils::createJsonResponse(200, response);
                }
            }
            
            // Regular mining confirmation
            if (blockchain.addTransaction(tx)) {
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Transaction added to pending pool";
                response["transaction_hash"] = tx.getHash();
                response["confirmation_type"] = "mining";
                response["estimated_time"] = "2.4 seconds";
                
                return Utils::createJsonResponse(201, response);
            } else {
                return Utils::createJsonErrorResponse(400, "Failed to add transaction");
            }
            
        } catch (const std::exception& e) {
            return Utils::createJsonErrorResponse(400, std::string("Error: ") + e.what());
        }
    }
};

// ============================================================================
// USAGE EXAMPLE
// ============================================================================

/*
// In your main.cpp, replace the existing mining and transaction handling:

// 1. Initialize improved components
ImprovedMiningEngine improvedMiner(blockchain);
ImprovedAPI improvedAPI(blockchain);

// 2. Use improved mining
Block newBlock = improvedMiner.minePendingTransactions(minerAddress);

// 3. Use improved transaction handling
std::string response = improvedAPI.handleImprovedTransaction(requestBody);

// 4. Add transaction status endpoint
std::string statusResponse = improvedAPI.handleTransactionStatus(path, method);
*/ 