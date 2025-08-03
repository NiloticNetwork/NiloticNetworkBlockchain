#include "mining.h"
#include "utils.h"
#include "logger.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

// MiningStats implementation
void MiningStats::updateStats(uint64_t blockTime, uint64_t difficulty, double reward, double fees) {
    totalBlocksMined++;
    totalRewardsEarned += reward;
    totalFeesEarned += fees;
    currentDifficulty = difficulty;
    
    if (fastestBlockTime == 0 || blockTime < fastestBlockTime) {
        fastestBlockTime = blockTime;
    }
    if (blockTime > slowestBlockTime) {
        slowestBlockTime = blockTime;
    }
    
    recentBlockTimes.push_back(blockTime);
    if (recentBlockTimes.size() > 100) {
        recentBlockTimes.erase(recentBlockTimes.begin());
    }
    
    // Calculate average mining time
    if (!recentBlockTimes.empty()) {
        averageMiningTime = std::accumulate(recentBlockTimes.begin(), recentBlockTimes.end(), 0ULL) / recentBlockTimes.size();
    }
    
    lastBlockTime = std::chrono::steady_clock::now();
}

void MiningStats::reset() {
    totalBlocksMined = 0;
    totalTransactionsProcessed = 0;
    totalRewardsEarned = 0.0;
    totalFeesEarned = 0.0;
    averageMiningTime = 0;
    fastestBlockTime = 0;
    slowestBlockTime = 0;
    currentDifficulty = 0;
    difficultyChanges = 0;
    recentBlockTimes.clear();
}

nlohmann::json MiningStats::toJson() const {
    nlohmann::json json;
    json["totalBlocksMined"] = totalBlocksMined;
    json["totalTransactionsProcessed"] = totalTransactionsProcessed;
    json["totalRewardsEarned"] = totalRewardsEarned;
    json["totalFeesEarned"] = totalFeesEarned;
    json["averageMiningTime"] = averageMiningTime;
    json["fastestBlockTime"] = fastestBlockTime;
    json["slowestBlockTime"] = slowestBlockTime;
    json["currentDifficulty"] = currentDifficulty;
    json["difficultyChanges"] = difficultyChanges;
    json["recentBlockTimes"] = recentBlockTimes;
    return json;
}

// MiningWorker implementation
MiningWorker::MiningWorker(uint64_t startNonce, uint64_t endNonce, uint64_t difficulty,
                           const std::string& targetHash, const std::string& blockData,
                           const std::string& minerAddress)
    : startNonce(startNonce), endNonce(endNonce), currentDifficulty(difficulty),
      targetHash(targetHash), blockData(blockData), minerAddress(minerAddress),
      running(false), shouldStop(false), solutionFound(false), hashesComputed(0) {
}

MiningWorker::~MiningWorker() {
    stop();
}

void MiningWorker::start() {
    if (running) return;
    
    running = true;
    shouldStop = false;
    solutionFound = false;
    hashesComputed = 0;
    startTime = std::chrono::steady_clock::now();
    
    workerThread = std::thread(&MiningWorker::miningLoop, this);
    Logger::info("Mining worker started for range: " + std::to_string(startNonce) + " - " + std::to_string(endNonce));
}

void MiningWorker::stop() {
    if (!running) return;
    
    shouldStop = true;
    running = false;
    
    if (workerThread.joinable()) {
        workerThread.join();
    }
    
    Logger::info("Mining worker stopped");
}

void MiningWorker::miningLoop() {
    for (uint64_t nonce = startNonce; nonce <= endNonce && !shouldStop; ++nonce) {
        hashesComputed++;
        
        std::string hash = calculateHash(nonce);
        if (checkHash(hash, nonce)) {
            solutionFound = true;
            solutionHash = hash;
            solutionNonce = nonce;
            Logger::info("Mining solution found! Nonce: " + std::to_string(nonce) + ", Hash: " + hash);
            break;
        }
        
        // Check if another worker found a solution
        if (solutionFound) {
            break;
        }
    }
}

