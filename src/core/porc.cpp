#include "porc.h"
#include "logger.h"
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cstring>

// Utility function to convert bytes to hex string
std::string bytesToHex(const unsigned char* data, size_t length) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

// Utility function to calculate SHA-256 hash
std::string calculateSHA256(const std::string& input) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, input.c_str(), input.length());
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    
    return bytesToHex(hash, hash_len);
}

// PoRCTask implementation
std::string PoRCTask::serialize() const {
    nlohmann::json j;
    j["type"] = static_cast<int>(type);
    j["taskId"] = taskId;
    j["assignedWallet"] = assignedWallet;
    j["timestamp"] = timestamp;
    j["blockHeight"] = blockHeight;
    j["data"] = data;
    j["estimatedBandwidth"] = estimatedBandwidth;
    j["estimatedTransactions"] = estimatedTransactions;
    return j.dump();
}

PoRCTask PoRCTask::deserialize(const std::string& data) {
    PoRCTask task;
    try {
        nlohmann::json j = nlohmann::json::parse(data);
        task.type = static_cast<PoRCTaskType>(j["type"].get<int>());
        task.taskId = j["taskId"];
        task.assignedWallet = j["assignedWallet"];
        task.timestamp = j["timestamp"];
        task.blockHeight = j["blockHeight"];
        task.data = j["data"];
        task.estimatedBandwidth = j["estimatedBandwidth"];
        task.estimatedTransactions = j["estimatedTransactions"];
    } catch (const std::exception& e) {
        Logger::error("Failed to deserialize PoRCTask: " + std::string(e.what()));
    }
    return task;
}

std::string PoRCTask::calculateHash() const {
    std::string data = taskId + assignedWallet + std::to_string(timestamp) + 
                      std::to_string(blockHeight) + std::to_string(estimatedBandwidth) +
                      std::to_string(estimatedTransactions);
    return calculateSHA256(data);
}

// PoRCContribution implementation
std::string PoRCContribution::serialize() const {
    nlohmann::json j;
    j["walletAddress"] = walletAddress;
    j["taskId"] = taskId;
    j["timestamp"] = timestamp;
    j["blockHeight"] = blockHeight;
    j["bandwidthUsed"] = bandwidthUsed;
    j["transactionsRelayed"] = transactionsRelayed;
    j["uptimeSeconds"] = uptimeSeconds;
    j["proofHash"] = proofHash;
    j["signature"] = signature;
    return j.dump();
}

PoRCContribution PoRCContribution::deserialize(const std::string& data) {
    PoRCContribution contribution;
    try {
        nlohmann::json j = nlohmann::json::parse(data);
        contribution.walletAddress = j["walletAddress"];
        contribution.taskId = j["taskId"];
        contribution.timestamp = j["timestamp"];
        contribution.blockHeight = j["blockHeight"];
        contribution.bandwidthUsed = j["bandwidthUsed"];
        contribution.transactionsRelayed = j["transactionsRelayed"];
        contribution.uptimeSeconds = j["uptimeSeconds"];
        contribution.proofHash = j["proofHash"];
        contribution.signature = j["signature"];
    } catch (const std::exception& e) {
        Logger::error("Failed to deserialize PoRCContribution: " + std::string(e.what()));
    }
    return contribution;
}

std::string PoRCContribution::calculateHash() const {
    std::string data = walletAddress + taskId + std::to_string(timestamp) + 
                      std::to_string(blockHeight) + std::to_string(bandwidthUsed) +
                      std::to_string(transactionsRelayed) + std::to_string(uptimeSeconds) +
                      proofHash;
    return calculateSHA256(data);
}

bool PoRCContribution::verifySignature(const std::string& publicKey) const {
    // Implementation would use OpenSSL to verify ECDSA signature
    // For now, return true as placeholder
    return true;
}

uint64_t PoRCContribution::calculateResourcePoints() const {
    uint64_t points = 0;
    points += bandwidthUsed * PoRCConfig::RESOURCE_POINT_MB;
    points += (transactionsRelayed / PoRCConfig::RESOURCE_POINT_TX);
    return points;
}

