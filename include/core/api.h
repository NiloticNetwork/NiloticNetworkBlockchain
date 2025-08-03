#ifndef API_H
#define API_H

#include "blockchain.h"
#include "mining.h"
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>

class API {
private:
    Blockchain& blockchain;
    MiningEngine miningEngine;
    std::atomic<bool> running;
    std::thread server_thread;
    int server_fd;
    
    void serverLoop();
    void handleClient(int client_fd, struct sockaddr_in client_addr);
    std::string generateResponse(const std::string& method, const std::string& path, const std::string& body);

public:
    API(Blockchain& blockchain);
    ~API();
    
    void start(int port);
    void stop();
    bool isRunning() const { return running; }
};

#endif // API_H