#include "api.h"
#include "utils.h"
#include "oderoslw.h"
#include "wallet.h"
#include "mining.h"
#include "networking.h"
#include "porc.h"
#include "unified_consensus.h"
#include <thread>
#include <mutex>
#include <sstream>
#include <map>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "json.hpp"
#include <chrono>

// Helper function to get current timestamp
uint64_t getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Constructor
API::API(Blockchain& blockchain) : blockchain(blockchain), miningEngine(blockchain), porcSystem(), consensusSystem(blockchain, miningEngine, porcSystem), running(false), server_fd(-1) {}

// Destructor
API::~API() {
    stop();
}

// Start the API server
void API::start(int port) {
    if (running) {
        Utils::logWarning("API server is already running");
        return;
    }
    
    Utils::logInfo("Starting API server on port " + std::to_string(port));
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        Utils::logError("Failed to create socket");
        return;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Utils::logError("Failed to set socket options");
        close(server_fd);
        return;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Utils::logError("Failed to bind to port " + std::to_string(port));
        close(server_fd);
        return;
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        Utils::logError("Failed to listen on socket");
        close(server_fd);
        return;
    }
    
    running = true;
    Utils::logInfo("API server started successfully on port " + std::to_string(port));
    
    // Start server thread
    server_thread = std::thread([this]() {
        this->serverLoop();
    });
}

// Stop the API server
void API::stop() {
    if (!running) return;
    
    running = false;
    
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
    
    Utils::logInfo("API server stopped");
}

// Server main loop
void API::serverLoop() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        // Set timeout for accept
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (running) {
                Utils::logError("Select failed");
            }
            break;
        }
        
        if (activity == 0) {
            // Timeout, continue loop
            continue;
        }
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            Utils::logError("Failed to accept connection");
            continue;
        }
        
        // Handle client in a separate thread
        std::thread([this, client_fd, client_addr]() {
            this->handleClient(client_fd, client_addr);
        }).detach();
    }
}

// Handle individual client connection
void API::handleClient(int client_fd, struct sockaddr_in client_addr) {
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    // Parse HTTP request
    std::string method, path, body;
    std::map<std::string, std::string> headers;
    Utils::parseHttpRequest(request, method, path, headers, body);
    
    // Log request
    Utils::logInfo("Request: " + method + " " + path + " from " + 
                   inet_ntoa(client_addr.sin_addr) + ":" + std::to_string(ntohs(client_addr.sin_port)));
    Utils::logInfo("Parsed path: '" + path + "'");
    
    // Generate response
    std::string response = generateResponse(method, path, body);
    
    // Send response
    ssize_t bytes_sent = send(client_fd, response.c_str(), response.length(), 0);
    if (bytes_sent < 0) {
        Utils::logError("Failed to send response");
    } else {
        Utils::logInfo("Response sent successfully: " + std::to_string(bytes_sent) + " bytes");
    }
    close(client_fd);
}