// PoRCWalletStatus implementation
nlohmann::json PoRCWalletStatus::toJson() const {
    nlohmann::json j;
    j["address"] = address;
    j["isEnabled"] = isEnabled;
    j["totalResourcePoints"] = totalResourcePoints;
    j["totalRewards"] = totalRewards;
    j["lastContribution"] = lastContribution;
    j["reputationScore"] = reputationScore;
    j["bandwidthLimit"] = bandwidthLimit;
    j["isEarlyAdopter"] = isEarlyAdopter;
    j["poolIndex"] = poolIndex;
    return j;
}

PoRCWalletStatus PoRCWalletStatus::fromJson(const nlohmann::json& json) {
    PoRCWalletStatus status;
    status.address = json["address"];
    status.isEnabled = json["isEnabled"];
    status.totalResourcePoints = json["totalResourcePoints"];
    status.totalRewards = json["totalRewards"];
    status.lastContribution = json["lastContribution"];
    status.reputationScore = json["reputationScore"];
    status.bandwidthLimit = json["bandwidthLimit"];
    status.isEarlyAdopter = json["isEarlyAdopter"];
    status.poolIndex = json["poolIndex"];
    return status;
}

// PoRCPool implementation
void PoRCPool::addWallet(const std::string& address) {
    if (!containsWallet(address)) {
        walletAddresses.push_back(address);
    }
}

void PoRCPool::removeWallet(const std::string& address) {
    walletAddresses.erase(
        std::remove(walletAddresses.begin(), walletAddresses.end(), address),
        walletAddresses.end()
    );
}

bool PoRCPool::containsWallet(const std::string& address) const {
    return std::find(walletAddresses.begin(), walletAddresses.end(), address) != walletAddresses.end();
}

nlohmann::json PoRCPool::toJson() const {
    nlohmann::json j;
    j["poolIndex"] = poolIndex;
    j["walletAddresses"] = walletAddresses;
    j["totalResourcePoints"] = totalResourcePoints;
    j["blockStart"] = blockStart;
    j["blockEnd"] = blockEnd;
    j["isActive"] = isActive;
    return j;
}

// PoRCStats implementation
nlohmann::json PoRCStats::toJson() const {
    nlohmann::json j;
    j["totalWallets"] = totalWallets;
    j["activeWallets"] = activeWallets;
    j["totalResourcePoints"] = totalResourcePoints;
    j["totalRewardsDistributed"] = totalRewardsDistributed;
    j["totalBurned"] = totalBurned;
    j["currentBlockReward"] = currentBlockReward;
    j["activePools"] = activePools;
    j["averageBandwidth"] = averageBandwidth;
    j["averageUptime"] = averageUptime;
    return j;
}

// PoRCSystem implementation
PoRCSystem::PoRCSystem() : db(nullptr), currentBlockHeight(0), totalWalletsRegistered(0), isRunning(false) {
    if (!initializeDatabase()) {
        Logger::error("Failed to initialize PoRC database");
    }
}

PoRCSystem::~PoRCSystem() {
    stop();
    if (db) {
        sqlite3_close(db);
    }
}

bool PoRCSystem::initializeDatabase() {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    int rc = sqlite3_open("porc.db", &db);
    if (rc != SQLITE_OK) {
        Logger::error("Failed to open PoRC database: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    
    return createTables();
}

bool PoRCSystem::createTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS wallet_status (
            address TEXT PRIMARY KEY,
            is_enabled INTEGER,
            total_resource_points INTEGER,
            total_rewards INTEGER,
            last_contribution INTEGER,
            reputation_score INTEGER,
            bandwidth_limit INTEGER,
            is_early_adopter INTEGER,
            pool_index INTEGER,
            created_at INTEGER,
            updated_at INTEGER
        );
        
        CREATE TABLE IF NOT EXISTS contributions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            wallet_address TEXT,
            task_id TEXT,
            timestamp INTEGER,
            block_height INTEGER,
            bandwidth_used INTEGER,
            transactions_relayed INTEGER,
            uptime_seconds INTEGER,
            proof_hash TEXT,
            signature TEXT,
            resource_points INTEGER,
            created_at INTEGER
        );
        
        CREATE TABLE IF NOT EXISTS pools (
            pool_index INTEGER PRIMARY KEY,
            wallet_addresses TEXT,
            total_resource_points INTEGER,
            block_start INTEGER,
            block_end INTEGER,
            is_active INTEGER,
            created_at INTEGER
        );
        
        CREATE TABLE IF NOT EXISTS tasks (
            task_id TEXT PRIMARY KEY,
            type INTEGER,
            assigned_wallet TEXT,
            timestamp INTEGER,
            block_height INTEGER,
            data TEXT,
            estimated_bandwidth INTEGER,
            estimated_transactions INTEGER,
            status INTEGER,
            created_at INTEGER
        );
        
        CREATE INDEX IF NOT EXISTS idx_contributions_wallet ON contributions(wallet_address);
        CREATE INDEX IF NOT EXISTS idx_contributions_block ON contributions(block_height);
        CREATE INDEX IF NOT EXISTS idx_tasks_wallet ON tasks(assigned_wallet);
        CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);
    )";
    
    char* errMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::error("Failed to create PoRC tables: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool PoRCSystem::start() {
    if (isRunning) {
        return true;
    }
    
    isRunning = true;
    
    // Start worker threads
    taskAssignmentThread = std::thread(&PoRCSystem::taskAssignmentLoop, this);
    rewardDistributionThread = std::thread(&PoRCSystem::rewardDistributionLoop, this);
    poolRotationThread = std::thread(&PoRCSystem::poolRotationLoop, this);
    
    Logger::info("PoRC system started successfully");
    return true;
}

