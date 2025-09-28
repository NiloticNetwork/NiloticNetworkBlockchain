#include "utils.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <algorithm>
#include <cctype>

// Sign data using RSA private key
std::string Utils::signData(const std::string& data, const std::string& privateKeyPEM) {
    if (data.empty() || privateKeyPEM.empty()) {
        return "";
    }
    
    // Create BIO from private key PEM string
    BIO* bio = BIO_new_mem_buf(privateKeyPEM.c_str(), -1);
    if (!bio) {
        logError("Failed to create BIO for private key");
        return "";
    }
    
    // Read private key from BIO
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!pkey) {
        logError("Failed to read private key from PEM");
        return "";
    }
    
    // Create signing context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        logError("Failed to create signing context");
        return "";
    }
    
    // Initialize signing
    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        logError("Failed to initialize signing");
        return "";
    }
    
    // Update with data to sign
    if (EVP_DigestSignUpdate(ctx, data.c_str(), data.length()) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        logError("Failed to update signing context");
        return "";
    }
    
    // Determine signature length
    size_t sigLen = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sigLen) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        logError("Failed to determine signature length");
        return "";
    }
    
    // Create signature
    std::vector<unsigned char> signature(sigLen);
    if (EVP_DigestSignFinal(ctx, signature.data(), &sigLen) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        logError("Failed to create signature");
        return "";
    }
    
    // Clean up
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    // Convert to hex string
    std::stringstream ss;
    for (size_t i = 0; i < sigLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)signature[i];
    }
    
    return ss.str();
}

// Verify signature using RSA public key
bool Utils::verifySignature(const std::string& data, const std::string& signature, const std::string& publicKeyPEM) {
    if (data.empty() || signature.empty() || publicKeyPEM.empty()) {
        return false;
    }
    
    // Convert hex signature back to bytes
    if (signature.length() % 2 != 0) {
        logError("Invalid signature format");
        return false;
    }
    
    std::vector<unsigned char> sigBytes;
    for (size_t i = 0; i < signature.length(); i += 2) {
        std::string byteString = signature.substr(i, 2);
        try {
            unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
            sigBytes.push_back(byte);
        } catch (const std::exception& e) {
            logError("Failed to parse signature hex: " + std::string(e.what()));
            return false;
        }
    }
    
    // Create BIO from public key PEM string
    BIO* bio = BIO_new_mem_buf(publicKeyPEM.c_str(), -1);
    if (!bio) {
        logError("Failed to create BIO for public key");
        return false;
    }
    
    // Read public key from BIO
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!pkey) {
        logError("Failed to read public key from PEM");
        return false;
    }
    
    // Create verification context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        logError("Failed to create verification context");
        return false;
    }
    
    // Initialize verification
    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        logError("Failed to initialize verification");
        return false;
    }
    
    // Update with data to verify
    if (EVP_DigestVerifyUpdate(ctx, data.c_str(), data.length()) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        logError("Failed to update verification context");
        return false;
    }
    
    // Verify signature
    int result = EVP_DigestVerifyFinal(ctx, sigBytes.data(), sigBytes.size());
    
    // Clean up
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return result == 1;
}

// Generate RSA key pair and return private key PEM, set public key PEM in parameter
std::string Utils::generateKeyPair(std::string& publicKeyPEM) {
    // Generate RSA key pair
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        logError("Failed to create key generation context");
        return "";
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        logError("Failed to initialize key generation");
        return "";
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        logError("Failed to set RSA key size");
        return "";
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        logError("Failed to generate key pair");
        return "";
    }
    
    EVP_PKEY_CTX_free(ctx);
    
    // Extract private key as PEM
    BIO* privateBio = BIO_new(BIO_s_mem());
    if (!privateBio) {
        EVP_PKEY_free(pkey);
        logError("Failed to create private key BIO");
        return "";
    }
    
    if (PEM_write_bio_PrivateKey(privateBio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(privateBio);
        EVP_PKEY_free(pkey);
        logError("Failed to write private key to PEM");
        return "";
    }
    
    char* privateKeyData = nullptr;
    long privateKeyLen = BIO_get_mem_data(privateBio, &privateKeyData);
    std::string privateKeyPEM(privateKeyData, privateKeyLen);
    BIO_free(privateBio);
    
    // Extract public key as PEM
    BIO* publicBio = BIO_new(BIO_s_mem());
    if (!publicBio) {
        EVP_PKEY_free(pkey);
        logError("Failed to create public key BIO");
        return "";
    }
    
    if (PEM_write_bio_PUBKEY(publicBio, pkey) != 1) {
        BIO_free(publicBio);
        EVP_PKEY_free(pkey);
        logError("Failed to write public key to PEM");
        return "";
    }
    
    char* publicKeyData = nullptr;
    long publicKeyLen = BIO_get_mem_data(publicBio, &publicKeyData);
    publicKeyPEM = std::string(publicKeyData, publicKeyLen);
    BIO_free(publicBio);
    
    EVP_PKEY_free(pkey);
    
    return privateKeyPEM;
}

// Validate input to prevent injection attacks
bool Utils::validateInput(const std::string& input, size_t maxLength) {
    if (input.empty() || input.length() > maxLength) {
        return false;
    }
    
    // Check for null bytes
    if (input.find('\0') != std::string::npos) {
        return false;
    }
    
    // Check for control characters (except newline and tab)
    for (char c : input) {
        if (std::iscntrl(c) && c != '\n' && c != '\t') {
            return false;
        }
    }
    
    return true;
}

// Sanitize input by removing dangerous characters
std::string Utils::sanitizeInput(const std::string& input) {
    std::string sanitized;
    sanitized.reserve(input.length());
    
    for (char c : input) {
        // Allow alphanumeric, space, and common punctuation
        if (std::isalnum(c) || c == ' ' || c == '.' || c == '-' || c == '_' || 
            c == '@' || c == ':' || c == '/' || c == '?' || c == '=' || c == '&') {
            sanitized += c;
        }
    }
    
    return sanitized;
}