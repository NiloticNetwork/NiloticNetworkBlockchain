#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "blockchain.h"
#include "mining.h"
#include "json.hpp"

// Network message types
enum class MessageType {
    HANDSHAKE = 0,
    PING = 1,
    PONG = 2,
    GET_BLOCKS = 3,
    BLOCKS = 4,
    GET_TRANSACTIONS = 5,
    TRANSACTIONS = 6,
    NEW_BLOCK = 7,
    NEW_TRANSACTION = 8,
    PEER_LIST = 9,
    ADD_PEER = 10,
    REMOVE_PEER = 11,
    MINING_REQUEST = 12,
    MINING_RESPONSE = 13,
    CONSENSUS_REQUEST = 14,
    CONSENSUS_RESPONSE = 15
};

// Network message structure
struct NetworkMessage {
    MessageType type;
    std::string sender;
    std::string recipient;
    uint64_t timestamp;
    uint64_t sequence;
    nlohmann::json data;
    std::string signature;
    
    NetworkMessage() : type(MessageType::HANDSHAKE), timestamp(0), sequence(0) {}
    
    std::string serialize() const;
    static NetworkMessage deserialize(const std::string& data);
    std::string calculateHash() const;
    bool isValid() const;
};

// Peer node information
struct PeerNode {
    std::string address;
    uint16_t port;
    std::string nodeId;
    std::string version;
    uint64_t lastSeen;
    bool isConnected;
    uint64_t latency;
    uint64_t blockHeight;
    std::vector<std::string> capabilities;
    
    PeerNode() : port(0), lastSeen(0), isConnected(false), latency(0), blockHeight(0) {}
    
    std::string getFullAddress() const;
    bool isActive() const;
    nlohmann::json toJson() const;
    static PeerNode fromJson(const nlohmann::json& json);
};

// Network configuration
struct NetworkConfig {
    uint16_t listenPort = 8333;
    std::string bindAddress = "0.0.0.0";
    uint64_t maxPeers = 50;
    uint64_t maxConnections = 100;
    uint64_t handshakeTimeout = 30;
    uint64_t pingInterval = 60;
    uint64_t peerDiscoveryInterval = 300;
    uint64_t blockSyncInterval = 10;
    uint64_t transactionBroadcastInterval = 5;
    bool enableUPnP = true;
    bool enableNATTraversal = true;
    std::vector<std::string> seedNodes;
    uint64_t maxMessageSize = 1024 * 1024; // 1MB
    uint64_t maxBlockSize = 1024 * 1024; // 1MB
    bool enableCompression = true;
    bool enableEncryption = false;
    std::string networkMagic = "NILOTIC";
    uint32_t protocolVersion = 1;
};

// Connection state
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    HANDSHAKING,
    CONNECTED,
    DISCONNECTING
};

// Network connection
class NetworkConnection {
private:
    int socketFd;
    std::string remoteAddress;
    uint16_t remotePort;
    ConnectionState state;
    std::atomic<bool> shouldClose;
    std::thread readThread;
    std::thread writeThread;
    std::queue<NetworkMessage> sendQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    
    // Connection statistics
    uint64_t bytesReceived;
    uint64_t bytesSent;
    uint64_t messagesReceived;
    uint64_t messagesSent;
    std::chrono::steady_clock::time_point lastActivity;
    
public:
    NetworkConnection(int fd, const std::string& address, uint16_t port);
    ~NetworkConnection();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return state == ConnectionState::CONNECTED; }
    ConnectionState getState() const { return state; }
    
    // Message handling
    bool sendMessage(const NetworkMessage& message);
    bool receiveMessage(NetworkMessage& message);
    void processMessage(const NetworkMessage& message);
    
    // Statistics
    uint64_t getBytesReceived() const { return bytesReceived; }
    uint64_t getBytesSent() const { return bytesSent; }
    uint64_t getMessagesReceived() const { return messagesReceived; }
    uint64_t getMessagesSent() const { return messagesSent; }
    std::chrono::steady_clock::time_point getLastActivity() const { return lastActivity; }
    
    // Getters
    std::string getRemoteAddress() const { return remoteAddress; }
    uint16_t getRemotePort() const { return remotePort; }
    std::string getFullAddress() const { return remoteAddress + ":" + std::to_string(remotePort); }
    
private:
    void readLoop();
    void writeLoop();
    bool sendData(const std::string& data);
    bool receiveData(std::string& data);
    void updateActivity();
};

// Main networking engine
class NetworkEngine {
private:
    Blockchain& blockchain;
    MiningEngine& miningEngine;
    NetworkConfig config;
    
    // Network state
    std::atomic<bool> isRunning;
    std::thread listenerThread;
    std::thread discoveryThread;
    std::thread syncThread;
    std::thread broadcastThread;
    std::thread messageProcessingThread;
    
    // Socket management
    int listenerSocket;
    std::vector<std::unique_ptr<NetworkConnection>> connections;
    std::mutex connectionsMutex;
    
    // Peer management
    std::map<std::string, PeerNode> peers;
    mutable std::mutex peersMutex;
    
    // Message handling
    std::map<MessageType, std::function<void(const NetworkMessage&)>> messageHandlers;
    std::queue<NetworkMessage> messageQueue;
    mutable std::mutex messageMutex;
    std::condition_variable messageCV;
    
    // Statistics
    uint64_t totalMessagesReceived;
    uint64_t totalMessagesSent;
    uint64_t totalBytesReceived;
    uint64_t totalBytesSent;
    uint64_t activeConnections;
    uint64_t totalPeers;
    
public:
    NetworkEngine(Blockchain& blockchain, MiningEngine& miningEngine, const NetworkConfig& config = NetworkConfig());
    ~NetworkEngine();
    
