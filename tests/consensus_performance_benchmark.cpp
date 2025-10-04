#include "../include/core/consensus_performance_optimizer.h"
#include "../include/core/consensus_harmony.h"
#include "../include/core/logger.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <fstream>
#include <iomanip>

// Mock consensus engine with configurable performance characteristics
class BenchmarkConsensusEngine : public ConsensusEngine {
private:
    ConsensusType type;
    std::string name;
    std::chrono::microseconds baseProcessingTime;
    std::chrono::microseconds variability;
    double successRate;
    bool healthy;
    std::mt19937 rng;
    
public:
    BenchmarkConsensusEngine(ConsensusType t, const std::string& n, 
                           std::chrono::microseconds baseTime,
                           std::chrono::microseconds var = std::chrono::microseconds(50),
                           double success = 0.95)
        : type(t), name(n), baseProcessingTime(baseTime), variability(var), 
          successRate(success), healthy(true), rng(std::random_device{}()) {}
    
    bool validateBlock(const Block& block) override {
        simulateProcessing();
        return (std::uniform_real_distribution<>(0.0, 1.0)(rng)) < successRate;
    }
    
    bool validateTransaction(const Transaction& transaction) override {
        simulateProcessing();
        return (std::uniform_real_distribution<>(0.0, 1.0)(rng)) < successRate;
    }
    
    ConsensusResult processRequest(const ConsensusRequest& request) override {
        simulateProcessing();
        
        bool isValid = (std::uniform_real_distribution<>(0.0, 1.0)(rng)) < successRate;
        double confidence = isValid ? 
            0.7 + (std::uniform_real_distribution<>(0.0, 1.0)(rng)) * 0.3 :
            0.1 + (std::uniform_real_distribution<>(0.0, 1.0)(rng)) * 0.4;
        
        return ConsensusResult(isValid, type, confidence, 
                              isValid ? "Benchmark validation passed" : "Benchmark validation failed");
    }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return healthy; }
    
    ConsensusType getType() const override { return type; }
    std::string getName() const override { return name; }
    
    nlohmann::json getStatus() const override {
        return nlohmann::json{
            {"healthy", healthy}, 
            {"name", name},
            {"baseProcessingTime", baseProcessingTime.count()},
            {"successRate", successRate}
        };
    }
    
    nlohmann::json getMetrics() const override {
        return nlohmann::json{
            {"baseProcessingTime", baseProcessingTime.count()},
            {"variability", variability.count()},
            {"successRate", successRate}
        };
    }
    
    bool adjustParameters(const std::map<std::string, double>& parameters) override { return true; }
    std::map<std::string, double> getParameters() const override { return {}; }
    
    void setProcessingTime(std::chrono::microseconds time) { baseProcessingTime = time; }
    void setSuccessRate(double rate) { successRate = rate; }
    void setHealthy(bool h) { healthy = h; }
    
private:
    void simulateProcessing() {
        auto variance = std::uniform_int_distribution<int>(-variability.count(), variability.count())(rng);
        auto actualTime = baseProcessingTime + std::chrono::microseconds(variance);
        if (actualTime.count() > 0) {
            std::this_thread::sleep_for(actualTime);
        }
    }
};

// Benchmark configuration
struct BenchmarkConfig {
    size_t numRequests = 100;
    size_t numEngines = 4;
    size_t numThreads = std::thread::hardware_concurrency();
    bool enableCaching = true;
    bool enableParallel = true;
    bool enableMemoryOptimization = true;
    std::chrono::microseconds engineProcessingTime{1000}; // 1ms
    double engineSuccessRate = 0.95;
    size_t cacheSize = 1000;
    std::chrono::seconds cacheExpiration{300}; // 5 minutes
};

// Benchmark results
struct BenchmarkResults {
    std::string testName;
    size_t numRequests;
    size_t numEngines;
    double totalTime;
    double averageTime;
    double minTime;
    double maxTime;
    double throughput;
    double successRate;
    PerformanceMetrics metrics;
    nlohmann::json cacheStats;
    nlohmann::json threadPoolStats;
};

class ConsensusPerformanceBenchmark {
private:
    BenchmarkConfig config;
    std::vector<std::unique_ptr<BenchmarkConsensusEngine>> engines;
    std::mt19937 rng;
    
public:
    explicit ConsensusPerformanceBenchmark(const BenchmarkConfig& cfg = BenchmarkConfig{})
        : config(cfg), rng(std::random_device{}()) {
        createEngines();
    }
    
