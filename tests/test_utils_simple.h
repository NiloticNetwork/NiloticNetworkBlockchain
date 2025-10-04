#ifndef TEST_UTILS_SIMPLE_H
#define TEST_UTILS_SIMPLE_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <random>
#include <iostream>
#include <ctime>
#include "../include/core/json.hpp"

// Simplified utils class for testing without OpenSSL dependency
class TestUtils {
public:
    // Safely parse JSON string
    static nlohmann::json safeParseJson(const std::string& jsonStr) {
        try {
            if (jsonStr.empty()) {
                return nlohmann::json();
            }
            return nlohmann::json::parse(jsonStr);
        } catch (const std::exception& e) {
            logError("JSON parsing error: " + std::string(e.what()));
            return nlohmann::json();
        }
    }
    
    // Log information message
    static inline void logInfo(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
    }
    
    // Log warning message
    static inline void logWarning(const std::string& message) {
        std::cout << "[WARNING] " << message << std::endl;
    }
    
    // Log error message
    static inline void logError(const std::string& message) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
    
    // Generate a random string
    static inline std::string randomString(size_t length) {
        static const char charset[] = 
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
            
        std::string result;
        result.reserve(length);
        
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, sizeof(charset) - 2);
        
        for (size_t i = 0; i < length; ++i) {
            result += charset[distribution(generator)];
        }
        
        return result;
    }
    
    // Simple hash function for testing (not cryptographically secure)
    static std::string calculateSimpleHash(const std::string& str) {
        std::hash<std::string> hasher;
        size_t hashValue = hasher(str);
        
        std::stringstream ss;
        ss << std::hex << hashValue;
        return ss.str();
    }
    
    // Convert Unix timestamp to human-readable format
    static std::string timestampToHumanReadable(time_t timestamp) {
        char buffer[26];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    
    // Create standard HTTP response
    static std::string createHttpResponse(int status_code, 
                                        const std::string& content_type, 
                                        const std::string& body) {
        std::string status_text;
        
        switch (status_code) {
            case 200: status_text = "OK"; break;
            case 201: status_text = "Created"; break;
            case 204: status_text = "No Content"; break;
            case 400: status_text = "Bad Request"; break;
            case 401: status_text = "Unauthorized"; break;
            case 403: status_text = "Forbidden"; break;
            case 404: status_text = "Not Found"; break;
            case 500: status_text = "Internal Server Error"; break;
            default: status_text = "Unknown"; break;
        }
        
        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        response << "Content-Type: " << content_type << "\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        
        return response.str();
    }
    
    // Create standard JSON HTTP response
    static std::string createJsonResponse(int status_code, const nlohmann::json& data) {
        return createHttpResponse(status_code, "application/json", data.dump(4));
    }
    
    // Create error JSON response
    static std::string createJsonErrorResponse(int status_code, const std::string& error_message) {
        nlohmann::json error;
        error["error"] = true;
        error["message"] = error_message;
        
        return createJsonResponse(status_code, error);
    }
};

#endif // TEST_UTILS_SIMPLE_H