    // Network control
    bool start();
    void stop();
    bool isNetworkRunning() const { return isRunning.load(); }
    
    // Peer management
    bool addPeer(const std::string& address, uint16_t port);
    bool removePeer(const std::string& address);
    std::vector<PeerNode> getPeers() const;
    std::vector<PeerNode> getActivePeers() const;
    bool isPeerConnected(const std::string& address) const;
    
    // Message broadcasting
    bool broadcastMessage(const NetworkMessage& message);
    bool sendMessageToPeer(const std::string& peerAddress, const NetworkMessage& message);
    bool broadcastBlock(const Block& block);
    bool broadcastTransaction(const Transaction& transaction);
    
    // Network discovery
    void discoverPeers();
    void pingPeers();
    void syncWithPeers();
    
    // Configuration
    void updateConfig(const NetworkConfig& newConfig);
    NetworkConfig getConfig() const { return config; }
    
    // Statistics
    nlohmann::json getNetworkStats() const;
    uint64_t getActiveConnections() const { return activeConnections; }
    uint64_t getTotalPeers() const { return totalPeers; }
    uint64_t getTotalMessagesReceived() const { return totalMessagesReceived; }
    uint64_t getTotalMessagesSent() const { return totalMessagesSent; }
    
    // Message handlers
    void registerMessageHandler(MessageType type, std::function<void(const NetworkMessage&)> handler);
    void unregisterMessageHandler(MessageType type);
    
private:
    void listenerLoop();
    void discoveryLoop();
    void syncLoop();
    void broadcastLoop();
    void messageProcessingLoop();
    
    // Helper functions
    bool acceptConnection();
    void handleNewConnection(int clientSocket, const std::string& clientAddress, uint16_t clientPort);
    void handleMessage(const NetworkMessage& message);
    void processMessageQueue();
    bool validateMessage(const NetworkMessage& message);
    std::string generateNodeId();
    
    // Message handlers
    void handleHandshake(const NetworkMessage& message);
    void handlePing(const NetworkMessage& message);
    void handlePong(const NetworkMessage& message);
    void handleGetBlocks(const NetworkMessage& message);
    void handleBlocks(const NetworkMessage& message);
    void handleGetTransactions(const NetworkMessage& message);
    void handleTransactions(const NetworkMessage& message);
    void handleNewBlock(const NetworkMessage& message);
    void handleNewTransaction(const NetworkMessage& message);
    void handlePeerList(const NetworkMessage& message);
    void handleAddPeer(const NetworkMessage& message);
    void handleRemovePeer(const NetworkMessage& message);
    void handleMiningRequest(const NetworkMessage& message);
    void handleMiningResponse(const NetworkMessage& message);
    void handleConsensusRequest(const NetworkMessage& message);
    void handleConsensusResponse(const NetworkMessage& message);
};

// Network utilities
class NetworkUtils {
public:
    // Address utilities
    static bool isValidAddress(const std::string& address);
    static bool isValidPort(uint16_t port);
    static std::string resolveHostname(const std::string& hostname);
    static std::vector<std::string> getLocalAddresses();
    
    // Connection utilities
    static int createSocket();
    static bool bindSocket(int socket, const std::string& address, uint16_t port);
    static bool listenSocket(int socket, int backlog = 10);
    static int acceptConnection(int socket, std::string& clientAddress, uint16_t& clientPort);
    static bool connectSocket(int socket, const std::string& address, uint16_t port);
    static void closeSocket(int socket);
    
    // Data transmission
    static bool sendData(int socket, const std::string& data);
    static bool receiveData(int socket, std::string& data, size_t maxSize = 4096);
    static bool sendMessage(int socket, const NetworkMessage& message);
    static bool receiveMessage(int socket, NetworkMessage& message);
    
    // Network discovery
    static std::vector<std::string> discoverLocalPeers(uint16_t port);
    static std::vector<std::string> queryDNSPeers(const std::string& domain);
    static bool isPortOpen(const std::string& address, uint16_t port);
    
    // Security utilities
    static std::string generateNodeId();
    static std::string signMessage(const std::string& message, const std::string& privateKey);
    static bool verifyMessage(const std::string& message, const std::string& signature, const std::string& publicKey);
    static std::string encryptMessage(const std::string& message, const std::string& key);
    static std::string decryptMessage(const std::string& encryptedMessage, const std::string& key);
    
    // Compression utilities
    static std::string compressData(const std::string& data);
    static std::string decompressData(const std::string& compressedData);
    
    // Protocol utilities
    static std::string createHandshakeMessage(const std::string& nodeId, uint32_t version);
    static bool validateHandshake(const NetworkMessage& message);
    static std::string createPingMessage();
    static std::string createPongMessage(const std::string& pingData);
    static std::string createGetBlocksMessage(uint64_t startHeight, uint64_t endHeight);
    static std::string createBlocksMessage(const std::vector<Block>& blocks);
    static std::string createGetTransactionsMessage(const std::vector<std::string>& transactionIds);
    static std::string createTransactionsMessage(const std::vector<Transaction>& transactions);
    static std::string createNewBlockMessage(const Block& block);
    static std::string createNewTransactionMessage(const Transaction& transaction);
    static std::string createPeerListMessage(const std::vector<PeerNode>& peers);
    static std::string createAddPeerMessage(const PeerNode& peer);
    static std::string createRemovePeerMessage(const std::string& peerAddress);
    static std::string createMiningRequestMessage(const std::string& minerAddress, uint64_t difficulty);
    static std::string createMiningResponseMessage(const Block& block, bool success);
    static std::string createConsensusRequestMessage(uint64_t blockHeight);
    static std::string createConsensusResponseMessage(uint64_t blockHeight, bool consensus);
};

#endif // NETWORKING_H 