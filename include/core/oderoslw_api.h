#ifndef ODEROSLW_API_H
#define ODEROSLW_API_H

#include <string>
#include <map>
#include "json.hpp"
#include "oderoslw_manager.h"

// Forward declarations
class Blockchain;

// API response structure
struct ApiResponse {
    int statusCode;
    std::string status;
    nlohmann::json data;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class OderoSLWAPI {
private:
    OderoSLWManager* tokenManager;
    Blockchain* blockchain;
    
    // Rate limiting
    std::map<std::string, std::pair<int, time_t>> rateLimits; // IP -> (count, last_reset)
    int maxRequestsPerMinute;
    
    // Security
    std::string apiKey;
    bool requireAuthentication;
    
    // Helper methods
    bool validateApiKey(const std::string& providedKey);
    bool checkRateLimit(const std::string& clientIP);
    ApiResponse createErrorResponse(int code, const std::string& message);
    ApiResponse createSuccessResponse(const nlohmann::json& data);
    bool validateRequestData(const nlohmann::json& data, const std::vector<std::string>& requiredFields);

public:
    // Constructor
    OderoSLWAPI(OderoSLWManager* manager, Blockchain* blockchainPtr = nullptr);
    
    // Configuration
    void setApiKey(const std::string& key) { apiKey = key; }
    void setRequireAuthentication(bool require) { requireAuthentication = require; }
    void setRateLimit(int requestsPerMinute) { maxRequestsPerMinute = requestsPerMinute; }
    
    // Token creation endpoints
    ApiResponse createToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse createTokenBatch(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse createEnterpriseToken(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Token management endpoints
    ApiResponse activateToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse redeemToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse transferToken(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Token state management
    ApiResponse freezeToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse unfreezeToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse revokeToken(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Token validation and verification
    ApiResponse validateToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse validateTokenWithContext(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse verifyTokenBatch(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Token queries
    ApiResponse getToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getUserTokens(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getTokensByState(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse searchTokens(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Security endpoints
    ApiResponse blacklistToken(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse blacklistAddress(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getSecurityReport(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Statistics and monitoring
    ApiResponse getStatistics(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getDetailedStatistics(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getHealthStatus(const nlohmann::json& request, const std::string& clientIP = "");
    
    // QR Code generation
    ApiResponse generateQRCode(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse generateSecureQRCode(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Audit and compliance
    ApiResponse getAuditTrail(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse generateComplianceReport(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Blockchain integration
    ApiResponse integrateWithBlockchain(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getBlockchainStatus(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Utility endpoints
    ApiResponse validateTokenId(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse validateAddress(const nlohmann::json& request, const std::string& clientIP = "");
    ApiResponse getApiInfo(const nlohmann::json& request, const std::string& clientIP = "");
    
    // Process HTTP request
    std::string processRequest(const std::string& method, const std::string& path, 
                              const std::string& body, const std::string& clientIP = "",
                              const std::map<std::string, std::string>& headers = {});
};

#endif // ODEROSLW_API_H