void PoRCSystem::stop() {
    if (!isRunning) {
        return;
    }
    
    isRunning = false;
    
    // Wait for threads to finish
    if (taskAssignmentThread.joinable()) {
        taskAssignmentThread.join();
    }
    if (rewardDistributionThread.joinable()) {
        rewardDistributionThread.join();
    }
    if (poolRotationThread.joinable()) {
        poolRotationThread.join();
    }
    
    Logger::info("PoRC system stopped");
}

bool PoRCSystem::enablePoRC(const std::string& address, uint64_t bandwidthLimit) {
    std::lock_guard<std::mutex> lock(walletMutex);
    
    PoRCWalletStatus status;
    status.address = address;
    status.isEnabled = true;
    status.bandwidthLimit = bandwidthLimit;
    status.isEarlyAdopter = isEarlyAdopter(address);
    status.poolIndex = totalWalletsRegistered % PoRCConfig::POOL_SIZE;
    
    walletStatuses[address] = status;
    totalWalletsRegistered++;
    
    if (!saveWalletStatus(status)) {
        Logger::error("Failed to save wallet status for " + address);
        return false;
    }
    
    Logger::info("PoRC enabled for wallet: " + address);
    return true;
}

bool PoRCSystem::disablePoRC(const std::string& address) {
    std::lock_guard<std::mutex> lock(walletMutex);
    
    auto it = walletStatuses.find(address);
    if (it != walletStatuses.end()) {
        it->second.isEnabled = false;
        saveWalletStatus(it->second);
        Logger::info("PoRC disabled for wallet: " + address);
        return true;
    }
    
    return false;
}

bool PoRCSystem::isWalletEnabled(const std::string& address) const {
    std::lock_guard<std::mutex> lock(walletMutex);
    auto it = walletStatuses.find(address);
    return it != walletStatuses.end() && it->second.isEnabled;
}

PoRCWalletStatus PoRCSystem::getWalletStatus(const std::string& address) const {
    std::lock_guard<std::mutex> lock(walletMutex);
    auto it = walletStatuses.find(address);
    if (it != walletStatuses.end()) {
        return it->second;
    }
    return PoRCWalletStatus();
}

std::vector<PoRCTask> PoRCSystem::getTasksForWallet(const std::string& address) {
    std::lock_guard<std::mutex> lock(taskMutex);
    
    std::vector<PoRCTask> tasks;
    std::queue<PoRCTask> tempQueue = taskQueue;
    
    while (!tempQueue.empty()) {
        PoRCTask task = tempQueue.front();
        tempQueue.pop();
        
        if (task.assignedWallet == address) {
            tasks.push_back(task);
        }
    }
    
    return tasks;
}

