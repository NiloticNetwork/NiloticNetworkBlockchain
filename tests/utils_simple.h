#ifndef UTILS_SIMPLE_H
#define UTILS_SIMPLE_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <random>
#include <iostream>
#include <ctime>
#include <chrono>
#include "../lib/nlohmann_json/single_include/nlohmann/json.hpp"

class Utils {
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

    // Cryptographic functions (simplified for testing)
    static std::string signData(const std::string& data, const std::string& privateKeyPEM);
    static bool verifySignature(const std::string& data, const std::string& signature, const std::string& publicKeyPEM);
    static std::string generateKeyPair();
    static std::string sha256(const std::string& data);
    
    // Utility functions
    static std::string generateRandomString(size_t length);
    static std::string toHex(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> fromHex(const std::string& hex);
    static std::string trim(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static bool isValidAddress(const std::string& address);
    static uint64_t getCurrentTimestamp();
    
    // Logging functions
    static void logError(const std::string& message);
    static void logInfo(const std::string& message);
    static void logDebug(const std::string& message);
};

#endif // UTILS_SIMPLE_H