    void createEngines() {
        engines.clear();
        
        // Create different types of engines with varying performance characteristics
        engines.push_back(std::make_unique<BenchmarkConsensusEngine>(
            ConsensusType::PROOF_OF_WORK, "BenchmarkPoW", 
            config.engineProcessingTime, std::chrono::microseconds(200), config.engineSuccessRate));
        
        engines.push_back(std::make_unique<BenchmarkConsensusEngine>(
            ConsensusType::PROOF_OF_STAKE, "BenchmarkPoS", 
            config.engineProcessingTime / 2, std::chrono::microseconds(100), config.engineSuccessRate));
        
        engines.push_back(std::make_unique<BenchmarkConsensusEngine>(
            ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, "BenchmarkPoRC", 
            config.engineProcessingTime * 2, std::chrono::microseconds(300), config.engineSuccessRate));
        
        engines.push_back(std::make_unique<BenchmarkConsensusEngine>(
            ConsensusType::VOTING_CONSENSUS, "BenchmarkVoting", 
            config.engineProcessingTime * 3, std::chrono::microseconds(500), config.engineSuccessRate));
        
        // Limit to requested number of engines
        if (engines.size() > config.numEngines) {
            engines.resize(config.numEngines);
        }
    }
    
    std::vector<ConsensusRequest> generateRequests() {
        std::vector<ConsensusRequest> requests;
        requests.reserve(config.numRequests);
        
        std::uniform_int_distribution<> typeDist(0, 4);
        std::uniform_int_distribution<> sizeDist(100, 1000);
        
        for (size_t i = 0; i < config.numRequests; ++i) {
            RequestType type = static_cast<RequestType>(typeDist(rng));
            
            // Generate variable-sized data
            size_t dataSize = sizeDist(rng);
            std::string data;
            data.reserve(dataSize);
            for (size_t j = 0; j < dataSize; ++j) {
                data += static_cast<char>('a' + (rng() % 26));
            }
            
            requests.emplace_back(type, data);
        }
        
        return requests;
    }
    
