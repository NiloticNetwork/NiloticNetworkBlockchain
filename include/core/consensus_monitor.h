#ifndef CONSENSUS_MONITOR_H
#define CONSENSUS_MONITOR_H

#include "consensus_harmony.h"
#include "logger.h"
#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <functional>

// Forward declarations
class ConsensusEngine;

// Comprehensive performance metrics for individual consensus mechanisms
struct PerformanceMetrics {
    // Basic metrics
    double throughput = 0.0;                    // validations per second
    double averageResponseTime = 0.0;           // in milliseconds
    double minResponseTime = 0.0;               // minimum response time
    double maxResponseTime = 0.0;               // maximum response time
    double p95ResponseTime = 0.0;               // 95th percentile response time
    double p99ResponseTime = 0.0;               // 99th percentile response time
    
    // Resource utilization
    double cpuUsage = 0.0;                      // CPU usage percentage
    double memoryUsage = 0.0;                   // Memory usage in MB
    double networkBandwidth = 0.0;              // Network bandwidth usage in MB/s
    double diskIO = 0.0;                        // Disk I/O operations per second
    
    // Quality metrics
    double accuracy = 1.0;                      // Validation accuracy (0.0 - 1.0)
    double consistency = 1.0;                   // Result consistency across validations
    double reliability = 1.0;                   // System reliability score
    
    // Trend indicators
    double throughputTrend = 0.0;               // Positive = improving, negative = degrading
    double responseTimeTrend = 0.0;             // Positive = slower, negative = faster
    double errorRateTrend = 0.0;                // Positive = more errors, negative = fewer errors
    
    PerformanceMetrics() = default;
    
    nlohmann::json toJson() const;
    void updateFromResponseTime(double responseTime);
    void updateTrends(const PerformanceMetrics& previous);
};

// Historical data point for trend analysis
struct HistoricalDataPoint {
    uint64_t timestamp;
    double value;
    std::string metric;                         // Name of the metric
    ConsensusType consensusType;
    
    HistoricalDataPoint(const std::string& m, double v, ConsensusType type)
        : metric(m), value(v), consensusType(type),
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// Health metrics for individual consensus mechanisms
struct HealthMetrics {
    double healthScore = 1.0;           // 0.0 (unhealthy) to 1.0 (healthy)
    uint64_t totalValidations = 0;
    uint64_t successfulValidations = 0;
    uint64_t failedValidations = 0;
    double averageResponseTime = 0.0;   // in milliseconds
    uint64_t lastValidationTime = 0;
    std::string lastError;
    bool isActive = true;
    
    // Performance metrics
    double throughput = 0.0;            // validations per second
    double errorRate = 0.0;             // percentage of failed validations
    double availability = 1.0;          // uptime percentage
    
    // Enhanced metrics
    PerformanceMetrics performance;     // Detailed performance metrics
    std::vector<double> recentResponseTimes; // Last 100 response times for percentile calculation
    
    HealthMetrics() = default;
    
