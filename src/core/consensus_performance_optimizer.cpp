#include "../../include/core/consensus_performance_optimizer.h"
#include "../../include/core/logger.h"
#ifndef TEST_BUILD
#include "../../include/core/block.h"
#include "../../include/core/transaction.h"
#endif
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <random>

// ConsensusThreadPool implementation
ConsensusThreadPool::ConsensusThreadPool(size_t numThreads) : stop(false), activeTasks(0) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    
                    if (this->stop && this->tasks.empty()) {
                        return;
                    }
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                    activeTasks++;
                }
                
                try {
                    task();
                } catch (const std::exception& e) {
                    Logger::error("Thread pool task failed: " + std::string(e.what()));
                }
                
                activeTasks--;
            }
        });
    }
    
    Logger::info("ConsensusThreadPool created with " + std::to_string(numThreads) + " threads");
}

ConsensusThreadPool::~ConsensusThreadPool() {
    shutdown();
}

void ConsensusThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers.clear();
    Logger::info("ConsensusThreadPool shut down");
}

size_t ConsensusThreadPool::getQueueSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queueMutex));
    return tasks.size();
}

// ConsensusPerformanceOptimizer implementation
ConsensusPerformanceOptimizer::ConsensusPerformanceOptimizer() 
    : shouldStop(false), currentMemoryUsage(0), initialized(false) {
    Logger::info("ConsensusPerformanceOptimizer created with default configuration");
}

ConsensusPerformanceOptimizer::ConsensusPerformanceOptimizer(const OptimizationConfig& cfg)
    : config(cfg), shouldStop(false), currentMemoryUsage(0), initialized(false) {
    Logger::info("ConsensusPerformanceOptimizer created with custom configuration");
}

ConsensusPerformanceOptimizer::~ConsensusPerformanceOptimizer() {
    shutdown();
}

bool ConsensusPerformanceOptimizer::initialize() {
    if (initialized.load()) {
        Logger::warning("ConsensusPerformanceOptimizer already initialized");
        return true;
    }
    
    try {
        Logger::info("Initializing ConsensusPerformanceOptimizer");
        
        // Initialize thread pool
        if (config.enableParallelValidation && config.maxWorkerThreads > 0) {
            threadPool = std::make_unique<ConsensusThreadPool>(config.maxWorkerThreads);
            Logger::info("Thread pool initialized with " + std::to_string(config.maxWorkerThreads) + " threads");
        }
        
        // Start metrics monitoring thread
        if (config.enablePerformanceMonitoring) {
            shouldStop = false;
            metricsThread = std::thread(&ConsensusPerformanceOptimizer::metricsUpdateLoop, this);
            Logger::info("Performance monitoring thread started");
        }
        
        // Start memory cleanup thread
        if (config.enableMemoryOptimization) {
            memoryCleanupThread = std::thread(&ConsensusPerformanceOptimizer::memoryCleanupLoop, this);
            Logger::info("Memory cleanup thread started");
        }
        
        // Initialize metrics
        metrics = OptimizationMetrics{};
        metrics.lastUpdate = std::chrono::steady_clock::now();
        
        initialized = true;
        Logger::info("ConsensusPerformanceOptimizer initialized successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize ConsensusPerformanceOptimizer: " + std::string(e.what()));
        return false;
    }
}

void ConsensusPerformanceOptimizer::shutdown() {
    if (!initialized.load()) {
        return;
    }
    
    Logger::info("Shutting down ConsensusPerformanceOptimizer");
    
    // Stop background threads first
    shouldStop = true;
    
    // Give threads a moment to see the stop signal
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    if (metricsThread.joinable()) {
        metricsThread.join();
    }
    
    if (memoryCleanupThread.joinable()) {
        memoryCleanupThread.join();
    }
    
    // Shutdown thread pool
    if (threadPool) {
        threadPool->shutdown();
        threadPool.reset();
    }
    
    // Clear caches
    clearCache();
    
    initialized = false;
    Logger::info("ConsensusPerformanceOptimizer shut down successfully");
}

bool ConsensusPerformanceOptimizer::updateConfiguration(const OptimizationConfig& newConfig) {
    if (!initialized.load()) {
        config = newConfig;
        return true;
    }
    
    // For now, require restart for configuration changes
    Logger::info("Configuration updated - restart required for full effect");
    config = newConfig;
    return true;
}

