#include "consensus_monitor.h"
#include "logger.h"
#include <algorithm>
#include <sstream>
#include <random>

// PerformanceMetrics implementation
nlohmann::json PerformanceMetrics::toJson() const {
    nlohmann::json j;
    j["throughput"] = throughput;
    j["averageResponseTime"] = averageResponseTime;
    j["minResponseTime"] = minResponseTime;
    j["maxResponseTime"] = maxResponseTime;
    j["p95ResponseTime"] = p95ResponseTime;
    j["p99ResponseTime"] = p99ResponseTime;
    j["cpuUsage"] = cpuUsage;
    j["memoryUsage"] = memoryUsage;
    j["networkBandwidth"] = networkBandwidth;
    j["diskIO"] = diskIO;
    j["accuracy"] = accuracy;
    j["consistency"] = consistency;
    j["reliability"] = reliability;
    j["throughputTrend"] = throughputTrend;
    j["responseTimeTrend"] = responseTimeTrend;
    j["errorRateTrend"] = errorRateTrend;
    return j;
}

void PerformanceMetrics::updateFromResponseTime(double responseTime) {
    // Simple exponential moving average
    averageResponseTime = (averageResponseTime * 0.9) + (responseTime * 0.1);
    
    // Update min/max
    if (minResponseTime == 0.0 || responseTime < minResponseTime) {
        minResponseTime = responseTime;
    }
    if (responseTime > maxResponseTime) {
        maxResponseTime = responseTime;
    }
}

void PerformanceMetrics::updateTrends(const PerformanceMetrics& previous) {
    throughputTrend = throughput - previous.throughput;
    responseTimeTrend = averageResponseTime - previous.averageResponseTime;
}

// DashboardData implementation
nlohmann::json DashboardData::toJson() const {
    nlohmann::json j;
    j["systemThroughput"] = systemThroughput;
    j["systemAverageResponseTime"] = systemAverageResponseTime;
    j["systemErrorRate"] = systemErrorRate;
    j["systemHealthScore"] = systemHealthScore;
    j["totalCpuUsage"] = totalCpuUsage;
    j["totalMemoryUsage"] = totalMemoryUsage;
    j["totalNetworkBandwidth"] = totalNetworkBandwidth;
    j["totalDiskIO"] = totalDiskIO;
    j["timestamp"] = timestamp;
    
    // Convert trends map to JSON
    j["trends"] = trends;
    j["insights"] = insights;
    j["predictions"] = predictions;
    
    return j;
}

// HistoricalMetrics implementation
void HistoricalMetrics::addDataPoint(ConsensusType type, const std::string& metric, double value) {
    HistoricalDataPoint point(metric, value, type);
    metricsHistory[type].push_back(point);
    
    // Cleanup if we exceed max history size
    if (metricsHistory[type].size() > maxHistorySize) {
        metricsHistory[type].erase(metricsHistory[type].begin());
    }
}

std::vector<HistoricalDataPoint> HistoricalMetrics::getMetricHistory(ConsensusType type, const std::string& metric, 
                                                                    uint64_t fromTimestamp, uint64_t toTimestamp) const {
    std::vector<HistoricalDataPoint> result;
    
    auto it = metricsHistory.find(type);
    if (it != metricsHistory.end()) {
        for (const auto& point : it->second) {
            if (point.metric == metric) {
                if ((fromTimestamp == 0 || point.timestamp >= fromTimestamp) &&
                    (toTimestamp == 0 || point.timestamp <= toTimestamp)) {
                    result.push_back(point);
                }
            }
        }
    }
    
    return result;
}

void HistoricalMetrics::cleanupOldData() {
    uint64_t cutoffTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - retentionPeriod;
    
    for (auto& [type, history] : metricsHistory) {
        history.erase(
            std::remove_if(history.begin(), history.end(),
                [cutoffTime](const HistoricalDataPoint& point) {
                    return point.timestamp < cutoffTime;
                }),
            history.end()
        );
    }
}

double HistoricalMetrics::calculateTrend(ConsensusType type, const std::string& metric, uint64_t timeWindow) const {
    auto history = getMetricHistory(type, metric);
    if (history.size() < 2) return 0.0;
    
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t startTime = currentTime - timeWindow;
    
    std::vector<double> recentValues;
    for (const auto& point : history) {
        if (point.timestamp >= startTime) {
            recentValues.push_back(point.value);
        }
    }
    
    if (recentValues.size() < 2) return 0.0;
    
    // Simple linear trend calculation
    double first = recentValues.front();
    double last = recentValues.back();
    return last - first;
}

