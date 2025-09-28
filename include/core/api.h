#ifndef API_H
#define API_H

#include "blockchain.h"
#include "mining.h"
#include "porc.h"
#include "unified_consensus.h"
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>

class API {
private:
    Blockchain& blockchain;
    MiningEngine miningEngine;
    PoRCSystem porcSystem;
    UnifiedConsensusSystem consensusSystem;
    std::atomic<bool> running;
    std::thread server_thread;
    int server_fd;
    
    void serverLoop();
    void handleClient(int client_fd, struct sockaddr_in client_addr);
    std::string generateResponse(const std::string& method, const std::string& path, const std::string& body);
    std::string handlePoRCRequest(const std::string& method, const std::string& path, const std::string& body);
    std::string handleConsensusRequest(const std::string& method, const std::string& path, const std::string& body);
    std::string generateResponse(const std::string& method, const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body);
    bool isAuthorized(const std::map<std::string, std::string>& headers, const std::string& method) const;

public:
    API(Blockchain& blockchain);
    ~API();
    
    void start(int port);
    void stop();
    bool isRunning() const { return running; }
    
    // PoRC system access
    PoRCSystem& getPoRCSystem() { return porcSystem; }
    
    // Unified consensus system access
    UnifiedConsensusSystem& getConsensusSystem() { return consensusSystem; }
};

#endif // API_H