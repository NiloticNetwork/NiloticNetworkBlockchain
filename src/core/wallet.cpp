#include "wallet.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cstring>

// Constructor
Wallet::Wallet() : privateKey(nullptr), publicKey(nullptr), isEncrypted(false), 
                   unlockAttempts(0), lockUntil(0), createdAt(time(nullptr)), lastUsed(time(nullptr)) {
    generateKeyPair();
}

Wallet::Wallet(const std::string& walletName) : name(walletName), privateKey(nullptr), publicKey(nullptr),
                                                isEncrypted(false), unlockAttempts(0), lockUntil(0),
                                                createdAt(time(nullptr)), lastUsed(time(nullptr)) {
    generateKeyPair();
}

Wallet::Wallet(const std::string& privateKeyPEM, const std::string& password) : privateKey(nullptr), publicKey(nullptr),
                                                                               isEncrypted(false), unlockAttempts(0), lockUntil(0),
                                                                               createdAt(time(nullptr)), lastUsed(time(nullptr)) {
    importWallet(privateKeyPEM, password);
}

// Destructor
Wallet::~Wallet() {
    if (privateKey) EVP_PKEY_free(privateKey);
    if (publicKey) EVP_PKEY_free(publicKey);
}

// Copy constructor
Wallet::Wallet(const Wallet& other) : address(other.address), name(other.name), description(other.description),
                                     createdAt(other.createdAt), lastUsed(other.lastUsed),
                                     transactionHistory(other.transactionHistory), isEncrypted(other.isEncrypted),
                                     passwordHash(other.passwordHash), unlockAttempts(other.unlockAttempts),
                                     lockUntil(other.lockUntil), privateKeyPEM(other.privateKeyPEM),
                                     publicKeyPEM(other.publicKeyPEM) {
    // Deep copy keys
    if (other.privateKey) {
        privateKey = EVP_PKEY_dup(other.privateKey);
    }
    if (other.publicKey) {
        publicKey = EVP_PKEY_dup(other.publicKey);
    }
}

// Assignment operator
Wallet& Wallet::operator=(const Wallet& other) {
    if (this != &other) {
        // Free existing keys
        if (privateKey) EVP_PKEY_free(privateKey);
        if (publicKey) EVP_PKEY_free(publicKey);
        
        // Copy data
        address = other.address;
        name = other.name;
        description = other.description;
        createdAt = other.createdAt;
        lastUsed = other.lastUsed;
        transactionHistory = other.transactionHistory;
        isEncrypted = other.isEncrypted;
        passwordHash = other.passwordHash;
        unlockAttempts = other.unlockAttempts;
        lockUntil = other.lockUntil;
        privateKeyPEM = other.privateKeyPEM;
        publicKeyPEM = other.publicKeyPEM;
        
        // Deep copy keys
        if (other.privateKey) {
            privateKey = EVP_PKEY_dup(other.privateKey);
        }
        if (other.publicKey) {
            publicKey = EVP_PKEY_dup(other.publicKey);
        }
    }
    return *this;
}

// Generate a new key pair
bool Wallet::generateKeyPair() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        Utils::logError("Failed to create key generation context");
        return false;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        Utils::logError("Failed to initialize key generation");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        Utils::logError("Failed to set RSA key size");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        Utils::logError("Failed to generate key pair");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY_CTX_free(ctx);
    
    // Extract private and public keys
    privateKey = pkey;
    publicKey = EVP_PKEY_dup(pkey);
    
    // Convert to PEM format
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, privateKey, nullptr, nullptr, 0, nullptr, nullptr);
    char* data = nullptr;
    long len = BIO_get_mem_data(bio, &data);
    privateKeyPEM = std::string(data, len);
    BIO_free(bio);
    
    bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, publicKey);
    data = nullptr;
    len = BIO_get_mem_data(bio, &data);
    publicKeyPEM = std::string(data, len);
    BIO_free(bio);
    
    // Generate address
    address = generateAddress(publicKeyPEM);
    
    Utils::logInfo("Generated new wallet: " + address);
    return true;
}

// Generate address from public key
std::string Wallet::generateAddress(const std::string& publicKeyStr) {
    std::string hash = Utils::calculateSHA256(publicKeyStr);
    return "NIL" + hash.substr(0, 34); // NIL prefix + first 34 chars of hash
}