    BenchmarkResults runBenchmark(const std::string& testName, 
                                 const OptimizationConfig& optimizerConfig) {
        std::cout << "Running benchmark: " << testName << std::endl;
        
        ConsensusPerformanceOptimizer optimizer(optimizerConfig);
        if (!optimizer.initialize()) {
            throw std::runtime_error("Failed to initialize optimizer for benchmark");
        }
        
        auto requests = generateRequests();
        std::vector<ConsensusEngine*> enginePtrs;
        for (auto& engine : engines) {
            enginePtrs.push_back(engine.get());
        }
        
        // Warm up
        std::cout << "Warming up..." << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), config.numRequests / 10); ++i) {
            optimizer.optimizedValidation(requests[i], {enginePtrs[0]});
        }
        
        // Reset metrics after warmup
        optimizer.resetMetrics();
        
        std::cout << "Starting benchmark..." << std::endl;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<double> executionTimes;
        std::vector<bool> results;
        executionTimes.reserve(config.numRequests);
        results.reserve(config.numRequests);
        
        for (const auto& request : requests) {
            auto requestStart = std::chrono::high_resolution_clock::now();
            
            ConsensusResult result = optimizer.optimizedValidation(request, enginePtrs);
            
            auto requestEnd = std::chrono::high_resolution_clock::now();
            double executionTime = std::chrono::duration<double, std::milli>(requestEnd - requestStart).count();
            
            executionTimes.push_back(executionTime);
            results.push_back(result.isValid);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        // Calculate statistics
        double avgTime = std::accumulate(executionTimes.begin(), executionTimes.end(), 0.0) / executionTimes.size();
        double minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
        double maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
        size_t successCount = std::count(results.begin(), results.end(), true);
        double successRate = static_cast<double>(successCount) / results.size();
        double throughput = config.numRequests / (totalTime / 1000.0);
        
        BenchmarkResults benchmarkResults;
        benchmarkResults.testName = testName;
        benchmarkResults.numRequests = config.numRequests;
        benchmarkResults.numEngines = config.numEngines;
        benchmarkResults.totalTime = totalTime;
        benchmarkResults.averageTime = avgTime;
        benchmarkResults.minTime = minTime;
        benchmarkResults.maxTime = maxTime;
        benchmarkResults.throughput = throughput;
        benchmarkResults.successRate = successRate;
        benchmarkResults.metrics = optimizer.getPerformanceMetrics();
        benchmarkResults.cacheStats = optimizer.getCacheStatistics();
        benchmarkResults.threadPoolStats = optimizer.getThreadPoolStatistics();
        
        optimizer.shutdown();
        
        std::cout << "Benchmark completed: " << testName << std::endl;
        std::cout << "  Total time: " << totalTime << "ms" << std::endl;
        std::cout << "  Average time: " << avgTime << "ms" << std::endl;
        std::cout << "  Throughput: " << throughput << " req/s" << std::endl;
        std::cout << "  Success rate: " << (successRate * 100) << "%" << std::endl;
        
        return benchmarkResults;
    }
    
    void runComprehensiveBenchmark() {
        std::cout << "=== Comprehensive Consensus Performance Benchmark ===" << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Requests: " << config.numRequests << std::endl;
        std::cout << "  Engines: " << config.numEngines << std::endl;
        std::cout << "  Threads: " << config.numThreads << std::endl;
        std::cout << "  Engine processing time: " << config.engineProcessingTime.count() << "μs" << std::endl;
        std::cout << std::endl;
        
        std::vector<BenchmarkResults> allResults;
        
        // Baseline - no optimizations
        {
            OptimizationConfig baselineConfig;
            baselineConfig.enableResultCaching = false;
            baselineConfig.enableComputationMemoization = false;
            baselineConfig.enableParallelValidation = false;
            baselineConfig.enableMemoryOptimization = false;
            baselineConfig.maxWorkerThreads = 1;
            
            allResults.push_back(runBenchmark("Baseline (No Optimizations)", baselineConfig));
        }
        
        // Caching only
        {
            OptimizationConfig cachingConfig;
            cachingConfig.enableResultCaching = true;
            cachingConfig.enableComputationMemoization = true;
            cachingConfig.enableParallelValidation = false;
            cachingConfig.enableMemoryOptimization = true;
            cachingConfig.maxCacheSize = config.cacheSize;
            cachingConfig.cacheExpirationTime = config.cacheExpiration;
            cachingConfig.maxWorkerThreads = 1;
            
            allResults.push_back(runBenchmark("Caching Only", cachingConfig));
        }
        
        // Parallel only
        {
            OptimizationConfig parallelConfig;
            parallelConfig.enableResultCaching = false;
            parallelConfig.enableComputationMemoization = false;
            parallelConfig.enableParallelValidation = true;
            parallelConfig.enableMemoryOptimization = false;
            parallelConfig.maxWorkerThreads = config.numThreads;
            parallelConfig.minParallelEngines = 2;
            
            allResults.push_back(runBenchmark("Parallel Only", parallelConfig));
        }
        
        // All optimizations
        {
            OptimizationConfig fullConfig;
            fullConfig.enableResultCaching = true;
            fullConfig.enableComputationMemoization = true;
            fullConfig.enableParallelValidation = true;
            fullConfig.enableMemoryOptimization = true;
            fullConfig.maxCacheSize = config.cacheSize;
            fullConfig.cacheExpirationTime = config.cacheExpiration;
            fullConfig.maxWorkerThreads = config.numThreads;
            fullConfig.minParallelEngines = 2;
            
            allResults.push_back(runBenchmark("All Optimizations", fullConfig));
        }
        
        // Print comparison
        printBenchmarkComparison(allResults);
        
        // Save results to file
        saveBenchmarkResults(allResults);
    }
    
    void printBenchmarkComparison(const std::vector<BenchmarkResults>& results) {
        std::cout << "\n=== Benchmark Comparison ===" << std::endl;
        
        if (results.empty()) {
            std::cout << "No results to compare" << std::endl;
            return;
        }
        
        const auto& baseline = results[0];
        
        std::cout << std::left << std::setw(25) << "Test Name" 
                  << std::setw(12) << "Total (ms)"
                  << std::setw(12) << "Avg (ms)"
                  << std::setw(12) << "Throughput"
                  << std::setw(12) << "Speedup"
                  << std::setw(12) << "Cache Hit%"
                  << std::setw(12) << "Success%"
                  << std::endl;
        
        std::cout << std::string(100, '-') << std::endl;
        
        for (const auto& result : results) {
            double speedup = baseline.totalTime / result.totalTime;
            double cacheHitRate = result.metrics.getCacheHitRatio() * 100;
            
            std::cout << std::left << std::setw(25) << result.testName
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.totalTime
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.averageTime
                      << std::setw(12) << std::fixed << std::setprecision(1) << result.throughput
                      << std::setw(12) << std::fixed << std::setprecision(2) << speedup << "x"
                      << std::setw(12) << std::fixed << std::setprecision(1) << cacheHitRate
                      << std::setw(12) << std::fixed << std::setprecision(1) << (result.successRate * 100)
                      << std::endl;
        }
        
        std::cout << std::endl;
        
        // Find best performing configuration
        auto bestResult = std::max_element(results.begin(), results.end(),
            [](const BenchmarkResults& a, const BenchmarkResults& b) {
                return a.throughput < b.throughput;
            });
        
        if (bestResult != results.end()) {
            std::cout << "🏆 Best performing configuration: " << bestResult->testName << std::endl;
            std::cout << "   Throughput: " << bestResult->throughput << " req/s" << std::endl;
            std::cout << "   Speedup: " << (baseline.totalTime / bestResult->totalTime) << "x" << std::endl;
        }
    }
    
    void saveBenchmarkResults(const std::vector<BenchmarkResults>& results) {
        std::string filename = "consensus_performance_benchmark_" + 
                              std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                  std::chrono::system_clock::now().time_since_epoch()).count()) + ".json";
        
        nlohmann::json jsonResults;
        jsonResults["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        jsonResults["configuration"] = {
            {"numRequests", config.numRequests},
            {"numEngines", config.numEngines},
            {"numThreads", config.numThreads},
            {"engineProcessingTime", config.engineProcessingTime.count()},
            {"engineSuccessRate", config.engineSuccessRate},
            {"cacheSize", config.cacheSize}
        };
        
        for (const auto& result : results) {
            nlohmann::json resultJson;
            resultJson["testName"] = result.testName;
            resultJson["numRequests"] = result.numRequests;
            resultJson["numEngines"] = result.numEngines;
            resultJson["totalTime"] = result.totalTime;
            resultJson["averageTime"] = result.averageTime;
            resultJson["minTime"] = result.minTime;
            resultJson["maxTime"] = result.maxTime;
            resultJson["throughput"] = result.throughput;
            resultJson["successRate"] = result.successRate;
            resultJson["cacheHitRatio"] = result.metrics.getCacheHitRatio();
            resultJson["parallelSpeedup"] = result.metrics.getParallelSpeedup();
            
            jsonResults["results"].push_back(resultJson);
        }
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << jsonResults.dump(4);
            file.close();
            std::cout << "Benchmark results saved to: " << filename << std::endl;
        } else {
            std::cout << "Failed to save benchmark results to file" << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== Consensus Performance Benchmark Suite ===" << std::endl;
    
    // Initialize logger
    Logger::setLevel(Logger::Level::INFO);
    
    // Parse command line arguments
    BenchmarkConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--requests" && i + 1 < argc) {
            config.numRequests = std::stoul(argv[++i]);
        } else if (arg == "--engines" && i + 1 < argc) {
            config.numEngines = std::stoul(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.numThreads = std::stoul(argv[++i]);
        } else if (arg == "--processing-time" && i + 1 < argc) {
            config.engineProcessingTime = std::chrono::microseconds(std::stoul(argv[++i]));
        } else if (arg == "--cache-size" && i + 1 < argc) {
            config.cacheSize = std::stoul(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --requests N        Number of requests to process (default: 100)" << std::endl;
            std::cout << "  --engines N         Number of consensus engines (default: 4)" << std::endl;
            std::cout << "  --threads N         Number of worker threads (default: hardware_concurrency)" << std::endl;
            std::cout << "  --processing-time N Engine processing time in microseconds (default: 1000)" << std::endl;
            std::cout << "  --cache-size N      Cache size (default: 1000)" << std::endl;
            std::cout << "  --help              Show this help message" << std::endl;
            return 0;
        }
    }
    
    std::cout << "Benchmark configuration:" << std::endl;
    std::cout << "  Requests: " << config.numRequests << std::endl;
    std::cout << "  Engines: " << config.numEngines << std::endl;
    std::cout << "  Threads: " << config.numThreads << std::endl;
    std::cout << "  Processing time: " << config.engineProcessingTime.count() << "μs" << std::endl;
    std::cout << "  Cache size: " << config.cacheSize << std::endl;
    std::cout << std::endl;
    
    try {
        ConsensusPerformanceBenchmark benchmark(config);
        benchmark.runComprehensiveBenchmark();
        
        std::cout << "\n🎉 Benchmark completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Benchmark failed: " << e.what() << std::endl;
        return 1;
    }
}