nlohmann::json HistoricalMetrics::getHistoricalSummary() const {
    nlohmann::json j;
    for (const auto& [type, history] : metricsHistory) {
        j[std::to_string(static_cast<int>(type))] = history.size();
    }
    return j;
}

// MonitoringReport implementation
nlohmann::json MonitoringReport::toJson() const {
    nlohmann::json j;
    j["totalConflictsDetected"] = totalConflictsDetected;
    j["totalAlertsGenerated"] = totalAlertsGenerated;
    j["reportTimestamp"] = reportTimestamp;
    j["overallHealthScore"] = overallHealthScore;
    j["systemStable"] = systemStable;
    j["recommendations"] = recommendations;
    j["dashboardData"] = dashboardData.toJson();
    return j;
}

// ConsensusMonitor implementation
ConsensusMonitor::ConsensusMonitor() {
    Logger::log(LogLevel::INFO, "ConsensusMonitor created");
}

ConsensusMonitor::~ConsensusMonitor() {
    shutdown();
    Logger::log(LogLevel::INFO, "ConsensusMonitor destroyed");
}

bool ConsensusMonitor::initialize(const MonitoringConfig& cfg) {
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    config = cfg;
    running.store(true);
    
    Logger::log(LogLevel::INFO, "ConsensusMonitor initialized with health check interval: " + 
                std::to_string(config.healthCheckInterval) + "s");
    
    return true;
}

void ConsensusMonitor::shutdown() {
    if (!running.load()) return;
    
    running.store(false);
    monitoringActive.store(false);
    
    // Wait for threads to finish
    if (monitoringThread.joinable()) {
        monitoringThread.join();
    }
    if (conflictDetectionThread.joinable()) {
        conflictDetectionThread.join();
    }
    if (alertProcessingThread.joinable()) {
        alertProcessingThread.join();
    }
    if (historicalDataThread.joinable()) {
        historicalDataThread.join();
    }
    
    Logger::log(LogLevel::INFO, "ConsensusMonitor shutdown complete");
}

void ConsensusMonitor::monitorConsensusHealth() {
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    for (auto& [type, metrics] : healthMetrics) {
        // Stub implementation - just log that we're monitoring
        logMonitoringEvent("Monitoring health for consensus type: " + 
                          consensusTypeToString(type));
        
        // Update some basic metrics for demonstration
        metrics.isActive = true;
        metrics.healthScore = 0.95; // Assume good health
    }
}

void ConsensusMonitor::startContinuousMonitoring() {
    if (monitoringActive.load()) return;
    
    monitoringActive.store(true);
    
    monitoringThread = std::thread(&ConsensusMonitor::monitoringLoop, this);
    conflictDetectionThread = std::thread(&ConsensusMonitor::conflictDetectionLoop, this);
    alertProcessingThread = std::thread(&ConsensusMonitor::alertProcessingLoop, this);
    historicalDataThread = std::thread(&ConsensusMonitor::historicalDataLoop, this);
    
    Logger::log(LogLevel::INFO, "Continuous monitoring started");
}

void ConsensusMonitor::stopContinuousMonitoring() {
    monitoringActive.store(false);
    Logger::log(LogLevel::INFO, "Continuous monitoring stopped");
}

void ConsensusMonitor::detectConflicts() {
    std::lock_guard<std::mutex> lock(conflictsMutex);
    
    // Stub implementation - just log
    logMonitoringEvent("Detecting conflicts between consensus mechanisms");
    
    // For demonstration, we could add a fake conflict occasionally
    static int conflictCounter = 0;
    if (++conflictCounter % 100 == 0) {
        MonitoringConflictInfo conflict("conflict_" + std::to_string(conflictCounter),
                                       {ConsensusType::PROOF_OF_STAKE, ConsensusType::VOTING_CONSENSUS},
                                       "Simulated conflict for testing",
                                       "LOW");
        activeConflicts.push_back(conflict);
        totalConflictsDetected.fetch_add(1);
    }
}

bool ConsensusMonitor::detectConflictBetweenMechanisms(ConsensusType type1, ConsensusType type2) {
    // Stub implementation - return false (no conflict detected)
    logMonitoringEvent("Checking conflict between " + consensusTypeToString(type1) + 
                      " and " + consensusTypeToString(type2));
    return false;
}