bool MiningWorker::checkHash(const std::string& hash, uint64_t nonce) {
    // Check if hash meets difficulty requirement
    std::string target(currentDifficulty, '0');
    return hash.substr(0, currentDifficulty) == target;
}

std::string MiningWorker::calculateHash(uint64_t nonce) {
    std::string data = blockData + std::to_string(nonce);
    return Utils::calculateSHA256(data);
}

double MiningWorker::getHashRate() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    if (duration.count() == 0) return 0.0;
    return static_cast<double>(hashesComputed) / duration.count();
}

// MiningEngine implementation
MiningEngine::MiningEngine(Blockchain& blockchain, const MiningConfig& config)
    : blockchain(blockchain), config(config), isMining(false), shouldStop(false),
      currentDifficulty(config.targetDifficulty), lastDifficultyAdjustment(0) {
    Logger::info("Mining engine initialized with difficulty: " + std::to_string(currentDifficulty));
}

MiningEngine::~MiningEngine() {
    stopMining();
}

bool MiningEngine::startMining(const std::string& minerAddress) {
    if (isMining) {
        Logger::warning("Mining already in progress");
        return false;
    }
    
    isMining = true;
    shouldStop = false;
    
    miningThread = std::thread(&MiningEngine::miningLoop, this, minerAddress);
    Logger::info("Mining started for address: " + minerAddress);
    return true;
}

void MiningEngine::stopMining() {
    if (!isMining) return;
    
    shouldStop = true;
    isMining = false;
    
    // Stop all workers
    for (auto& worker : workers) {
        worker->stop();
    }
    workers.clear();
    
    if (miningThread.joinable()) {
        miningThread.join();
    }
    
    Logger::info("Mining stopped");
}

bool MiningEngine::addTransaction(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    // Check if transaction is already in queue
    for (const auto& tx : pendingTransactions) {
        if (tx.calculateHash() == transaction.calculateHash()) {
            return false;
        }
    }
    
    pendingTransactions.push_back(transaction);
    Logger::debug("Transaction added to mining queue: " + transaction.calculateHash());
    return true;
}

bool MiningEngine::removeTransaction(const std::string& transactionId) {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    auto it = std::find_if(pendingTransactions.begin(), pendingTransactions.end(),
                           [&](const Transaction& tx) { return tx.calculateHash() == transactionId; });
    
    if (it != pendingTransactions.end()) {
        pendingTransactions.erase(it);
        Logger::debug("Transaction removed from mining queue: " + transactionId);
        return true;
    }
    
    return false;
}

std::vector<Transaction> MiningEngine::getPendingTransactions() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return pendingTransactions;
}

void MiningEngine::clearPendingTransactions() {
    std::lock_guard<std::mutex> lock(queueMutex);
    pendingTransactions.clear();
    Logger::info("Mining queue cleared");
}

