#ifndef ODEROSLW_H
#define ODEROSLW_H

#include <string>

class OderoSLW {
private:
    std::string tokenId;        // Unique token ID
    double amount;              // Token amount
    std::string creator;        // Creator's address
    std::string creationTime;   // Creation timestamp

public:
    // Default constructor
    OderoSLW();
    
    // Constructor with parameters
    OderoSLW(const std::string& tokenId, double amount, const std::string& creator);
    
    // Generate a QR code for the token
    std::string generateQrCode() const;
    
    // Verify the token validity
    bool verify() const;
    
    // Get token metadata as JSON
    std::string getMetadata() const;
    
    // Export token to JSON
    std::string toJson() const;
    
    // Import token from JSON
    static OderoSLW fromJson(const std::string& json);
    
    // Getters and setters
    const std::string& getTokenId() const { return tokenId; }
    void setTokenId(const std::string& id) { tokenId = id; }
    
    double getAmount() const { return amount; }
    void setAmount(double amt) { amount = amt; }
    
    const std::string& getCreator() const { return creator; }
    void setCreator(const std::string& c) { creator = c; }
    
    const std::string& getCreationTime() const { return creationTime; }
    void setCreationTime(const std::string& time) { creationTime = time; }
};

#endif // ODEROSLW_H