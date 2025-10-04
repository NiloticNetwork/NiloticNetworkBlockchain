#include "api.h"
#include "utils.h"
#include "oderoslw.h"
#include "wallet.h"
#include "mining.h"
#include "networking.h"
#include <thread>
#include <mutex>
#include <sstream>
#include <map>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "json.hpp"

// Constructor
API::API(Blockchain& blockchain, ConsensusHarmonyManager* consensus) 
    : blockchain(blockchain), miningEngine(blockchain), consensusManager(consensus), 
      running(false), server_fd(-1), websocketRunning(false) {}

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
    
    // Start WebSocket broadcast thread if consensus manager is available
    if (consensusManager) {
        websocketRunning = true;
        websocketBroadcastThread = std::thread([this]() {
            this->websocketBroadcastLoop();
        });
    }
}

// Stop the API server
void API::stop() {
    if (!running) return;
    
    running = false;
    websocketRunning = false;
    
    // Close all WebSocket connections
    {
        std::lock_guard<std::mutex> lock(websocketMutex);
        for (int fd : websocketConnections) {
            close(fd);
        }
        websocketConnections.clear();
    }
    
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
    
    if (websocketBroadcastThread.joinable()) {
        websocketBroadcastThread.join();
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

// WebSocket handling methods
bool API::isWebSocketRequest(const std::map<std::string, std::string>& headers) const {
    auto it = headers.find("Upgrade");
    if (it != headers.end() && it->second == "websocket") {
        return true;
    }
    return false;
}

void API::handleWebSocketConnection(int client_fd, const std::map<std::string, std::string>& headers) {
    // Simple WebSocket handshake
    std::string response = "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Upgrade: websocket\r\n";
    response += "Connection: Upgrade\r\n";
    response += "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"; // Simplified
    response += "\r\n";
    
    send(client_fd, response.c_str(), response.length(), 0);
    
    // Add to WebSocket connections
    {
        std::lock_guard<std::mutex> lock(websocketMutex);
        websocketConnections.insert(client_fd);
    }
    
    Utils::logInfo("WebSocket connection established");
}

void API::websocketBroadcastLoop() {
    while (websocketRunning && consensusManager) {
        try {
            // Get real-time consensus data
            nlohmann::json consensusData;
            consensusData["type"] = "consensus_update";
            consensusData["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            if (consensusManager->isInitialized()) {
                consensusData["status"] = consensusManager->getDetailedStatus();
                consensusData["metrics"] = consensusManager->getMetrics();
            } else {
                consensusData["status"] = "not_initialized";
            }
            
            broadcastToWebSockets(consensusData.dump());
        } catch (const std::exception& e) {
            Utils::logError("WebSocket broadcast error: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Broadcast every 5 seconds
    }
}

void API::broadcastToWebSockets(const std::string& message) {
    std::lock_guard<std::mutex> lock(websocketMutex);
    std::vector<int> toRemove;
    
    for (int fd : websocketConnections) {
        // Simple WebSocket frame format (text frame)
        std::string frame;
        frame.push_back(0x81); // FIN + text frame
        
        if (message.length() < 126) {
            frame.push_back(static_cast<char>(message.length()));
        } else {
            frame.push_back(126);
            frame.push_back((message.length() >> 8) & 0xFF);
            frame.push_back(message.length() & 0xFF);
        }
        
        frame += message;
        
        if (send(fd, frame.c_str(), frame.length(), MSG_NOSIGNAL) < 0) {
            toRemove.push_back(fd);
        }
    }
    
    // Remove failed connections
    for (int fd : toRemove) {
        websocketConnections.erase(fd);
        close(fd);
    }
}

void API::removeWebSocketConnection(int client_fd) {
    std::lock_guard<std::mutex> lock(websocketMutex);
    websocketConnections.erase(client_fd);
}

// Consensus API helper methods
std::string API::handleConsensusStatusRequest() {
    nlohmann::json response;
    
    if (!consensusManager) {
        response["error"] = "Consensus manager not available";
        return response.dump(4);
    }
    
    try {
        response["status"] = "success";
        response["initialized"] = consensusManager->isInitialized();
        response["running"] = consensusManager->isRunning();
        response["emergency_mode"] = consensusManager->isInEmergencyMode();
        response["active_engines"] = nlohmann::json::array();
        
        for (auto type : consensusManager->getActiveEngines()) {
            std::string typeName;
            switch (type) {
                case ConsensusType::PROOF_OF_WORK: typeName = "PROOF_OF_WORK"; break;
                case ConsensusType::PROOF_OF_STAKE: typeName = "PROOF_OF_STAKE"; break;
                case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION: typeName = "PROOF_OF_RESOURCE_CONTRIBUTION"; break;
                case ConsensusType::VOTING_CONSENSUS: typeName = "VOTING_CONSENSUS"; break;
                case ConsensusType::SMART_CONTRACT_VALIDATION: typeName = "SMART_CONTRACT_VALIDATION"; break;
            }
            response["active_engines"].push_back(typeName);
        }
        
        response["detailed_status"] = consensusManager->getDetailedStatus();
    } catch (const std::exception& e) {
        response["error"] = e.what();
    }
    
    return response.dump(4);
}

std::string API::handleConsensusMetricsRequest() {
    nlohmann::json response;
    
    if (!consensusManager) {
        response["error"] = "Consensus manager not available";
        return response.dump(4);
    }
    
    try {
        response["status"] = "success";
        response["metrics"] = consensusManager->getMetrics();
        response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    } catch (const std::exception& e) {
        response["error"] = e.what();
    }
    
    return response.dump(4);
}

std::string API::handleConsensusParameterAdjustment(const std::string& body) {
    nlohmann::json response;
    
    if (!consensusManager) {
        response["error"] = "Consensus manager not available";
        return response.dump(4);
    }
    
    try {
        nlohmann::json requestData = nlohmann::json::parse(body);
        std::string consensusTypeStr = requestData["consensus_type"];
        std::string parameter = requestData["parameter"];
        double value = requestData["value"];
        
        // Convert string to ConsensusType
        ConsensusType type;
        if (consensusTypeStr == "PROOF_OF_WORK") {
            type = ConsensusType::PROOF_OF_WORK;
        } else if (consensusTypeStr == "PROOF_OF_STAKE") {
            type = ConsensusType::PROOF_OF_STAKE;
        } else if (consensusTypeStr == "PROOF_OF_RESOURCE_CONTRIBUTION") {
            type = ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION;
        } else if (consensusTypeStr == "VOTING_CONSENSUS") {
            type = ConsensusType::VOTING_CONSENSUS;
        } else if (consensusTypeStr == "SMART_CONTRACT_VALIDATION") {
            type = ConsensusType::SMART_CONTRACT_VALIDATION;
        } else {
            response["error"] = "Invalid consensus type";
            return response.dump(4);
        }
        
        bool success = consensusManager->setConsensusParameter(type, parameter, value);
        
        if (success) {
            response["status"] = "success";
            response["message"] = "Parameter adjusted successfully";
            response["consensus_type"] = consensusTypeStr;
            response["parameter"] = parameter;
            response["value"] = value;
        } else {
            response["error"] = "Failed to adjust parameter";
        }
    } catch (const std::exception& e) {
        response["error"] = e.what();
    }
    
    return response.dump(4);
}

std::string API::handleConsensusConfigUpdate(const std::string& body) {
    nlohmann::json response;
    
    if (!consensusManager) {
        response["error"] = "Consensus manager not available";
        return response.dump(4);
    }
    
    try {
        nlohmann::json configData = nlohmann::json::parse(body);
        
        ConsensusConfig newConfig;
        newConfig.fromJson(configData);
        
        bool success = consensusManager->updateConfiguration(newConfig);
        
        if (success) {
            response["status"] = "success";
            response["message"] = "Configuration updated successfully";
            response["config"] = consensusManager->getConfiguration().toJson();
        } else {
            response["error"] = "Failed to update configuration";
        }
    } catch (const std::exception& e) {
        response["error"] = e.what();
    }
    
    return response.dump(4);
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
    
    // Check if this is a WebSocket upgrade request
    if (isWebSocketRequest(headers) && path == "/consensus/monitor") {
        handleWebSocketConnection(client_fd, headers);
        return; // Don't close the connection, keep it open for WebSocket
    }
    
    // Generate response
    std::string response = generateResponse(method, path, headers, body);
    
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
bool API::isAuthorized(const std::map<std::string, std::string>& headers, const std::string& method) const {
    // Require Authorization: Bearer <token> for mutating endpoints (POST)
    if (method != "POST") return true;
    auto it = headers.find("Authorization");
    if (it == headers.end()) return false;
    const std::string& auth = it->second;
    const std::string prefix = "Bearer ";
    if (auth.rfind(prefix, 0) != 0) return false;
    const std::string token = auth.substr(prefix.size());
    // TODO: load from config/ENV; minimal check here
    return !token.empty();
}

std::string API::generateResponse(const std::string& method, const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
    nlohmann::json response;
    std::string status = "200 OK";
    
    try {
        if (method == "OPTIONS") {
            // Handle CORS preflight requests
            response["status"] = "ok";
            status = "200 OK";
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
        else if (path.substr(0, 9) == "/balance/") {
            // Get wallet balance
            std::string address = path.substr(9);
            double balance = blockchain.getBalance(address);
            double stake = 0.0; // TODO: implement staking
            
            response["address"] = address;
            response["balance"] = balance;
            response["stake"] = stake;
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
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
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
        }
        else if (path == "/mine" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
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
        else if (path == "/network/peers" && method == "GET") {
            // Get peer list
            response["status"] = "success";
            response["peers"] = nlohmann::json::array();
            // TODO: get actual peers from network engine
        }
        else if (path == "/network/connect" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
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
        }
        else if (path == "/network/disconnect" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
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
        }
        else if (path == "/token" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
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
                    // Do not return seed phrase or private key via API
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
        else if (path == "/consensus/status" && method == "GET") {
            // Get consensus status
            std::string consensusResponse = handleConsensusStatusRequest();
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n"
                   "Access-Control-Allow-Origin: *\r\n"
                   "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                   "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                   "Content-Length: " + std::to_string(consensusResponse.length()) + "\r\n"
                   "\r\n" + consensusResponse;
        }
        else if (path == "/consensus/metrics" && method == "GET") {
            // Get consensus metrics
            std::string consensusResponse = handleConsensusMetricsRequest();
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n"
                   "Access-Control-Allow-Origin: *\r\n"
                   "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                   "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                   "Content-Length: " + std::to_string(consensusResponse.length()) + "\r\n"
                   "\r\n" + consensusResponse;
        }
        else if (path == "/consensus/parameters" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
                // Adjust consensus parameters
                std::string consensusResponse = handleConsensusParameterAdjustment(body);
                return "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                       "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                       "Content-Length: " + std::to_string(consensusResponse.length()) + "\r\n"
                       "\r\n" + consensusResponse;
            }
        }
        else if (path == "/consensus/config" && method == "GET") {
            // Get consensus configuration
            if (!consensusManager) {
                response["error"] = "Consensus manager not available";
                status = "503 Service Unavailable";
            } else {
                try {
                    response["status"] = "success";
                    response["config"] = consensusManager->getConfiguration().toJson();
                } catch (const std::exception& e) {
                    response["error"] = e.what();
                    status = "500 Internal Server Error";
                }
            }
        }
        else if (path == "/consensus/config" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else {
                // Update consensus configuration
                std::string consensusResponse = handleConsensusConfigUpdate(body);
                return "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                       "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                       "Content-Length: " + std::to_string(consensusResponse.length()) + "\r\n"
                       "\r\n" + consensusResponse;
            }
        }
        else if (path == "/consensus/engines" && method == "GET") {
            // Get active consensus engines
            if (!consensusManager) {
                response["error"] = "Consensus manager not available";
                status = "503 Service Unavailable";
            } else {
                try {
                    response["status"] = "success";
                    response["engines"] = nlohmann::json::array();
                    
                    for (auto type : consensusManager->getActiveEngines()) {
                        nlohmann::json engineInfo;
                        switch (type) {
                            case ConsensusType::PROOF_OF_WORK:
                                engineInfo["type"] = "PROOF_OF_WORK";
                                engineInfo["name"] = "Proof of Work";
                                break;
                            case ConsensusType::PROOF_OF_STAKE:
                                engineInfo["type"] = "PROOF_OF_STAKE";
                                engineInfo["name"] = "Proof of Stake";
                                break;
                            case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
                                engineInfo["type"] = "PROOF_OF_RESOURCE_CONTRIBUTION";
                                engineInfo["name"] = "Proof of Resource Contribution";
                                break;
                            case ConsensusType::VOTING_CONSENSUS:
                                engineInfo["type"] = "VOTING_CONSENSUS";
                                engineInfo["name"] = "Voting Consensus";
                                break;
                            case ConsensusType::SMART_CONTRACT_VALIDATION:
                                engineInfo["type"] = "SMART_CONTRACT_VALIDATION";
                                engineInfo["name"] = "Smart Contract Validation";
                                break;
                        }
                        engineInfo["parameters"] = consensusManager->getConsensusParameters(type);
                        response["engines"].push_back(engineInfo);
                    }
                } catch (const std::exception& e) {
                    response["error"] = e.what();
                    status = "500 Internal Server Error";
                }
            }
        }
        else if (path == "/consensus/emergency" && method == "GET") {
            // Get emergency mode status
            if (!consensusManager) {
                response["error"] = "Consensus manager not available";
                status = "503 Service Unavailable";
            } else {
                try {
                    response["status"] = "success";
                    response["emergency_mode"] = consensusManager->isInEmergencyMode();
                    if (consensusManager->isInEmergencyMode()) {
                        auto emergencyMode = consensusManager->getEmergencyMode();
                        if (emergencyMode) {
                            response["emergency_details"] = emergencyMode->getStatus();
                        }
                    }
                } catch (const std::exception& e) {
                    response["error"] = e.what();
                    status = "500 Internal Server Error";
                }
            }
        }
        else if (path == "/consensus/emergency/enter" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else if (!consensusManager) {
                response["error"] = "Consensus manager not available";
                status = "503 Service Unavailable";
            } else {
                try {
                    bool success = consensusManager->enterEmergencyMode();
                    if (success) {
                        response["status"] = "success";
                        response["message"] = "Emergency mode activated";
                    } else {
                        response["error"] = "Failed to enter emergency mode";
                        status = "500 Internal Server Error";
                    }
                } catch (const std::exception& e) {
                    response["error"] = e.what();
                    status = "500 Internal Server Error";
                }
            }
        }
        else if (path == "/consensus/emergency/exit" && method == "POST") {
            if (!isAuthorized(headers, method)) {
                response["error"] = "Unauthorized";
                status = "401 Unauthorized";
            } else if (!consensusManager) {
                response["error"] = "Consensus manager not available";
                status = "503 Service Unavailable";
            } else {
                try {
                    bool success = consensusManager->exitEmergencyMode();
                    if (success) {
                        response["status"] = "success";
                        response["message"] = "Emergency mode deactivated";
                    } else {
                        response["error"] = "Failed to exit emergency mode";
                        status = "500 Internal Server Error";
                    }
                } catch (const std::exception& e) {
                    response["error"] = e.what();
                    status = "500 Internal Server Error";
                }
            }
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