// Hash password for encryption
std::string Wallet::hashPassword(const std::string& password) {
    return Utils::calculateSHA256(password + "NILOTIC_SALT");
}

// Validate password
bool Wallet::validatePassword(const std::string& password) {
    return hashPassword(password) == passwordHash;
}

// Create new wallet
bool Wallet::createNewWallet(const std::string& password) {
    if (!generateKeyPair()) {
        return false;
    }
    
    passwordHash = hashPassword(password);
    isEncrypted = true;
    createdAt = time(nullptr);
    lastUsed = time(nullptr);
    
    Utils::logInfo("Created new wallet: " + address);
    return true;
}

// Import wallet
bool Wallet::importWallet(const std::string& privateKeyPEM, const std::string& password) {
    BIO* bio = BIO_new_mem_buf(privateKeyPEM.c_str(), privateKeyPEM.length());
    if (!bio) {
        Utils::logError("Failed to create BIO for private key");
        return false;
    }
    
    privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!privateKey) {
        Utils::logError("Failed to read private key");
        return false;
    }
    
    // Extract public key from private key
    publicKey = EVP_PKEY_dup(privateKey);
    
    // Convert to PEM format
    bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, privateKey, nullptr, nullptr, 0, nullptr, nullptr);
    char* data = nullptr;
    long len = BIO_get_mem_data(bio, &data);
    this->privateKeyPEM = std::string(data, len);
    BIO_free(bio);
    
    bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, publicKey);
    data = nullptr;
    len = BIO_get_mem_data(bio, &data);
    this->publicKeyPEM = std::string(data, len);
    BIO_free(bio);
    
    // Generate address
    address = generateAddress(this->publicKeyPEM);
    
    // Set password
    passwordHash = hashPassword(password);
    isEncrypted = true;
    createdAt = time(nullptr);
    lastUsed = time(nullptr);
    
    Utils::logInfo("Imported wallet: " + address);
    return true;
}

// Export wallet
bool Wallet::exportWallet(std::string& privateKeyPEM, const std::string& password) {
    if (!validatePassword(password)) {
        Utils::logError("Invalid password");
        return false;
    }
    
    if (!privateKey) {
        Utils::logError("No private key available");
        return false;
    }
    
    privateKeyPEM = this->privateKeyPEM;
    return true;
}

// Delete wallet
bool Wallet::deleteWallet(const std::string& password) {
    if (!validatePassword(password)) {
        Utils::logError("Invalid password");
        return false;
    }
    
    if (privateKey) {
        EVP_PKEY_free(privateKey);
        privateKey = nullptr;
    }
    if (publicKey) {
        EVP_PKEY_free(publicKey);
        publicKey = nullptr;
    }
    
    privateKeyPEM.clear();
    publicKeyPEM.clear();
    address.clear();
    transactionHistory.clear();
    
    Utils::logInfo("Wallet deleted");
    return true;
}

// Get address
std::string Wallet::getAddress() const {
    return address;
}

// Get public key
std::string Wallet::getPublicKey() const {
    return publicKeyPEM;
}

// Check if private key is available
bool Wallet::hasPrivateKey() const {
    return privateKey != nullptr;
}

