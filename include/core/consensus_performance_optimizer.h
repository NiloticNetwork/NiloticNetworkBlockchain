#ifndef CONSENSUS_PERFORMANCE_OPTIMIZER_H
#define CONSENSUS_PERFORMANCE_OPTIMIZER_H

#include "consensus_harmony.h"
#include "consensus_monitor.h"
#include <memory>
#include <unordered_map>
#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <type_traits>

// Forward declarations
#ifndef TEST_BUILD
class Block;
class Transaction;
#endif
class ConsensusEngine;

/**
 * Cache entry for consensus results
 */
struct CacheEntry {
    ConsensusResult result;
    std::chrono::steady_clock::time_point timestamp;
    uint64_t accessCount;
    double computationTime; // Time taken to compute this result
    
    CacheEntry() : accessCount(0), computationTime(0.0) {}
    
    CacheEntry(const ConsensusResult& r, double compTime = 0.0)
        : result(r), timestamp(std::chrono::steady_clock::now()), 
          accessCount(1), computationTime(compTime) {}
    
    bool isExpired(std::chrono::seconds maxAge) const {
        return std::chrono::steady_clock::now() - timestamp > maxAge;
    }
};

/**
 * Optimization-specific performance metrics extending the base PerformanceMetrics
 */
struct OptimizationMetrics : public PerformanceMetrics {
    // Cache metrics
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    uint64_t cacheEvictions = 0;
    double averageCacheHitTime = 0.0;
    double averageCacheMissTime = 0.0;
    
    // Parallel processing metrics
    uint64_t parallelValidations = 0;
    uint64_t sequentialValidations = 0;
    double averageParallelTime = 0.0;
    double averageSequentialTime = 0.0;
    uint32_t maxConcurrentValidations = 0;
    
    // Memory metrics
    size_t currentMemoryUsage = 0;
    size_t peakMemoryUsage = 0;
    uint64_t memoryOptimizations = 0;
    
    // Computation metrics
    uint64_t totalValidations = 0;
    double totalComputationTime = 0.0;
    double averageValidationTime = 0.0;
    
    // Thread pool metrics
    uint32_t activeThreads = 0;
    uint32_t queuedTasks = 0;
    double threadUtilization = 0.0;
    
    std::chrono::steady_clock::time_point lastUpdate;
    
    OptimizationMetrics() : lastUpdate(std::chrono::steady_clock::now()) {}
    
    double getCacheHitRatio() const {
        uint64_t total = cacheHits + cacheMisses;
        return total > 0 ? static_cast<double>(cacheHits) / total : 0.0;
    }
    
    double getParallelSpeedup() const {
        return (averageSequentialTime > 0 && averageParallelTime > 0) ? 
               averageSequentialTime / averageParallelTime : 1.0;
    }
};

/**
 * Configuration for performance optimization
 */
struct OptimizationConfig {
    // Cache configuration
    size_t maxCacheSize = 10000;
    std::chrono::seconds cacheExpirationTime{3600}; // 1 hour
    bool enableResultCaching = true;
    bool enableComputationMemoization = true;
    
    // Parallel processing configuration
    uint32_t maxWorkerThreads = std::thread::hardware_concurrency();
    uint32_t minParallelEngines = 2; // Minimum engines to use parallel processing
    bool enableParallelValidation = true;
    std::chrono::milliseconds taskTimeout{30000}; // 30 seconds
    
    // Memory optimization configuration
    bool enableMemoryOptimization = true;
    size_t maxMemoryUsage = 1024 * 1024 * 1024; // 1GB
    double memoryCleanupThreshold = 0.8; // Cleanup when 80% full
    std::chrono::seconds memoryCleanupInterval{300}; // 5 minutes
    
    // Performance monitoring
    bool enablePerformanceMonitoring = true;
    std::chrono::seconds metricsUpdateInterval{60}; // 1 minute
    bool enableBenchmarking = false;
};

/**
 * Task for parallel execution
 */
struct ValidationTask {
    std::string taskId;
    ConsensusRequest request;
    ConsensusEngine* engine;
    std::promise<ConsensusResult> promise;
    std::chrono::steady_clock::time_point startTime;
    
    ValidationTask(const std::string& id, const ConsensusRequest& req, ConsensusEngine* eng)
        : taskId(id), request(req), engine(eng), startTime(std::chrono::steady_clock::now()) {}
};

/**
 * Thread pool for parallel consensus validation
 */
class ConsensusThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<uint32_t> activeTasks;
    
public:
    explicit ConsensusThreadPool(size_t numThreads);
    ~ConsensusThreadPool();
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>>;
    
    void shutdown();
    size_t getQueueSize() const;
    uint32_t getActiveTasks() const { return activeTasks.load(); }
    bool isShutdown() const { return stop.load(); }
};

