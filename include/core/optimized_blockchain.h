#ifndef OPTIMIZED_BLOCKCHAIN_H
#define OPTIMIZED_BLOCKCHAIN_H

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <condition_variable>
#include "block.h"
#include "transaction.h"
#include "smart_contract_vm.h"

// Performance monitoring
struct PerformanceMetrics {
    std::atomic<uint64_t> transactionsProcessed{0};
    std::atomic<uint64_t> blocksMined{0};
    std::atomic<uint64_t> averageResponseTime{0};
    std::atomic<uint64_t> memoryUsage{0};
    std::atomic<uint64_t> cpuUsage{0};
    std::chrono::steady_clock::time_point lastUpdate;
};

// Optimized transaction pool
class TransactionPool {
private:
    std::priority_queue<Transaction, std::vector<Transaction>, 
                       std::function<bool(const Transaction&, const Transaction&)>> pendingTransactions;
    std::mutex poolMutex;
    std::condition_variable poolCV;
    size_t maxPoolSize;
    std::atomic<bool> shutdown{false};

public:
    TransactionPool(size_t maxSize = 10000) 
        : pendingTransactions([](const Transaction& a, const Transaction& b) {
            return a.getTimestamp() > b.getTimestamp(); // Priority by timestamp
        }), maxPoolSize(maxSize) {}

    void addTransaction(const Transaction& tx) {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (pendingTransactions.size() < maxPoolSize) {
            pendingTransactions.push(tx);
            poolCV.notify_one();
        }
    }

    std::vector<Transaction> getTransactions(size_t maxCount) {
        std::lock_guard<std::mutex> lock(poolMutex);
        std::vector<Transaction> result;
        
        while (!pendingTransactions.empty() && result.size() < maxCount) {
            result.push_back(pendingTransactions.top());
            pendingTransactions.pop();
        }
        
        return result;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        return pendingTransactions.size();
    }
};

// Memory pool for efficient allocation
template<typename T>
class MemoryPool {
private:
    std::vector<std::unique_ptr<T[]>> pools;
    std::vector<size_t> poolSizes;
    std::mutex poolMutex;
    size_t currentPool;
    size_t currentIndex;

public:
    MemoryPool(size_t initialSize = 1000) : currentPool(0), currentIndex(0) {
        pools.push_back(std::make_unique<T[]>(initialSize));
        poolSizes.push_back(initialSize);
    }

    T* allocate() {
        std::lock_guard<std::mutex> lock(poolMutex);
        
        if (currentIndex >= poolSizes[currentPool]) {
            // Create new pool
            size_t newSize = poolSizes[currentPool] * 2;
            pools.push_back(std::make_unique<T[]>(newSize));
            poolSizes.push_back(newSize);
            currentPool++;
            currentIndex = 0;
        }
        
        return &pools[currentPool][currentIndex++];
    }
};

// Optimized blockchain with caching and performance monitoring
class OptimizedBlockchain {
private:
    std::vector<Block> chain;
    std::unordered_map<std::string, double> balances;
    std::unordered_map<std::string, double> stakes;
    std::map<std::string, std::any> contractStates;
    
    // Performance optimizations
    TransactionPool transactionPool;
    MemoryPool<Block> blockPool;
    MemoryPool<Transaction> transactionPool2;
    
    // Caching
    std::unordered_map<std::string, std::any> cache;
    std::mutex cacheMutex;
    std::chrono::steady_clock::time_point lastCacheCleanup;
    
    // Threading
    std::thread miningThread;
    std::thread validationThread;
    std::atomic<bool> shutdown{false};
    std::mutex chainMutex;
    
    // Performance monitoring
    PerformanceMetrics metrics;
    std::thread monitoringThread;
    
    // Smart contract VM
    std::unique_ptr<SmartContractVM> vm;
    
    // Rate limiting
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> rateLimitMap;
    std::mutex rateLimitMutex;
    const size_t maxRequestsPerMinute = 100;

public:
    OptimizedBlockchain() {
        vm = std::make_unique<SmartContractVM>();
        initializeChain();
        startBackgroundThreads();
    }

    ~OptimizedBlockchain() {
        shutdown = true;
        if (miningThread.joinable()) miningThread.join();
        if (validationThread.joinable()) validationThread.join();
        if (monitoringThread.joinable()) monitoringThread.join();
    }

    // Optimized block creation
    Block createBlock(const std::string& minerAddress) {
        auto start = std::chrono::steady_clock::now();
        
        Block* newBlock = blockPool.allocate();
        *newBlock = Block(chain.size(), getLatestBlock().getHash());
        
        // Add transactions from pool
        auto transactions = transactionPool.getTransactions(1000);
        for (const auto& tx : transactions) {
            newBlock->addTransaction(tx);
        }
        
        // Mine the block
        newBlock->mineBlock(4); // Difficulty level
        newBlock->setValidator(minerAddress);
        
        // Update metrics
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        metrics.averageResponseTime = (metrics.averageResponseTime + duration.count()) / 2;
        
        return *newBlock;
    }

    // Optimized transaction processing
    bool addTransaction(const Transaction& transaction) {
        // Rate limiting
        if (!checkRateLimit(transaction.getSender())) {
            return false;
        }
        
        // Validate transaction
        if (!transaction.isValid()) {
            return false;
        }
        
        // Check balance
        if (transaction.getSender() != "COINBASE") {
            double senderBalance = getBalance(transaction.getSender());
            if (senderBalance < transaction.getAmount()) {
                return false;
            }
        }
        
        // Add to transaction pool
        transactionPool.addTransaction(transaction);
        metrics.transactionsProcessed++;
        
        return true;
    }

