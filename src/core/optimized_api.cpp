#include "../include/optimized_blockchain.h"
#include "../include/utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

// Optimized HTTP server with connection pooling and async processing
class OptimizedAPIServer {
private:
    int server_fd;
    int port;
    bool debug_mode;
    std::unique_ptr<OptimizedBlockchain> blockchain;
    
    // Performance optimizations
    std::thread workerThread;
    std::atomic<bool> shutdown{false};
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    
    // Connection pooling
    std::vector<int> connectionPool;
    std::mutex poolMutex;
    const size_t maxConnections = 100;
    
    // Response caching
    std::unordered_map<std::string, std::pair<std::string, std::chrono::steady_clock::time_point>> responseCache;
    std::mutex cacheMutex;
    const int cacheTimeout = 30; // seconds

public:
    OptimizedAPIServer(int port = 8080, bool debug = false) 
        : port(port), debug_mode(debug), server_fd(-1) {
        blockchain = std::make_unique<OptimizedBlockchain>();
    }

    ~OptimizedAPIServer() {
        shutdown = true;
        if (workerThread.joinable()) workerThread.join();
        if (server_fd != -1) {
            close(server_fd);
        }
    }

    bool start() {
        // Create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            Utils::logError("Failed to create socket");
            return false;
        }

        // Set socket options for reuse
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            Utils::logError("Failed to set socket options");
            return false;
        }

        // Bind socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            Utils::logError("Failed to bind to port " + std::to_string(port));
            return false;
        }

        // Listen for connections
        if (listen(server_fd, 100) < 0) {
            Utils::logError("Failed to listen on port " + std::to_string(port));
            return false;
        }

        Utils::logInfo("Optimized API server started on port " + std::to_string(port));

        // Start worker thread
        workerThread = std::thread([this]() {
            while (!shutdown) {
                processTaskQueue();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        return true;
    }

    bool accept_connections() {
        struct sockaddr_in client_address;
        int addrlen = sizeof(client_address);

        while (!shutdown) {
            // Set timeout for accept
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int activity = select(server_fd + 1, &readfds, NULL, NULL, &timeout);
            if (activity < 0) {
                Utils::logError("Select error");
                continue;
            }

            if (activity == 0) {
                // Timeout, continue
                continue;
            }

            int client_socket = accept(server_fd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);
            if (client_socket < 0) {
                Utils::logError("Failed to accept connection");
                continue;
            }

            // Check connection pool
            {
                std::lock_guard<std::mutex> lock(poolMutex);
                if (connectionPool.size() >= maxConnections) {
                    close(client_socket);
                    continue;
                }
                connectionPool.push_back(client_socket);
            }

            // Add task to queue
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                taskQueue.push([this, client_socket]() {
                    handle_client_optimized(client_socket);
                });
                queueCV.notify_one();
            }
        }

        return true;
    }

private:
    void processTaskQueue() {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (taskQueue.empty()) {
                return;
            }
            task = taskQueue.front();
            taskQueue.pop();
        }
        
        task();
    }

    void handle_client_optimized(int client_socket) {
        // Set receive timeout
        struct timeval recv_timeout;
        recv_timeout.tv_sec = 5;
        recv_timeout.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

        char buffer[4096] = {0};
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }

        std::string request(buffer);
        std::string response = process_request_optimized(request);
        
        send(client_socket, response.c_str(), response.length(), 0);
        close(client_socket);

        // Remove from connection pool
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            auto it = std::find(connectionPool.begin(), connectionPool.end(), client_socket);
            if (it != connectionPool.end()) {
                connectionPool.erase(it);
            }
        }
    }

    std::string process_request_optimized(const std::string& request) {
        auto start = std::chrono::steady_clock::now();

        // Parse request
        std::istringstream request_stream(request);
        std::string method, path, protocol;
        request_stream >> method >> path >> protocol;

        // Check cache first
        std::string cacheKey = method + ":" + path;
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto it = responseCache.find(cacheKey);
            if (it != responseCache.end()) {
                auto now = std::chrono::steady_clock::now();
                auto age = std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->second.second).count();
                
                if (age < cacheTimeout) {
                    return it->second.first;
                }
            }
        }

        // Process request
        std::string response;
        if (path == "/" || path == "/index.html") {
            response = handle_info_request();
        } else if (path == "/chain") {
            response = handle_chain_request();
        } else if (path == "/mine" && method == "POST") {
            response = handle_mine_request(request);
        } else if (path == "/transaction" && method == "POST") {
            response = handle_transaction_request(request);
        } else if (path.find("/balance") == 0) {
            response = handle_balance_request(path);
        } else if (path.find("/contract/") == 0) {
            response = handle_contract_request(path, method, request);
        } else if (path == "/metrics") {
            response = handle_metrics_request();
        } else if (path == "/health") {
            response = handle_health_request();
        } else {
            response = create_error_response(404, "Endpoint not found");
        }

        // Cache response
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            responseCache[cacheKey] = {response, std::chrono::steady_clock::now()};
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        if (debug_mode) {
            Utils::logInfo("Request processed in " + std::to_string(duration.count()) + "ms");
        }

        return response;
    }

    std::string handle_info_request() {
        nlohmann::json response;
        response["status"] = "Nilotic Blockchain API is running";
        response["version"] = "2.0.0";
        response["optimized"] = true;
        response["endpoints"] = {
            "/", "/chain", "/mine", "/transaction", "/balance", 
            "/contract/*", "/metrics", "/health"
        };
        
        return create_json_response(200, response);
    }

    std::string handle_chain_request() {
        // This would be implemented to return the blockchain data
        nlohmann::json response;
        response["chain"] = nlohmann::json::array();
        response["length"] = 0;
        
        return create_json_response(200, response);
    }

    std::string handle_mine_request(const std::string& request) {
        // Parse request body
        std::string body = extract_request_body(request);
        std::string miner_address = "system_miner";
        
        if (!body.empty()) {
            try {
                nlohmann::json body_json = nlohmann::json::parse(body);
                if (body_json.contains("address")) {
                    miner_address = body_json["address"];
                }
            } catch (const std::exception& e) {
                return create_error_response(400, "Invalid JSON in request body");
            }
        }

        // Mine block
        try {
            auto block = blockchain->createBlock(miner_address);
            
            nlohmann::json response;
            response["message"] = "New block mined";
            response["block"] = {
                {"index", block.getIndex()},
                {"hash", block.getHash()},
                {"previousHash", block.getPreviousHash()},
                {"timestamp", block.getTimestamp()}
            };
            response["miner"] = miner_address;
            
            return create_json_response(200, response);
        } catch (const std::exception& e) {
            return create_error_response(500, "Mining failed: " + std::string(e.what()));
        }
    }

    std::string handle_transaction_request(const std::string& request) {
        std::string body = extract_request_body(request);
        
        if (body.empty()) {
            return create_error_response(400, "Request body required");
        }

        try {
            nlohmann::json body_json = nlohmann::json::parse(body);
            
            if (body_json.contains("type") && body_json["type"] == "contract_deployment") {
                return handle_contract_deployment(body_json);
            } else if (body_json.contains("type") && body_json["type"] == "contract_call") {
                return handle_contract_call(body_json);
            } else {
                return handle_regular_transaction(body_json);
            }
        } catch (const std::exception& e) {
            return create_error_response(400, "Invalid JSON: " + std::string(e.what()));
        }
    }

    std::string handle_balance_request(const std::string& path) {
        size_t query_pos = path.find('?');
        std::string address;
        
        if (query_pos != std::string::npos) {
            std::string query = path.substr(query_pos + 1);
            size_t addr_pos = query.find("address=");
            if (addr_pos != std::string::npos) {
                address = query.substr(addr_pos + 8);
            }
        }

        if (address.empty()) {
            return create_error_response(400, "Address parameter required");
        }

        try {
            double balance = blockchain->getBalance(address);
            
            nlohmann::json response;
            response["address"] = address;
            response["balance"] = balance;
            
            return create_json_response(200, response);
        } catch (const std::exception& e) {
            return create_error_response(500, "Failed to get balance: " + std::string(e.what()));
        }
    }

    std::string handle_contract_request(const std::string& path, const std::string& method, const std::string& request) {
        // Parse contract address from path
        size_t contract_start = path.find("/contract/") + 10;
        size_t contract_end = path.find("/", contract_start);
        std::string contractAddress = path.substr(contract_start, contract_end - contract_start);

        if (path.find("/state") != std::string::npos) {
            // Get contract state
            nlohmann::json response;
            response["contractAddress"] = contractAddress;
            response["state"] = nlohmann::json::object();
            
            return create_json_response(200, response);
        } else if (path.find("/events") != std::string::npos) {
            // Get contract events
            nlohmann::json response;
            response["contractAddress"] = contractAddress;
            response["events"] = nlohmann::json::array();
            
            return create_json_response(200, response);
        }

        return create_error_response(404, "Contract endpoint not found");
    }

    std::string handle_metrics_request() {
        auto metrics = blockchain->getMetrics();
        
        nlohmann::json response;
        response["transactionsProcessed"] = metrics.transactionsProcessed;
        response["blocksMined"] = metrics.blocksMined;
        response["averageResponseTime"] = metrics.averageResponseTime;
        response["memoryUsage"] = metrics.memoryUsage;
        response["cpuUsage"] = metrics.cpuUsage;
        
        return create_json_response(200, response);
    }

    std::string handle_health_request() {
        bool healthy = blockchain->isHealthy();
        
        nlohmann::json response;
        response["status"] = healthy ? "healthy" : "unhealthy";
        response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return create_json_response(healthy ? 200 : 503, response);
    }

    std::string handle_contract_deployment(const nlohmann::json& body) {
        std::string sender = body["sender"];
        std::string contractCode = body["contractCode"];
        
        try {
            // Deploy contract (simplified)
            std::string contractAddress = "0x" + Utils::calculateSHA256(contractCode + sender).substr(0, 40);
            
            nlohmann::json response;
            response["success"] = true;
            response["contractAddress"] = contractAddress;
            response["message"] = "Contract deployed successfully";
            
            return create_json_response(200, response);
        } catch (const std::exception& e) {
            return create_error_response(500, "Contract deployment failed: " + std::string(e.what()));
        }
    }

    std::string handle_contract_call(const nlohmann::json& body) {
        std::string sender = body["sender"];
        std::string contractAddress = body["contractAddress"];
        std::string functionName = body["functionName"];
        
        try {
            // Execute contract call (simplified)
            nlohmann::json response;
            response["success"] = true;
            response["result"] = "Function executed successfully";
            response["gasUsed"] = 21000;
            
            return create_json_response(200, response);
        } catch (const std::exception& e) {
            return create_error_response(500, "Contract call failed: " + std::string(e.what()));
        }
    }

    std::string handle_regular_transaction(const nlohmann::json& body) {
        std::string sender = body["sender"];
        std::string recipient = body["recipient"];
        double amount = body["amount"];

        try {
            Transaction tx(sender, recipient, amount);
            bool success = blockchain->addTransaction(tx);
            
            nlohmann::json response;
            if (success) {
                response["success"] = true;
                response["message"] = "Transaction added to pool";
                response["transaction"] = {
                    {"sender", sender},
                    {"recipient", recipient},
                    {"amount", amount},
                    {"hash", tx.getHash()}
                };
                return create_json_response(200, response);
            } else {
                response["success"] = false;
                response["message"] = "Transaction failed";
                return create_json_response(400, response);
            }
        } catch (const std::exception& e) {
            return create_error_response(500, "Transaction failed: " + std::string(e.what()));
        }
    }

    // Helper methods
    std::string extract_request_body(const std::string& request) {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            return request.substr(body_start + 4);
        }
        return "";
    }

    std::string create_json_response(int status_code, const nlohmann::json& data) {
        std::string response = "HTTP/1.1 " + std::to_string(status_code) + " OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Content-Length: " + std::to_string(data.dump().length()) + "\r\n";
        response += "\r\n";
        response += data.dump(4);
        return response;
    }

    std::string create_error_response(int status_code, const std::string& message) {
        nlohmann::json error;
        error["error"] = message;
        error["status_code"] = status_code;
        return create_json_response(status_code, error);
    }
};

// Main function for optimized API server
int main(int argc, char* argv[]) {
    int port = 8080;
    bool debug_mode = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--debug") {
            debug_mode = true;
        }
    }

    Utils::logInfo("Starting Optimized Nilotic Blockchain API Server");
    Utils::logInfo("Port: " + std::to_string(port));
    Utils::logInfo("Debug mode: " + std::string(debug_mode ? "enabled" : "disabled"));

    OptimizedAPIServer server(port, debug_mode);
    
    if (!server.start()) {
        Utils::logError("Failed to start server");
        return 1;
    }

    Utils::logInfo("Server is ready to accept connections");
    server.accept_connections();

    return 0;
} 