bool PoRCSystem::submitContribution(const PoRCContribution& contribution) {
    std::lock_guard<std::mutex> lock(contributionMutex);
    
    if (!validateContribution(contribution)) {
        Logger::error("Invalid contribution submitted by " + contribution.walletAddress);
        return false;
    }
    
    pendingContributions.push_back(contribution);
    
    if (!saveContribution(contribution)) {
        Logger::error("Failed to save contribution for " + contribution.walletAddress);
        return false;
    }
    
    Logger::info("Contribution submitted by " + contribution.walletAddress + 
                " - Points: " + std::to_string(contribution.calculateResourcePoints()));
    return true;
}

bool PoRCSystem::verifyTaskCompletion(const std::string& taskId, const std::string& walletAddress) {
    std::lock_guard<std::mutex> lock(contributionMutex);
    
    for (const auto& contribution : pendingContributions) {
        if (contribution.taskId == taskId && contribution.walletAddress == walletAddress) {
            return true;
        }
    }
    
    return false;
}

PoRCStats PoRCSystem::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

std::vector<PoRCPool> PoRCSystem::getActivePools() const {
    std::lock_guard<std::mutex> lock(poolsMutex);
    return pools;
}

std::vector<PoRCWalletStatus> PoRCSystem::getTopContributors(uint64_t limit) const {
    std::lock_guard<std::mutex> lock(walletMutex);
    
    std::vector<PoRCWalletStatus> contributors;
    for (const auto& pair : walletStatuses) {
        if (pair.second.isEnabled) {
            contributors.push_back(pair.second);
        }
    }
    
    // Sort by total resource points
    std::sort(contributors.begin(), contributors.end(),
              [](const PoRCWalletStatus& a, const PoRCWalletStatus& b) {
                  return a.totalResourcePoints > b.totalResourcePoints;
              });
    
    if (contributors.size() > limit) {
        contributors.resize(limit);
    }
    
    return contributors;
}

// Database operations
bool PoRCSystem::saveWalletStatus(const PoRCWalletStatus& status) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    const char* sql = R"(
        INSERT OR REPLACE INTO wallet_status 
        (address, is_enabled, total_resource_points, total_rewards, last_contribution,
         reputation_score, bandwidth_limit, is_early_adopter, pool_index, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        Logger::error("Failed to prepare wallet status statement: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    sqlite3_bind_text(stmt, 1, status.address.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, status.isEnabled ? 1 : 0);
    sqlite3_bind_int64(stmt, 3, status.totalResourcePoints);
    sqlite3_bind_int64(stmt, 4, status.totalRewards);
    sqlite3_bind_int64(stmt, 5, status.lastContribution);
    sqlite3_bind_int64(stmt, 6, status.reputationScore);
    sqlite3_bind_int64(stmt, 7, status.bandwidthLimit);
    sqlite3_bind_int(stmt, 8, status.isEarlyAdopter ? 1 : 0);
    sqlite3_bind_int64(stmt, 9, status.poolIndex);
    sqlite3_bind_int64(stmt, 10, now);
    sqlite3_bind_int64(stmt, 11, now);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool PoRCSystem::loadWalletStatus(const std::string& address, PoRCWalletStatus& status) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    const char* sql = "SELECT * FROM wallet_status WHERE address = ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, address.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        status.address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        status.isEnabled = sqlite3_column_int(stmt, 1) != 0;
        status.totalResourcePoints = sqlite3_column_int64(stmt, 2);
        status.totalRewards = sqlite3_column_int64(stmt, 3);
        status.lastContribution = sqlite3_column_int64(stmt, 4);
        status.reputationScore = sqlite3_column_int64(stmt, 5);
        status.bandwidthLimit = sqlite3_column_int64(stmt, 6);
        status.isEarlyAdopter = sqlite3_column_int(stmt, 7) != 0;
        status.poolIndex = sqlite3_column_int64(stmt, 8);
        
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}

bool PoRCSystem::saveContribution(const PoRCContribution& contribution) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    const char* sql = R"(
        INSERT INTO contributions 
        (wallet_address, task_id, timestamp, block_height, bandwidth_used,
         transactions_relayed, uptime_seconds, proof_hash, signature, resource_points, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    sqlite3_bind_text(stmt, 1, contribution.walletAddress.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, contribution.taskId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, contribution.timestamp);
    sqlite3_bind_int64(stmt, 4, contribution.blockHeight);
    sqlite3_bind_int64(stmt, 5, contribution.bandwidthUsed);
    sqlite3_bind_int64(stmt, 6, contribution.transactionsRelayed);
    sqlite3_bind_int64(stmt, 7, contribution.uptimeSeconds);
    sqlite3_bind_text(stmt, 8, contribution.proofHash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, contribution.signature.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 10, contribution.calculateResourcePoints());
    sqlite3_bind_int64(stmt, 11, now);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// Thread functions
void PoRCSystem::taskAssignmentLoop() {
    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        generateTasks();
    }
}

void PoRCSystem::rewardDistributionLoop() {
    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        distributeRewards();
    }
}