void ConsensusMonitor::reportConflict(const MonitoringConflictInfo& conflict) {
    std::lock_guard<std::mutex> lock(conflictsMutex);
    
    activeConflicts.push_back(conflict);
    totalConflictsDetected.fetch_add(1);
    
    logMonitoringEvent("Conflict reported: " + conflict.description, LogLevel::WARNING);
}

void ConsensusMonitor::generateAlerts() {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    // Stub implementation - check for basic alert conditions
    for (const auto& [type, metrics] : healthMetrics) {
        if (metrics.healthScore < config.healthCriticalThreshold) {
            AlertInfo alert("alert_" + std::to_string(totalAlertsGenerated.load()),
                           "HEALTH",
                           "Critical health score for " + consensusTypeToString(type),
                           "CRITICAL",
                           {type});
            activeAlerts.push_back(alert);
            totalAlertsGenerated.fetch_add(1);
        }
    }
}

void ConsensusMonitor::generateAlert(const AlertInfo& alert) {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    activeAlerts.push_back(alert);
    totalAlertsGenerated.fetch_add(1);
    
    logMonitoringEvent("Alert generated: " + alert.message, LogLevel::WARNING);
}

void ConsensusMonitor::acknowledgeAlert(const std::string& alertId) {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    for (auto& alert : activeAlerts) {
        if (alert.alertId == alertId) {
            alert.acknowledged = true;
            logMonitoringEvent("Alert acknowledged: " + alertId);
            break;
        }
    }
}

void ConsensusMonitor::updatePerformanceMetrics(ConsensusType type, bool success, double responseTime) {
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    auto& metrics = healthMetrics[type];
    metrics.updateMetrics(success, responseTime);
    
    // Also update historical data
    {
        std::lock_guard<std::mutex> histLock(historicalDataMutex);
        historicalMetrics.addDataPoint(type, "responseTime", responseTime);
        historicalMetrics.addDataPoint(type, "throughput", metrics.throughput);
    }
}

void ConsensusMonitor::identifyPerformanceIssues() {
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    for (const auto& [type, metrics] : healthMetrics) {
        if (metrics.averageResponseTime > config.responseTimeWarningThreshold) {
            logMonitoringEvent("Performance issue detected for " + consensusTypeToString(type) + 
                              ": high response time", LogLevel::WARNING);
        }
        
        if (metrics.errorRate > config.errorRateWarningThreshold) {
            logMonitoringEvent("Performance issue detected for " + consensusTypeToString(type) + 
                              ": high error rate", LogLevel::WARNING);
        }
    }
}

std::vector<std::string> ConsensusMonitor::generateRebalancingRecommendations() {
    std::vector<std::string> recommendations;
    
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    // Stub implementation - generate some basic recommendations
    for (const auto& [type, metrics] : healthMetrics) {
        if (metrics.healthScore < config.healthWarningThreshold) {
            recommendations.push_back("Consider reducing load on " + consensusTypeToString(type));
        }
        
        if (metrics.errorRate > config.errorRateWarningThreshold) {
            recommendations.push_back("Investigate errors in " + consensusTypeToString(type));
        }
    }
    
    if (recommendations.empty()) {
        recommendations.push_back("System is performing well, no rebalancing needed");
    }
    
    return recommendations;
}

void ConsensusMonitor::trackForkContribution(ConsensusType type, const std::string& forkInfo) {
    std::lock_guard<std::mutex> lock(forksMutex);
    
    forkContributions[type].push_back(forkInfo);
    logMonitoringEvent("Fork contribution tracked for " + consensusTypeToString(type) + 
                      ": " + forkInfo);
}

MonitoringReport ConsensusMonitor::getReport() {
    MonitoringReport report;
    
    {
        std::lock_guard<std::mutex> lock(healthMetricsMutex);
        report.healthMetrics = healthMetrics;
    }
    
    {
        std::lock_guard<std::mutex> lock(conflictsMutex);
        report.activeConflicts = activeConflicts;
    }
    
    {
        std::lock_guard<std::mutex> lock(alertsMutex);
        report.recentAlerts = activeAlerts;
    }
    
    report.totalConflictsDetected = totalConflictsDetected.load();
    report.totalAlertsGenerated = totalAlertsGenerated.load();
    report.overallHealthScore = calculateOverallHealthScore();
    report.systemStable = isSystemStable();
    report.recommendations = generateRebalancingRecommendations();
    
    return report;
}