Block MiningEngine::mineBlock(const std::string& minerAddress, uint64_t maxAttempts) {
    auto startTime = std::chrono::steady_clock::now();
    
    // Create a new block
    uint64_t blockIndex = blockchain.getLatestBlock().getIndex() + 1;
    std::string previousHash = blockchain.getChain().empty() ? "0" : blockchain.getLatestBlock().getHash();
    
    Block block(blockIndex, previousHash);
    
    // Add coinbase transaction
    double reward = calculateBlockReward(blockIndex);
    Transaction coinbaseTx("COINBASE", minerAddress, reward);
    block.addTransaction(coinbaseTx);
    
    // Add pending transactions
    std::vector<Transaction> selectedTransactions = selectTransactionsForBlock();
    for (const auto& tx : selectedTransactions) {
        block.addTransaction(tx);
    }
    
    // Mine the block
    uint64_t nonce = 0;
    uint64_t blockchainDifficulty = blockchain.getDifficulty();
    std::string target(blockchainDifficulty, '0');
    std::string blockHash;
    
    Logger::info("Starting to mine block " + std::to_string(blockIndex) + " with difficulty " + std::to_string(blockchainDifficulty));
    
    while (!shouldStop && (maxAttempts == 0 || nonce < maxAttempts)) {
        block.setNonce(nonce);
        block.updateHash(); // Update the block's stored hash
        blockHash = block.getHash(); // Get the updated hash
        
        // Debug: Log every 1000th attempt
        if (nonce % 1000 == 0) {
            Logger::info("Mining attempt " + std::to_string(nonce) + ", Hash: " + blockHash.substr(0, 10) + "...");
        }
        
        if (blockHash.substr(0, blockchainDifficulty) == target) {
            Logger::info("Block mined successfully! Hash: " + blockHash + ", Nonce: " + std::to_string(nonce));
            
            // Update the block's hash with the valid hash
            // Note: The block's hash is calculated in calculateHash(), but we need to ensure it's set
            // The block's hash should be updated when we call calculateHash() after setting the nonce
            
            auto endTime = std::chrono::steady_clock::now();
            auto miningTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            
            // Update statistics
            updateMiningStats(block, miningTime);
            
            // Adjust difficulty if needed
            if (config.enableDynamicDifficulty) {
                adjustDifficulty();
            }
            
            return block;
        }
        
        nonce++;
    }
    
    Logger::warning("Mining stopped without finding a solution");
    return Block(-1, "0"); // Return invalid block to indicate failure
}

Block MiningEngine::mineBlockWithTransactions(const std::string& minerAddress, 
                                             const std::vector<Transaction>& transactions) {
    uint64_t blockIndex = blockchain.getLatestBlock().getIndex() + 1;
    std::string previousHash = blockchain.getChain().empty() ? "0" : blockchain.getLatestBlock().getHash();
    
    Block block(blockIndex, previousHash);
    
    // Add transactions
    for (const auto& tx : transactions) {
        block.addTransaction(tx);
    }
    
    // Mine the block
    return mineBlock(minerAddress);
}

uint64_t MiningEngine::getCurrentDifficulty() const {
    return currentDifficulty;
}

uint64_t MiningEngine::calculateNewDifficulty() {
    std::lock_guard<std::mutex> lock(difficultyMutex);
    
    if (recentBlockTimes.size() < 2) {
        return currentDifficulty;
    }
    
    // Calculate average block time
    uint64_t totalTime = std::accumulate(recentBlockTimes.begin(), recentBlockTimes.end(), 0ULL);
    uint64_t averageTime = totalTime / recentBlockTimes.size();
    
    // Adjust difficulty based on target block time
    uint64_t newDifficulty = currentDifficulty;
    
    if (averageTime < config.targetBlockTime * 0.8) {
        // Blocks are being mined too fast, increase difficulty
        newDifficulty = std::min(currentDifficulty + 1, config.maxDifficulty);
    } else if (averageTime > config.targetBlockTime * 1.2) {
        // Blocks are being mined too slow, decrease difficulty
        newDifficulty = std::max(currentDifficulty - 1, config.minDifficulty);
    }
    
    return newDifficulty;
}

void MiningEngine::adjustDifficulty() {
    uint64_t newDifficulty = calculateNewDifficulty();
    
    if (newDifficulty != currentDifficulty) {
        std::lock_guard<std::mutex> lock(difficultyMutex);
        currentDifficulty = newDifficulty;
        stats.difficultyChanges++;
        
        Logger::info("Difficulty adjusted to: " + std::to_string(currentDifficulty));
        logMiningEvent("difficulty_adjusted", {{"old_difficulty", currentDifficulty}, {"new_difficulty", newDifficulty}});
    }
}

bool MiningEngine::validateDifficulty(const Block& block) const {
    // Skip difficulty validation for genesis block (index 0)
    if (block.getIndex() == 0) {
        return true;
    }
    
    std::string hash = block.calculateHash();
    std::string target(currentDifficulty, '0');
    return hash.substr(0, currentDifficulty) == target;
}

