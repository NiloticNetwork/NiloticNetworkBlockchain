#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <signal.h>
#include "json.hpp"
#include "logger.h"
#include "blockchain.h"
#include "utils.h"
#include "oderoslw.h"
#include "api.h"

// Global blockchain instance
Blockchain blockchain;

// Signal handling for clean shutdown
volatile sig_atomic_t running = 1;

void signalHandler(int signum) {
    Logger::info("Received signal " + std::to_string(signum) + ", shutting down...");
    running = 0;
}

// Function to handle HTTP requests
std::string handleRequest(const std::string& request_data) {
    std::string method, uri, body;
    std::map<std::string, std::string> headers;
    
    // Parse the HTTP request
    Utils::parseHttpRequest(request_data, method, uri, headers, body);
    
    // Extract path and query from URI
    std::string path = uri;
    std::string query = "";
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        path = uri.substr(0, query_pos);
        query = uri.substr(query_pos + 1);
    }
    
    // Parse query parameters
    auto params = Utils::parseQueryParams(query);
    
    // Handle different API endpoints
    if (path == "/") {
        // Root endpoint - return info about the blockchain
        nlohmann::json response;
        response["status"] = "Nilotic Blockchain API is running";
        response["version"] = "0.1.0";
        response["chain_height"] = blockchain.getChainHeight();
        response["pending_transactions"] = blockchain.getPendingTransactions().size();
        response["difficulty"] = blockchain.getDifficulty();
        response["mining_reward"] = blockchain.getMiningReward();
        
        return Utils::createJsonResponse(200, response);
    }
    else if (path == "/chain") {
        // Return information about the blockchain
        nlohmann::json response;
        response["chain_height"] = blockchain.getChainHeight();
        
        // Include blocks if requested
        if (params.find("include_blocks") != params.end() && params["include_blocks"] == "true") {
            nlohmann::json blocks = nlohmann::json::array();
            
            // Limit the number of blocks to return to avoid excessive responses
            size_t limit = 10;
            if (params.find("limit") != params.end()) {
                try {
                    limit = std::stoul(params["limit"]);
                } catch (...) {
                    // Default to 10 if invalid
                    limit = 10;
                }
            }
            
            // Get the blocks
            auto chain = blockchain.getChain();
            size_t start = chain.size() > limit ? chain.size() - limit : 0;
            
            for (size_t i = start; i < chain.size(); i++) {
                nlohmann::json block_json = nlohmann::json::parse(chain[i].serialize());
                blocks.push_back(block_json);
            }
            
            response["blocks"] = blocks;
        }
        
        return Utils::createJsonResponse(200, response);
    }
    else if (path == "/transaction" && method == "POST") {
        // Create a new transaction
        Logger::debug("Transaction endpoint hit with body: " + body);
        try {
            if (body.empty()) {
                Logger::error("Empty request body for transaction");
                return Utils::createJsonErrorResponse(400, "Empty request body");
            }
            
            nlohmann::json tx_data;
            try {
                tx_data = nlohmann::json::parse(body);
                Logger::debug("Successfully parsed JSON body: " + tx_data.dump());
            } catch (const std::exception& e) {
                Logger::error("JSON parsing error: " + std::string(e.what()));
                return Utils::createJsonErrorResponse(400, std::string("Invalid JSON: ") + e.what());
            }
            
            // Validate required fields
            if (!tx_data.contains("sender") || !tx_data.contains("recipient") || !tx_data.contains("amount")) {
                Logger::error("Missing required fields in transaction data");
                return Utils::createJsonErrorResponse(400, "Missing required fields: sender, recipient, or amount");
            }
            
            std::string sender = tx_data["sender"].get<std::string>();
            std::string recipient = tx_data["recipient"].get<std::string>();
            double amount = tx_data["amount"].get<double>();
            
            Logger::debug("Creating transaction: " + sender + " -> " + recipient + " for " + std::to_string(amount));
            
            // Create transaction
            Transaction tx(sender, recipient, amount);
            
            // Sign transaction (in a real implementation, this would be done by the client)
            tx.signTransaction("demo-key");
            Logger::debug("Transaction signed with hash: " + tx.getHash());
            
            // Add to pending transactions
            if (blockchain.addTransaction(tx)) {
                Logger::info("Transaction added: " + sender + " -> " + recipient + " for " + std::to_string(amount));
                
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Transaction added to pending pool";
                response["transaction_hash"] = tx.getHash();
                
                return Utils::createJsonResponse(201, response);
            } else {
                Logger::error("Failed to add transaction to blockchain");
                return Utils::createJsonErrorResponse(400, "Failed to add transaction");
            }
        } catch (const std::exception& e) {
            Logger::error("Transaction handling exception: " + std::string(e.what()));
            return Utils::createJsonErrorResponse(400, std::string("Error processing transaction: ") + e.what());
        }
    }
    else if (path == "/mine" && method == "POST") {
        // Mine a new block
        try {
            nlohmann::json mine_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!mine_data.contains("miner_address")) {
                return Utils::createJsonErrorResponse(400, "Missing miner_address field");
            }
            
            std::string miner_address = mine_data["miner_address"];
            
            // Mine the block
            Block newBlock = blockchain.minePendingTransactions(miner_address);
            
            nlohmann::json response;
            response["success"] = true;
            response["message"] = "Block mined successfully";
            response["block_hash"] = newBlock.getHash();
            response["block_index"] = newBlock.getIndex();
            
            return Utils::createJsonResponse(201, response);
        } catch (const std::exception& e) {
            return Utils::createJsonErrorResponse(400, std::string("Mining failed: ") + e.what());
        }
    }
    else if (path == "/balance") {
        // Get balance for an address
        if (params.find("address") == params.end()) {
            return Utils::createJsonErrorResponse(400, "Missing address parameter");
        }
        
        std::string address = params["address"];
        double balance = blockchain.getBalance(address);
        
        nlohmann::json response;
        response["address"] = address;
        response["balance"] = balance;
        
        return Utils::createJsonResponse(200, response);
    }
    else if (path == "/odero/create" && method == "POST") {
        // Create a new Odero SLW token for offline payments
        try {
            nlohmann::json token_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!token_data.contains("creator") || !token_data.contains("amount")) {
                return Utils::createJsonErrorResponse(400, "Missing required fields: creator or amount");
            }
            
            std::string creator = token_data["creator"];
            double amount = token_data["amount"];
            
            // Generate a unique token ID
            std::string tokenId = "OSLW" + Utils::calculateSHA256(creator + std::to_string(amount) + std::to_string(time(nullptr))).substr(0, 16);
            
            // Create the Odero SLW token
            OderoSLW token(tokenId, amount, creator);
            
            // Create a transaction for this token creation
            Transaction tx(creator, "", amount, true);
            tx.signTransaction("demo-key");
            
            // Add to pending transactions
            if (blockchain.addTransaction(tx)) {
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Odero SLW token created successfully";
                response["tokenId"] = tokenId;
                response["amount"] = amount;
                response["creator"] = creator;
                response["qrCode"] = token.generateQrCode();
                response["transaction_hash"] = tx.getHash();
                response["metadata"] = token.getMetadata();
                
                return Utils::createJsonResponse(201, response);
            } else {
                return Utils::createJsonErrorResponse(400, "Failed to create Odero SLW token");
            }
        } catch (const std::exception& e) {
            Logger::error("Odero token creation exception: " + std::string(e.what()));
            return Utils::createJsonErrorResponse(400, std::string("Error creating Odero SLW token: ") + e.what());
        }
    }
    else if (path == "/odero/redeem" && method == "POST") {
        // Redeem an Odero SLW token
        try {
            nlohmann::json redeem_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!redeem_data.contains("redeemer") || !redeem_data.contains("tokenId")) {
                return Utils::createJsonErrorResponse(400, "Missing required fields: redeemer or tokenId");
            }
            
            std::string redeemer = redeem_data["redeemer"];
            std::string tokenId = redeem_data["tokenId"];
            
            // Remove duplicate token prefix if present
            size_t duplicatePos = tokenId.find("OSLW", 4);
            if (duplicatePos != std::string::npos) {
                tokenId = tokenId.substr(0, duplicatePos);
                Logger::debug("Corrected duplicated tokenId for redemption: " + tokenId);
            }
            
            // Verify token format
            if (tokenId.substr(0, 4) != "OSLW") {
                return Utils::createJsonErrorResponse(400, "Invalid token ID format");
            }
            
            // For redemption, we need to use a special transaction where the sender is COINBASE
            // This allows us to bypass the balance check since we're redeeming from the blockchain itself
            Transaction tx("COINBASE", redeemer, 25.5, true);  // Using standard amount for demo
            tx.signTransaction("demo-key");
            
            // Add to pending transactions
            if (blockchain.addTransaction(tx)) {
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Odero SLW token redemption request added to the pending pool";
                response["tokenId"] = tokenId;
                response["redeemer"] = redeemer;
                response["transaction_hash"] = tx.getHash();
                
                return Utils::createJsonResponse(200, response);
            } else {
                return Utils::createJsonErrorResponse(400, "Failed to redeem Odero SLW token");
            }
        } catch (const std::exception& e) {
            Logger::error("Odero token redemption exception: " + std::string(e.what()));
            return Utils::createJsonErrorResponse(400, std::string("Error redeeming Odero SLW token: ") + e.what());
        }
    }
    else if (path == "/odero/verify" && method == "POST") {
        // Verify an Odero SLW token
        try {
            nlohmann::json verify_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!verify_data.contains("tokenId")) {
                return Utils::createJsonErrorResponse(400, "Missing required field: tokenId");
            }
            
            std::string tokenId = verify_data["tokenId"];
            // Remove duplicate token prefix if present
            size_t duplicatePos = tokenId.find("OSLW", 4);
            if (duplicatePos != std::string::npos) {
                tokenId = tokenId.substr(0, duplicatePos);
                Logger::debug("Corrected duplicated tokenId: " + tokenId);
            }
            
            // Create a token object with the ID and minimal data for verification
            OderoSLW token(tokenId, 1.0, "VERIFIER");
            
            // Verify the token
            bool isValid = token.verify();
            
            nlohmann::json response;
            response["tokenId"] = tokenId;
            response["isValid"] = isValid;
            
            if (isValid) {
                response["message"] = "Token is valid";
            } else {
                response["message"] = "Token verification failed";
            }
            
            return Utils::createJsonResponse(200, response);
        } catch (const std::exception& e) {
            Logger::error("Odero token verification exception: " + std::string(e.what()));
            return Utils::createJsonErrorResponse(400, std::string("Error verifying Odero SLW token: ") + e.what());
        }
    }
    else if (path == "/stake" && method == "POST") {
        // Stake tokens for PoS validation
        try {
            nlohmann::json stake_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!stake_data.contains("address") || !stake_data.contains("amount")) {
                return Utils::createJsonErrorResponse(400, "Missing required fields");
            }
            
            std::string address = stake_data["address"];
            double amount = stake_data["amount"];
            
            // Stake tokens
            if (blockchain.stakeTokens(address, amount)) {
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Tokens staked successfully";
                response["address"] = address;
                response["staked_amount"] = amount;
                
                return Utils::createJsonResponse(200, response);
            } else {
                return Utils::createJsonErrorResponse(400, "Failed to stake tokens");
            }
        } catch (const std::exception& e) {
            return Utils::createJsonErrorResponse(400, std::string("Staking failed: ") + e.what());
        }
    }
    else if (path == "/validate" && method == "POST") {
        // Validate a block using PoS
        try {
            nlohmann::json validate_data = nlohmann::json::parse(body);
            
            // Validate required fields
            if (!validate_data.contains("validator_address") || 
                !validate_data.contains("block_index") || 
                !validate_data.contains("signature")) {
                return Utils::createJsonErrorResponse(400, "Missing required fields");
            }
            
            std::string validator_address = validate_data["validator_address"];
            uint64_t block_index = validate_data["block_index"];
            std::string signature = validate_data["signature"];
            
            // Create a new block based on the latest
            Block latestBlock = blockchain.getLatestBlock();
            Block newBlock(latestBlock.getIndex() + 1, latestBlock.getHash());
            
            // Validate the block
            if (blockchain.validateBlockPoS(newBlock, validator_address, signature)) {
                // Add the block to the chain
                if (blockchain.addBlock(newBlock)) {
                    nlohmann::json response;
                    response["success"] = true;
                    response["message"] = "Block validated and added successfully";
                    response["block_hash"] = newBlock.getHash();
                    response["validator"] = validator_address;
                    
                    return Utils::createJsonResponse(201, response);
                } else {
                    return Utils::createJsonErrorResponse(400, "Failed to add validated block to chain");
                }
            } else {
                return Utils::createJsonErrorResponse(400, "Block validation failed");
            }
        } catch (const std::exception& e) {
            return Utils::createJsonErrorResponse(400, std::string("Validation failed: ") + e.what());
        }
    }
    else {
        // Endpoint not found
        return Utils::createJsonErrorResponse(404, "Endpoint not found");
    }
}