    void updateMetrics(bool success, double responseTime) {
        totalValidations++;
        if (success) {
            successfulValidations++;
        } else {
            failedValidations++;
        }
        
        // Update average response time (exponential moving average)
        averageResponseTime = (averageResponseTime * 0.9) + (responseTime * 0.1);
        
        // Update error rate
        errorRate = static_cast<double>(failedValidations) / totalValidations;
        
        // Update performance metrics
        performance.updateFromResponseTime(responseTime);
        
        // Store recent response times for percentile calculation
        recentResponseTimes.push_back(responseTime);
        if (recentResponseTimes.size() > 100) {
            recentResponseTimes.erase(recentResponseTimes.begin());
        }
        
        // Calculate percentiles
        if (!recentResponseTimes.empty()) {
            std::vector<double> sorted = recentResponseTimes;
            std::sort(sorted.begin(), sorted.end());
            
            performance.minResponseTime = sorted.front();
            performance.maxResponseTime = sorted.back();
            
            size_t p95Index = static_cast<size_t>(sorted.size() * 0.95);
            size_t p99Index = static_cast<size_t>(sorted.size() * 0.99);
            
            performance.p95ResponseTime = sorted[std::min(p95Index, sorted.size() - 1)];
            performance.p99ResponseTime = sorted[std::min(p99Index, sorted.size() - 1)];
        }
        
        // Update health score based on error rate and response time
        healthScore = calculateHealthScore();
        
        lastValidationTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
private:
    double calculateHealthScore() const {
        double score = 1.0;
        
        // Penalize high error rates
        score -= (errorRate * 0.5);
        
        // Penalize slow response times (assuming 1000ms is the threshold)
        if (averageResponseTime > 1000.0) {
            score -= ((averageResponseTime - 1000.0) / 10000.0);
        }
        
        // Ensure score is between 0 and 1
        return std::max(0.0, std::min(1.0, score));
    }
};

// Conflict detection information
struct MonitoringConflictInfo {
    std::string conflictId;
    std::vector<ConsensusType> involvedMechanisms;
    std::string description;
    uint64_t timestamp;
    std::string severity;              // LOW, MEDIUM, HIGH, CRITICAL
    bool resolved = false;
    std::string resolution;
    
    MonitoringConflictInfo(const std::string& id, const std::vector<ConsensusType>& mechanisms,
                const std::string& desc, const std::string& sev = "MEDIUM")
        : conflictId(id), involvedMechanisms(mechanisms), description(desc), 
          severity(sev), timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// Alert information
struct AlertInfo {
    std::string alertId;
    std::string type;                  // HEALTH, CONFLICT, PERFORMANCE, SECURITY
    std::string message;
    std::string severity;              // LOW, MEDIUM, HIGH, CRITICAL
    uint64_t timestamp;
    bool acknowledged = false;
    std::vector<ConsensusType> affectedMechanisms;
    
