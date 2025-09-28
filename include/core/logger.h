#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
private:
    static std::mutex logMutex;
    static LogLevel minLevel;
    static std::string logFilePath;
    static bool logToFile;

    static std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        auto now_tm = std::localtime(&now_c);
        
        std::stringstream ss;
        ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
        
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch() % std::chrono::seconds(1)
        ).count();
        
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds;
        return ss.str();
    }
    
    static std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARNING";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }
    
public:
    static void setLevel(LogLevel level) {
        minLevel = level;
    }
    
    static void log(LogLevel level, const std::string& message);
    
    static void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }
    
    static void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }
    
    static void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }
    
    static void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }
    
    static void critical(const std::string& message) {
        log(LogLevel::CRITICAL, message);
    }
    
    // Security-specific logging methods
    static void logSecurityEvent(const std::string& event, const std::string& clientIP = "", const std::string& details = "");
    static void logAuthentication(const std::string& user, bool success, const std::string& clientIP = "");
    static void logTransaction(const std::string& txHash, const std::string& sender, const std::string& recipient, double amount);
    static void logBlockMined(const std::string& blockHash, const std::string& miner, int blockIndex);
    static void logApiAccess(const std::string& endpoint, const std::string& method, const std::string& clientIP, int statusCode);
    
    // Configuration methods
    static void enableFileLogging(const std::string& filePath);
    static void disableFileLogging();
    static void setLogFile(const std::string& filePath) { logFilePath = filePath; }
};

#endif // LOGGER_H