// Generate HTTP response
std::string API::generateResponse(const std::string& method, const std::string& path, const std::string& body) {
    nlohmann::json response;
    std::string status = "200 OK";
    
    try {
        if (method == "OPTIONS") {
            // Handle CORS preflight requests
            response["status"] = "ok";
            status = "200 OK";
        }
        else if (path.substr(0, 5) == "/porc") {
            // Handle PoRC endpoints
            return handlePoRCRequest(method, path, body);
        }
        else if (path.substr(0, 10) == "/consensus") {
            // Handle unified consensus endpoints
            return handleConsensusRequest(method, path, body);
        }
        else if (path == "/") {
            // Root endpoint
            response["status"] = "Nilotic Blockchain API is running";
            response["version"] = "1.0.0";
            response["chain_height"] = blockchain.getChainHeight();
            response["pending_transactions"] = blockchain.getPendingTransactions().size();
            response["difficulty"] = blockchain.getDifficulty();
            response["mining_reward"] = blockchain.getMiningReward();
            response["success"] = true;
        }
        else if (path == "/info") {
            // Blockchain info
            response["chainId"] = "nilotic-chain-1";
            response["chainHeight"] = blockchain.getChainHeight();
            response["blockCount"] = blockchain.getChain().size();
            response["isValid"] = true; // TODO: implement validation
            response["pendingTransactions"] = blockchain.getPendingTransactions().size();
            response["difficulty"] = blockchain.getDifficulty();
            response["miningReward"] = blockchain.getMiningReward();
            response["status"] = "success";
        }
        else if (path == "/chain") {
            // Get blockchain data
            response["chainHeight"] = blockchain.getChainHeight();
            response["blockCount"] = blockchain.getChain().size();
            response["status"] = "success";
            
            // Include blocks if requested via query parameter
            // Note: This is a simplified version - in a full implementation,
            // you would parse query parameters from the request
            nlohmann::json blocks = nlohmann::json::array();
            const auto& chain = blockchain.getChain();
            
            // Limit the number of blocks to return (last 10 by default)
            size_t limit = 10;
            size_t start = chain.size() > limit ? chain.size() - limit : 0;
            for (size_t i = start; i < chain.size(); i++) {
                blocks.push_back(nlohmann::json::parse(chain[i].serialize()));
            }
            response["blocks"] = blocks;
        }
        else if (path == "/transactions") {
            // Get pending transactions
            const auto& pending = blockchain.getPendingTransactions();
            nlohmann::json transactions = nlohmann::json::array();
            
            for (const auto& tx : pending) {
                nlohmann::json tx_json;
                tx_json["sender"] = tx.getSender();
                tx_json["recipient"] = tx.getRecipient();
                tx_json["amount"] = tx.getAmount();
                tx_json["hash"] = tx.calculateHash();
                tx_json["timestamp"] = tx.getTimestamp();
                transactions.push_back(tx_json);
            }
            
            response["transactions"] = transactions;
            response["count"] = pending.size();
            response["status"] = "success";
        }
        else if (path == "/transactions/pending") {
            // Get pending transactions count
            response["pending_count"] = blockchain.getPendingTransactions().size();
            response["status"] = "success";
        }
        else if (path == "/transactions/confirmed") {
            // Get confirmed transactions count
            size_t confirmed_count = 0;
            const auto& chain = blockchain.getChain();
            for (const auto& block : chain) {
                confirmed_count += block.getTransactions().size();
            }
            response["confirmed_count"] = confirmed_count;
            response["status"] = "success";
        }
        else if (path.substr(0, 9) == "/balance/") {
            // Get wallet balance
            std::string address = path.substr(9);
            double balance = blockchain.getBalance(address);
            double stake = 0.0; // TODO: implement staking
            
            response["address"] = address;
            response["balance"] = balance;
            response["stake"] = stake;
            response["status"] = "success";
        }
        else if (path == "/block/latest") {
            // Get latest block
            try {
                Block latestBlock = blockchain.getLatestBlock();
                response = nlohmann::json::parse(latestBlock.serialize());
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path.substr(0, 7) == "/block/") {
            // Get block by index
            try {
                std::string index_str = path.substr(7);
                int index = std::stoi(index_str);
                
                const auto& chain = blockchain.getChain();
                if (index < 0 || static_cast<size_t>(index) >= chain.size()) {
                    response["error"] = "Block index out of range";
                    status = "400 Bad Request";
                } else {
                    response = nlohmann::json::parse(chain[index].serialize());
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/transaction" && method == "POST") {
            // Create transaction
            try {
                nlohmann::json tx_data = nlohmann::json::parse(body);
                std::string sender = tx_data["sender"];
                std::string recipient = tx_data["recipient"];
                double amount = tx_data["amount"];
                std::string type = tx_data.value("type", "transfer");
                
                Transaction tx(sender, recipient, amount);
                
                if (blockchain.addTransaction(tx)) {
                    response["status"] = "success";
                    response["message"] = "Transaction added to pending pool";
                    response["transaction_id"] = tx.calculateHash();
                } else {
                    response["error"] = "Failed to add transaction";
                    status = "400 Bad Request";
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/mine" && method == "POST") {
            // Mine a new block
            try {
                nlohmann::json mine_data = nlohmann::json::parse(body);
                std::string miner_address = mine_data["miner_address"];
                
                // Mine a block using the mining engine
                Block minedBlock = miningEngine.mineBlock(miner_address);
                
                if (minedBlock.getIndex() >= 0 && !minedBlock.getHash().empty()) { // Valid block mined
                    // Add the block to the blockchain
                    if (blockchain.addBlock(minedBlock)) {
                        response["status"] = "success";
                        response["message"] = "Block mined successfully";
                        response["block_index"] = minedBlock.getIndex();
                        response["block_hash"] = minedBlock.getHash();
                        response["miner_address"] = miner_address;
                        response["difficulty"] = miningEngine.getCurrentDifficulty();
                        response["reward"] = miningEngine.calculateBlockReward(minedBlock.getIndex());
                    } else {
                        response["status"] = "error";
                        response["message"] = "Failed to add block to blockchain";
                        status = "400 Bad Request";
                    }
                } else {
                    response["status"] = "error";
                    response["message"] = "Failed to mine block";
                    status = "400 Bad Request";
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/mining/status" && method == "GET") {
            // Get mining status
            response["status"] = "success";
            response["isMining"] = miningEngine.isMiningActive();
            response["currentDifficulty"] = miningEngine.getCurrentDifficulty();
            response["hashRate"] = miningEngine.getCurrentHashRate();
            response["estimatedTimeToNextBlock"] = miningEngine.getEstimatedTimeToNextBlock();
            response["pendingTransactions"] = blockchain.getPendingTransactions().size();
            response["miningStats"] = miningEngine.getMiningStats().toJson();
        }
        else if (path == "/mining/start" && method == "POST") {
            // Start mining
            try {
                nlohmann::json start_data = nlohmann::json::parse(body);
                std::string miner_address = start_data["miner_address"];
                
                if (miningEngine.startMining(miner_address)) {
                    response["status"] = "success";
                    response["message"] = "Mining started successfully";
                    response["miner_address"] = miner_address;
                    response["difficulty"] = miningEngine.getCurrentDifficulty();
                } else {
                    response["status"] = "error";
                    response["message"] = "Failed to start mining";
                    status = "400 Bad Request";
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/mining/stop" && method == "POST") {
            // Stop mining
            miningEngine.stopMining();
            response["status"] = "success";
            response["message"] = "Mining stopped successfully";
            response["isMining"] = miningEngine.isMiningActive();
        }
        else if (path == "/token" && method == "POST") {
            // Create token
            try {
                nlohmann::json token_data = nlohmann::json::parse(body);
                std::string tokenId = token_data["token_id"];
                double amount = token_data["amount"];
                std::string creator = token_data["creator"];
                
                OderoSLW token(tokenId, amount, creator);
                
                response["status"] = "success";
                response["message"] = "Token created successfully";
                response["token_id"] = tokenId;
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/wallet/create" && method == "POST") {
            // Create new wallet
            try {
                nlohmann::json wallet_data = nlohmann::json::parse(body);
                std::string name = wallet_data["name"];
                std::string password = wallet_data["password"];
                
                Wallet wallet(name);
                if (wallet.createNewWallet(password)) {
                    response["status"] = "success";
                    response["message"] = "Wallet created successfully";
                    response["address"] = wallet.getAddress();
                    response["name"] = wallet.getName();
                    response["seedPhrase"] = wallet.toMnemonic(password);
                } else {
                    response["error"] = "Failed to create wallet";
                    status = "400 Bad Request";
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/wallet/import" && method == "POST") {
            // Import wallet
            try {
                nlohmann::json wallet_data = nlohmann::json::parse(body);
                std::string name = wallet_data["name"];
                std::string password = wallet_data["password"];
                
                // Create a new wallet with the name (this will generate the same address)
                Wallet wallet(name);
                
                // Create the wallet with the password
                if (wallet.createNewWallet(password)) {
                    response["status"] = "success";
                    response["message"] = "Wallet imported successfully";
                    response["address"] = wallet.getAddress();
                    response["name"] = name; // Use the provided name instead of getName()
                } else {
                    response["error"] = "Failed to import wallet";
                    status = "400 Bad Request";
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/wallet/sign" && method == "POST") {
            // Sign transaction
            try {
                nlohmann::json sign_data = nlohmann::json::parse(body);
                std::string privateKeyPEM = sign_data["private_key"];
                std::string password = sign_data["password"];
                std::string transactionData = sign_data["transaction_data"];
                
                Wallet wallet(privateKeyPEM, password);
                if (wallet.isValid()) {
                    std::string signature = wallet.signTransaction(transactionData);
                    if (!signature.empty()) {
                        response["status"] = "success";
                        response["message"] = "Transaction signed successfully";
                        response["signature"] = signature;
                        response["address"] = wallet.getAddress();
                    } else {
                        response["error"] = "Failed to sign transaction";
                        status = "400 Bad Request";
                    }
                } else {
                    response["error"] = "Invalid wallet";
                    status = "400 Bad Request";
                }
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/stake" && method == "POST") {
            // Stake tokens
            try {
                nlohmann::json stake_data = nlohmann::json::parse(body);
                std::string address = stake_data["address"];
                double amount = stake_data["amount"];
                std::string type = stake_data.value("type", "validator");
                
                // TODO: Implement actual staking logic
                response["status"] = "success";
                response["message"] = "Staking request received";
                response["address"] = address;
                response["amount"] = amount;
                response["type"] = type;
                response["stake_id"] = "stake_" + std::to_string(getCurrentTimestamp());
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/stake/unstake" && method == "POST") {
            // Unstake tokens
            try {
                nlohmann::json unstake_data = nlohmann::json::parse(body);
                std::string address = unstake_data["address"];
                double amount = unstake_data["amount"];
                
                // TODO: Implement actual unstaking logic
                response["status"] = "success";
                response["message"] = "Unstaking request received";
                response["address"] = address;
                response["amount"] = amount;
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/stake/rewards" && method == "POST") {
            // Claim staking rewards
            try {
                nlohmann::json reward_data = nlohmann::json::parse(body);
                std::string address = reward_data["address"];
                
                // TODO: Implement actual reward claiming logic
                response["status"] = "success";
                response["message"] = "Reward claim request received";
                response["address"] = address;
                response["reward_amount"] = 0.0; // TODO: calculate actual reward
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/stake/status" && method == "GET") {
            // Get staking status
            response["status"] = "success";
            response["total_staked"] = 0.0; // TODO: implement staking tracking
            response["active_validators"] = 0;
            response["staking_rewards_pool"] = 0.0;
        }
        else if (path == "/network/peers" && method == "GET") {
            // Get peer list
            response["status"] = "success";
            response["peers"] = nlohmann::json::array();
            response["total_peers"] = 0;
            response["connected_peers"] = 0;
            // TODO: get actual peers from network engine
        }
        else if (path == "/network/connect" && method == "POST") {
            // Connect to peer
            try {
                nlohmann::json connect_data = nlohmann::json::parse(body);
                std::string address = connect_data["address"];
                uint16_t port = connect_data["port"];
                
                // TODO: connect to peer using network engine
                response["status"] = "success";
                response["message"] = "Connection request sent";
                response["address"] = address;
                response["port"] = port;
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/network/disconnect" && method == "POST") {
            // Disconnect from peer
            try {
                nlohmann::json disconnect_data = nlohmann::json::parse(body);
                std::string address = disconnect_data["address"];
                
                // TODO: disconnect from peer using network engine
                response["status"] = "success";
                response["message"] = "Disconnection request sent";
                response["address"] = address;
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/network/status" && method == "GET") {
            // Get network status
            response["status"] = "success";
            response["isRunning"] = false; // TODO: get from network engine
            response["activeConnections"] = 0;
            response["totalPeers"] = 0;
            response["totalMessagesReceived"] = 0;
            response["totalMessagesSent"] = 0;
            response["listenPort"] = 8333;
        }
        else if (path == "/analytics/blockchain" && method == "GET") {
            // Get blockchain analytics
            const auto& chain = blockchain.getChain();
            size_t total_transactions = 0;
            double total_volume = 0.0;
            
            for (const auto& block : chain) {
                total_transactions += block.getTransactions().size();
                for (const auto& tx : block.getTransactions()) {
                    total_volume += tx.getAmount();
                }
            }
            
            response["status"] = "success";
            response["total_blocks"] = chain.size();
            response["total_transactions"] = total_transactions;
            response["total_volume"] = total_volume;
            response["average_block_time"] = 0; // TODO: calculate
            response["current_difficulty"] = blockchain.getDifficulty();
            response["mining_reward"] = blockchain.getMiningReward();
        }
        else if (path == "/analytics/mining" && method == "GET") {
            // Get mining analytics
            response["status"] = "success";
            response["is_mining"] = miningEngine.isMiningActive();
            response["current_difficulty"] = miningEngine.getCurrentDifficulty();
            response["hash_rate"] = miningEngine.getCurrentHashRate();
            response["estimated_time_to_next_block"] = miningEngine.getEstimatedTimeToNextBlock();
            response["mining_stats"] = miningEngine.getMiningStats().toJson();
        }
        else if (path == "/analytics/consensus" && method == "GET") {
            // Get consensus analytics
            response["status"] = "success";
            response["consensus_stats"] = consensusSystem.getStats().toJson();
            response["active_rounds"] = consensusSystem.getActiveRounds().size();
            response["active_participants"] = consensusSystem.getActiveParticipants().size();
        }
        else if (path == "/analytics/porc" && method == "GET") {
            // Get PoRC analytics
            response["status"] = "success";
            response["porc_stats"] = porcSystem.getStats().toJson();
        }
        else if (path == "/health" && method == "GET") {
            // Health check endpoint
            response["status"] = "healthy";
            response["timestamp"] = getCurrentTimestamp();
            response["version"] = "1.0.0";
            response["blockchain_height"] = blockchain.getChainHeight();
            response["pending_transactions"] = blockchain.getPendingTransactions().size();
            response["mining_active"] = miningEngine.isMiningActive();
            response["consensus_active"] = consensusSystem.isSystemRunning();
            response["porc_active"] = true; // TODO: check PoRC status
        }
        else if (path == "/metrics" && method == "GET") {
            // Metrics endpoint for monitoring
            response["status"] = "success";
            response["metrics"] = {
                {"blockchain_height", blockchain.getChainHeight()},
                {"pending_transactions", blockchain.getPendingTransactions().size()},
                {"total_blocks", blockchain.getChain().size()},
                {"current_difficulty", blockchain.getDifficulty()},
                {"mining_reward", blockchain.getMiningReward()},
                {"mining_active", miningEngine.isMiningActive()},
                {"consensus_active", consensusSystem.isSystemRunning()},
                {"timestamp", getCurrentTimestamp()}
            };
        }
        else {
            response["error"] = "Endpoint not found";
            status = "404 Not Found";
        }
    } catch (const std::exception& e) {
        response["error"] = e.what();
        status = "500 Internal Server Error";
    }
    
    // Create HTTP response
    std::string json_body = response.dump(4);
    std::string http_response = "HTTP/1.1 " + status + "\r\n";
    http_response += "Content-Type: application/json\r\n";
    http_response += "Access-Control-Allow-Origin: *\r\n";
    http_response += "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    http_response += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    http_response += "Content-Length: " + std::to_string(json_body.length()) + "\r\n";
    http_response += "\r\n";
    http_response += json_body;
    
    return http_response;
}

// Handle PoRC-specific requests
std::string API::handlePoRCRequest(const std::string& method, const std::string& path, const std::string& body) {
    nlohmann::json response;
    std::string status = "200 OK";
    
    try {
        if (path == "/porc/enable" && method == "POST") {
            // Enable PoRC for a wallet
            nlohmann::json request = nlohmann::json::parse(body);
            response = porcSystem.handleEnableRequest(request);
        }
        else if (path == "/porc/stats" && method == "GET") {
            // Get PoRC statistics
            response = porcSystem.handleStatsRequest(nlohmann::json());
        }
        else if (path == "/porc/submit_log" && method == "POST") {
            // Submit contribution log
            nlohmann::json request = nlohmann::json::parse(body);
            response = porcSystem.handleSubmitLogRequest(request);
        }
        else if (path.substr(0, 18) == "/porc/wallet/" && method == "GET") {
            // Get wallet PoRC status
            std::string address = path.substr(18);
            nlohmann::json request;
            request["address"] = address;
            response = porcSystem.handleWalletStatusRequest(request);
        }
        else if (path == "/porc/pools" && method == "GET") {
            // Get active pools
            response = porcSystem.handlePoolStatusRequest(nlohmann::json());
        }
        else if (path == "/porc/tasks" && method == "GET") {
            // Get tasks for a wallet (requires address parameter)
            // This would need to be implemented with query parameters
            response["success"] = false;
            response["message"] = "Use /porc/tasks?address=<wallet_address>";
        }
        else {
            response["success"] = false;
            response["message"] = "Unknown PoRC endpoint";
            status = "404 Not Found";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Error processing PoRC request: " + std::string(e.what());
        status = "500 Internal Server Error";
    }
    
    // Format response
    std::string responseStr = response.dump();
    std::string httpResponse = "HTTP/1.1 " + status + "\r\n";
    httpResponse += "Content-Type: application/json\r\n";
    httpResponse += "Access-Control-Allow-Origin: *\r\n";
    httpResponse += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    httpResponse += "Access-Control-Allow-Headers: Content-Type\r\n";
    httpResponse += "Content-Length: " + std::to_string(responseStr.length()) + "\r\n";
    httpResponse += "\r\n";
    httpResponse += responseStr;
    
    return httpResponse;
}

// Handle unified consensus-specific requests
std::string API::handleConsensusRequest(const std::string& method, const std::string& path, const std::string& body) {
    nlohmann::json response;
    std::string status = "200 OK";
    
    try {
        if (path == "/consensus/join" && method == "POST") {
            // Join consensus as participant
            nlohmann::json request = nlohmann::json::parse(body);
            response = consensusSystem.handleJoinRequest(request);
        }
        else if (path == "/consensus/leave" && method == "POST") {
            // Leave consensus
            nlohmann::json request = nlohmann::json::parse(body);
            response = consensusSystem.handleLeaveRequest(request);
        }
        else if (path == "/consensus/stats" && method == "GET") {
            // Get consensus statistics
            response = consensusSystem.handleStatsRequest(nlohmann::json());
        }
        else if (path == "/consensus/rounds" && method == "GET") {
            // Get active consensus rounds
            response = consensusSystem.handleRoundsRequest(nlohmann::json());
        }
        else if (path == "/consensus/participants" && method == "GET") {
            // Get active participants
            response = consensusSystem.handleParticipantsRequest(nlohmann::json());
        }
        else if (path == "/consensus/submit_transaction" && method == "POST") {
            // Submit transaction for validation
            nlohmann::json request = nlohmann::json::parse(body);
            response = consensusSystem.handleSubmitTransactionRequest(request);
        }
        else if (path == "/consensus/submit_result" && method == "POST") {
            // Submit validation result
            nlohmann::json request = nlohmann::json::parse(body);
            response = consensusSystem.handleSubmitResultRequest(request);
        }
        else {
            response["success"] = false;
            response["message"] = "Unknown consensus endpoint";
            status = "404 Not Found";
        }
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Error processing consensus request: " + std::string(e.what());
        status = "500 Internal Server Error";
    }
    
    // Format response
    std::string responseStr = response.dump();
    std::string httpResponse = "HTTP/1.1 " + status + "\r\n";
    httpResponse += "Content-Type: application/json\r\n";
    httpResponse += "Access-Control-Allow-Origin: *\r\n";
    httpResponse += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    httpResponse += "Access-Control-Allow-Headers: Content-Type\r\n";
    httpResponse += "Content-Length: " + std::to_string(responseStr.length()) + "\r\n";
    httpResponse += "\r\n";
    httpResponse += responseStr;
    
    return httpResponse;
}