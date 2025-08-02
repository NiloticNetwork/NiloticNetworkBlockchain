#include "oderoslw.h"
#include "utils.h"
#include "json.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>

// Default constructor
OderoSLW::OderoSLW() 
    : tokenId(""), amount(0.0), creator(""), creationTime("") {
}

// Constructor with parameters
OderoSLW::OderoSLW(const std::string& tokenIdIn, double amountIn, const std::string& creatorIn)
    : tokenId(tokenIdIn), amount(amountIn), creator(creatorIn) {
    
    // Set creation time to current timestamp in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    creationTime = ss.str();
}

// Generate a QR code representation as a string
// In a real implementation, this would generate an actual QR code
// For now, we just return a string representation of what would be encoded
std::string OderoSLW::generateQrCode() const {
    // Create a string that contains the token data in a format that could be encoded in a QR code
    std::stringstream qr_data;
    qr_data << "ODEROSLW:" << tokenId << ":" << amount << ":" << creator << ":" << creationTime;
    
    // In a real implementation, we would call a QR code generation library here
    // For our prototype, we'll just return the string that would be encoded
    return "QR Code data: " + qr_data.str();
}

// Verify the token validity
bool OderoSLW::verify() const {
    // Ensure the tokenId doesn't have duplications
    std::string cleanTokenId = tokenId;
    // Check if we have a duplicated tokenId pattern (sometimes happens in our API responses)
    size_t duplicatePos = cleanTokenId.find("OSLW", 4);
    if (duplicatePos != std::string::npos) {
        cleanTokenId = cleanTokenId.substr(0, duplicatePos);
    }
    
    // Basic validation
    if (cleanTokenId.empty() || creator.empty() || creationTime.empty() || amount <= 0) {
        return false;
    }
    
    // In a real implementation, we would verify the token's cryptographic properties
    // For our prototype, we'll just do some basic validation
    
    // Check if the token ID follows our expected format (for example, starts with "OSLW")
    if (cleanTokenId.substr(0, 4) != "OSLW") {
        return false;
    }
    
    // Check if the creation time is in the past
    // For our prototype, we'll assume any properly formatted date is valid
    
    return true;
}

// Get token metadata as JSON
std::string OderoSLW::getMetadata() const {
    nlohmann::json metadata;
    metadata["tokenType"] = "OderoSLW";
    metadata["version"] = "1.0";
    metadata["tokenId"] = tokenId;
    metadata["creator"] = creator;
    metadata["creationTime"] = creationTime;
    metadata["amount"] = amount;
    
    return metadata.dump(4);
}

// Export token to JSON
std::string OderoSLW::toJson() const {
    nlohmann::json j;
    j["tokenId"] = tokenId;
    j["amount"] = amount;
    j["creator"] = creator;
    j["creationTime"] = creationTime;
    
    return j.dump(4);
}

// Import token from JSON
OderoSLW OderoSLW::fromJson(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    
    OderoSLW token;
    token.tokenId = j["tokenId"].get<std::string>();
    token.amount = j["amount"].get<double>();
    token.creator = j["creator"].get<std::string>();
    token.creationTime = j["creationTime"].get<std::string>();
    
    return token;
}