OptimizationConfig ConsensusPerformanceOptimizer::getConfiguration() const {
    return config;
}

ConsensusResult ConsensusPerformanceOptimizer::optimizedValidation(
    const ConsensusRequest& request, const std::vector<ConsensusEngine*>& engines) {
    
    if (!initialized.load()) {
        Logger::error("ConsensusPerformanceOptimizer not initialized");
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "Optimizer not initialized");
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        // Check cache first if enabled
        if (config.enableResultCaching && engines.size() == 1) {
            std::string cacheKey = generateCacheKey(request, engines[0]->getType());
            ConsensusResult cachedResult;
            
            if (getCachedResult(cacheKey, cachedResult)) {
                auto endTime = std::chrono::steady_clock::now();
                double accessTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
                recordCacheHit(accessTime);
                
                Logger::debug("Cache hit for request: " + request.requestId);
                return cachedResult;
            }
        }
        
        // Determine whether to use parallel or sequential processing
        ConsensusResult result;
        if (shouldUseParallelProcessing(engines)) {
            auto parallelResults = parallelValidation(request, engines);
            if (!parallelResults.empty()) {
                // For optimization, return the first result (could be enhanced with aggregation)
                result = parallelResults[0];
            } else {
                result = ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "Parallel validation failed");
            }
        } else {
            result = sequentialValidation(request, engines);
        }
        
        // Cache the result if enabled
        if (config.enableResultCaching && engines.size() == 1) {
            auto endTime = std::chrono::steady_clock::now();
            double computationTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            
            std::string cacheKey = generateCacheKey(request, engines[0]->getType());
            cacheResult(cacheKey, result, computationTime);
            recordCacheMiss(computationTime);
        }
        
        // Update metrics
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            metrics.totalValidations++;
            auto endTime = std::chrono::steady_clock::now();
            double totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            metrics.totalComputationTime += totalTime;
            metrics.averageValidationTime = metrics.totalComputationTime / metrics.totalValidations;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        Logger::error("Optimized validation failed: " + std::string(e.what()));
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                              "Optimization failed: " + std::string(e.what()));
    }
}

std::vector<ConsensusResult> ConsensusPerformanceOptimizer::parallelValidation(
    const ConsensusRequest& request, const std::vector<ConsensusEngine*>& engines) {
    
    if (!threadPool || engines.empty()) {
        return {};
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        Logger::debug("Starting parallel validation with " + std::to_string(engines.size()) + " engines");
        
        // Submit tasks to thread pool
        auto futures = submitParallelTasks(request, engines);
        
        // Collect results
        std::vector<ConsensusResult> results;
        results.reserve(futures.size());
        
        for (auto& future : futures) {
            try {
                // Wait for result with timeout
                if (future.wait_for(config.taskTimeout) == std::future_status::ready) {
                    results.push_back(future.get());
                } else {
                    Logger::warning("Parallel validation task timed out");
                    results.push_back(ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "Task timeout"));
                }
            } catch (const std::exception& e) {
                Logger::error("Parallel validation task failed: " + std::string(e.what()));
                results.push_back(ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                                                "Task failed: " + std::string(e.what())));
            }
        }
        
        // Record metrics
        auto endTime = std::chrono::steady_clock::now();
        double executionTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        recordParallelValidation(executionTime, engines.size());
        
        Logger::debug("Parallel validation completed in " + std::to_string(executionTime) + "ms");
        
        return results;
        
    } catch (const std::exception& e) {
        Logger::error("Parallel validation failed: " + std::string(e.what()));
        return {};
    }
}

ConsensusResult ConsensusPerformanceOptimizer::sequentialValidation(
    const ConsensusRequest& request, const std::vector<ConsensusEngine*>& engines) {
    
    if (engines.empty()) {
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "No engines provided");
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        Logger::debug("Starting sequential validation with " + std::to_string(engines.size()) + " engines");
        
        // Process engines sequentially
        for (ConsensusEngine* engine : engines) {
            if (!engine) {
                continue;
            }
            
            try {
                ConsensusResult result = engine->processRequest(request);
                
                // Record metrics and return first successful result
                auto endTime = std::chrono::steady_clock::now();
                double executionTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
                recordSequentialValidation(executionTime, engines.size());
                
                Logger::debug("Sequential validation completed in " + std::to_string(executionTime) + "ms");
                
                return result;
                
            } catch (const std::exception& e) {
                Logger::error("Engine " + engine->getName() + " failed: " + std::string(e.what()));
                continue;
            }
        }
        
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, "All engines failed");
        
    } catch (const std::exception& e) {
        Logger::error("Sequential validation failed: " + std::string(e.what()));
        return ConsensusResult(false, ConsensusType::PROOF_OF_WORK, 0.0, 
                              "Sequential validation failed: " + std::string(e.what()));
    }
}

