#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>

class RateLimiter {
private:
    struct ClientInfo {
        std::vector<std::chrono::steady_clock::time_point> requests;
        bool blocked;
        std::chrono::steady_clock::time_point blockUntil;
    };
    
    std::map<std::string, ClientInfo> clients;
    std::mutex clientsMutex;
    
    // Configuration
    size_t maxRequestsPerMinute;
    size_t maxRequestsPerHour;
    std::chrono::minutes blockDuration;
    
public:
    RateLimiter(size_t requestsPerMinute = 60, size_t requestsPerHour = 1000, 
                std::chrono::minutes blockTime = std::chrono::minutes(5))
        : maxRequestsPerMinute(requestsPerMinute), maxRequestsPerHour(requestsPerHour), 
          blockDuration(blockTime) {}
    
    // Check if request is allowed for given IP
    bool isAllowed(const std::string& clientIP);
    
    // Block a specific IP
    void blockClient(const std::string& clientIP, std::chrono::minutes duration = std::chrono::minutes(60));
    
    // Unblock a specific IP
    void unblockClient(const std::string& clientIP);
    
    // Clean up old request records
    void cleanup();
    
    // Get client statistics
    size_t getRequestCount(const std::string& clientIP, std::chrono::minutes timeWindow = std::chrono::minutes(1));
    
    // Check if client is currently blocked
    bool isBlocked(const std::string& clientIP);
};

#endif // RATE_LIMITER_H