void PoRCSystem::poolRotationLoop() {
    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(24)); // Rotate every 24 seconds (10 blocks)
        rotatePools();
    }
}

// Task generation
void PoRCSystem::generateTasks() {
    std::lock_guard<std::mutex> lock(poolsMutex);
    
    for (auto& pool : pools) {
        if (pool.isActive) {
            assignTasksToPool(pool);
        }
    }
}

void PoRCSystem::assignTasksToPool(PoRCPool& pool) {
    for (const auto& address : pool.walletAddresses) {
        if (isWalletEnabled(address)) {
            // Create different types of tasks
            PoRCTask relayTask = createRelayTask(address);
            PoRCTask blockTask = createBlockPropagationTask(address);
            
            std::lock_guard<std::mutex> lock(taskMutex);
            taskQueue.push(relayTask);
            taskQueue.push(blockTask);
        }
    }
}

PoRCTask PoRCSystem::createRelayTask(const std::string& walletAddress) {
    PoRCTask task;
    task.type = PoRCTaskType::RELAY_TRANSACTIONS;
    task.taskId = generateTaskId();
    task.assignedWallet = walletAddress;
    task.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    task.blockHeight = currentBlockHeight;
    task.estimatedBandwidth = 10; // 10 MB
    task.estimatedTransactions = 50; // 50 transactions
    
    return task;
}

PoRCTask PoRCSystem::createBlockPropagationTask(const std::string& walletAddress) {
    PoRCTask task;
    task.type = PoRCTaskType::PROPAGATE_BLOCK;
    task.taskId = generateTaskId();
    task.assignedWallet = walletAddress;
    task.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    task.blockHeight = currentBlockHeight;
    task.estimatedBandwidth = 5; // 5 MB
    task.estimatedTransactions = 0;
    
    return task;
}

PoRCTask PoRCSystem::createCacheTask(const std::string& walletAddress) {
    PoRCTask task;
    task.type = PoRCTaskType::CACHE_DATA;
    task.taskId = generateTaskId();
    task.assignedWallet = walletAddress;
    task.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    task.blockHeight = currentBlockHeight;
    task.estimatedBandwidth = 20; // 20 MB
    task.estimatedTransactions = 0;
    
    return task;
}

// Reward distribution
void PoRCSystem::distributeRewards() {
    std::lock_guard<std::mutex> lock(contributionMutex);
    
    if (pendingContributions.empty()) {
        return;
    }
    
    uint64_t totalPoints = 0;
    std::map<std::string, uint64_t> walletPoints;
    
    // Calculate total points and points per wallet
    for (const auto& contribution : pendingContributions) {
        uint64_t points = contribution.calculateResourcePoints();
        totalPoints += points;
        walletPoints[contribution.walletAddress] += points;
    }
    
    if (totalPoints == 0) {
        return;
    }
    
    // Calculate rewards
    double blockReward = static_cast<double>(PoRCConfig::DAILY_REWARD_POOL) / PoRCConfig::BLOCKS_PER_DAY;
    
    for (const auto& pair : walletPoints) {
        const std::string& address = pair.first;
        uint64_t points = pair.second;
        
        double reward = calculateReward(address, points, totalPoints);
        
        // Update wallet status
        std::lock_guard<std::mutex> walletLock(walletMutex);
        auto it = walletStatuses.find(address);
        if (it != walletStatuses.end()) {
            it->second.totalRewards += static_cast<uint64_t>(reward * 1000000); // Convert to micro NIL
            it->second.totalResourcePoints += points;
            saveWalletStatus(it->second);
        }
    }
    
    // Clear processed contributions
    pendingContributions.clear();
    
    // Update statistics
    std::lock_guard<std::mutex> statsLock(statsMutex);
    stats.totalRewardsDistributed += static_cast<uint64_t>(blockReward * 1000000);
    stats.currentBlockReward = static_cast<uint64_t>(blockReward * 1000000);
}