bool ConsensusPerformanceOptimizer::getCachedResult(const std::string& key, ConsensusResult& result) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = resultCache.find(key);
    if (it != resultCache.end()) {
        const CacheEntry& entry = it->second;
        
        // Check if entry is expired
        if (entry.isExpired(config.cacheExpirationTime)) {
            resultCache.erase(it);
            return false;
        }
        
        // Update access count and return result
        it->second.accessCount++;
        result = entry.result;
        return true;
    }
    
    return false;
}

void ConsensusPerformanceOptimizer::cacheResult(const std::string& key, const ConsensusResult& result, double computationTime) {
    if (!config.enableResultCaching) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Check cache size and evict if necessary
    if (resultCache.size() >= config.maxCacheSize) {
        evictLeastRecentlyUsed(config.maxCacheSize * 0.8); // Evict to 80% capacity
    }
    
    resultCache[key] = CacheEntry(result, computationTime);
    
    // Update memory usage
    currentMemoryUsage += key.size() + sizeof(CacheEntry);
}

void ConsensusPerformanceOptimizer::invalidateCache(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    if (pattern.empty()) {
        size_t removedSize = 0;
        for (const auto& [key, entry] : resultCache) {
            removedSize += key.size() + sizeof(CacheEntry);
        }
        resultCache.clear();
        currentMemoryUsage -= removedSize;
        
        Logger::info("All cache entries invalidated");
    } else {
        auto it = resultCache.begin();
        size_t removedSize = 0;
        
        while (it != resultCache.end()) {
            if (it->first.find(pattern) != std::string::npos) {
                removedSize += it->first.size() + sizeof(CacheEntry);
                it = resultCache.erase(it);
            } else {
                ++it;
            }
        }
        
        currentMemoryUsage -= removedSize;
        Logger::info("Cache entries matching pattern '" + pattern + "' invalidated");
    }
}

void ConsensusPerformanceOptimizer::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    size_t removedSize = 0;
    for (const auto& [key, entry] : resultCache) {
        removedSize += key.size() + sizeof(CacheEntry);
    }
    
    resultCache.clear();
    computationCache.clear();
    
    currentMemoryUsage -= removedSize;
    
    Logger::info("All caches cleared");
}

bool ConsensusPerformanceOptimizer::getMemoizedComputation(const std::string& key, ConsensusResult& result) {
    if (!config.enableComputationMemoization) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = computationCache.find(key);
    if (it != computationCache.end()) {
        const CacheEntry& entry = it->second;
        
        if (!entry.isExpired(config.cacheExpirationTime)) {
            it->second.accessCount++;
            result = entry.result;
            return true;
        } else {
            computationCache.erase(it);
        }
    }
    
    return false;
}

void ConsensusPerformanceOptimizer::memoizeComputation(const std::string& key, const ConsensusResult& result, double computationTime) {
    if (!config.enableComputationMemoization) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    computationCache[key] = CacheEntry(result, computationTime);
    currentMemoryUsage += key.size() + sizeof(CacheEntry);
}

void ConsensusPerformanceOptimizer::optimizeMemoryUsage() {
    if (!config.enableMemoryOptimization) {
        return;
    }
    
    Logger::debug("Optimizing memory usage");
    
    // Perform cache cleanup
    evictExpiredEntries();
    
    // Compact cache if memory usage is high
    size_t currentUsage = getCurrentMemoryUsage();
    if (currentUsage > config.maxMemoryUsage * config.memoryCleanupThreshold) {
        compactCache();
    }
    
    updateMemoryMetrics();
    
    Logger::debug("Memory optimization completed");
}

