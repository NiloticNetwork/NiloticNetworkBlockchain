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
API::API(Blockchain& blockchain) : blockchain(blockchain), running(false), server_fd(-1) {}

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
    send(client_fd, response.c_str(), response.length(), 0);
    close(client_fd);
}

// Generate HTTP response
std::string API::generateResponse(const std::string& method, const std::string& path, const std::string& body) {
    nlohmann::json response;
    std::string status = "200 OK";
    
    try {
        if (path == "/") {
            // Root endpoint
            response["status"] = "Nilotic Blockchain API is running";
            response["version"] = "1.0.0";
            response["chain_height"] = blockchain.getChainHeight();
            response["pending_transactions"] = blockchain.getPendingTransactions().size();
            response["difficulty"] = blockchain.getDifficulty();
            response["mining_reward"] = blockchain.getMiningReward();
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
                
                // TODO: implement mining with MiningEngine
                response["status"] = "success";
                response["message"] = "Mining endpoint - implementation pending";
                response["miner_address"] = miner_address;
                response["difficulty"] = 4; // TODO: get from mining engine
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/mining/status" && method == "GET") {
            // Get mining status
            response["status"] = "success";
            response["isMining"] = false; // TODO: get from mining engine
            response["currentDifficulty"] = 4;
            response["hashRate"] = 0.0;
            response["estimatedTimeToNextBlock"] = 0;
            response["pendingTransactions"] = blockchain.getPendingTransactions().size();
        }
        else if (path == "/mining/start" && method == "POST") {
            // Start mining
            try {
                nlohmann::json start_data = nlohmann::json::parse(body);
                std::string miner_address = start_data["miner_address"];
                
                // TODO: start mining engine
                response["status"] = "success";
                response["message"] = "Mining started";
                response["miner_address"] = miner_address;
            } catch (const std::exception& e) {
                response["error"] = e.what();
                status = "400 Bad Request";
            }
        }
        else if (path == "/mining/stop" && method == "POST") {
            // Stop mining
            // TODO: stop mining engine
            response["status"] = "success";
            response["message"] = "Mining stopped";
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
                std::string privateKeyPEM = wallet_data["private_key"];
                std::string password = wallet_data["password"];
                
                Wallet wallet(privateKeyPEM, password);
                if (wallet.isValid()) {
                    wallet.setName(name);
                    response["status"] = "success";
                    response["message"] = "Wallet imported successfully";
                    response["address"] = wallet.getAddress();
                    response["name"] = wallet.getName();
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
        else {
            response["error"] = "Endpoint not found";
            status = "404 Not Found";
        }
    } catch (const std::exception& e) {
        response["error"] = e.what();
        status = "500 Internal Server Error";
    }
    
    // Create HTTP response
    std::string http_response = "HTTP/1.1 " + status + "\r\n";
    http_response += "Content-Type: application/json\r\n";
    http_response += "Access-Control-Allow-Origin: *\r\n";
    http_response += "Content-Length: " + std::to_string(response.dump().length()) + "\r\n";
    http_response += "\r\n";
    http_response += response.dump(4);
    
    return http_response;
}