void logRequestInfo(const char* buffer, const struct sockaddr_in& client_addr) {
    // Extract method and path from request
    std::string request(buffer);
    std::string method = "UNKNOWN";
    std::string path = "/";
    
    if (!request.empty()) {
        size_t methodEnd = request.find(' ');
        if (methodEnd != std::string::npos) {
            method = request.substr(0, methodEnd);
            size_t pathStart = methodEnd + 1;
            size_t pathEnd = request.find(' ', pathStart);
            if (pathEnd != std::string::npos) {
                path = request.substr(pathStart, pathEnd - pathStart);
            }
        }
    }
    
    // Get client IP address
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    // Log request details
    std::ostringstream logMsg;
    logMsg << "Request: " << method << " " << path << " from " << client_ip 
           << ":" << ntohs(client_addr.sin_port);
    Logger::info(logMsg.str());
}

// Background task to periodically save the blockchain state
void blockchainMaintenanceTask() {
    Logger::info("Starting blockchain maintenance task");
    
    while (running) {
        // Sleep for 60 seconds
        for (int i = 0; i < 60 && running; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (!running) break;
        
        // Save blockchain state
        Logger::info("Performing blockchain maintenance...");
        
        if (blockchain.saveToFile("blockchain_data.json")) {
            Logger::info("Blockchain state saved successfully");
        } else {
            Logger::error("Failed to save blockchain state");
        }
        
        // Validate the chain
        if (blockchain.isChainValid()) {
            Logger::info("Blockchain validation: PASSED");
        } else {
            Logger::error("Blockchain validation: FAILED");
        }
    }
    
    Logger::info("Blockchain maintenance task stopped");
}

int main(int argc, char* argv[]) {
    // Initialize with banner
    Logger::info("******************************************************");
    Logger::info("*          Nilotic Blockchain Server v0.1.0          *");
    Logger::info("******************************************************");
    
    int port = 5000;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            i++;
            Logger::info("Port set to: " + std::to_string(port));
        } else if (arg == "--debug") {
            Logger::setLevel(LogLevel::DEBUG);
            Logger::debug("Debug logging enabled");
        }
    }
    
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Try to load existing blockchain data
    if (blockchain.loadFromFile("blockchain_data.json")) {
        Logger::info("Loaded existing blockchain data");
    } else {
        Logger::info("No existing blockchain data found, starting with a new chain");
    }
    
    // Start the maintenance thread
    std::thread maintenance_thread(blockchainMaintenanceTask);
    
    // Create and start API server
    Logger::info("Creating API server...");
    API api(blockchain);
    
    // Start PoRC system
    Logger::info("Starting PoRC (Proof of Resource Contribution) system...");
    if (api.getPoRCSystem().start()) {
        Logger::info("PoRC system started successfully");
    } else {
        Logger::error("Failed to start PoRC system");
    }
    
    Logger::info("Starting API server on port " + std::to_string(port));
    api.start(port);
    Logger::info("API server start called");
    
    Logger::info("Starting Nilotic Blockchain server on port " + std::to_string(port));
    Logger::info("Server is ready to accept connections");
    
    // Keep the main thread alive
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Clean shutdown
    Logger::info("Shutting down Nilotic Blockchain server...");
    
    // Stop PoRC system
    Logger::info("Stopping PoRC system...");
    api.getPoRCSystem().stop();
    
    api.stop();
    
    // Save blockchain state before exiting
    if (blockchain.saveToFile("blockchain_data.json")) {
        Logger::info("Final blockchain state saved successfully");
    } else {
        Logger::error("Failed to save final blockchain state");
    }
    
    // Wait for maintenance thread to finish
    maintenance_thread.join();
    
    Logger::info("Server shutdown complete");
    
    return 0;
}