    AlertInfo(const std::string& id, const std::string& t, const std::string& msg,
             const std::string& sev = "MEDIUM", const std::vector<ConsensusType>& affected = {})
        : alertId(id), type(t), message(msg), severity(sev), affectedMechanisms(affected),
          timestamp(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// Dashboard data structure for real-time monitoring
struct DashboardData {
    // Real-time metrics
    std::map<ConsensusType, HealthMetrics> currentMetrics;
    std::map<ConsensusType, std::vector<HistoricalDataPoint>> historicalData;
    
    // System-wide statistics
    double systemThroughput = 0.0;
    double systemAverageResponseTime = 0.0;
    double systemErrorRate = 0.0;
    double systemHealthScore = 1.0;
    
    // Trend analysis
    std::map<std::string, double> trends;       // Metric name -> trend value
    std::vector<std::string> insights;          // AI-generated insights
    std::vector<std::string> predictions;       // Performance predictions
    
    // Resource utilization
    double totalCpuUsage = 0.0;
    double totalMemoryUsage = 0.0;
    double totalNetworkBandwidth = 0.0;
    double totalDiskIO = 0.0;
    
    uint64_t timestamp;
    
    DashboardData() : timestamp(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()) {}
    
    nlohmann::json toJson() const;
};

// Historical metrics storage for trend analysis
struct HistoricalMetrics {
    std::map<ConsensusType, std::vector<HistoricalDataPoint>> metricsHistory;
    uint64_t maxHistorySize = 10000;            // Maximum number of data points per metric
    uint64_t retentionPeriod = 2592000;         // 30 days in seconds
    
    void addDataPoint(ConsensusType type, const std::string& metric, double value);
    std::vector<HistoricalDataPoint> getMetricHistory(ConsensusType type, const std::string& metric, 
                                                     uint64_t fromTimestamp = 0, uint64_t toTimestamp = 0) const;
    void cleanupOldData();
    double calculateTrend(ConsensusType type, const std::string& metric, uint64_t timeWindow = 3600) const;
    nlohmann::json getHistoricalSummary() const;
};

// Monitoring report structure
struct MonitoringReport {
    std::map<ConsensusType, HealthMetrics> healthMetrics;
    std::vector<MonitoringConflictInfo> activeConflicts;
    std::vector<AlertInfo> recentAlerts;
    uint64_t totalConflictsDetected = 0;
    uint64_t totalAlertsGenerated = 0;
    uint64_t reportTimestamp;
    
    // Overall system health
    double overallHealthScore = 1.0;
    bool systemStable = true;
    std::vector<std::string> recommendations;
    
    // Enhanced reporting
    DashboardData dashboardData;
    HistoricalMetrics historicalMetrics;
    
    MonitoringReport() : reportTimestamp(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()) {}
    
    nlohmann::json toJson() const;
};

// Configuration for monitoring
struct MonitoringConfig {
    uint64_t healthCheckInterval = 30;      // seconds
    uint64_t conflictCheckInterval = 10;    // seconds
    uint64_t reportGenerationInterval = 300; // seconds (5 minutes)
    
    // Health thresholds
    double healthWarningThreshold = 0.7;
    double healthCriticalThreshold = 0.5;
    double errorRateWarningThreshold = 0.1;  // 10%
    double errorRateCriticalThreshold = 0.25; // 25%
    
    // Performance thresholds
    double responseTimeWarningThreshold = 1000.0;  // 1 second
    double responseTimeCriticalThreshold = 5000.0; // 5 seconds
    
    // Conflict detection settings
    bool enableConflictDetection = true;
    bool enablePerformanceMonitoring = true;
    bool enableHealthMonitoring = true;
    
    // Alert settings
    uint64_t maxAlertsPerHour = 100;
    bool enableEmailAlerts = false;
    std::string alertEmailAddress;
};

/**
 * Consensus Monitor - Monitors health and performance of all consensus mechanisms
 * 
 * This class provides comprehensive monitoring capabilities for the consensus harmony system:
 * - Health and performance tracking for all consensus mechanisms
 * - Conflict detection between different consensus mechanisms
 * - Alert generation for administrators
 * - Real-time status reporting and dashboards
 * 
 * Requirements addressed: 5.1, 5.2, 5.3, 5.4, 5.5
 */
class ConsensusMonitor {
public:
    ConsensusMonitor();
    ~ConsensusMonitor();
    
    // Initialization and shutdown
    bool initialize(const MonitoringConfig& config = MonitoringConfig{});
    void shutdown();
    bool isRunning() const { return running.load(); }
    
    // Core monitoring functions (Requirement 5.1)
    void monitorConsensusHealth();
    void startContinuousMonitoring();
    void stopContinuousMonitoring();
    
    // Conflict detection (Requirement 5.2)
    void detectConflicts();
    bool detectConflictBetweenMechanisms(ConsensusType type1, ConsensusType type2);
    void reportConflict(const MonitoringConflictInfo& conflict);
    
    // Alert generation (Requirement 5.2, 5.3)
    void generateAlerts();
    void generateAlert(const AlertInfo& alert);
    void acknowledgeAlert(const std::string& alertId);
    
    // Performance monitoring (Requirement 5.3)
    void updatePerformanceMetrics(ConsensusType type, bool success, double responseTime);
    void identifyPerformanceIssues();
    
    // Rebalancing recommendations (Requirement 5.4)
    std::vector<std::string> generateRebalancingRecommendations();
    
    // Fork tracking (Requirement 5.5)
    void trackForkContribution(ConsensusType type, const std::string& forkInfo);
    
    // Reporting and status
    MonitoringReport getReport();
    MonitoringReport getDetailedReport();
    nlohmann::json getRealtimeStatus();
    nlohmann::json getDashboardData();
    
    // Enhanced metrics and dashboard methods
    DashboardData generateDashboardData();
    void updateHistoricalMetrics();
    std::vector<std::string> generateInsights();
    std::vector<std::string> generatePredictions();
    
    // Trend analysis
    double calculateMetricTrend(ConsensusType type, const std::string& metric, uint64_t timeWindow = 3600);
    std::map<std::string, double> getSystemTrends();
    
    // Resource monitoring
    void updateResourceMetrics(ConsensusType type, double cpu, double memory, double network, double diskIO);
    nlohmann::json getResourceUtilization();
    
    // Historical data management
    std::vector<HistoricalDataPoint> getHistoricalData(ConsensusType type, const std::string& metric, 
                                                      uint64_t fromTime = 0, uint64_t toTime = 0);
    void exportHistoricalData(const std::string& filename);
    void importHistoricalData(const std::string& filename);
    
    // Engine registration
    void registerConsensusEngine(ConsensusType type, std::shared_ptr<ConsensusEngine> engine);
    void unregisterConsensusEngine(ConsensusType type);
    
    // Configuration
    void updateConfig(const MonitoringConfig& config);
    MonitoringConfig getConfig() const { return config; }
    
    // Alert management
    std::vector<AlertInfo> getActiveAlerts();
    std::vector<AlertInfo> getAlertsForMechanism(ConsensusType type);
    void clearOldAlerts(uint64_t olderThanSeconds = 86400); // 24 hours default
    
    // Statistics
    uint64_t getTotalConflictsDetected() const { return totalConflictsDetected.load(); }
    uint64_t getTotalAlertsGenerated() const { return totalAlertsGenerated.load(); }
    
private:
    // Configuration
    MonitoringConfig config;
    
    // Registered consensus engines
    std::map<ConsensusType, std::shared_ptr<ConsensusEngine>> engines;
    mutable std::mutex enginesMutex;
    
    // Health metrics storage
    std::map<ConsensusType, HealthMetrics> healthMetrics;
    mutable std::mutex healthMetricsMutex;
    
    // Conflict tracking
    std::vector<MonitoringConflictInfo> activeConflicts;
    std::vector<MonitoringConflictInfo> resolvedConflicts;
    mutable std::mutex conflictsMutex;
    
    // Alert management
    std::queue<AlertInfo> alertQueue;
    std::vector<AlertInfo> activeAlerts;
    mutable std::mutex alertsMutex;
    
    // Fork tracking
    std::map<ConsensusType, std::vector<std::string>> forkContributions;
    mutable std::mutex forksMutex;
    
    // Historical data storage
    HistoricalMetrics historicalMetrics;
    mutable std::mutex historicalDataMutex;
    
    // Dashboard data cache
    DashboardData cachedDashboardData;
    mutable std::mutex dashboardMutex;
    uint64_t lastDashboardUpdate = 0;
    uint64_t dashboardCacheTimeout = 30; // seconds
    
    // Monitoring state
    std::atomic<bool> running{false};
    std::atomic<bool> monitoringActive{false};
    std::thread monitoringThread;
    std::thread conflictDetectionThread;
    std::thread alertProcessingThread;
    std::thread historicalDataThread;
    
    // Statistics
    std::atomic<uint64_t> totalConflictsDetected{0};
    std::atomic<uint64_t> totalAlertsGenerated{0};
    
    // Private helper methods
    void monitoringLoop();
    void conflictDetectionLoop();
    void alertProcessingLoop();
    void historicalDataLoop();
    
    void checkEngineHealth(ConsensusType type);
    void processHealthMetrics();
    void detectPerformanceDegradation();
    void detectUnbalancedMechanisms();
    
    std::string generateConflictId() const;
    std::string generateAlertId() const;
    
    void logMonitoringEvent(const std::string& event, LogLevel level = LogLevel::INFO);
    void notifyAdministrators(const AlertInfo& alert);
    
    // Enhanced helper methods
    void collectSystemMetrics();
    void updateDashboardCache();
    void analyzePerformanceTrends();
    void generatePerformanceInsights();
    void predictPerformanceIssues();
    
    // Data analysis methods
    double calculateMovingAverage(const std::vector<HistoricalDataPoint>& data, uint64_t timeWindow) const;
    double calculateStandardDeviation(const std::vector<HistoricalDataPoint>& data) const;
    std::vector<double> detectAnomalies(const std::vector<HistoricalDataPoint>& data) const;
    
    // Utility methods
    std::string consensusTypeToString(ConsensusType type) const;
    double calculateOverallHealthScore() const;
    bool isSystemStable() const;
};

#endif // CONSENSUS_MONITOR_H