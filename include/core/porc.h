#ifndef PORC_H
#define PORC_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>
#include <functional>
#include <sqlite3.h>
#include "json.hpp"
#include "wallet.h"
#include "networking.h"

// PoRC Configuration Constants
namespace PoRCConfig {
    constexpr uint64_t MIN_BALANCE = 5;                    // Minimum NIL balance required
    constexpr uint64_t MIN_ACTIVITY = 1;                   // Minimum transactions in 30 days
    constexpr uint64_t DAILY_REWARD_POOL = 500;            // Total daily reward pool in NIL
    constexpr uint64_t BLOCKS_PER_DAY = 36000;             // Blocks per day (2.4s block time)
    constexpr double BONDING_CURVE_EARLY = 1.5;            // Early adopter multiplier
    constexpr uint64_t EARLY_ADOPTER_LIMIT = 1000;         // First 1000 wallets get 1.5x
    constexpr double MAX_REWARD_PER_BLOCK = 0.5;         // Max reward per wallet per block
    constexpr uint64_t POOL_SIZE = 100;                    // Wallets per rotating pool
    constexpr uint64_t POOL_ROTATION_BLOCKS = 10;          // Rotate pools every 10 blocks
    constexpr double BURN_RATE = 0.5;                      // 50% of transaction fees burned
    constexpr double TRANSACTION_FEE = 0.001;              // Transaction fee in NIL
    constexpr uint64_t RESOURCE_POINT_MB = 1;              // 1 point per MB relayed
    constexpr uint64_t RESOURCE_POINT_TX = 10;             // 1 point per 10 transactions
}

// PoRC Task Types
enum class PoRCTaskType {
    RELAY_TRANSACTIONS = 0,
    PROPAGATE_BLOCK = 1,
    CACHE_DATA = 2,
    VERIFY_PEERS = 3
};

// PoRC Task Structure
struct PoRCTask {
    PoRCTaskType type;
    std::string taskId;
    std::string assignedWallet;
    uint64_t timestamp;
    uint64_t blockHeight;
    nlohmann::json data;
    uint64_t estimatedBandwidth;  // in MB
    uint64_t estimatedTransactions;
    
    PoRCTask() : type(PoRCTaskType::RELAY_TRANSACTIONS), timestamp(0), blockHeight(0), 
                 estimatedBandwidth(0), estimatedTransactions(0) {}
    
    std::string serialize() const;
    static PoRCTask deserialize(const std::string& data);
    std::string calculateHash() const;
};

// PoRC Contribution Log
struct PoRCContribution {
    std::string walletAddress;
    std::string taskId;
    uint64_t timestamp;
    uint64_t blockHeight;
    uint64_t bandwidthUsed;       // MB relayed
    uint64_t transactionsRelayed; // Number of transactions
    uint64_t uptimeSeconds;       // Seconds of active contribution
    std::string proofHash;        // Cryptographic proof
    std::string signature;        // ECDSA signature
    
    PoRCContribution() : timestamp(0), blockHeight(0), bandwidthUsed(0), 
                        transactionsRelayed(0), uptimeSeconds(0) {}
    
    std::string serialize() const;
    static PoRCContribution deserialize(const std::string& data);
    std::string calculateHash() const;
    bool verifySignature(const std::string& publicKey) const;
    uint64_t calculateResourcePoints() const;
};

// PoRC Wallet Status
struct PoRCWalletStatus {
    std::string address;
    bool isEnabled;
    uint64_t totalResourcePoints;
    uint64_t totalRewards;
    uint64_t lastContribution;
    uint64_t reputationScore;
    uint64_t bandwidthLimit;      // MB per day
    bool isEarlyAdopter;
    uint64_t poolIndex;
    
    PoRCWalletStatus() : isEnabled(false), totalResourcePoints(0), totalRewards(0),
                        lastContribution(0), reputationScore(0), bandwidthLimit(50),
                        isEarlyAdopter(false), poolIndex(0) {}
    
    nlohmann::json toJson() const;
    static PoRCWalletStatus fromJson(const nlohmann::json& json);
};

// PoRC Pool Management
struct PoRCPool {
    uint64_t poolIndex;
    std::vector<std::string> walletAddresses;
    uint64_t totalResourcePoints;
    uint64_t blockStart;
    uint64_t blockEnd;
    bool isActive;
    
    PoRCPool() : poolIndex(0), totalResourcePoints(0), blockStart(0), 
                 blockEnd(0), isActive(false) {}
    
    void addWallet(const std::string& address);
    void removeWallet(const std::string& address);
    bool containsWallet(const std::string& address) const;
    nlohmann::json toJson() const;
};

// PoRC Statistics
struct PoRCStats {
    uint64_t totalWallets;
    uint64_t activeWallets;
    uint64_t totalResourcePoints;
    uint64_t totalRewardsDistributed;
    uint64_t totalBurned;
    uint64_t currentBlockReward;
    uint64_t activePools;
    double averageBandwidth;
    double averageUptime;
    
    PoRCStats() : totalWallets(0), activeWallets(0), totalResourcePoints(0),
                  totalRewardsDistributed(0), totalBurned(0), currentBlockReward(0),
                  activePools(0), averageBandwidth(0.0), averageUptime(0.0) {}
    
    nlohmann::json toJson() const;
};