bool MiningEngine::addMiningPool(const std::string& name, const std::string& address, double fee) {
    for (const auto& pool : miningPools) {
        if (pool.name == name) {
            return false;
        }
    }
    
    miningPools.push_back({name, address, fee, true});
    Logger::info("Mining pool added: " + name);
    return true;
}

bool MiningEngine::removeMiningPool(const std::string& name) {
    auto it = std::find_if(miningPools.begin(), miningPools.end(),
                           [&](const MiningPool& pool) { return pool.name == name; });
    
    if (it != miningPools.end()) {
        miningPools.erase(it);
        Logger::info("Mining pool removed: " + name);
        return true;
    }
    
    return false;
}

std::vector<MiningEngine::MiningPool> MiningEngine::getMiningPools() const {
    return miningPools;
}

MiningStats MiningEngine::getMiningStats() const {
    return stats;
}

nlohmann::json MiningEngine::getMiningStatus() const {
    nlohmann::json status;
    status["isMining"] = isMining.load();
    status["currentDifficulty"] = currentDifficulty;
    status["pendingTransactions"] = pendingTransactions.size();
    status["stats"] = stats.toJson();
    status["config"] = {
        {"targetDifficulty", config.targetDifficulty},
        {"maxDifficulty", config.maxDifficulty},
        {"minDifficulty", config.minDifficulty},
        {"targetBlockTime", config.targetBlockTime},
        {"miningReward", config.miningReward},
        {"transactionFee", config.transactionFee}
    };
    return status;
}

double MiningEngine::getCurrentHashRate() const {
    double totalHashRate = 0.0;
    for (const auto& worker : workers) {
        totalHashRate += worker->getHashRate();
    }
    return totalHashRate;
}

uint64_t MiningEngine::getEstimatedTimeToNextBlock() const {
    double hashRate = getCurrentHashRate();
    if (hashRate == 0.0) return 0;
    
    // Calculate probability of finding a hash with required difficulty
    double probability = 1.0 / (1ULL << (currentDifficulty * 4)); // 4 bits per hex character
    double expectedAttempts = 1.0 / probability;
    
    return static_cast<uint64_t>(expectedAttempts / hashRate);
}

void MiningEngine::updateConfig(const MiningConfig& newConfig) {
    config = newConfig;
    Logger::info("Mining configuration updated");
}

bool MiningEngine::validateBlock(const Block& block) const {
    // Validate block structure
    if (block.getIndex() < 0) return false;
    if (block.getHash().empty()) return false;
    
    // Validate difficulty (skip for genesis block)
    if (block.getIndex() > 0 && !validateDifficulty(block)) return false;
    
    // Validate transactions
    for (const auto& tx : block.getTransactions()) {
        if (!validateTransaction(tx)) return false;
    }
    
    return true;
}

bool MiningEngine::validateTransaction(const Transaction& transaction) const {
    // Basic transaction validation
    if (transaction.getSender().empty() && transaction.getRecipient().empty()) return false;
    if (transaction.getAmount() < 0) return false;
    
    return true;
}