void ConsensusPerformanceOptimizer::performMemoryCleanup() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    size_t initialSize = resultCache.size() + computationCache.size();
    size_t removedSize = 0;
    
    // Remove expired entries
    auto it = resultCache.begin();
    while (it != resultCache.end()) {
        if (it->second.isExpired(config.cacheExpirationTime)) {
            removedSize += it->first.size() + sizeof(CacheEntry);
            it = resultCache.erase(it);
        } else {
            ++it;
        }
    }
    
    auto compIt = computationCache.begin();
    while (compIt != computationCache.end()) {
        if (compIt->second.isExpired(config.cacheExpirationTime)) {
            removedSize += compIt->first.size() + sizeof(CacheEntry);
            compIt = computationCache.erase(compIt);
        } else {
            ++compIt;
        }
    }
    
    currentMemoryUsage -= removedSize;
    
    size_t finalSize = resultCache.size() + computationCache.size();
    
    if (initialSize != finalSize) {
        Logger::debug("Memory cleanup removed " + std::to_string(initialSize - finalSize) + " expired entries");
    }
}

OptimizationMetrics ConsensusPerformanceOptimizer::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return metrics;
}

void ConsensusPerformanceOptimizer::resetMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    metrics = OptimizationMetrics{};
    Logger::info("Performance metrics reset");
}

void ConsensusPerformanceOptimizer::updateMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    // Update thread pool metrics
    if (threadPool) {
        metrics.queuedTasks = static_cast<uint32_t>(threadPool->getQueueSize());
        metrics.activeThreads = threadPool->getActiveTasks();
        metrics.threadUtilization = static_cast<double>(metrics.activeThreads) / config.maxWorkerThreads;
    }
    
    // Update memory metrics
    updateMemoryMetrics();
    
    metrics.lastUpdate = std::chrono::steady_clock::now();
}

