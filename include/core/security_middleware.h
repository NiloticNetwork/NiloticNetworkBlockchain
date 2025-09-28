#ifndef SECURITY_MIDDLEWARE_H
#define SECURITY_MIDDLEWARE_H

#include <string>
#include <map>
#include <vector>
#include <chrono>

class SecurityMiddleware {
private:
    bool httpsOnly;
    std::map<std::string, std::string> securityHeaders;
    std::vector<std::string> allowedOrigins;
    
public:
    SecurityMiddleware(bool enforceHttps = true);
    
    // Add security headers to HTTP response
    std::string addSecurityHeaders(const std::string& response);
    
    // Validate request security
    bool validateRequest(const std::string& request, const std::string& clientIP);
    
    // Check if HTTPS is required
    bool requiresHttps(const std::string& request);
    
    // Add CORS headers
    std::string addCorsHeaders(const std::string& response, const std::string& origin = "");
    
    // Validate Content-Type for POST requests
    bool validateContentType(const std::string& request);
    
    // Check for suspicious patterns
    bool detectSuspiciousActivity(const std::string& request, const std::string& clientIP);
    
    // Configure security settings
    void setHttpsOnly(bool enabled) { httpsOnly = enabled; }
    void addAllowedOrigin(const std::string& origin) { allowedOrigins.push_back(origin); }
    void setCustomHeader(const std::string& name, const std::string& value) { securityHeaders[name] = value; }
};

#endif // SECURITY_MIDDLEWARE_H