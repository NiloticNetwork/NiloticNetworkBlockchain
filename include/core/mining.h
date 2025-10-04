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
#include "consensus_harmony.h"
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

// Enhanced mining engine with consensus harmony integration
class MiningEngine : public ConsensusEngine {
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
    
    // Harmony integration state
    std::atomic<bool> harmonyInitialized;
    std::atomic<bool> harmonyHealthy;
    
    // Harmony metrics
    struct HarmonyMetrics {
        uint64_t totalHarmonyValidations = 0;
        uint64_t successfulHarmonyValidations = 0;
        uint64_t harmonyConflicts = 0;
        double averageConfidence = 0.0;
        std::chrono::steady_clock::time_point lastHarmonyUpdate;
        std::map<std::string, double> parameterHistory;
        
        HarmonyMetrics() : lastHarmonyUpdate(std::chrono::steady_clock::now()) {}
    };
    HarmonyMetrics harmonyMetrics;
    mutable std::mutex harmonyMutex;
    
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
    
    // ConsensusEngine interface implementation
    bool validateBlock(const Block& block) override;
    bool validateTransaction(const Transaction& transaction) override;
    ConsensusResult processRequest(const ConsensusRequest& request) override;
    bool initialize() override;
    void shutdown() override;
    bool isHealthy() const override;
    ConsensusType getType() const override { return ConsensusType::PROOF_OF_WORK; }
    std::string getName() const override { return "ProofOfWorkEngine"; }
    nlohmann::json getStatus() const override;
    nlohmann::json getMetrics() const override;
    bool adjustParameters(const std::map<std::string, double>& parameters) override;
    std::map<std::string, double> getParameters() const override;
    
    // Harmony-specific validation methods
    bool validateBlockHarmony(const Block& block, const ConsensusRequest& request);
    bool validateTransactionHarmony(const Transaction& transaction, const ConsensusRequest& request);
    double calculateValidationConfidence(const Block& block) const;
    double calculateValidationConfidence(const Transaction& transaction) const;
    
    // Metrics collection for harmony integration
    void collectHarmonyMetrics();
    void updateHarmonyStats(const ConsensusResult& result);
    nlohmann::json getHarmonyMetrics() const;
    
    // Coordination with other consensus mechanisms
    bool coordinateWithRouter(const ConsensusRequest& request);
    void notifyValidationResult(const ConsensusResult& result);
    
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



#endif // MINING_H 