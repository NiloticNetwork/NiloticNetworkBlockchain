#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include "json.hpp"

class Wallet {
private:
    std::string address;
    EVP_PKEY* privateKey;
    EVP_PKEY* publicKey;
    std::string privateKeyPEM;
    std::string publicKeyPEM;
    
    // Wallet metadata
    std::string name;
    std::string description;
    time_t createdAt;
    time_t lastUsed;
    
    // Transaction history
    std::vector<std::string> transactionHistory;
    
    // Security features
    bool isEncrypted;
    std::string passwordHash;
    int unlockAttempts;
    time_t lockUntil;
    
    // Generate a new key pair
    bool generateKeyPair();
    
    // Generate address from public key
    std::string generateAddress(const std::string& publicKeyStr);
    
    // Hash password for encryption
    std::string hashPassword(const std::string& password);
    
    // Validate password
    bool validatePassword(const std::string& password);

public:
    // Constructor
    Wallet();
    Wallet(const std::string& walletName);
    Wallet(const std::string& privateKeyPEM, const std::string& password);
    
    // Destructor
    ~Wallet();
    
    // Copy constructor and assignment operator
    Wallet(const Wallet& other);
    Wallet& operator=(const Wallet& other);
    
    // Wallet management
    bool createNewWallet(const std::string& password);
    bool importWallet(const std::string& privateKeyPEM, const std::string& password);
    bool exportWallet(std::string& privateKeyPEM, const std::string& password);
    bool deleteWallet(const std::string& password);
    
    // Key management
    std::string getAddress() const;
    std::string getPublicKey() const;
    bool hasPrivateKey() const;
    
    // Transaction signing
    std::string signTransaction(const std::string& transactionData);
    bool verifySignature(const std::string& data, const std::string& signature, const std::string& publicKey);
    
    // Security features
    bool encryptWallet(const std::string& password);
    bool decryptWallet(const std::string& password);
    bool isLocked() const;
    bool unlock(const std::string& password);
    void lock();
    void setLockTimeout(int minutes);
    
    // Wallet metadata
    void setName(const std::string& name);
    void setDescription(const std::string& description);
    std::string getName() const;
    std::string getDescription() const;
    time_t getCreatedAt() const;
    time_t getLastUsed() const;
    
    // Transaction history
    void addTransaction(const std::string& txHash);
    std::vector<std::string> getTransactionHistory() const;
    void clearTransactionHistory();
    
    // Backup and restore
    nlohmann::json toJson() const;
    static Wallet fromJson(const nlohmann::json& json);
    bool saveToFile(const std::string& filename, const std::string& password);
    static Wallet loadFromFile(const std::string& filename, const std::string& password);
    
    // Utility functions
    static std::string generateMnemonic(int wordCount = 12);
    static Wallet fromMnemonic(const std::string& mnemonic, const std::string& password);
    std::string toMnemonic(const std::string& password);
    
    // Validation
    bool isValid() const;
    std::string getValidationErrors() const;
    
    // Advanced features
    bool addMultiSigKey(const std::string& publicKey);
    bool removeMultiSigKey(const std::string& publicKey);
    std::vector<std::string> getMultiSigKeys() const;
    bool isMultiSig() const;
    
    // Hardware wallet support (placeholder)
    bool connectHardwareWallet();
    bool disconnectHardwareWallet();
    bool isHardwareWalletConnected() const;
};

// Wallet manager for handling multiple wallets
class WalletManager {
private:
    std::map<std::string, Wallet> wallets;
    std::string defaultWallet;
    std::string walletDirectory;
    
public:
    WalletManager(const std::string& directory = "./wallets");
    
    // Wallet management
    bool createWallet(const std::string& name, const std::string& password);
    bool importWallet(const std::string& name, const std::string& privateKeyPEM, const std::string& password);
    bool deleteWallet(const std::string& name, const std::string& password);
    bool exportWallet(const std::string& name, std::string& privateKeyPEM, const std::string& password);
    
    // Wallet access
    Wallet* getWallet(const std::string& name);
    Wallet* getDefaultWallet();
    void setDefaultWallet(const std::string& name);
    
    // List wallets
    std::vector<std::string> listWallets() const;
    bool walletExists(const std::string& name) const;
    
    // Persistence
    bool saveAllWallets();
    bool loadAllWallets();
    bool backupWallets(const std::string& backupPath);
    bool restoreWallets(const std::string& backupPath);
    
    // Security
    bool lockAllWallets();
    bool unlockWallet(const std::string& name, const std::string& password);
    bool isWalletLocked(const std::string& name) const;
};

#endif // WALLET_H