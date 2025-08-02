#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <random>
#include <iostream>
#include <ctime>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include "json.hpp"
#include "transaction_types.h"

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
    
    // Calculate SHA-256 hash of a string using the modern EVP interface
    static std::string calculateSHA256(const std::string& str) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        
        // Create and initialize digest context
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        if (mdctx == nullptr) {
            return "";
        }
        
        // Initialize digest context for SHA256
        if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
        
        // Add data to be hashed
        if (EVP_DigestUpdate(mdctx, str.c_str(), str.size()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
        
        // Finalize digest and get the result
        unsigned int len = 0;
        if (EVP_DigestFinal_ex(mdctx, hash, &len) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
        
        // Free resources
        EVP_MD_CTX_free(mdctx);
        
        // Convert hash to hex string
        std::stringstream ss;
        for(unsigned int i = 0; i < len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        
        return ss.str();
    }
    
    // Convert transaction type enum to string
    static std::string transactionTypeToString(TransactionType type) {
        switch (type) {
            case TransactionType::REGULAR: return "REGULAR";
            case TransactionType::STAKE: return "STAKE";
            case TransactionType::UNSTAKE: return "UNSTAKE";
            case TransactionType::CREATE_ODERO: return "CREATE_ODERO";
            case TransactionType::REDEEM_ODERO: return "REDEEM_ODERO";
            case TransactionType::CONTRACT: return "CONTRACT";
            default: return "UNKNOWN";
        }
    }
    
    // Convert string to transaction type enum
    static TransactionType stringToTransactionType(const std::string& typeStr) {
        if (typeStr == "REGULAR") return TransactionType::REGULAR;
        if (typeStr == "STAKE") return TransactionType::STAKE;
        if (typeStr == "UNSTAKE") return TransactionType::UNSTAKE;
        if (typeStr == "CREATE_ODERO") return TransactionType::CREATE_ODERO;
        if (typeStr == "REDEEM_ODERO") return TransactionType::REDEEM_ODERO;
        if (typeStr == "CONTRACT") return TransactionType::CONTRACT;
        
        // Default
        return TransactionType::REGULAR;
    }
    
    // Convert network type enum to string
    static std::string networkTypeToString(NetworkType network) {
        switch (network) {
            case NetworkType::LIVEWIRE: return "LIVEWIRE";
            case NetworkType::TESTWIRE: return "TESTWIRE";
            case NetworkType::PIPE: return "PIPE";
            default: return "UNKNOWN";
        }
    }
    
    // Convert string to network type enum
    static NetworkType stringToNetworkType(const std::string& networkStr) {
        if (networkStr == "LIVEWIRE") return NetworkType::LIVEWIRE;
        if (networkStr == "TESTWIRE") return NetworkType::TESTWIRE;
        if (networkStr == "PIPE") return NetworkType::PIPE;
        
        // Default
        return NetworkType::PIPE;
    }
    
    // Convert Unix timestamp to human-readable format
    static std::string timestampToHumanReadable(time_t timestamp) {
        char buffer[26];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    
    // Parse query parameters from a URL
    static std::map<std::string, std::string> parseQueryParams(const std::string& query) {
        std::map<std::string, std::string> params;
        
        if (query.empty()) {
            return params;
        }
        
        std::string::size_type start = 0;
        std::string::size_type end;
        
        while ((end = query.find('&', start)) != std::string::npos) {
            std::string param = query.substr(start, end - start);
            std::string::size_type eq = param.find('=');
            
            if (eq != std::string::npos) {
                params[param.substr(0, eq)] = param.substr(eq + 1);
            } else {
                params[param] = "";
            }
            
            start = end + 1;
        }
        
        std::string param = query.substr(start);
        std::string::size_type eq = param.find('=');
        
        if (eq != std::string::npos) {
            params[param.substr(0, eq)] = param.substr(eq + 1);
        } else {
            params[param] = "";
        }
        
        return params;
    }
    
    // Parse HTTP request to extract method, URI, and headers
    static void parseHttpRequest(const std::string& request, 
                               std::string& method, 
                               std::string& uri, 
                               std::map<std::string, std::string>& headers,
                               std::string& body) {
        std::istringstream iss(request);
        std::string line;
        
        // Parse request line
        std::getline(iss, line);
        std::istringstream request_line(line);
        request_line >> method >> uri;
        
        // Parse headers
        headers.clear();
        bool headers_done = false;
        while (std::getline(iss, line)) {
            // Check for \r at the end of line and remove it
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            // An empty line marks the end of headers and start of body
            if (line.empty()) {
                headers_done = true;
                break;
            }
            
            auto colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string header_name = line.substr(0, colon_pos);
                std::string header_value = line.substr(colon_pos + 1);
                
                // Trim leading whitespace from header value
                header_value.erase(0, header_value.find_first_not_of(" \t"));
                
                headers[header_name] = header_value;
            }
        }
        
        // Parse body (if any)
        body.clear();
        if (headers_done) {
            // Read the rest of the input as the body
            std::string body_line;
            while (std::getline(iss, body_line)) {
                if (body.empty()) {
                    body = body_line;
                } else {
                    body += "\n" + body_line;
                }
            }
            
            // If we have Content-Length, use it to extract the exact body
            if (headers.find("Content-Length") != headers.end()) {
                // Content-Length was found, ensure we read exactly that many bytes
                size_t content_length = std::stoul(headers["Content-Length"]);
                
                // If we need more data, read it directly from the original request
                size_t header_end = request.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    header_end += 4; // Move past the \r\n\r\n
                    if (header_end + content_length <= request.size()) {
                        body = request.substr(header_end, content_length);
                    }
                }
            }
        }
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

#endif // UTILS_H