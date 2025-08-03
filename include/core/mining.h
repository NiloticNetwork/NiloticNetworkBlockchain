#ifndef MINING_H
#define MINING_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <mutex>
#include <condition_variable>
#include "block.h"
#include "transaction.h"
#include "blockchain.h"
#include "wallet.h"
#include "json.hpp"

// Mining configuration
struct MiningConfig {
    uint64_t targetDifficulty = 4;           // Target difficulty (leading zeros)
    uint64_t maxDifficulty = 8;              // Maximum difficulty
    uint64_t minDifficulty = 2;              // Minimum difficulty
    uint64_t difficultyAdjustmentBlocks = 2016; // Blocks between difficulty adjustments
    uint64_t targetBlockTime = 600;          // Target block time in seconds (10 minutes)
    uint64_t maxBlockSize = 1024 * 1024;    // Maximum block size in bytes
    uint64_t maxTransactionsPerBlock = 1000; // Maximum transactions per block
    double miningReward = 100.0;             // Mining reward in coins
    double transactionFee = 0.001;           // Transaction fee in coins
    bool enableDynamicDifficulty = true;     // Enable dynamic difficulty adjustment
    bool enableMiningPool = false;           // Enable mining pool support
    uint64_t maxNonce = 0xFFFFFFFF;         // Maximum nonce value
    uint64_t miningThreads = 4;              // Number of mining threads
};

// Mining statistics
struct MiningStats {
    uint64_t totalBlocksMined = 0;
    uint64_t totalTransactionsProcessed = 0;
    double totalRewardsEarned = 0.0;
    double totalFeesEarned = 0.0;
    uint64_t averageMiningTime = 0;
    uint64_t fastestBlockTime = 0;
    uint64_t slowestBlockTime = 0;
    uint64_t currentDifficulty = 0;
    uint64_t difficultyChanges = 0;
    std::chrono::steady_clock::time_point lastBlockTime;
    std::vector<uint64_t> recentBlockTimes;
    
    void updateStats(uint64_t blockTime, uint64_t difficulty, double reward, double fees);
    void reset();
    nlohmann::json toJson() const;
};

// Mining worker thread
class MiningWorker {
private:
    std::thread workerThread;
    std::atomic<bool> running;
    std::atomic<bool> shouldStop;
    std::mutex workerMutex;
    std::condition_variable workerCV;
    
    // Mining parameters
    uint64_t startNonce;
    uint64_t endNonce;
    uint64_t currentDifficulty;
    std::string targetHash;
    std::string blockData;
    std::string minerAddress;
    
    // Results
    std::atomic<bool> solutionFound;
    std::string solutionHash;
    uint64_t solutionNonce;
    
    // Statistics
    uint64_t hashesComputed;
    std::chrono::steady_clock::time_point startTime;
    
public:
    MiningWorker(uint64_t startNonce, uint64_t endNonce, uint64_t difficulty, 
                 const std::string& targetHash, const std::string& blockData, 
                 const std::string& minerAddress);
    ~MiningWorker();
    
    void start();
    void stop();
    bool isRunning() const { return running; }
    bool hasSolution() const { return solutionFound; }
    std::string getSolutionHash() const { return solutionHash; }
    uint64_t getSolutionNonce() const { return solutionNonce; }
    uint64_t getHashesComputed() const { return hashesComputed; }
    double getHashRate() const;
    
private:
    void miningLoop();
    bool checkHash(const std::string& hash, uint64_t nonce);
    std::string calculateHash(uint64_t nonce);
};

// Main mining engine
class MiningEngine {
private:
    Blockchain& blockchain;
    MiningConfig config;
    MiningStats stats;
    
    // Mining state
    std::atomic<bool> isMining;
    std::atomic<bool> shouldStop;
    std::vector<std::unique_ptr<MiningWorker>> workers;
    std::thread miningThread;
    std::mutex miningMutex;
    std::condition_variable miningCV;
    
    // Mining queue
    std::vector<Transaction> pendingTransactions;
    mutable std::mutex queueMutex;
    
    // Difficulty management
    uint64_t currentDifficulty;
    uint64_t lastDifficultyAdjustment;
    std::vector<uint64_t> recentBlockTimes;
    mutable std::mutex difficultyMutex;
    
    // Mining pool support
    struct MiningPool {
        std::string name;
        std::string address;
        double fee;
        bool active;
    };
    std::vector<MiningPool> miningPools;
    
public:
    MiningEngine(Blockchain& blockchain, const MiningConfig& config = MiningConfig());
    ~MiningEngine();
    
    // Mining control
    bool startMining(const std::string& minerAddress);
    void stopMining();
    bool isMiningActive() const { return isMining.load(); }
    
