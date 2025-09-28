#include "logger.h"
#include <fstream>
#include <filesystem>

// Define static members
std::mutex Logger::logMutex;
LogLevel Logger::minLevel = LogLevel::INFO;
std::string Logger::logFilePath = "";
bool Logger::logToFile = false;

// Enhanced log method that supports file logging
void Logger::log(LogLevel level, const std::string& message) {
    if (level < minLevel) return;
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string levelStr = levelToString(level);
    std::string timestamp = getTimestamp();
    std::string logEntry = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Always log to console
    std::cout << logEntry << std::endl;
    
    // Log to file if enabled
    if (logToFile && !logFilePath.empty()) {
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << logEntry << std::endl;
            logFile.close();
        }
    }
}

// Security-specific logging methods
void Logger::logSecurityEvent(const std::string& event, const std::string& clientIP, const std::string& details) {
    std::string message = "SECURITY_EVENT: " + event;
    if (!clientIP.empty()) {
        message += " | IP: " + clientIP;
    }
    if (!details.empty()) {
        message += " | Details: " + details;
    }
    log(LogLevel::WARNING, message);
}

void Logger::logAuthentication(const std::string& user, bool success, const std::string& clientIP) {
    std::string message = "AUTH: User '" + user + "' " + (success ? "SUCCESS" : "FAILED");
    if (!clientIP.empty()) {
        message += " | IP: " + clientIP;
    }
    log(success ? LogLevel::INFO : LogLevel::WARNING, message);
}

void Logger::logTransaction(const std::string& txHash, const std::string& sender, const std::string& recipient, double amount) {
    std::string message = "TRANSACTION: " + txHash + " | " + sender + " -> " + recipient + " | Amount: " + std::to_string(amount);
    log(LogLevel::INFO, message);
}

void Logger::logBlockMined(const std::string& blockHash, const std::string& miner, int blockIndex) {
    std::string message = "BLOCK_MINED: #" + std::to_string(blockIndex) + " | Hash: " + blockHash + " | Miner: " + miner;
    log(LogLevel::INFO, message);
}

void Logger::logApiAccess(const std::string& endpoint, const std::string& method, const std::string& clientIP, int statusCode) {
    std::string message = "API_ACCESS: " + method + " " + endpoint + " | IP: " + clientIP + " | Status: " + std::to_string(statusCode);
    log(LogLevel::INFO, message);
}

// Configuration methods
void Logger::enableFileLogging(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Create directory if it doesn't exist
    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());
    
    logFilePath = filePath;
    logToFile = true;
    
    log(LogLevel::INFO, "File logging enabled: " + filePath);
}

void Logger::disableFileLogging() {
    std::lock_guard<std::mutex> lock(logMutex);
    logToFile = false;
    log(LogLevel::INFO, "File logging disabled");
} 