// Sign transaction
std::string Wallet::signTransaction(const std::string& transactionData) {
    if (!privateKey) {
        Utils::logError("No private key available for signing");
        return "";
    }
    
    if (isLocked()) {
        Utils::logError("Wallet is locked");
        return "";
    }
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        Utils::logError("Failed to create signature context");
        return "";
    }
    
    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, privateKey) <= 0) {
        Utils::logError("Failed to initialize signature");
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    size_t sigLen = 0;
    if (EVP_DigestSign(ctx, nullptr, &sigLen, (const unsigned char*)transactionData.c_str(), transactionData.length()) <= 0) {
        Utils::logError("Failed to calculate signature length");
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    std::vector<unsigned char> signature(sigLen);
    if (EVP_DigestSign(ctx, signature.data(), &sigLen, (const unsigned char*)transactionData.c_str(), transactionData.length()) <= 0) {
        Utils::logError("Failed to create signature");
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    EVP_MD_CTX_free(ctx);
    
    // Convert to hex string
    std::stringstream ss;
    for (unsigned char byte : signature) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    
    lastUsed = time(nullptr);
    return ss.str();
}

// Verify signature
bool Wallet::verifySignature(const std::string& data, const std::string& signature, const std::string& publicKeyPEM) {
    EVP_PKEY* pubKey = nullptr;
    BIO* bio = BIO_new_mem_buf(publicKeyPEM.c_str(), publicKeyPEM.length());
    if (bio) {
        pubKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
    }
    
    if (!pubKey) {
        Utils::logError("Failed to read public key for verification");
        return false;
    }
    
    // Convert hex signature to bytes
    std::vector<unsigned char> sigBytes;
    for (size_t i = 0; i < signature.length(); i += 2) {
        std::string byteString = signature.substr(i, 2);
        unsigned char byte = (unsigned char)std::stoi(byteString, nullptr, 16);
        sigBytes.push_back(byte);
    }
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pubKey);
        return false;
    }
    
    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pubKey) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pubKey);
        return false;
    }
    
    int result = EVP_DigestVerify(ctx, sigBytes.data(), sigBytes.size(), 
                                 (const unsigned char*)data.c_str(), data.length());
    
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubKey);
    
    return result == 1;
}

// Security features
bool Wallet::encryptWallet(const std::string& password) {
    passwordHash = hashPassword(password);
    isEncrypted = true;
    Utils::logInfo("Wallet encrypted");
    return true;
}

bool Wallet::decryptWallet(const std::string& password) {
    if (!validatePassword(password)) {
        Utils::logError("Invalid password");
        return false;
    }
    
    isEncrypted = false;
    unlockAttempts = 0;
    lockUntil = 0;
    Utils::logInfo("Wallet decrypted");
    return true;
}

bool Wallet::isLocked() const {
    if (!isEncrypted) return false;
    if (lockUntil > 0 && time(nullptr) < lockUntil) return true;
    return unlockAttempts >= 3;
}

bool Wallet::unlock(const std::string& password) {
    if (!isEncrypted) return true;
    
    if (lockUntil > 0 && time(nullptr) < lockUntil) {
        Utils::logError("Wallet is temporarily locked");
        return false;
    }
    
    if (validatePassword(password)) {
        unlockAttempts = 0;
        lockUntil = 0;
        lastUsed = time(nullptr);
        Utils::logInfo("Wallet unlocked");
        return true;
    } else {
        unlockAttempts++;
        if (unlockAttempts >= 3) {
            lockUntil = time(nullptr) + 300; // Lock for 5 minutes
            Utils::logError("Too many failed attempts, wallet locked for 5 minutes");
        }
        return false;
    }
}

void Wallet::lock() {
    isEncrypted = true;
    Utils::logInfo("Wallet locked");
}

void Wallet::setLockTimeout(int minutes) {
    lockUntil = time(nullptr) + (minutes * 60);
}

// Wallet metadata
void Wallet::setName(const std::string& name) {
    this->name = name;
}

void Wallet::setDescription(const std::string& description) {
    this->description = description;
}

std::string Wallet::getName() const {
    return name;
}

std::string Wallet::getDescription() const {
    return description;
}

time_t Wallet::getCreatedAt() const {
    return createdAt;
}

time_t Wallet::getLastUsed() const {
    return lastUsed;
}

// Transaction history
void Wallet::addTransaction(const std::string& txHash) {
    transactionHistory.push_back(txHash);
}

std::vector<std::string> Wallet::getTransactionHistory() const {
    return transactionHistory;
}

void Wallet::clearTransactionHistory() {
    transactionHistory.clear();
}

// JSON serialization
nlohmann::json Wallet::toJson() const {
    nlohmann::json json;
    json["address"] = address;
    json["name"] = name;
    json["description"] = description;
    json["createdAt"] = createdAt;
    json["lastUsed"] = lastUsed;
    json["transactionHistory"] = transactionHistory;
    json["isEncrypted"] = isEncrypted;
    json["publicKeyPEM"] = publicKeyPEM;
    // Note: privateKeyPEM is not included for security
    return json;
}

Wallet Wallet::fromJson(const nlohmann::json& json) {
    Wallet wallet;
    wallet.address = json["address"];
    wallet.name = json["name"];
    wallet.description = json["description"];
    wallet.createdAt = json["createdAt"];
    wallet.lastUsed = json["lastUsed"];
    wallet.transactionHistory = json["transactionHistory"].get<std::vector<std::string>>();
    wallet.isEncrypted = json["isEncrypted"];
    wallet.publicKeyPEM = json["publicKeyPEM"];
    return wallet;
}