    // Transaction management
    bool addTransaction(const Transaction& transaction);
    bool removeTransaction(const std::string& transactionId);
    std::vector<Transaction> getPendingTransactions() const;
    void clearPendingTransactions();
    
    // Block mining
    Block mineBlock(const std::string& minerAddress, uint64_t maxAttempts = 0);
    Block mineBlockWithTransactions(const std::string& minerAddress, 
                                   const std::vector<Transaction>& transactions);
    
    // Difficulty management
    uint64_t getCurrentDifficulty() const;
    uint64_t calculateNewDifficulty();
    void adjustDifficulty();
    bool validateDifficulty(const Block& block) const;
    
    // Mining pool management
    bool addMiningPool(const std::string& name, const std::string& address, double fee);
    bool removeMiningPool(const std::string& name);
    std::vector<MiningPool> getMiningPools() const;
    
    // Statistics and monitoring
    MiningStats getMiningStats() const;
    nlohmann::json getMiningStatus() const;
    double getCurrentHashRate() const;
    uint64_t getEstimatedTimeToNextBlock() const;
    
    // Configuration
    void updateConfig(const MiningConfig& newConfig);
    MiningConfig getConfig() const { return config; }
    
    // Validation
    bool validateBlock(const Block& block) const;
    bool validateTransaction(const Transaction& transaction) const;
    
    // Block reward calculation
    uint64_t calculateBlockReward(uint64_t blockHeight);
    
private:
    void miningLoop(const std::string& minerAddress);
    std::vector<Transaction> selectTransactionsForBlock();
    double calculateTransactionFees(const std::vector<Transaction>& transactions);
    void updateMiningStats(const Block& block, uint64_t miningTime);
    void logMiningEvent(const std::string& event, const nlohmann::json& data = {});
    
    // Helper functions
    std::string createBlockHeader(const Block& block, uint64_t nonce);
    bool isHashValid(const std::string& hash, uint64_t difficulty);
    std::string createCoinbaseTransaction(const std::string& minerAddress, double reward);
};

// Mining pool implementation
class MiningPool {
private:
    std::string poolName;
    std::string poolAddress;
    double poolFee;
    std::vector<std::string> miners;
    std::map<std::string, double> minerShares;
    std::atomic<bool> active;
    
public:
    MiningPool(const std::string& name, const std::string& address, double fee);
    
    // Pool management
    bool addMiner(const std::string& minerAddress);
    bool removeMiner(const std::string& minerAddress);
    bool isMinerActive(const std::string& minerAddress) const;
    
    // Share management
    void addShare(const std::string& minerAddress, double share);
    double getMinerShares(const std::string& minerAddress) const;
    void distributeRewards(double totalReward);
    
    // Pool statistics
    size_t getMinerCount() const { return miners.size(); }
    double getTotalShares() const;
    nlohmann::json getPoolStats() const;
    
    // Getters
    std::string getName() const { return poolName; }
    std::string getAddress() const { return poolAddress; }
    double getFee() const { return poolFee; }
    bool isActive() const { return active; }
    void setActive(bool status) { active = status; }
};

// Consensus mechanism
class ConsensusEngine {
private:
    Blockchain& blockchain;
    MiningEngine& miningEngine;
    
    // Consensus parameters
    uint64_t requiredConfirmations = 6;
    uint64_t maxBlockSize = 1024 * 1024;
    uint64_t maxBlockTime = 600;
    double minimumStake = 1000.0;
    
public:
    ConsensusEngine(Blockchain& blockchain, MiningEngine& miningEngine);
    
    // Consensus validation
    bool validateBlockConsensus(const Block& block) const;
    bool validateTransactionConsensus(const Transaction& transaction) const;
    bool isBlockFinalized(uint64_t blockHeight) const;
    
    // Fork resolution
    std::vector<Block> resolveFork(const std::vector<Block>& blocks) const;
    bool isLongestChain(const std::vector<Block>& chain) const;
    
    // Stake validation
    bool validateStake(const std::string& address, double amount) const;
    double getStakeWeight(const std::string& address) const;
    
    // Configuration
    void setRequiredConfirmations(uint64_t confirmations) { requiredConfirmations = confirmations; }
    void setMaxBlockSize(uint64_t size) { maxBlockSize = size; }
    void setMaxBlockTime(uint64_t time) { maxBlockTime = time; }
    void setMinimumStake(double stake) { minimumStake = stake; }
    
    // Getters
    uint64_t getRequiredConfirmations() const { return requiredConfirmations; }
    uint64_t getMaxBlockSize() const { return maxBlockSize; }
    uint64_t getMaxBlockTime() const { return maxBlockTime; }
    double getMinimumStake() const { return minimumStake; }
};

#endif // MINING_H 