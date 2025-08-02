#include "../include/persistence.h"
#include "../include/utils.h"
#include <filesystem>
#include <iostream>

Persistence::Persistence(const std::string& dbPath) : db(nullptr), dbPath(dbPath) {
    // Create data directory if it doesn't exist
    std::filesystem::create_directories(std::filesystem::path(dbPath).parent_path());
}

Persistence::~Persistence() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Persistence::initDatabase() {
    // Open database (creates if doesn't exist)
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        Utils::logError("Database initialization error: " + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    
    // Create tables if they don't exist
    createTables();
    
    return true;
}

bool Persistence::executeQuery(const std::string& query) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        Utils::logError("SQL error: " + std::string(errMsg ? errMsg : "unknown error"));
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Persistence::executeQueryWithCallback(const std::string& query, 
                                       int (*callback)(void*, int, char**, char**), 
                                       void* data) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), callback, data, &errMsg);
    
    if (rc != SQLITE_OK) {
        Utils::logError("SQL error: " + std::string(errMsg ? errMsg : "unknown error"));
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

void Persistence::createTables() {
    // Create blocks table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS blocks ("
        "   height INTEGER PRIMARY KEY,"
        "   hash TEXT NOT NULL,"
        "   prev_hash TEXT NOT NULL,"
        "   validator TEXT NOT NULL,"
        "   timestamp INTEGER NOT NULL,"
        "   nonce INTEGER NOT NULL,"
        "   data TEXT NOT NULL"  // Serialized block data
        ")"
    );
    
    // Create transactions table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS transactions ("
        "   id TEXT PRIMARY KEY,"
        "   block_hash TEXT,"
        "   from_address TEXT NOT NULL,"
        "   to_address TEXT NOT NULL,"
        "   amount REAL NOT NULL,"
        "   fee REAL NOT NULL,"
        "   type INTEGER NOT NULL,"
        "   timestamp INTEGER NOT NULL,"
        "   data TEXT,"
        "   signature TEXT,"
        "   FOREIGN KEY (block_hash) REFERENCES blocks(hash)"
        ")"
    );
    
    // Create balances table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS balances ("
        "   address TEXT PRIMARY KEY,"
        "   amount REAL NOT NULL"
        ")"
    );
    
    // Create stakes table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS stakes ("
        "   address TEXT PRIMARY KEY,"
        "   amount REAL NOT NULL"
        ")"
    );
    
    // Create Odero tokens table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS odero_tokens ("
        "   token_id TEXT PRIMARY KEY,"
        "   amount REAL NOT NULL,"
        "   pin_hash TEXT,"
        "   creator TEXT NOT NULL,"
        "   spent INTEGER NOT NULL,"
        "   creation_time INTEGER NOT NULL"
        ")"
    );
    
    // Create mempool table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS mempool ("
        "   id TEXT PRIMARY KEY,"
        "   data TEXT NOT NULL"  // Serialized transaction data
        ")"
    );
    
    // Create metadata table
    executeQuery(
        "CREATE TABLE IF NOT EXISTS metadata ("
        "   key TEXT PRIMARY KEY,"
        "   value TEXT NOT NULL"
        ")"
    );
}

// Callback for loading blocks
static int loadBlocksCallback(void* data, int argc, char** argv, char** azColName) {
    auto blocks = static_cast<std::vector<Block>*>(data);
    
    if (argc >= 1 && argv[0]) {
        std::string blockData = argv[0];
        blocks->push_back(Block::deserialize(blockData));
    }
    
    return 0;
}

// Callback for loading balances
static int loadBalancesCallback(void* data, int argc, char** argv, char** azColName) {
    auto balances = static_cast<std::unordered_map<std::string, double>*>(data);
    
    if (argc >= 2 && argv[0] && argv[1]) {
        std::string address = argv[0];
        double amount = std::stod(argv[1]);
        (*balances)[address] = amount;
    }
    
    return 0;
}

// Callback for loading stakes
static int loadStakesCallback(void* data, int argc, char** argv, char** azColName) {
    auto stakes = static_cast<std::unordered_map<std::string, double>*>(data);
    
    if (argc >= 2 && argv[0] && argv[1]) {
        std::string address = argv[0];
        double amount = std::stod(argv[1]);
        (*stakes)[address] = amount;
    }
    
    return 0;
}

// Simplified implementations for basic functionality (non-functional stubs for now)
bool Persistence::saveBlocks(const std::vector<Block>& blocks) {
    return true;
}

bool Persistence::saveBlock(const Block& block) {
    return true;
}

bool Persistence::loadBlocks(std::vector<Block>& blocks) {
    blocks.clear();
    return executeQueryWithCallback("SELECT data FROM blocks ORDER BY height ASC", loadBlocksCallback, &blocks);
}

bool Persistence::saveBalances(const std::unordered_map<std::string, double>& balances) {
    return true;
}

bool Persistence::loadBalances(std::unordered_map<std::string, double>& balances) {
    balances.clear();
    return executeQueryWithCallback("SELECT address, amount FROM balances", loadBalancesCallback, &balances);
}

bool Persistence::saveStakes(const std::unordered_map<std::string, double>& stakes) {
    return true;
}

bool Persistence::loadStakes(std::unordered_map<std::string, double>& stakes) {
    stakes.clear();
    return executeQueryWithCallback("SELECT address, amount FROM stakes", loadStakesCallback, &stakes);
}

bool Persistence::saveOderoTokens(const std::unordered_map<std::string, OderoSLW>& tokens) {
    return true;
}

bool Persistence::saveOderoToken(const OderoSLW& token) {
    return true;
}

bool Persistence::loadOderoTokens(std::unordered_map<std::string, OderoSLW>& tokens) {
    tokens.clear();
    return true;
}

bool Persistence::markOderoTokenSpent(const std::string& tokenId) {
    return executeQuery("UPDATE odero_tokens SET spent = 1 WHERE token_id = '" + tokenId + "'");
}

bool Persistence::saveMempool(const std::vector<Transaction>& transactions) {
    return true;
}

bool Persistence::loadMempool(std::vector<Transaction>& transactions) {
    transactions.clear();
    return true;
}

bool Persistence::getMetadata(std::string& network, std::string& chainId, double& currentSupply) {
    // Default values
    network = "Livewire";
    chainId = "nilotic_mainnet";
    currentSupply = 194250000.0; // 35% of 555,000,000
    return true;
}

bool Persistence::saveMetadata(const std::string& network, const std::string& chainId, double currentSupply) {
    return true;
}