    // Optimized balance checking with caching
    double getBalance(const std::string& address) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        // Check cache first
        auto cacheKey = "balance_" + address;
        auto it = cache.find(cacheKey);
        if (it != cache.end()) {
            auto cacheTime = std::chrono::steady_clock::now();
            auto cacheAge = std::chrono::duration_cast<std::chrono::seconds>(
                cacheTime - lastCacheCleanup).count();
            
            if (cacheAge < 60) { // Cache for 1 minute
                return std::any_cast<double>(it->second);
            }
        }
        
        // Calculate balance
        double balance = 0.0;
        for (const auto& block : chain) {
            for (const auto& tx : block.getTransactions()) {
                if (tx.getSender() == address) {
                    balance -= tx.getAmount();
                }
                if (tx.getRecipient() == address) {
                    balance += tx.getAmount();
                }
            }
        }
        
        // Cache the result
        cache[cacheKey] = balance;
        
        return balance;
    }

    // Smart contract execution with optimization
    std::any executeContract(const std::string& contractAddress, const std::string& functionName, 
                           const std::vector<std::any>& args) {
        auto start = std::chrono::steady_clock::now();
        
        // Check if contract exists
        auto it = contractStates.find(contractAddress);
        if (it == contractStates.end()) {
            throw std::runtime_error("Contract not found");
        }
        
        // Create execution context
        SmartContractContext context;
        context.sender = "system";
        context.contractAddress = contractAddress;
        context.gasLimit = 1000000;
        context.gasUsed = 0;
        
        // Execute contract
        try {
            vm->execute(context);
            
            // Update metrics
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            metrics.averageResponseTime = (metrics.averageResponseTime + duration.count()) / 2;
            
            return context.stack.empty() ? std::any() : context.stack.back();
        } catch (const std::exception& e) {
            throw std::runtime_error("Contract execution failed: " + std::string(e.what()));
        }
    }

    // Performance monitoring
    PerformanceMetrics getMetrics() const {
        return metrics;
    }

    // Health check
    bool isHealthy() const {
        auto now = std::chrono::steady_clock::now();
        auto lastUpdate = metrics.lastUpdate;
        auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count();
        
        return timeSinceUpdate < 300; // Consider healthy if updated in last 5 minutes
    }

private:
    void initializeChain() {
        // Create genesis block
        Block genesis(0, "0");
        genesis.addTransaction(Transaction("COINBASE", "genesis_wallet", 1000));
        chain.push_back(genesis);
        
        // Initialize balances
        balances["genesis_wallet"] = 1000;
    }

    void startBackgroundThreads() {
        // Mining thread
        miningThread = std::thread([this]() {
            while (!shutdown) {
                if (transactionPool.size() > 0) {
                    auto block = createBlock("system_miner");
                    addBlock(block);
                    metrics.blocksMined++;
                }
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        });
        
        // Validation thread
        validationThread = std::thread([this]() {
            while (!shutdown) {
                validateChain();
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        });
        
        // Monitoring thread
        monitoringThread = std::thread([this]() {
            while (!shutdown) {
                updateMetrics();
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        });
    }

    void addBlock(const Block& block) {
        std::lock_guard<std::mutex> lock(chainMutex);
        chain.push_back(block);
        
        // Update balances
        for (const auto& tx : block.getTransactions()) {
            if (tx.getSender() != "COINBASE") {
                balances[tx.getSender()] -= tx.getAmount();
            }
            balances[tx.getRecipient()] += tx.getAmount();
        }
    }

    Block getLatestBlock() const {
        std::lock_guard<std::mutex> lock(chainMutex);
        return chain.back();
    }

    void validateChain() {
        std::lock_guard<std::mutex> lock(chainMutex);
        
        for (size_t i = 1; i < chain.size(); i++) {
            if (chain[i].getPreviousHash() != chain[i-1].getHash()) {
                // Chain is invalid, trigger recovery
                recoverChain();
                break;
            }
        }
    }

    void recoverChain() {
        // Simple recovery: revert to last known good state
        if (chain.size() > 1) {
            chain.pop_back();
        }
    }

    bool checkRateLimit(const std::string& address) {
        std::lock_guard<std::mutex> lock(rateLimitMutex);
        
        auto now = std::chrono::steady_clock::now();
        auto it = rateLimitMap.find(address);
        
        if (it != rateLimitMap.end()) {
            auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second).count();
            
            if (timeSinceLastRequest < 60) { // 1 minute window
                return false;
            }
        }
        
        rateLimitMap[address] = now;
        return true;
    }

    void updateMetrics() {
        metrics.lastUpdate = std::chrono::steady_clock::now();
        
        // Clean up old cache entries
        auto now = std::chrono::steady_clock::now();
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastCacheCleanup).count();
        
        if (timeSinceCleanup > 300) { // Clean up every 5 minutes
            std::lock_guard<std::mutex> lock(cacheMutex);
            cache.clear();
            lastCacheCleanup = now;
        }
    }
};

#endif // OPTIMIZED_BLOCKCHAIN_H 