MonitoringReport ConsensusMonitor::getDetailedReport() {
    auto report = getReport();
    
    // Add dashboard data and historical metrics
    report.dashboardData = generateDashboardData();
    
    {
        std::lock_guard<std::mutex> lock(historicalDataMutex);
        report.historicalMetrics = historicalMetrics;
    }
    
    return report;
}

nlohmann::json ConsensusMonitor::getRealtimeStatus() {
    nlohmann::json status;
    
    status["running"] = running.load();
    status["monitoringActive"] = monitoringActive.load();
    status["totalConflicts"] = totalConflictsDetected.load();
    status["totalAlerts"] = totalAlertsGenerated.load();
    status["overallHealthScore"] = calculateOverallHealthScore();
    status["systemStable"] = isSystemStable();
    
    return status;
}

nlohmann::json ConsensusMonitor::getDashboardData() {
    return generateDashboardData().toJson();
}

DashboardData ConsensusMonitor::generateDashboardData() {
    std::lock_guard<std::mutex> lock(dashboardMutex);
    
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Check if we need to update the cache
    if (currentTime - lastDashboardUpdate > dashboardCacheTimeout) {
        updateDashboardCache();
        lastDashboardUpdate = currentTime;
    }
    
    return cachedDashboardData;
}

void ConsensusMonitor::updateHistoricalMetrics() {
    std::lock_guard<std::mutex> lock(historicalDataMutex);
    historicalMetrics.cleanupOldData();
}

std::vector<std::string> ConsensusMonitor::generateInsights() {
    std::vector<std::string> insights;
    
    // Stub implementation - generate some basic insights
    insights.push_back("System is operating within normal parameters");
    insights.push_back("No significant performance degradation detected");
    
    return insights;
}

std::vector<std::string> ConsensusMonitor::generatePredictions() {
    std::vector<std::string> predictions;
    
    // Stub implementation - generate some basic predictions
    predictions.push_back("System performance expected to remain stable");
    predictions.push_back("No resource constraints anticipated in the next hour");
    
    return predictions;
}

double ConsensusMonitor::calculateMetricTrend(ConsensusType type, const std::string& metric, uint64_t timeWindow) {
    std::lock_guard<std::mutex> lock(historicalDataMutex);
    return historicalMetrics.calculateTrend(type, metric, timeWindow);
}

std::map<std::string, double> ConsensusMonitor::getSystemTrends() {
    std::map<std::string, double> trends;
    
    // Stub implementation - return some basic trends
    trends["throughput"] = 0.05;  // Slight improvement
    trends["responseTime"] = -0.02; // Slight improvement (negative is better)
    trends["errorRate"] = 0.0;    // Stable
    
    return trends;
}

void ConsensusMonitor::updateResourceMetrics(ConsensusType type, double cpu, double memory, double network, double diskIO) {
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    auto& metrics = healthMetrics[type];
    metrics.performance.cpuUsage = cpu;
    metrics.performance.memoryUsage = memory;
    metrics.performance.networkBandwidth = network;
    metrics.performance.diskIO = diskIO;
}

nlohmann::json ConsensusMonitor::getResourceUtilization() {
    nlohmann::json resources;
    
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    double totalCpu = 0.0, totalMemory = 0.0, totalNetwork = 0.0, totalDisk = 0.0;
    int count = 0;
    
    for (const auto& [type, metrics] : healthMetrics) {
        totalCpu += metrics.performance.cpuUsage;
        totalMemory += metrics.performance.memoryUsage;
        totalNetwork += metrics.performance.networkBandwidth;
        totalDisk += metrics.performance.diskIO;
        count++;
    }
    
    if (count > 0) {
        resources["cpu"] = totalCpu / count;
        resources["memory"] = totalMemory / count;
        resources["network"] = totalNetwork / count;
        resources["disk"] = totalDisk / count;
    }
    
    return resources;
}

std::vector<HistoricalDataPoint> ConsensusMonitor::getHistoricalData(ConsensusType type, const std::string& metric, 
                                                                    uint64_t fromTime, uint64_t toTime) {
    std::lock_guard<std::mutex> lock(historicalDataMutex);
    return historicalMetrics.getMetricHistory(type, metric, fromTime, toTime);
}

void ConsensusMonitor::exportHistoricalData(const std::string& filename) {
    // Stub implementation - just log
    logMonitoringEvent("Exporting historical data to: " + filename);
}

void ConsensusMonitor::importHistoricalData(const std::string& filename) {
    // Stub implementation - just log
    logMonitoringEvent("Importing historical data from: " + filename);
}

