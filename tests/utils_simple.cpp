#include "utils_simple.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <random>

// Simple implementations for testing (without OpenSSL dependency)

std::string Utils::signData(const std::string& data, const std::string& privateKeyPEM) {
    // Simple mock implementation for testing
    return "MOCK_SIGNATURE_" + std::to_string(std::hash<std::string>{}(data + privateKeyPEM));
}

bool Utils::verifySignature(const std::string& data, const std::string& signature, const std::string& publicKeyPEM) {
    // Simple mock implementation for testing
    std::string expectedSignature = "MOCK_SIGNATURE_" + std::to_string(std::hash<std::string>{}(data + publicKeyPEM));
    return signature == expectedSignature;
}

std::string Utils::generateKeyPair() {
    // Simple mock implementation for testing
    return "MOCK_KEY_PAIR_" + std::to_string(std::random_device{}());
}

std::string Utils::sha256(const std::string& data) {
    // Simple hash implementation for testing (not cryptographically secure)
    std::hash<std::string> hasher;
    size_t hashValue = hasher(data);
    
    std::stringstream ss;
    ss << std::hex << hashValue;
    return ss.str();
}

std::string Utils::generateRandomString(size_t length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    
    return result;
}

std::string Utils::toHex(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::vector<uint8_t> Utils::fromHex(const std::string& hex) {
    std::vector<uint8_t> result;
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        result.push_back(byte);
    }
    
    return result;
}

std::string Utils::trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }
    
    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::string(start, end + 1);
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

bool Utils::isValidAddress(const std::string& address) {
    // Simple validation for testing
    return !address.empty() && address.length() >= 10 && address.length() <= 100;
}

uint64_t Utils::getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void Utils::logError(const std::string& message) {
    std::cerr << "ERROR: " << message << std::endl;
}

void Utils::logInfo(const std::string& message) {
    std::cout << "INFO: " << message << std::endl;
}

void Utils::logDebug(const std::string& message) {
    std::cout << "DEBUG: " << message << std::endl;
}