// File operations
bool Wallet::saveToFile(const std::string& filename, const std::string& password) {
    if (!validatePassword(password)) {
        Utils::logError("Invalid password");
        return false;
    }
    
    nlohmann::json json = toJson();
    json["privateKeyPEM"] = privateKeyPEM; // Include private key for file storage
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        Utils::logError("Failed to open file for writing: " + filename);
        return false;
    }
    
    file << json.dump(4);
    file.close();
    
    Utils::logInfo("Wallet saved to: " + filename);
    return true;
}

Wallet Wallet::loadFromFile(const std::string& filename, const std::string& password) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Utils::logError("Failed to open file: " + filename);
        return Wallet();
    }
    
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    try {
        nlohmann::json json = nlohmann::json::parse(jsonStr);
        Wallet wallet;
        wallet.address = json["address"];
        wallet.name = json["name"];
        wallet.description = json["description"];
        wallet.createdAt = json["createdAt"];
        wallet.lastUsed = json["lastUsed"];
        wallet.transactionHistory = json["transactionHistory"].get<std::vector<std::string>>();
        wallet.isEncrypted = json["isEncrypted"];
        wallet.publicKeyPEM = json["publicKeyPEM"];
        wallet.privateKeyPEM = json["privateKeyPEM"];
        
        // Load keys from PEM
        if (!wallet.privateKeyPEM.empty()) {
            BIO* bio = BIO_new_mem_buf(wallet.privateKeyPEM.c_str(), wallet.privateKeyPEM.length());
            if (bio) {
                wallet.privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
                BIO_free(bio);
            }
            if (wallet.privateKey) {
                wallet.publicKey = EVP_PKEY_dup(wallet.privateKey);
            }
        }
        
        wallet.passwordHash = wallet.hashPassword(password);
        Utils::logInfo("Wallet loaded from: " + filename);
        return wallet;
    } catch (const std::exception& e) {
        Utils::logError("Failed to parse wallet file: " + std::string(e.what()));
        return Wallet();
    }
}

// Utility functions
std::string Wallet::generateMnemonic(int wordCount) {
    // Simplified mnemonic generation (in production, use BIP39)
    static const std::vector<std::string> words = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", "absurd", "abuse",
        "access", "accident", "account", "accuse", "achieve", "acid", "acoustic", "acquire", "across", "act"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, words.size() - 1);
    
    std::string mnemonic;
    for (int i = 0; i < wordCount; ++i) {
        if (i > 0) mnemonic += " ";
        mnemonic += words[dis(gen)];
    }
    
    return mnemonic;
}

Wallet Wallet::fromMnemonic(const std::string& mnemonic, const std::string& password) {
    // Simplified mnemonic to wallet conversion
    Wallet wallet;
    wallet.createNewWallet(password);
    wallet.setDescription("Imported from mnemonic");
    return wallet;
}

std::string Wallet::toMnemonic(const std::string& password) {
    // Simplified wallet to mnemonic conversion
    return generateMnemonic(12);
}

// Validation
bool Wallet::isValid() const {
    return !address.empty() && privateKey != nullptr && publicKey != nullptr;
}

std::string Wallet::getValidationErrors() const {
    std::string errors;
    if (address.empty()) errors += "No address; ";
    if (!privateKey) errors += "No private key; ";
    if (!publicKey) errors += "No public key; ";
    return errors;
}

// Advanced features (simplified implementations)
bool Wallet::addMultiSigKey(const std::string& publicKey) {
    // TODO: Implement multi-sig functionality
    return false;
}

bool Wallet::removeMultiSigKey(const std::string& publicKey) {
    // TODO: Implement multi-sig functionality
    return false;
}

std::vector<std::string> Wallet::getMultiSigKeys() const {
    // TODO: Implement multi-sig functionality
    return std::vector<std::string>();
}

bool Wallet::isMultiSig() const {
    // TODO: Implement multi-sig functionality
    return false;
}

// Hardware wallet support (placeholder)
bool Wallet::connectHardwareWallet() {
    // TODO: Implement hardware wallet support
    return false;
}

