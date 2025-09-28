#include "security_middleware.h"
#include "utils.h"
#include <algorithm>
#include <sstream>

SecurityMiddleware::SecurityMiddleware(bool enforceHttps) : httpsOnly(enforceHttps) {
    // Set default security headers
    securityHeaders["X-Content-Type-Options"] = "nosniff";
    securityHeaders["X-Frame-Options"] = "DENY";
    securityHeaders["X-XSS-Protection"] = "1; mode=block";
    securityHeaders["Referrer-Policy"] = "strict-origin-when-cross-origin";
    securityHeaders["Content-Security-Policy"] = "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'";
    
    if (httpsOnly) {
        securityHeaders["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains; preload";
    }
    
    // Default allowed origins
    allowedOrigins.push_back("http://localhost:3000");
    allowedOrigins.push_back("http://localhost:5000");
    allowedOrigins.push_back("http://127.0.0.1:3000");
    allowedOrigins.push_back("http://127.0.0.1:5000");
}

std::string SecurityMiddleware::addSecurityHeaders(const std::string& response) {
    std::string secureResponse = response;
    
    // Find the end of the status line
    size_t headerStart = response.find("\r\n");
    if (headerStart == std::string::npos) {
        return response; // Invalid response format
    }
    
    // Find the end of headers
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return response; // Invalid response format
    }
    
    // Insert security headers before the end of headers
    std::stringstream additionalHeaders;
    for (const auto& header : securityHeaders) {
        additionalHeaders << header.first << ": " << header.second << "\r\n";
    }
    
    secureResponse.insert(headerEnd, additionalHeaders.str());
    
    return secureResponse;
}

bool SecurityMiddleware::validateRequest(const std::string& request, const std::string& clientIP) {
    // Check for HTTPS requirement
    if (httpsOnly && !requiresHttps(request)) {
        Utils::logWarning("HTTP request rejected, HTTPS required from " + clientIP);
        return false;
    }
    
    // Validate Content-Type for POST requests
    if (!validateContentType(request)) {
        Utils::logWarning("Invalid Content-Type from " + clientIP);
        return false;
    }
    
    // Check for suspicious activity
    if (detectSuspiciousActivity(request, clientIP)) {
        Utils::logWarning("Suspicious activity detected from " + clientIP);
        return false;
    }
    
    return true;
}

bool SecurityMiddleware::requiresHttps(const std::string& request) {
    // Check if request is HTTPS
    return request.find("HTTP/1.1") != std::string::npos || 
           request.find("HTTP/2") != std::string::npos;
}

std::string SecurityMiddleware::addCorsHeaders(const std::string& response, const std::string& origin) {
    std::string corsResponse = response;
    
    // Check if origin is allowed
    bool originAllowed = false;
    if (origin.empty()) {
        originAllowed = true; // Allow if no origin specified
    } else {
        for (const auto& allowedOrigin : allowedOrigins) {
            if (origin == allowedOrigin) {
                originAllowed = true;
                break;
            }
        }
    }
    
    if (!originAllowed) {
        Utils::logWarning("CORS: Origin not allowed: " + origin);
        return corsResponse;
    }
    
    // Find the end of headers
    size_t headerEnd = corsResponse.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return corsResponse;
    }
    
    // Add CORS headers
    std::stringstream corsHeaders;
    corsHeaders << "Access-Control-Allow-Origin: " << (origin.empty() ? "*" : origin) << "\r\n";
    corsHeaders << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    corsHeaders << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n";
    corsHeaders << "Access-Control-Max-Age: 86400\r\n";
    
    corsResponse.insert(headerEnd, corsHeaders.str());
    
    return corsResponse;
}

bool SecurityMiddleware::validateContentType(const std::string& request) {
    // Check if it's a POST request
    if (request.find("POST ") != 0) {
        return true; // Not a POST request, no Content-Type validation needed
    }
    
    // Check for Content-Type header
    if (request.find("Content-Type:") == std::string::npos) {
        return false; // POST request without Content-Type
    }
    
    // Check for valid Content-Type values
    std::vector<std::string> validContentTypes = {
        "application/json",
        "application/x-www-form-urlencoded",
        "multipart/form-data"
    };
    
    for (const auto& contentType : validContentTypes) {
        if (request.find("Content-Type: " + contentType) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool SecurityMiddleware::detectSuspiciousActivity(const std::string& request, const std::string& clientIP) {
    // Check for common attack patterns
    std::vector<std::string> suspiciousPatterns = {
        // SQL Injection
        "' OR '1'='1", "UNION SELECT", "DROP TABLE", "INSERT INTO",
        "DELETE FROM", "UPDATE SET", "'; --", "' UNION",
        
        // XSS
        "<script>", "</script>", "javascript:", "onload=", "onerror=",
        "alert(", "document.cookie", "window.location",
        
        // Path Traversal
        "../", "..\\", "..%2f", "..%5c", "%2e%2e%2f", "%2e%2e%5c",
        
        // Command Injection
        "; cat ", "; ls ", "; rm ", "; wget ", "; curl ",
        "| cat ", "| ls ", "| rm ", "| wget ", "| curl ",
        
        // LDAP Injection
        ")(cn=*", ")(objectClass=*", "*)(uid=*", "*)(mail=*",
        
        // XXE
        "<!ENTITY", "<!DOCTYPE", "SYSTEM \"file://", "SYSTEM \"http://",
        
        // SSRF
        "file://", "gopher://", "dict://", "ftp://localhost",
        "http://127.0.0.1", "http://localhost", "http://0.0.0.0"
    };
    
    std::string lowerRequest = request;
    std::transform(lowerRequest.begin(), lowerRequest.end(), lowerRequest.begin(), ::tolower);
    
    for (const auto& pattern : suspiciousPatterns) {
        std::string lowerPattern = pattern;
        std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
        
        if (lowerRequest.find(lowerPattern) != std::string::npos) {
            Utils::logError("Suspicious pattern detected: " + pattern + " from " + clientIP);
            return true;
        }
    }
    
    // Check for excessively long requests (potential buffer overflow)
    if (request.length() > 100000) { // 100KB limit
        Utils::logError("Excessively long request from " + clientIP);
        return true;
    }
    
    // Check for too many parameters (potential DoS)
    size_t paramCount = 0;
    size_t pos = 0;
    while ((pos = request.find("&", pos)) != std::string::npos) {
        paramCount++;
        pos++;
    }
    
    if (paramCount > 100) {
        Utils::logError("Too many parameters in request from " + clientIP);
        return true;
    }
    
    return false;
}