/**
 * Consensus Performance Optimizer
 * Implements parallel validation, caching, and memory optimization for consensus operations
 */
class ConsensusPerformanceOptimizer {
private:
    // Configuration
    OptimizationConfig config;
    
    // Cache management
    std::unordered_map<std::string, CacheEntry> resultCache;
    std::unordered_map<std::string, CacheEntry> computationCache;
    mutable std::mutex cacheMutex;
    
    // Thread pool for parallel processing
    std::unique_ptr<ConsensusThreadPool> threadPool;
    
    // Performance monitoring
    OptimizationMetrics metrics;
    mutable std::mutex metricsMutex;
    std::thread metricsThread;
    std::atomic<bool> shouldStop;
    
    // Memory management
    std::thread memoryCleanupThread;
    std::atomic<size_t> currentMemoryUsage;
    
    // Initialization state
    std::atomic<bool> initialized;

public:
    ConsensusPerformanceOptimizer();
    explicit ConsensusPerformanceOptimizer(const OptimizationConfig& config);
    ~ConsensusPerformanceOptimizer();
    
    // Lifecycle management
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized.load(); }
    
    // Configuration management
    bool updateConfiguration(const OptimizationConfig& newConfig);
    OptimizationConfig getConfiguration() const;
    
    // Core optimization methods
    ConsensusResult optimizedValidation(const ConsensusRequest& request, 
                                       const std::vector<ConsensusEngine*>& engines);
    
    std::vector<ConsensusResult> parallelValidation(const ConsensusRequest& request,
                                                   const std::vector<ConsensusEngine*>& engines);
    
    ConsensusResult sequentialValidation(const ConsensusRequest& request,
                                        const std::vector<ConsensusEngine*>& engines);
    
    // Caching methods
    bool getCachedResult(const std::string& key, ConsensusResult& result);
    void cacheResult(const std::string& key, const ConsensusResult& result, double computationTime = 0.0);
    void invalidateCache(const std::string& pattern = "");
    void clearCache();
    
    // Memoization methods
    bool getMemoizedComputation(const std::string& key, ConsensusResult& result);
    void memoizeComputation(const std::string& key, const ConsensusResult& result, double computationTime);
    
    // Memory optimization
    void optimizeMemoryUsage();
    void performMemoryCleanup();
    size_t getCurrentMemoryUsage() const { return currentMemoryUsage.load(); }
    
    // Performance monitoring
    OptimizationMetrics getPerformanceMetrics() const;
    void resetMetrics();
    void updateMetrics();
    
    // Benchmarking
    nlohmann::json runBenchmark(const std::vector<ConsensusRequest>& testRequests,
                               const std::vector<ConsensusEngine*>& engines);
    
    nlohmann::json compareBenchmark(const std::vector<ConsensusRequest>& testRequests,
                                   const std::vector<ConsensusEngine*>& engines,
                                   bool useOptimizations = true);
    
    // Statistics and reporting
    nlohmann::json getOptimizationReport() const;
    nlohmann::json getCacheStatistics() const;
    nlohmann::json getThreadPoolStatistics() const;

private:
    // Cache management helpers
    std::string generateCacheKey(const ConsensusRequest& request, ConsensusType engineType) const;
    std::string generateComputationKey(const std::string& operation, const std::string& data) const;
    void evictExpiredEntries();
    void evictLeastRecentlyUsed(size_t targetSize);
    
    // Parallel processing helpers
    bool shouldUseParallelProcessing(const std::vector<ConsensusEngine*>& engines) const;
    std::vector<std::future<ConsensusResult>> submitParallelTasks(
        const ConsensusRequest& request, const std::vector<ConsensusEngine*>& engines);
    
    // Memory management helpers
    void memoryCleanupLoop();
    size_t calculateMemoryUsage() const;
    void compactCache();
    
    // Metrics helpers
    void metricsUpdateLoop();
    void recordCacheHit(double accessTime);
    void recordCacheMiss(double computationTime);
    void recordParallelValidation(double executionTime, size_t engineCount);
    void recordSequentialValidation(double executionTime, size_t engineCount);
    void updateMemoryMetrics();
    
    // Utility methods
    double measureExecutionTime(const std::function<void()>& operation) const;
    std::string hashString(const std::string& input) const;
    bool isValidCacheEntry(const CacheEntry& entry) const;
};

// Template implementation for thread pool
template<class F, class... Args>
auto ConsensusThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>> {
    using return_type = typename std::invoke_result_t<F, Args...>;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        if (stop) {
            throw std::runtime_error("Cannot enqueue task on stopped thread pool");
        }
        
        tasks.emplace([task](){ (*task)(); });
    }
    
    condition.notify_one();
    return result;
}

#endif // CONSENSUS_PERFORMANCE_OPTIMIZER_H