void ConsensusMonitor::registerConsensusEngine(ConsensusType type, std::shared_ptr<ConsensusEngine> engine) {
    std::lock_guard<std::mutex> lock(enginesMutex);
    
    engines[type] = engine;
    
    // Initialize health metrics for this engine
    {
        std::lock_guard<std::mutex> healthLock(healthMetricsMutex);
        healthMetrics[type] = HealthMetrics{};
    }
    
    logMonitoringEvent("Registered consensus engine: " + consensusTypeToString(type));
}

void ConsensusMonitor::unregisterConsensusEngine(ConsensusType type) {
    std::lock_guard<std::mutex> lock(enginesMutex);
    
    engines.erase(type);
    
    logMonitoringEvent("Unregistered consensus engine: " + consensusTypeToString(type));
}

void ConsensusMonitor::updateConfig(const MonitoringConfig& cfg) {
    config = cfg;
    logMonitoringEvent("Configuration updated");
}

std::vector<AlertInfo> ConsensusMonitor::getActiveAlerts() {
    std::lock_guard<std::mutex> lock(alertsMutex);
    return activeAlerts;
}

std::vector<AlertInfo> ConsensusMonitor::getAlertsForMechanism(ConsensusType type) {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    std::vector<AlertInfo> result;
    for (const auto& alert : activeAlerts) {
        for (ConsensusType affectedType : alert.affectedMechanisms) {
            if (affectedType == type) {
                result.push_back(alert);
                break;
            }
        }
    }
    
    return result;
}

void ConsensusMonitor::clearOldAlerts(uint64_t olderThanSeconds) {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    uint64_t cutoffTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - olderThanSeconds;
    
    activeAlerts.erase(
        std::remove_if(activeAlerts.begin(), activeAlerts.end(),
            [cutoffTime](const AlertInfo& alert) {
                return alert.timestamp < cutoffTime;
            }),
        activeAlerts.end()
    );
}

// Private helper methods
void ConsensusMonitor::monitoringLoop() {
    while (monitoringActive.load()) {
        try {
            monitorConsensusHealth();
            identifyPerformanceIssues();
            
            std::this_thread::sleep_for(std::chrono::seconds(config.healthCheckInterval));
        } catch (const std::exception& e) {
            logMonitoringEvent("Error in monitoring loop: " + std::string(e.what()), LogLevel::ERROR);
        }
    }
}

void ConsensusMonitor::conflictDetectionLoop() {
    while (monitoringActive.load()) {
        try {
            detectConflicts();
            
            std::this_thread::sleep_for(std::chrono::seconds(config.conflictCheckInterval));
        } catch (const std::exception& e) {
            logMonitoringEvent("Error in conflict detection loop: " + std::string(e.what()), LogLevel::ERROR);
        }
    }
}

void ConsensusMonitor::alertProcessingLoop() {
    while (monitoringActive.load()) {
        try {
            generateAlerts();
            clearOldAlerts();
            
            std::this_thread::sleep_for(std::chrono::seconds(60)); // Check every minute
        } catch (const std::exception& e) {
            logMonitoringEvent("Error in alert processing loop: " + std::string(e.what()), LogLevel::ERROR);
        }
    }
}

void ConsensusMonitor::historicalDataLoop() {
    while (monitoringActive.load()) {
        try {
            updateHistoricalMetrics();
            
            std::this_thread::sleep_for(std::chrono::seconds(300)); // Every 5 minutes
        } catch (const std::exception& e) {
            logMonitoringEvent("Error in historical data loop: " + std::string(e.what()), LogLevel::ERROR);
        }
    }
}

void ConsensusMonitor::checkEngineHealth(ConsensusType type) {
    // Stub implementation - just update basic health metrics
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    auto& metrics = healthMetrics[type];
    metrics.isActive = true;
    metrics.healthScore = 0.95; // Assume good health
}

void ConsensusMonitor::processHealthMetrics() {
    // Stub implementation - basic processing
    logMonitoringEvent("Processing health metrics");
}

void ConsensusMonitor::detectPerformanceDegradation() {
    // Stub implementation - basic detection
    logMonitoringEvent("Checking for performance degradation");
}

void ConsensusMonitor::detectUnbalancedMechanisms() {
    // Stub implementation - basic detection
    logMonitoringEvent("Checking for unbalanced mechanisms");
}

std::string ConsensusMonitor::generateConflictId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return "conflict_" + std::to_string(dis(gen));
}