nlohmann::json ConsensusPerformanceOptimizer::runBenchmark(
    const std::vector<ConsensusRequest>& testRequests,
    const std::vector<ConsensusEngine*>& engines) {
    
    Logger::info("Running performance benchmark with " + std::to_string(testRequests.size()) + " requests");
    
    nlohmann::json benchmark;
    benchmark["testRequests"] = testRequests.size();
    benchmark["engines"] = engines.size();
    benchmark["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Reset metrics for clean benchmark
    resetMetrics();
    
    auto startTime = std::chrono::steady_clock::now();
    
    std::vector<double> executionTimes;
    std::vector<bool> results;
    
    for (const auto& request : testRequests) {
        auto requestStart = std::chrono::steady_clock::now();
        
        ConsensusResult result = optimizedValidation(request, engines);
        
        auto requestEnd = std::chrono::steady_clock::now();
        double executionTime = std::chrono::duration<double, std::milli>(requestEnd - requestStart).count();
        
        executionTimes.push_back(executionTime);
        results.push_back(result.isValid);
    }
    
    auto endTime = std::chrono::steady_clock::now();
    double totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    // Calculate statistics
    double avgTime = std::accumulate(executionTimes.begin(), executionTimes.end(), 0.0) / executionTimes.size();
    double minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
    double maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
    
    size_t successCount = std::count(results.begin(), results.end(), true);
    double successRate = static_cast<double>(successCount) / results.size();
    
    benchmark["results"] = {
        {"totalTime", totalTime},
        {"averageTime", avgTime},
        {"minTime", minTime},
        {"maxTime", maxTime},
        {"throughput", testRequests.size() / (totalTime / 1000.0)}, // requests per second
        {"successRate", successRate},
        {"successCount", successCount}
    };
    
    // Include current metrics
    auto metrics = getPerformanceMetrics();
    benchmark["metrics"] = {
        {"cacheHits", metrics.cacheHits},
        {"cacheMisses", metrics.cacheMisses},
        {"cacheHitRatio", metrics.getCacheHitRatio()},
        {"parallelValidations", metrics.parallelValidations},
        {"sequentialValidations", metrics.sequentialValidations},
        {"totalValidations", metrics.totalValidations},
        {"averageValidationTime", metrics.averageValidationTime}
    };
    
    Logger::info("Benchmark completed in " + std::to_string(totalTime) + "ms");
    
    return benchmark;
}

nlohmann::json ConsensusPerformanceOptimizer::compareBenchmark(
    const std::vector<ConsensusRequest>& testRequests,
    const std::vector<ConsensusEngine*>& engines,
    bool useOptimizations) {
    
    Logger::info("Running comparison benchmark (optimizations: " + 
                std::string(useOptimizations ? "enabled" : "disabled") + ")");
    
    // Temporarily disable optimizations if requested
    OptimizationConfig originalConfig = config;
    if (!useOptimizations) {
        OptimizationConfig disabledConfig = config;
        disabledConfig.enableResultCaching = false;
        disabledConfig.enableComputationMemoization = false;
        disabledConfig.enableParallelValidation = false;
        disabledConfig.enableMemoryOptimization = false;
        updateConfiguration(disabledConfig);
    }
    
    nlohmann::json result = runBenchmark(testRequests, engines);
    result["optimizationsEnabled"] = useOptimizations;
    
    // Restore original configuration
    if (!useOptimizations) {
        updateConfiguration(originalConfig);
    }
    
    return result;
}

nlohmann::json ConsensusPerformanceOptimizer::getOptimizationReport() const {
    nlohmann::json report;
    
    // Configuration
    report["configuration"] = {
        {"maxCacheSize", config.maxCacheSize},
        {"cacheExpirationTime", config.cacheExpirationTime.count()},
        {"enableResultCaching", config.enableResultCaching},
        {"enableComputationMemoization", config.enableComputationMemoization},
        {"maxWorkerThreads", config.maxWorkerThreads},
        {"enableParallelValidation", config.enableParallelValidation},
        {"enableMemoryOptimization", config.enableMemoryOptimization},
        {"maxMemoryUsage", config.maxMemoryUsage}
    };
    
    // Performance metrics
    auto metrics = getPerformanceMetrics();
    report["metrics"] = {
        {"cacheHits", metrics.cacheHits},
        {"cacheMisses", metrics.cacheMisses},
        {"cacheHitRatio", metrics.getCacheHitRatio()},
        {"parallelValidations", metrics.parallelValidations},
        {"sequentialValidations", metrics.sequentialValidations},
        {"totalValidations", metrics.totalValidations},
        {"averageValidationTime", metrics.averageValidationTime},
        {"currentMemoryUsage", metrics.currentMemoryUsage},
        {"peakMemoryUsage", metrics.peakMemoryUsage}
    };
    
    // Cache statistics
    report["cache"] = getCacheStatistics();
    
    // Thread pool statistics
    if (threadPool) {
        report["threadPool"] = getThreadPoolStatistics();
    }
    
    return report;
}

nlohmann::json ConsensusPerformanceOptimizer::getCacheStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    nlohmann::json stats;
    stats["resultCacheSize"] = resultCache.size();
    stats["computationCacheSize"] = computationCache.size();
    stats["totalCacheSize"] = resultCache.size() + computationCache.size();
    stats["maxCacheSize"] = config.maxCacheSize;
    stats["cacheUtilization"] = static_cast<double>(resultCache.size()) / config.maxCacheSize;
    
    // Calculate cache efficiency
    if (!resultCache.empty()) {
        uint64_t totalAccess = 0;
        double totalComputationTime = 0.0;
        
        for (const auto& [key, entry] : resultCache) {
            totalAccess += entry.accessCount;
            totalComputationTime += entry.computationTime;
        }
        
        stats["averageAccessCount"] = static_cast<double>(totalAccess) / resultCache.size();
        stats["averageComputationTime"] = totalComputationTime / resultCache.size();
    }
    
    return stats;
}

nlohmann::json ConsensusPerformanceOptimizer::getThreadPoolStatistics() const {
    nlohmann::json stats;
    
    if (threadPool) {
        stats["maxThreads"] = config.maxWorkerThreads;
        stats["activeThreads"] = threadPool->getActiveTasks();
        stats["queuedTasks"] = threadPool->getQueueSize();
        stats["utilization"] = static_cast<double>(threadPool->getActiveTasks()) / config.maxWorkerThreads;
        stats["isShutdown"] = threadPool->isShutdown();
    } else {
        stats["enabled"] = false;
    }
    
    return stats;
}

// Private helper methods
std::string ConsensusPerformanceOptimizer::generateCacheKey(const ConsensusRequest& request, ConsensusType engineType) const {
    std::stringstream ss;
    ss << static_cast<int>(request.type) << "_" 
       << static_cast<int>(engineType) << "_"
       << hashString(request.data);
    return ss.str();
}

