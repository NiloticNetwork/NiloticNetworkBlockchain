#ifndef ODEROSLW_H
#define ODEROSLW_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>

// Token states for lifecycle management
enum class TokenState {
    CREATED,        // Token created but not activated
    ACTIVE,         // Token is active and can be used
    REDEEMED,       // Token has been redeemed
    EXPIRED,        // Token has expired
    REVOKED,        // Token has been revoked
    FROZEN          // Token is temporarily frozen
};

// Security levels for different token types
enum class SecurityLevel {
    BASIC,          // Basic validation
    ENHANCED,       // Enhanced cryptographic validation
    ENTERPRISE      // Enterprise-grade security with multi-sig
};

// Token transaction record for audit trail
struct TokenTransaction {
    std::string transactionId;
    std::string action;         // CREATE, REDEEM, TRANSFER, FREEZE, etc.
    std::string actor;          // Who performed the action
    std::string timestamp;
    std::string details;
    std::string signature;
};

class OderoSLW {
private:
    std::string tokenId;                    // Unique token ID
    double amount;                          // Token amount
    std::string creator;                    // Creator's address
    std::string creationTime;               // Creation timestamp
    std::string expirationTime;             // Token expiration time
    TokenState state;                       // Current token state
    SecurityLevel securityLevel;            // Security level
    std::string digitalSignature;           // Cryptographic signature
    std::string publicKey;                  // Public key for verification
    std::string merkleRoot;                 // Merkle root for batch verification
    std::vector<TokenTransaction> auditTrail; // Transaction history
    std::map<std::string, std::string> metadata; // Additional metadata
    
    // Security features
    std::string nonce;                      // Cryptographic nonce
    std::string salt;                       // Salt for hash generation
    int validationAttempts;                 // Track validation attempts
    std::string lastValidationTime;         // Last validation timestamp
    bool isMultiSig;                        // Multi-signature requirement
    std::vector<std::string> requiredSigners; // Required signers for multi-sig
    std::vector<std::string> signatures;    // Collected signatures
    
    // Anti-fraud features
    std::string deviceFingerprint;          // Device identification
    std::string locationHash;               // Location verification
    double riskScore;                       // Fraud risk assessment
    
    // Generate cryptographic nonce
    std::string generateNonce() const;
    
    // Generate salt for hashing
    std::string generateSalt() const;
    
    // Calculate token hash with all security parameters
    std::string calculateSecureHash() const;
    
    // Validate token structure and format
    bool validateTokenStructure() const;
    
    // Validate cryptographic components
    bool validateCryptography() const;
    
    // Check token expiration
    bool isExpired() const;
    
    // Calculate fraud risk score
    double calculateRiskScore() const;

public:
    // Default constructor
    OderoSLW();
    
    // Constructor with parameters
    OderoSLW(const std::string& tokenId, double amount, const std::string& creator);
    
    // Enhanced constructor with security level
    OderoSLW(const std::string& tokenId, double amount, const std::string& creator, 
             SecurityLevel level, const std::string& publicKey = "");
    
    // Generate a QR code for the token
    std::string generateQrCode() const;
    
    // Enhanced QR code with security features
    std::string generateSecureQrCode() const;
    
    // Verify the token validity with enhanced security
    bool verify() const;
    
    // Verify token with specific public key
    bool verifyWithKey(const std::string& publicKey) const;
    
    // Verify token batch using merkle tree
    static bool verifyTokenBatch(const std::vector<OderoSLW>& tokens, const std::string& merkleRoot);
    
    // Sign the token with private key
    bool signToken(const std::string& privateKey);
    
    // Add signature for multi-sig tokens
    bool addSignature(const std::string& signerAddress, const std::string& signature);
    
    // Check if multi-sig requirements are met
    bool isMultiSigComplete() const;
    
    // Activate the token (change state from CREATED to ACTIVE)
    bool activate(const std::string& activatorKey);
    
    // Redeem the token
    bool redeem(const std::string& redeemer, const std::string& signature);
    
    // Transfer token ownership
    bool transfer(const std::string& newOwner, const std::string& signature);
    
    // Freeze/unfreeze token
    bool freeze(const std::string& authority, const std::string& reason);
    bool unfreeze(const std::string& authority, const std::string& reason);
    
    // Revoke token
    bool revoke(const std::string& authority, const std::string& reason);
    
    // Set expiration time
    void setExpirationTime(const std::string& expiration);
    void setExpirationDuration(int hours);
    
    // Add metadata
    void addMetadata(const std::string& key, const std::string& value);
    std::string getMetadata(const std::string& key) const;
    
    // Security features
    void setDeviceFingerprint(const std::string& fingerprint);
    void setLocationHash(const std::string& location);
    double getRiskScore() const;
    
    // Audit trail management
    void addAuditEntry(const std::string& action, const std::string& actor, const std::string& details);
    std::vector<TokenTransaction> getAuditTrail() const;
    
    // Get token metadata as JSON
    std::string getMetadata() const;
    
    // Get enhanced metadata with security info
    std::string getEnhancedMetadata() const;
    
    // Export token to JSON
    std::string toJson() const;
    
    // Export with security details
    std::string toSecureJson() const;
    
    // Import token from JSON
    static OderoSLW fromJson(const std::string& json);
    
    // Batch operations
    static std::vector<OderoSLW> createTokenBatch(const std::vector<std::pair<std::string, double>>& tokenData, 
                                                  const std::string& creator, SecurityLevel level);
    static std::string generateBatchMerkleRoot(const std::vector<OderoSLW>& tokens);
    
    // Validation utilities
    static bool validateTokenId(const std::string& tokenId);
    static bool validateAmount(double amount);
    static bool validateAddress(const std::string& address);
    
    // Getters and setters
    const std::string& getTokenId() const { return tokenId; }
    void setTokenId(const std::string& id) { tokenId = id; }
    
    double getAmount() const { return amount; }
    void setAmount(double amt) { amount = amt; }
    
    const std::string& getCreator() const { return creator; }
    void setCreator(const std::string& c) { creator = c; }
    
    const std::string& getCreationTime() const { return creationTime; }
    void setCreationTime(const std::string& time) { creationTime = time; }
    
    const std::string& getExpirationTime() const { return expirationTime; }
    
    TokenState getState() const { return state; }
    void setState(TokenState newState) { state = newState; }
    
    SecurityLevel getSecurityLevel() const { return securityLevel; }
    void setSecurityLevel(SecurityLevel level) { securityLevel = level; }
    
    const std::string& getDigitalSignature() const { return digitalSignature; }
    const std::string& getPublicKey() const { return publicKey; }
    
    const std::string& getNonce() const { return nonce; }
    const std::string& getSalt() const { return salt; }
    
    int getValidationAttempts() const { return validationAttempts; }
    const std::string& getLastValidationTime() const { return lastValidationTime; }
    
    // State check utilities
    bool isActive() const { return state == TokenState::ACTIVE; }
    bool isRedeemed() const { return state == TokenState::REDEEMED; }
    bool isFrozen() const { return state == TokenState::FROZEN; }
    bool isRevoked() const { return state == TokenState::REVOKED; }
    
    // Convert state to string
    std::string stateToString() const;
    static TokenState stringToState(const std::string& stateStr);
    
    // Convert security level to string
    std::string securityLevelToString() const;
    static SecurityLevel stringToSecurityLevel(const std::string& levelStr);
};

#endif // ODEROSLW_H