void MiningEngine::miningLoop(const std::string& minerAddress) {
    Logger::info("Mining loop started for address: " + minerAddress);
    
    while (isMining && !shouldStop) {
        // Check if we have pending transactions
        std::vector<Transaction> transactions = getPendingTransactions();
        
        if (!transactions.empty()) {
            // Mine a block with transactions
            Block block = mineBlockWithTransactions(minerAddress, transactions);
            
            if (block.getIndex() >= 0) {
                // Add block to blockchain
                if (blockchain.addBlock(block)) {
                    Logger::info("Block " + std::to_string(block.getIndex()) + " added to blockchain");
                    
                    // Remove mined transactions from queue
                    for (const auto& tx : block.getTransactions()) {
                        removeTransaction(tx.calculateHash());
                    }
                } else {
                    Logger::error("Failed to add block to blockchain");
                }
            }
        } else {
            // Mine empty block if no transactions
            Block block = mineBlock(minerAddress);
            
            if (block.getIndex() >= 0) {
                if (blockchain.addBlock(block)) {
                    Logger::info("Empty block " + std::to_string(block.getIndex()) + " added to blockchain");
                }
            }
        }
        
        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::vector<Transaction> MiningEngine::selectTransactionsForBlock() {
    std::vector<Transaction> selected;
    uint64_t blockSize = 0;
    
    // Get pending transactions from the blockchain
    auto blockchainPendingTxs = blockchain.getPendingTransactions();
    
    // Sort transactions by fee (higher fees first)
    std::sort(blockchainPendingTxs.begin(), blockchainPendingTxs.end(),
              [](const Transaction& a, const Transaction& b) {
                  // In a real implementation, you'd calculate actual fees
                  return a.getAmount() > b.getAmount();
              });
    
    for (const auto& tx : blockchainPendingTxs) {
        // Check block size limit
        if (blockSize + tx.calculateHash().length() > config.maxBlockSize) {
            break;
        }
        
        // Check transaction count limit
        if (selected.size() >= config.maxTransactionsPerBlock) {
            break;
        }
        
        selected.push_back(tx);
        blockSize += tx.calculateHash().length();
    }
    
    return selected;
}

double MiningEngine::calculateTransactionFees(const std::vector<Transaction>& transactions) {
    return transactions.size() * config.transactionFee;
}

void MiningEngine::updateMiningStats(const Block& block, uint64_t miningTime) {
    double reward = calculateBlockReward(block.getIndex());
    double fees = calculateTransactionFees(block.getTransactions());
    
    stats.updateStats(miningTime, currentDifficulty, reward, fees);
    stats.totalTransactionsProcessed += block.getTransactions().size();
}

void MiningEngine::logMiningEvent(const std::string& event, const nlohmann::json& data) {
    nlohmann::json logEntry;
    logEntry["event"] = event;
    logEntry["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    logEntry["data"] = data;
    
    Logger::info("Mining event: " + event + " - " + data.dump());
}

std::string MiningEngine::createBlockHeader(const Block& block, uint64_t nonce) {
    std::stringstream ss;
    ss << block.getIndex() << block.getTimestamp() << block.getPreviousHash() << nonce;
    
    for (const auto& tx : block.getTransactions()) {
        ss << tx.calculateHash();
    }
    
    return ss.str();
}

bool MiningEngine::isHashValid(const std::string& hash, uint64_t difficulty) {
    std::string target(difficulty, '0');
    return hash.substr(0, difficulty) == target;
}

uint64_t MiningEngine::calculateBlockReward(uint64_t blockHeight) {
    // Simple halving every 210,000 blocks (like Bitcoin)
    uint64_t halvings = blockHeight / 210000;
    double reward = config.miningReward;
    
    for (uint64_t i = 0; i < halvings; ++i) {
        reward /= 2.0;
    }
    
    return static_cast<uint64_t>(reward);
}

std::string MiningEngine::createCoinbaseTransaction(const std::string& minerAddress, double reward) {
    // In a real implementation, this would create a proper Transaction object
    return "COINBASE:" + minerAddress + ":" + std::to_string(reward);
}

// MiningPool implementation
MiningPool::MiningPool(const std::string& name, const std::string& address, double fee)
    : poolName(name), poolAddress(address), poolFee(fee), active(true) {
}

bool MiningPool::addMiner(const std::string& minerAddress) {
    if (std::find(miners.begin(), miners.end(), minerAddress) != miners.end()) {
        return false;
    }
    
    miners.push_back(minerAddress);
    minerShares[minerAddress] = 0.0;
    return true;
}

bool MiningPool::removeMiner(const std::string& minerAddress) {
    auto it = std::find(miners.begin(), miners.end(), minerAddress);
    if (it == miners.end()) {
        return false;
    }
    
    miners.erase(it);
    minerShares.erase(minerAddress);
    return true;
}

bool MiningPool::isMinerActive(const std::string& minerAddress) const {
    return std::find(miners.begin(), miners.end(), minerAddress) != miners.end();
}

void MiningPool::addShare(const std::string& minerAddress, double share) {
    if (isMinerActive(minerAddress)) {
        minerShares[minerAddress] += share;
    }
}

double MiningPool::getMinerShares(const std::string& minerAddress) const {
    auto it = minerShares.find(minerAddress);
    return it != minerShares.end() ? it->second : 0.0;
}

void MiningPool::distributeRewards(double totalReward) {
    double totalShares = getTotalShares();
    if (totalShares == 0.0) return;
    
    double poolFeeAmount = totalReward * poolFee;
    double remainingReward = totalReward - poolFeeAmount;
    
    for (const auto& miner : miners) {
        double minerShare = getMinerShares(miner);
        double minerReward = (minerShare / totalShares) * remainingReward;
        // In a real implementation, you'd actually transfer the reward
    }
}

double MiningPool::getTotalShares() const {
    double total = 0.0;
    for (const auto& pair : minerShares) {
        total += pair.second;
    }
    return total;
}

nlohmann::json MiningPool::getPoolStats() const {
    nlohmann::json stats;
    stats["name"] = poolName;
    stats["address"] = poolAddress;
    stats["fee"] = poolFee;
    stats["active"] = active.load();
    stats["minerCount"] = miners.size();
    stats["totalShares"] = getTotalShares();
    stats["miners"] = miners;
    return stats;
}

// ConsensusEngine implementation
ConsensusEngine::ConsensusEngine(Blockchain& blockchain, MiningEngine& miningEngine)
    : blockchain(blockchain), miningEngine(miningEngine) {
}

bool ConsensusEngine::validateBlockConsensus(const Block& block) const {
    // Validate block size
    if (block.getTransactions().size() > maxBlockSize) {
        return false;
    }
    
    // Validate block time
    auto now = std::chrono::system_clock::now();
    auto blockTime = std::chrono::system_clock::from_time_t(block.getTimestamp());
    auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - blockTime).count();
    
    if (timeDiff > maxBlockTime) {
        return false;
    }
    
    // Validate mining difficulty
    if (!miningEngine.validateDifficulty(block)) {
        return false;
    }
    
    return true;
}

bool ConsensusEngine::validateTransactionConsensus(const Transaction& transaction) const {
    // Basic transaction validation
    if (transaction.getSender().empty() && transaction.getRecipient().empty()) {
        return false;
    }
    
    if (transaction.getAmount() < 0) {
        return false;
    }
    
    // Check if sender has sufficient balance (simplified)
    // In a real implementation, you'd check the actual balance
    
    return true;
}

bool ConsensusEngine::isBlockFinalized(uint64_t blockHeight) const {
    uint64_t currentHeight = blockchain.getChainHeight();
    return (currentHeight - blockHeight) >= requiredConfirmations;
}

std::vector<Block> ConsensusEngine::resolveFork(const std::vector<Block>& blocks) const {
    // Simple longest chain rule
    if (blocks.empty()) return std::vector<Block>();
    
    // In a real implementation, you'd implement proper fork resolution
    // For now, just return the first block
    return {blocks[0]};
}

bool ConsensusEngine::isLongestChain(const std::vector<Block>& chain) const {
    return chain.size() >= blockchain.getChainHeight();
}

bool ConsensusEngine::validateStake(const std::string& address, double amount) const {
    // In a real implementation, you'd check actual stake
    return amount >= minimumStake;
}

double ConsensusEngine::getStakeWeight(const std::string& address) const {
    // In a real implementation, you'd calculate actual stake weight
    return 1.0; // Placeholder
} 