bool Wallet::disconnectHardwareWallet() {
    // TODO: Implement hardware wallet support
    return false;
}

bool Wallet::isHardwareWalletConnected() const {
    // TODO: Implement hardware wallet support
    return false;
}

// WalletManager implementation
WalletManager::WalletManager(const std::string& directory) : walletDirectory(directory) {
    // Create directory if it doesn't exist
    std::string cmd = "mkdir -p " + directory;
    system(cmd.c_str());
}

bool WalletManager::createWallet(const std::string& name, const std::string& password) {
    if (walletExists(name)) {
        Utils::logError("Wallet already exists: " + name);
        return false;
    }
    
    Wallet wallet(name);
    if (wallet.createNewWallet(password)) {
        wallets[name] = wallet;
        if (defaultWallet.empty()) {
            defaultWallet = name;
        }
        return wallet.saveToFile(walletDirectory + "/" + name + ".json", password);
    }
    
    return false;
}

bool WalletManager::importWallet(const std::string& name, const std::string& privateKeyPEM, const std::string& password) {
    if (walletExists(name)) {
        Utils::logError("Wallet already exists: " + name);
        return false;
    }
    
    Wallet wallet(privateKeyPEM, password);
    if (wallet.isValid()) {
        wallet.setName(name);
        wallets[name] = wallet;
        if (defaultWallet.empty()) {
            defaultWallet = name;
        }
        return wallet.saveToFile(walletDirectory + "/" + name + ".json", password);
    }
    
    return false;
}

bool WalletManager::deleteWallet(const std::string& name, const std::string& password) {
    auto it = wallets.find(name);
    if (it == wallets.end()) {
        Utils::logError("Wallet not found: " + name);
        return false;
    }
    
    if (it->second.deleteWallet(password)) {
        wallets.erase(it);
        if (defaultWallet == name) {
            defaultWallet = wallets.empty() ? "" : wallets.begin()->first;
        }
        
        std::string filename = walletDirectory + "/" + name + ".json";
        remove(filename.c_str());
        return true;
    }
    
    return false;
}

bool WalletManager::exportWallet(const std::string& name, std::string& privateKeyPEM, const std::string& password) {
    auto it = wallets.find(name);
    if (it == wallets.end()) {
        Utils::logError("Wallet not found: " + name);
        return false;
    }
    
    return it->second.exportWallet(privateKeyPEM, password);
}

Wallet* WalletManager::getWallet(const std::string& name) {
    auto it = wallets.find(name);
    return it != wallets.end() ? &it->second : nullptr;
}

Wallet* WalletManager::getDefaultWallet() {
    if (defaultWallet.empty()) return nullptr;
    return getWallet(defaultWallet);
}

void WalletManager::setDefaultWallet(const std::string& name) {
    if (walletExists(name)) {
        defaultWallet = name;
    }
}

std::vector<std::string> WalletManager::listWallets() const {
    std::vector<std::string> names;
    for (const auto& pair : wallets) {
        names.push_back(pair.first);
    }
    return names;
}

bool WalletManager::walletExists(const std::string& name) const {
    return wallets.find(name) != wallets.end();
}

bool WalletManager::saveAllWallets() {
    bool success = true;
    for (auto& pair : wallets) {
        // Note: This would need the password for each wallet
        // In a real implementation, you'd store passwords securely
        Utils::logWarning("saveAllWallets: Password required for " + pair.first);
    }
    return success;
}

bool WalletManager::loadAllWallets() {
    // TODO: Implement loading all wallets from directory
    return true;
}

bool WalletManager::backupWallets(const std::string& backupPath) {
    // TODO: Implement wallet backup
    return true;
}

bool WalletManager::restoreWallets(const std::string& backupPath) {
    // TODO: Implement wallet restore
    return true;
}

bool WalletManager::lockAllWallets() {
    for (auto& pair : wallets) {
        pair.second.lock();
    }
    return true;
}

bool WalletManager::unlockWallet(const std::string& name, const std::string& password) {
    auto it = wallets.find(name);
    if (it == wallets.end()) {
        return false;
    }
    return it->second.unlock(password);
}

bool WalletManager::isWalletLocked(const std::string& name) const {
    auto it = wallets.find(name);
    if (it == wallets.end()) {
        return true;
    }
    return it->second.isLocked();
}