std::string ConsensusPerformanceOptimizer::generateComputationKey(const std::string& operation, const std::string& data) const {
    return operation + "_" + hashString(data);
}

void ConsensusPerformanceOptimizer::evictExpiredEntries() {
    size_t removedSize = 0;
    
    auto it = resultCache.begin();
    while (it != resultCache.end()) {
        if (it->second.isExpired(config.cacheExpirationTime)) {
            removedSize += it->first.size() + sizeof(CacheEntry);
            it = resultCache.erase(it);
        } else {
            ++it;
        }
    }
    
    currentMemoryUsage -= removedSize;
    
    if (removedSize > 0) {
        std::lock_guard<std::mutex> lock(metricsMutex);
        metrics.cacheEvictions++;
    }
}

void ConsensusPerformanceOptimizer::evictLeastRecentlyUsed(size_t targetSize) {
    if (resultCache.size() <= targetSize) {
        return;
    }
    
    // Create vector of cache entries with their keys for sorting
    std::vector<std::pair<std::string, CacheEntry*>> entries;
    for (auto& [key, entry] : resultCache) {
        entries.emplace_back(key, &entry);
    }
    
    // Sort by access count (ascending) and timestamp (ascending)
    std::sort(entries.begin(), entries.end(), 
        [](const auto& a, const auto& b) {
            if (a.second->accessCount == b.second->accessCount) {
                return a.second->timestamp < b.second->timestamp;
            }
            return a.second->accessCount < b.second->accessCount;
        });
    
    // Remove least recently used entries
    size_t toRemove = resultCache.size() - targetSize;
    size_t removedSize = 0;
    
    for (size_t i = 0; i < toRemove && i < entries.size(); ++i) {
        const std::string& key = entries[i].first;
        removedSize += key.size() + sizeof(CacheEntry);
        resultCache.erase(key);
    }
    
    currentMemoryUsage -= removedSize;
    
    std::lock_guard<std::mutex> lock(metricsMutex);
    metrics.cacheEvictions += toRemove;
}

bool ConsensusPerformanceOptimizer::shouldUseParallelProcessing(const std::vector<ConsensusEngine*>& engines) const {
    return config.enableParallelValidation && 
           threadPool && 
           !threadPool->isShutdown() &&
           engines.size() >= config.minParallelEngines;
}

std::vector<std::future<ConsensusResult>> ConsensusPerformanceOptimizer::submitParallelTasks(
    const ConsensusRequest& request, const std::vector<ConsensusEngine*>& engines) {
    
    std::vector<std::future<ConsensusResult>> futures;
    futures.reserve(engines.size());
    
    for (ConsensusEngine* engine : engines) {
        if (!engine) {
            continue;
        }
        
        auto future = threadPool->enqueue([engine, request]() -> ConsensusResult {
            try {
                return engine->processRequest(request);
            } catch (const std::exception& e) {
                return ConsensusResult(false, engine->getType(), 0.0, 
                                     "Engine failed: " + std::string(e.what()));
            }
        });
        
        futures.push_back(std::move(future));
    }
    
    return futures;
}

void ConsensusPerformanceOptimizer::memoryCleanupLoop() {
    Logger::info("Memory cleanup thread started");
    
    while (!shouldStop.load()) {
        try {
            performMemoryCleanup();
            
            // Sleep for cleanup interval with periodic checks for stop signal
            auto sleepTime = std::chrono::duration_cast<std::chrono::milliseconds>(config.memoryCleanupInterval);
            auto checkInterval = std::chrono::milliseconds(100);
            
            while (sleepTime > std::chrono::milliseconds(0) && !shouldStop.load()) {
                auto currentSleep = (sleepTime < checkInterval) ? sleepTime : checkInterval;
                std::this_thread::sleep_for(currentSleep);
                sleepTime -= currentSleep;
            }
            
        } catch (const std::exception& e) {
            Logger::error("Memory cleanup error: " + std::string(e.what()));
        }
    }
    
    Logger::info("Memory cleanup thread stopped");
}

size_t ConsensusPerformanceOptimizer::calculateMemoryUsage() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    size_t usage = 0;
    
    for (const auto& [key, entry] : resultCache) {
        usage += key.size() + sizeof(CacheEntry);
    }
    
    for (const auto& [key, entry] : computationCache) {
        usage += key.size() + sizeof(CacheEntry);
    }
    
    return usage;
}