double PoRCSystem::calculateReward(const std::string& address, uint64_t resourcePoints, uint64_t totalPoints) {
    double baseReward = static_cast<double>(PoRCConfig::DAILY_REWARD_POOL) / PoRCConfig::BLOCKS_PER_DAY;
    double proportionalReward = (static_cast<double>(resourcePoints) / totalPoints) * baseReward;
    
    // Apply bonding curve
    double bondingMultiplier = 1.0;
    std::lock_guard<std::mutex> lock(walletMutex);
    auto it = walletStatuses.find(address);
    if (it != walletStatuses.end() && it->second.isEarlyAdopter) {
        bondingMultiplier = PoRCConfig::BONDING_CURVE_EARLY;
    }
    
    double finalReward = proportionalReward * bondingMultiplier;
    
    // Apply cap
    if (finalReward > PoRCConfig::MAX_REWARD_PER_BLOCK) {
        finalReward = PoRCConfig::MAX_REWARD_PER_BLOCK;
    }
    
    return finalReward;
}

// Utility functions
uint64_t PoRCSystem::calculateReputationScore(const std::string& address, uint64_t balance, uint64_t activity) {
    // Simple reputation calculation based on balance and activity
    uint64_t score = balance * 10 + activity * 100;
    
    // Cap the score
    if (score > 10000) {
        score = 10000;
    }
    
    return score;
}

bool PoRCSystem::isEarlyAdopter(const std::string& address) {
    // Check if wallet is among first 1000 registered
    return totalWalletsRegistered < PoRCConfig::EARLY_ADOPTER_LIMIT;
}

std::string PoRCSystem::generateTaskId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 999999);
    
    std::stringstream ss;
    ss << "task_" << std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "_" << dis(gen);
    return ss.str();
}

// Validation functions
bool PoRCSystem::validateWalletEligibility(const std::string& address, uint64_t balance, uint64_t activity) {
    return balance >= PoRCConfig::MIN_BALANCE && activity >= PoRCConfig::MIN_ACTIVITY;
}

bool PoRCSystem::validateContribution(const PoRCContribution& contribution) {
    // Basic validation
    if (contribution.walletAddress.empty() || contribution.taskId.empty()) {
        return false;
    }
    
    if (contribution.bandwidthUsed == 0 && contribution.transactionsRelayed == 0) {
        return false;
    }
    
    // Verify signature
    // This would require the wallet's public key
    // For now, return true as placeholder
    return true;
}

bool PoRCSystem::validateTask(const PoRCTask& task) {
    return !task.taskId.empty() && !task.assignedWallet.empty();
}

