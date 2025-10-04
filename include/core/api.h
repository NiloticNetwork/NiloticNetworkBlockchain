#ifndef API_H
#define API_H

#include "blockchain.h"
#include "mining.h"
#include "consensus_harmony_manager.h"
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <set>

class API {
private:
    Blockchain& blockchain;
    MiningEngine miningEngine;
    ConsensusHarmonyManager* consensusManager;
    std::atomic<bool> running;
    std::thread server_thread;
    int server_fd;
    
    // WebSocket connections for real-time monitoring
    std::set<int> websocketConnections;
    std::mutex websocketMutex;
    std::thread websocketBroadcastThread;
    std::atomic<bool> websocketRunning;
    
    void serverLoop();
    void handleClient(int client_fd, struct sockaddr_in client_addr);
    std::string generateResponse(const std::string& method, const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body);
    bool isAuthorized(const std::map<std::string, std::string>& headers, const std::string& method) const;
    
    // WebSocket handling
    bool isWebSocketRequest(const std::map<std::string, std::string>& headers) const;
    void handleWebSocketConnection(int client_fd, const std::map<std::string, std::string>& headers);
    void websocketBroadcastLoop();
    void broadcastToWebSockets(const std::string& message);
    void removeWebSocketConnection(int client_fd);
    
    // Consensus API helpers
    std::string handleConsensusStatusRequest();
    std::string handleConsensusMetricsRequest();
    std::string handleConsensusParameterAdjustment(const std::string& body);
    std::string handleConsensusConfigUpdate(const std::string& body);

public:
    API(Blockchain& blockchain, ConsensusHarmonyManager* consensus = nullptr);
    ~API();
    
    void start(int port);
    void stop();
    bool isRunning() const { return running; }
    
    // Consensus manager integration
    void setConsensusManager(ConsensusHarmonyManager* consensus) { consensusManager = consensus; }
};

#endif // API_H