void ConsensusPerformanceOptimizer::compactCache() {
    Logger::debug("Compacting cache to reduce memory usage");
    
    // Evict half of the least recently used entries
    size_t targetSize = resultCache.size() / 2;
    evictLeastRecentlyUsed(targetSize);
    
    // Also compact computation cache
    if (computationCache.size() > targetSize) {
        auto it = computationCache.begin();
        size_t toRemove = computationCache.size() - targetSize;
        size_t removed = 0;
        
        while (it != computationCache.end() && removed < toRemove) {
            it = computationCache.erase(it);
            removed++;
        }
    }
    
    std::lock_guard<std::mutex> lock(metricsMutex);
    metrics.memoryOptimizations++;
}

void ConsensusPerformanceOptimizer::metricsUpdateLoop() {
    Logger::info("Metrics update thread started");
    
    while (!shouldStop.load()) {
        try {
            updateMetrics();
            
            // Sleep for metrics update interval with periodic checks for stop signal
            auto sleepTime = std::chrono::duration_cast<std::chrono::milliseconds>(config.metricsUpdateInterval);
            auto checkInterval = std::chrono::milliseconds(100);
            
            while (sleepTime > std::chrono::milliseconds(0) && !shouldStop.load()) {
                auto currentSleep = (sleepTime < checkInterval) ? sleepTime : checkInterval;
                std::this_thread::sleep_for(currentSleep);
                sleepTime -= currentSleep;
            }
            
        } catch (const std::exception& e) {
            Logger::error("Metrics update error: " + std::string(e.what()));
        }
    }
    
    Logger::info("Metrics update thread stopped");
}

void ConsensusPerformanceOptimizer::recordCacheHit(double accessTime) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    metrics.cacheHits++;
    
    // Update average cache hit time
    double totalHitTime = metrics.averageCacheHitTime * (metrics.cacheHits - 1);
    metrics.averageCacheHitTime = (totalHitTime + accessTime) / metrics.cacheHits;
}

void ConsensusPerformanceOptimizer::recordCacheMiss(double computationTime) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    metrics.cacheMisses++;
    
    // Update average cache miss time
    double totalMissTime = metrics.averageCacheMissTime * (metrics.cacheMisses - 1);
    metrics.averageCacheMissTime = (totalMissTime + computationTime) / metrics.cacheMisses;
}

void ConsensusPerformanceOptimizer::recordParallelValidation(double executionTime, size_t engineCount) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    metrics.parallelValidations++;
    
    // Update average parallel time
    double totalParallelTime = metrics.averageParallelTime * (metrics.parallelValidations - 1);
    metrics.averageParallelTime = (totalParallelTime + executionTime) / metrics.parallelValidations;
    
    // Update max concurrent validations
    if (engineCount > metrics.maxConcurrentValidations) {
        metrics.maxConcurrentValidations = static_cast<uint32_t>(engineCount);
    }
}

void ConsensusPerformanceOptimizer::recordSequentialValidation(double executionTime, size_t /* engineCount */) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    metrics.sequentialValidations++;
    
    // Update average sequential time
    double totalSequentialTime = metrics.averageSequentialTime * (metrics.sequentialValidations - 1);
    metrics.averageSequentialTime = (totalSequentialTime + executionTime) / metrics.sequentialValidations;
}

void ConsensusPerformanceOptimizer::updateMemoryMetrics() {
    size_t currentUsage = calculateMemoryUsage();
    currentMemoryUsage = currentUsage;
    
    metrics.currentMemoryUsage = currentUsage;
    if (currentUsage > metrics.peakMemoryUsage) {
        metrics.peakMemoryUsage = currentUsage;
    }
}

double ConsensusPerformanceOptimizer::measureExecutionTime(const std::function<void()>& operation) const {
    auto start = std::chrono::steady_clock::now();
    operation();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string ConsensusPerformanceOptimizer::hashString(const std::string& input) const {
    // Simple hash function for cache keys
    std::hash<std::string> hasher;
    size_t hash = hasher(input);
    
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

bool ConsensusPerformanceOptimizer::isValidCacheEntry(const CacheEntry& entry) const {
    return !entry.isExpired(config.cacheExpirationTime);
}