#include "rate_limiter.h"
#include "utils.h"

bool RateLimiter::isAllowed(const std::string& clientIP) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto& client = clients[clientIP];
    
    // Check if client is currently blocked
    if (client.blocked && now < client.blockUntil) {
        return false;
    }
    
    // Unblock if block period has expired
    if (client.blocked && now >= client.blockUntil) {
        client.blocked = false;
        client.requests.clear();
    }
    
    // Clean up old requests (older than 1 hour)
    auto oneHourAgo = now - std::chrono::hours(1);
    client.requests.erase(
        std::remove_if(client.requests.begin(), client.requests.end(),
                      [oneHourAgo](const std::chrono::steady_clock::time_point& time) {
                          return time < oneHourAgo;
                      }),
        client.requests.end()
    );
    
    // Check hourly limit
    if (client.requests.size() >= maxRequestsPerHour) {
        Utils::logWarning("Client " + clientIP + " exceeded hourly rate limit");
        blockClient(clientIP, std::chrono::minutes(60));
        return false;
    }
    
    // Check per-minute limit
    auto oneMinuteAgo = now - std::chrono::minutes(1);
    size_t recentRequests = std::count_if(client.requests.begin(), client.requests.end(),
                                         [oneMinuteAgo](const std::chrono::steady_clock::time_point& time) {
                                             return time >= oneMinuteAgo;
                                         });
    
    if (recentRequests >= maxRequestsPerMinute) {
        Utils::logWarning("Client " + clientIP + " exceeded per-minute rate limit");
        blockClient(clientIP, blockDuration);
        return false;
    }
    
    // Record this request
    client.requests.push_back(now);
    return true;
}

void RateLimiter::blockClient(const std::string& clientIP, std::chrono::minutes duration) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto& client = clients[clientIP];
    client.blocked = true;
    client.blockUntil = std::chrono::steady_clock::now() + duration;
    
    Utils::logInfo("Blocked client " + clientIP + " for " + std::to_string(duration.count()) + " minutes");
}

void RateLimiter::unblockClient(const std::string& clientIP) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto it = clients.find(clientIP);
    if (it != clients.end()) {
        it->second.blocked = false;
        it->second.requests.clear();
        Utils::logInfo("Unblocked client " + clientIP);
    }
}

void RateLimiter::cleanup() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto oneHourAgo = now - std::chrono::hours(1);
    
    for (auto it = clients.begin(); it != clients.end();) {
        auto& client = it->second;
        
        // Remove old requests
        client.requests.erase(
            std::remove_if(client.requests.begin(), client.requests.end(),
                          [oneHourAgo](const std::chrono::steady_clock::time_point& time) {
                              return time < oneHourAgo;
                          }),
            client.requests.end()
        );
        
        // Remove clients with no recent activity and not blocked
        if (client.requests.empty() && (!client.blocked || now >= client.blockUntil)) {
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}

size_t RateLimiter::getRequestCount(const std::string& clientIP, std::chrono::minutes timeWindow) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto it = clients.find(clientIP);
    if (it == clients.end()) {
        return 0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto windowStart = now - timeWindow;
    
    return std::count_if(it->second.requests.begin(), it->second.requests.end(),
                        [windowStart](const std::chrono::steady_clock::time_point& time) {
                            return time >= windowStart;
                        });
}

bool RateLimiter::isBlocked(const std::string& clientIP) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto it = clients.find(clientIP);
    if (it == clients.end()) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    return it->second.blocked && now < it->second.blockUntil;
}