std::string ConsensusMonitor::generateAlertId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return "alert_" + std::to_string(dis(gen));
}

void ConsensusMonitor::logMonitoringEvent(const std::string& event, LogLevel level) {
    Logger::log(level, "[ConsensusMonitor] " + event);
}

void ConsensusMonitor::notifyAdministrators(const AlertInfo& alert) {
    // Stub implementation - just log
    logMonitoringEvent("Notifying administrators about alert: " + alert.message, LogLevel::WARNING);
}

void ConsensusMonitor::collectSystemMetrics() {
    // Stub implementation - collect basic system metrics
    logMonitoringEvent("Collecting system metrics");
}

void ConsensusMonitor::updateDashboardCache() {
    cachedDashboardData = DashboardData{};
    
    // Update basic dashboard data
    cachedDashboardData.systemHealthScore = calculateOverallHealthScore();
    cachedDashboardData.insights = generateInsights();
    cachedDashboardData.predictions = generatePredictions();
    cachedDashboardData.trends = getSystemTrends();
    
    // Update current metrics
    {
        std::lock_guard<std::mutex> lock(healthMetricsMutex);
        cachedDashboardData.currentMetrics = healthMetrics;
    }
}

void ConsensusMonitor::analyzePerformanceTrends() {
    // Stub implementation - basic trend analysis
    logMonitoringEvent("Analyzing performance trends");
}

void ConsensusMonitor::generatePerformanceInsights() {
    // Stub implementation - basic insight generation
    logMonitoringEvent("Generating performance insights");
}

void ConsensusMonitor::predictPerformanceIssues() {
    // Stub implementation - basic prediction
    logMonitoringEvent("Predicting performance issues");
}

double ConsensusMonitor::calculateMovingAverage(const std::vector<HistoricalDataPoint>& data, uint64_t timeWindow) const {
    if (data.empty()) return 0.0;
    
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t startTime = currentTime - timeWindow;
    
    double sum = 0.0;
    int count = 0;
    
    for (const auto& point : data) {
        if (point.timestamp >= startTime) {
            sum += point.value;
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}

double ConsensusMonitor::calculateStandardDeviation(const std::vector<HistoricalDataPoint>& data) const {
    if (data.size() < 2) return 0.0;
    
    double mean = 0.0;
    for (const auto& point : data) {
        mean += point.value;
    }
    mean /= data.size();
    
    double variance = 0.0;
    for (const auto& point : data) {
        variance += (point.value - mean) * (point.value - mean);
    }
    variance /= data.size();
    
    return std::sqrt(variance);
}

std::vector<double> ConsensusMonitor::detectAnomalies(const std::vector<HistoricalDataPoint>& data) const {
    std::vector<double> anomalies;
    
    if (data.size() < 10) return anomalies; // Need enough data points
    
    double mean = 0.0;
    for (const auto& point : data) {
        mean += point.value;
    }
    mean /= data.size();
    
    double stdDev = calculateStandardDeviation(data);
    double threshold = 2.0 * stdDev; // 2 standard deviations
    
    for (const auto& point : data) {
        if (std::abs(point.value - mean) > threshold) {
            anomalies.push_back(point.value);
        }
    }
    
    return anomalies;
}

std::string ConsensusMonitor::consensusTypeToString(ConsensusType type) const {
    switch (type) {
        case ConsensusType::PROOF_OF_WORK: return "PROOF_OF_WORK";
        case ConsensusType::PROOF_OF_STAKE: return "PROOF_OF_STAKE";
        case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION: return "PROOF_OF_RESOURCE_CONTRIBUTION";
        case ConsensusType::VOTING_CONSENSUS: return "VOTING_CONSENSUS";
        case ConsensusType::SMART_CONTRACT_VALIDATION: return "SMART_CONTRACT_VALIDATION";
        default: return "UNKNOWN";
    }
}

double ConsensusMonitor::calculateOverallHealthScore() const {
    std::lock_guard<std::mutex> lock(healthMetricsMutex);
    
    if (healthMetrics.empty()) return 1.0;
    
    double totalScore = 0.0;
    for (const auto& [type, metrics] : healthMetrics) {
        totalScore += metrics.healthScore;
    }
    
    return totalScore / healthMetrics.size();
}

bool ConsensusMonitor::isSystemStable() const {
    double healthScore = calculateOverallHealthScore();
    return healthScore >= config.healthWarningThreshold;
}