// Main PoRC System Class
class PoRCSystem {
private:
    // Database connection
    sqlite3* db;
    std::mutex dbMutex;
    
    // Active pools and wallets
    std::vector<PoRCPool> pools;
    std::map<std::string, PoRCWalletStatus> walletStatuses;
    std::queue<PoRCTask> taskQueue;
    std::vector<PoRCContribution> pendingContributions;
    
    // Statistics and state
    PoRCStats stats;
    uint64_t currentBlockHeight;
    uint64_t totalWalletsRegistered;
    std::atomic<bool> isRunning;
    
    // Threads
    std::thread taskAssignmentThread;
    std::thread rewardDistributionThread;
    std::thread poolRotationThread;
    
    // Mutexes for thread safety
    mutable std::mutex poolsMutex;
    mutable std::mutex walletMutex;
    mutable std::mutex taskMutex;
    mutable std::mutex contributionMutex;
    mutable std::mutex statsMutex;
    
    // Initialize database
    bool initializeDatabase();
    bool createTables();
    
    // Pool management
    void rotatePools();
    void assignTasksToPool(PoRCPool& pool);
    PoRCPool* getActivePoolForWallet(const std::string& address);
    
    // Task management
    void generateTasks();
    PoRCTask createRelayTask(const std::string& walletAddress);
    PoRCTask createBlockPropagationTask(const std::string& walletAddress);
    PoRCTask createCacheTask(const std::string& walletAddress);
    
    // Contribution processing
    void processContributions();
    bool verifyContribution(const PoRCContribution& contribution);
    void updateWalletStats(const std::string& address, uint64_t resourcePoints);
    
    // Reward calculation
    double calculateReward(const std::string& address, uint64_t resourcePoints, uint64_t totalPoints);
    void distributeRewards();
    void burnTransactionFees();
    
    // Database operations
    bool saveWalletStatus(const PoRCWalletStatus& status);
    bool loadWalletStatus(const std::string& address, PoRCWalletStatus& status);
    bool saveContribution(const PoRCContribution& contribution);
    bool savePool(const PoRCPool& pool);
    bool loadPools();
    
    // Thread functions
    void taskAssignmentLoop();
    void rewardDistributionLoop();
    void poolRotationLoop();
    
    // Utility functions
    uint64_t calculateReputationScore(const std::string& address, uint64_t balance, uint64_t activity);
    bool isEarlyAdopter(const std::string& address);
    std::string generateTaskId();
    std::string signContribution(const PoRCContribution& contribution, const std::string& privateKey);

public:
    PoRCSystem();
    ~PoRCSystem();
    
    // System management
    bool start();
    void stop();
    bool isSystemRunning() const { return isRunning; }
    
    // Wallet management
    bool enablePoRC(const std::string& address, uint64_t bandwidthLimit = 50);
    bool disablePoRC(const std::string& address);
    bool isWalletEnabled(const std::string& address) const;
    PoRCWalletStatus getWalletStatus(const std::string& address) const;
    
    // Task and contribution management
    std::vector<PoRCTask> getTasksForWallet(const std::string& address);
    bool submitContribution(const PoRCContribution& contribution);
    bool verifyTaskCompletion(const std::string& taskId, const std::string& walletAddress);
    
    // Statistics and monitoring
    PoRCStats getStats() const;
    std::vector<PoRCPool> getActivePools() const;
    std::vector<PoRCWalletStatus> getTopContributors(uint64_t limit = 10) const;
    
    // API endpoints
    nlohmann::json handleEnableRequest(const nlohmann::json& request);
    nlohmann::json handleStatsRequest(const nlohmann::json& request);
    nlohmann::json handleSubmitLogRequest(const nlohmann::json& request);
    nlohmann::json handleWalletStatusRequest(const nlohmann::json& request);
    nlohmann::json handlePoolStatusRequest(const nlohmann::json& request);
    
    // Integration with blockchain
    void onBlockMined(uint64_t blockHeight);
    void onTransactionCreated(const std::string& transactionId);
    void updateBlockHeight(uint64_t height) { currentBlockHeight = height; }
    
    // Configuration
    void setRewardPool(uint64_t dailyReward);
    void setBondingCurve(double earlyMultiplier);
    void setPoolSize(uint64_t size);
    void setRotationBlocks(uint64_t blocks);
    
    // Database operations
    bool backupDatabase(const std::string& backupPath);
    bool restoreDatabase(const std::string& backupPath);
    bool clearDatabase();
    
    // Validation
    bool validateWalletEligibility(const std::string& address, uint64_t balance, uint64_t activity);
    bool validateContribution(const PoRCContribution& contribution);
    bool validateTask(const PoRCTask& task);
};

// PoRC API Handler
class PoRCAPI {
private:
    PoRCSystem& porcSystem;
    
public:
    PoRCAPI(PoRCSystem& system) : porcSystem(system) {}
    
    // API endpoint handlers
    std::string handlePOST(const std::string& endpoint, const std::string& body);
    std::string handleGET(const std::string& endpoint);
    
    // Specific endpoint handlers
    std::string handleEnable(const nlohmann::json& request);
    std::string handleStats(const nlohmann::json& request);
    std::string handleSubmitLog(const nlohmann::json& request);
    std::string handleWalletStatus(const nlohmann::json& request);
    std::string handlePoolStatus(const nlohmann::json& request);
};

#endif // PORC_H