// API endpoint handlers
nlohmann::json PoRCSystem::handleEnableRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        std::string address = request["address"];
        uint64_t bandwidthLimit = request.value("bandwidthLimit", 50);
        
        if (enablePoRC(address, bandwidthLimit)) {
            response["success"] = true;
            response["message"] = "PoRC enabled successfully";
        } else {
            response["success"] = false;
            response["message"] = "Failed to enable PoRC";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

nlohmann::json PoRCSystem::handleStatsRequest(const nlohmann::json& request) {
    nlohmann::json response;
    response["success"] = true;
    response["stats"] = getStats().toJson();
    return response;
}

nlohmann::json PoRCSystem::handleSubmitLogRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        PoRCContribution contribution;
        contribution.walletAddress = request["walletAddress"];
        contribution.taskId = request["taskId"];
        contribution.timestamp = request["timestamp"];
        contribution.blockHeight = request["blockHeight"];
        contribution.bandwidthUsed = request["bandwidthUsed"];
        contribution.transactionsRelayed = request["transactionsRelayed"];
        contribution.uptimeSeconds = request["uptimeSeconds"];
        contribution.proofHash = request["proofHash"];
        contribution.signature = request["signature"];
        
        if (submitContribution(contribution)) {
            response["success"] = true;
            response["message"] = "Contribution submitted successfully";
        } else {
            response["success"] = false;
            response["message"] = "Failed to submit contribution";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

nlohmann::json PoRCSystem::handleWalletStatusRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        std::string address = request["address"];
        PoRCWalletStatus status = getWalletStatus(address);
        
        response["success"] = true;
        response["status"] = status.toJson();
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

nlohmann::json PoRCSystem::handlePoolStatusRequest(const nlohmann::json& request) {
    nlohmann::json response;
    response["success"] = true;
    
    std::vector<PoRCPool> pools = getActivePools();
    nlohmann::json poolsJson = nlohmann::json::array();
    
    for (const auto& pool : pools) {
        poolsJson.push_back(pool.toJson());
    }
    
    response["pools"] = poolsJson;
    return response;
}

// Integration with blockchain
void PoRCSystem::onBlockMined(uint64_t blockHeight) {
    currentBlockHeight = blockHeight;
    
    // Rotate pools if needed
    if (blockHeight % PoRCConfig::POOL_ROTATION_BLOCKS == 0) {
        rotatePools();
    }
    
    // Process contributions and distribute rewards
    distributeRewards();
}

void PoRCSystem::onTransactionCreated(const std::string& transactionId) {
    // Generate new tasks for transaction relay
    generateTasks();
}

void PoRCSystem::rotatePools() {
    std::lock_guard<std::mutex> lock(poolsMutex);
    
    // Create new pools
    pools.clear();
    
    std::vector<std::string> enabledWallets;
    {
        std::lock_guard<std::mutex> walletLock(walletMutex);
        for (const auto& pair : walletStatuses) {
            if (pair.second.isEnabled) {
                enabledWallets.push_back(pair.first);
            }
        }
    }
    
    // Distribute wallets across pools
    for (size_t i = 0; i < enabledWallets.size(); i += PoRCConfig::POOL_SIZE) {
        PoRCPool pool;
        pool.poolIndex = pools.size();
        pool.isActive = true;
        pool.blockStart = currentBlockHeight;
        pool.blockEnd = currentBlockHeight + PoRCConfig::POOL_ROTATION_BLOCKS;
        
        for (size_t j = i; j < std::min(i + PoRCConfig::POOL_SIZE, static_cast<uint64_t>(enabledWallets.size())); ++j) {
            pool.addWallet(enabledWallets[j]);
        }
        
        pools.push_back(pool);
        savePool(pool);
    }
    
    Logger::info("PoRC pools rotated - " + std::to_string(pools.size()) + " active pools");
}

bool PoRCSystem::savePool(const PoRCPool& pool) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    const char* sql = R"(
        INSERT OR REPLACE INTO pools 
        (pool_index, wallet_addresses, total_resource_points, block_start, block_end, is_active, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    nlohmann::json addressesJson = pool.walletAddresses;
    
    sqlite3_bind_int64(stmt, 1, pool.poolIndex);
    sqlite3_bind_text(stmt, 2, addressesJson.dump().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, pool.totalResourcePoints);
    sqlite3_bind_int64(stmt, 4, pool.blockStart);
    sqlite3_bind_int64(stmt, 5, pool.blockEnd);
    sqlite3_bind_int(stmt, 6, pool.isActive ? 1 : 0);
    sqlite3_bind_int64(stmt, 7, now);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// PoRCAPI implementation
std::string PoRCAPI::handlePOST(const std::string& endpoint, const std::string& body) {
    try {
        nlohmann::json request = nlohmann::json::parse(body);
        nlohmann::json response;
        
        if (endpoint == "/porc/enable") {
            response = porcSystem.handleEnableRequest(request);
        } else if (endpoint == "/porc/submit_log") {
            response = porcSystem.handleSubmitLogRequest(request);
        } else {
            response["success"] = false;
            response["message"] = "Unknown endpoint";
        }
        
        return response.dump();
    } catch (const std::exception& e) {
        nlohmann::json response;
        response["success"] = false;
        response["message"] = "Invalid JSON format";
        return response.dump();
    }
}

std::string PoRCAPI::handleGET(const std::string& endpoint) {
    nlohmann::json response;
    
    if (endpoint == "/porc/stats") {
        response = porcSystem.handleStatsRequest(nlohmann::json());
    } else if (endpoint == "/porc/pools") {
        response = porcSystem.handlePoolStatusRequest(nlohmann::json());
    } else {
        response["success"] = false;
        response["message"] = "Unknown endpoint";
    }
    
    return response.dump();
}
