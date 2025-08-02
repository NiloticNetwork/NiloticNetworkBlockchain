#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include "block.h"
#include "transaction.h"
#include "wallet.h"
#include "oderoslw.h"

class Persistence {
private:
    // File-based storage (no SQLite needed for this implementation)
    
public:
    // Constructor
    Persistence();
    ~Persistence();
    
    // Check if blockchain exists in storage
    bool blockchainExists() const;
    
    // Save blockchain to storage
    void saveBlockchain(const std::vector<Block>& blocks);
    
    // Load blockchain from storage
    std::vector<Block> loadBlockchain() const;
    
    // Save pending transactions to storage
    void savePendingTransactions(const std::vector<Transaction>& transactions);
    
    // Load pending transactions from storage
    std::vector<Transaction> loadPendingTransactions() const;
    
    // Save wallets to storage
    void saveWallets(const std::map<std::string, Wallet>& wallets);
    
    // Load wallets from storage
    std::map<std::string, Wallet> loadWallets() const;
    
    // Save stakes to storage
    void saveStakes(const std::map<std::string, double>& stakes);
    
    // Load stakes from storage
    std::map<std::string, double> loadStakes() const;
    
    // Save Odero SLW tokens to storage
    void saveOderoTokens(const std::map<std::string, OderoSLW>& tokens);
    
    // Load Odero SLW tokens from storage
    std::map<std::string, OderoSLW> loadOderoTokens() const;
};

#endif // PERSISTENCE_H