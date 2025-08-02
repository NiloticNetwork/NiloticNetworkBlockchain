#include "networking.h"
#include "utils.h"
#include "logger.h"
#include <algorithm>
#include <random>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>

// NetworkMessage implementation
std::string NetworkMessage::serialize() const {
    nlohmann::json json;
    json["type"] = static_cast<int>(type);
    json["sender"] = sender;
    json["recipient"] = recipient;
    json["timestamp"] = timestamp;
    json["sequence"] = sequence;
    json["data"] = data;
    json["signature"] = signature;
    return json.dump();
}

NetworkMessage NetworkMessage::deserialize(const std::string& data) {
    NetworkMessage message;
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        message.type = static_cast<MessageType>(json["type"].get<int>());
        message.sender = json["sender"];
        message.recipient = json["recipient"];
        message.timestamp = json["timestamp"];
        message.sequence = json["sequence"];
        message.data = json["data"];
        message.signature = json["signature"];
    } catch (const std::exception& e) {
        Logger::error("Failed to deserialize network message: " + std::string(e.what()));
    }
    return message;
}

std::string NetworkMessage::calculateHash() const {
    std::stringstream ss;
    ss << static_cast<int>(type) << sender << recipient << timestamp << sequence << data.dump();
    return Utils::calculateSHA256(ss.str());
}

bool NetworkMessage::isValid() const {
    return !sender.empty() && timestamp > 0;
}

// PeerNode implementation
std::string PeerNode::getFullAddress() const {
    return address + ":" + std::to_string(port);
}

bool PeerNode::isActive() const {
    auto now = std::chrono::system_clock::now();
    auto lastSeenTime = std::chrono::system_clock::from_time_t(lastSeen);
    auto timeSinceLastSeen = std::chrono::duration_cast<std::chrono::minutes>(now - lastSeenTime).count();
    return isConnected && timeSinceLastSeen < 10; // Consider active if seen in last 10 minutes
}

nlohmann::json PeerNode::toJson() const {
    nlohmann::json json;
    json["address"] = address;
    json["port"] = port;
    json["nodeId"] = nodeId;
    json["version"] = version;
    json["lastSeen"] = lastSeen;
    json["isConnected"] = isConnected;
    json["latency"] = latency;
    json["blockHeight"] = blockHeight;
    json["capabilities"] = capabilities;
    return json;
}

PeerNode PeerNode::fromJson(const nlohmann::json& json) {
    PeerNode peer;
    peer.address = json["address"];
    peer.port = json["port"];
    peer.nodeId = json["nodeId"];
    peer.version = json["version"];
    peer.lastSeen = json["lastSeen"];
    peer.isConnected = json["isConnected"];
    peer.latency = json["latency"];
    peer.blockHeight = json["blockHeight"];
    peer.capabilities = json["capabilities"].get<std::vector<std::string>>();
    return peer;
}

// NetworkConnection implementation
NetworkConnection::NetworkConnection(int fd, const std::string& address, uint16_t port)
    : socketFd(fd), remoteAddress(address), remotePort(port), state(ConnectionState::DISCONNECTED),
      shouldClose(false), bytesReceived(0), bytesSent(0), messagesReceived(0), messagesSent(0) {
    updateActivity();
}

NetworkConnection::~NetworkConnection() {
    disconnect();
}

bool NetworkConnection::connect() {
    if (state != ConnectionState::DISCONNECTED) {
        return false;
    }
    
    state = ConnectionState::CONNECTING;
    
    // Set socket to non-blocking
    int flags = fcntl(socketFd, F_GETFL, 0);
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);
    
    // Connect to remote address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remotePort);
    addr.sin_addr.s_addr = inet_addr(remoteAddress.c_str());
    
    int result = ::connect(socketFd, (struct sockaddr*)&addr, sizeof(addr));
    if (result < 0 && errno != EINPROGRESS) {
        Logger::error("Failed to connect to " + getFullAddress());
        state = ConnectionState::DISCONNECTED;
        return false;
    }
    
    // Wait for connection to complete
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socketFd, &writefds);
    
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    result = select(socketFd + 1, nullptr, &writefds, nullptr, &timeout);
    if (result <= 0) {
        Logger::error("Connection timeout to " + getFullAddress());
        state = ConnectionState::DISCONNECTED;
        return false;
    }
    
    state = ConnectionState::CONNECTED;
    updateActivity();
    
    // Start read and write threads
    readThread = std::thread(&NetworkConnection::readLoop, this);
    writeThread = std::thread(&NetworkConnection::writeLoop, this);
    
    Logger::info("Connected to " + getFullAddress());
    return true;
}

void NetworkConnection::disconnect() {
    if (state == ConnectionState::DISCONNECTED) {
        return;
    }
    
    shouldClose = true;
    state = ConnectionState::DISCONNECTING;
    
    // Wake up threads
    queueCV.notify_all();
    
    if (readThread.joinable()) {
        readThread.join();
    }
    if (writeThread.joinable()) {
        writeThread.join();
    }
    
    NetworkUtils::closeSocket(socketFd);
    state = ConnectionState::DISCONNECTED;
    
    Logger::info("Disconnected from " + getFullAddress());
}

bool NetworkConnection::sendMessage(const NetworkMessage& message) {
    if (state != ConnectionState::CONNECTED) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(queueMutex);
    sendQueue.push(message);
    queueCV.notify_one();
    return true;
}

bool NetworkConnection::receiveMessage(NetworkMessage& message) {
    if (state != ConnectionState::CONNECTED) {
        return false;
    }
    
    std::string data;
    if (!receiveData(data)) {
        return false;
    }
    
    message = NetworkMessage::deserialize(data);
    if (message.isValid()) {
        messagesReceived++;
        updateActivity();
        return true;
    }
    
    return false;
}

void NetworkConnection::processMessage(const NetworkMessage& message) {
    // Process the message based on its type
    switch (message.type) {
        case MessageType::HANDSHAKE:
            Logger::debug("Received handshake from " + message.sender);
            break;
        case MessageType::PING: {
            Logger::debug("Received ping from " + message.sender);
            // Send pong response
            NetworkMessage pong;
            pong.type = MessageType::PONG;
            pong.sender = message.recipient;
            pong.recipient = message.sender;
            pong.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            pong.data = message.data;
            sendMessage(pong);
            break;
        }
        case MessageType::PONG: {
            Logger::debug("Received pong from " + message.sender);
            break;
        }
        case MessageType::NEW_BLOCK: {
            Logger::info("Received new block from " + message.sender);
            break;
        }
        case MessageType::NEW_TRANSACTION: {
            Logger::debug("Received new transaction from " + message.sender);
            break;
        }
        default: {
            Logger::debug("Received message type " + std::to_string(static_cast<int>(message.type)) + " from " + message.sender);
            break;
        }
    }
}

void NetworkConnection::readLoop() {
    while (!shouldClose && state == ConnectionState::CONNECTED) {
        NetworkMessage message;
        if (receiveMessage(message)) {
            processMessage(message);
        } else {
            // Connection lost
            break;
        }
    }
}

void NetworkConnection::writeLoop() {
    while (!shouldClose && state == ConnectionState::CONNECTED) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this] { return !sendQueue.empty() || shouldClose; });
        
        while (!sendQueue.empty() && !shouldClose) {
            NetworkMessage message = sendQueue.front();
            sendQueue.pop();
            lock.unlock();
            
            if (!sendMessage(message)) {
                Logger::error("Failed to send message to " + getFullAddress());
                break;
            }
            
            lock.lock();
        }
    }
}

bool NetworkConnection::sendData(const std::string& data) {
    ssize_t bytesSent = send(socketFd, data.c_str(), data.length(), 0);
    if (bytesSent > 0) {
        this->bytesSent += bytesSent;
        updateActivity();
        return true;
    }
    return false;
}

bool NetworkConnection::receiveData(std::string& data) {
    char buffer[4096];
    ssize_t bytesReceived = recv(socketFd, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        data.assign(buffer, bytesReceived);
        this->bytesReceived += bytesReceived;
        updateActivity();
        return true;
    }
    return false;
}

void NetworkConnection::updateActivity() {
    lastActivity = std::chrono::steady_clock::now();
}

// NetworkEngine implementation
NetworkEngine::NetworkEngine(Blockchain& blockchain, MiningEngine& miningEngine, const NetworkConfig& config)
    : blockchain(blockchain), miningEngine(miningEngine), config(config), isRunning(false),
      listenerSocket(-1), totalMessagesReceived(0), totalMessagesSent(0), totalBytesReceived(0),
      totalBytesSent(0), activeConnections(0), totalPeers(0) {
    
    // Register default message handlers
    registerMessageHandler(MessageType::HANDSHAKE, [this](const NetworkMessage& msg) { handleHandshake(msg); });
    registerMessageHandler(MessageType::PING, [this](const NetworkMessage& msg) { handlePing(msg); });
    registerMessageHandler(MessageType::PONG, [this](const NetworkMessage& msg) { handlePong(msg); });
    registerMessageHandler(MessageType::GET_BLOCKS, [this](const NetworkMessage& msg) { handleGetBlocks(msg); });
    registerMessageHandler(MessageType::BLOCKS, [this](const NetworkMessage& msg) { handleBlocks(msg); });
    registerMessageHandler(MessageType::GET_TRANSACTIONS, [this](const NetworkMessage& msg) { handleGetTransactions(msg); });
    registerMessageHandler(MessageType::TRANSACTIONS, [this](const NetworkMessage& msg) { handleTransactions(msg); });
    registerMessageHandler(MessageType::NEW_BLOCK, [this](const NetworkMessage& msg) { handleNewBlock(msg); });
    registerMessageHandler(MessageType::NEW_TRANSACTION, [this](const NetworkMessage& msg) { handleNewTransaction(msg); });
    registerMessageHandler(MessageType::PEER_LIST, [this](const NetworkMessage& msg) { handlePeerList(msg); });
    registerMessageHandler(MessageType::ADD_PEER, [this](const NetworkMessage& msg) { handleAddPeer(msg); });
    registerMessageHandler(MessageType::REMOVE_PEER, [this](const NetworkMessage& msg) { handleRemovePeer(msg); });
    registerMessageHandler(MessageType::MINING_REQUEST, [this](const NetworkMessage& msg) { handleMiningRequest(msg); });
    registerMessageHandler(MessageType::MINING_RESPONSE, [this](const NetworkMessage& msg) { handleMiningResponse(msg); });
    registerMessageHandler(MessageType::CONSENSUS_REQUEST, [this](const NetworkMessage& msg) { handleConsensusRequest(msg); });
    registerMessageHandler(MessageType::CONSENSUS_RESPONSE, [this](const NetworkMessage& msg) { handleConsensusResponse(msg); });
    
    Logger::info("Network engine initialized");
}

NetworkEngine::~NetworkEngine() {
    stop();
}

bool NetworkEngine::start() {
    if (isRunning) {
        return false;
    }
    
    // Create listener socket
    listenerSocket = NetworkUtils::createSocket();
    if (listenerSocket < 0) {
        Logger::error("Failed to create listener socket");
        return false;
    }
    
    // Bind socket
    if (!NetworkUtils::bindSocket(listenerSocket, config.bindAddress, config.listenPort)) {
        Logger::error("Failed to bind socket to " + config.bindAddress + ":" + std::to_string(config.listenPort));
        NetworkUtils::closeSocket(listenerSocket);
        return false;
    }
    
    // Listen for connections
    if (!NetworkUtils::listenSocket(listenerSocket)) {
        Logger::error("Failed to listen on socket");
        NetworkUtils::closeSocket(listenerSocket);
        return false;
    }
    
    isRunning = true;
    
    // Start network threads
    listenerThread = std::thread(&NetworkEngine::listenerLoop, this);
    discoveryThread = std::thread(&NetworkEngine::discoveryLoop, this);
    syncThread = std::thread(&NetworkEngine::syncLoop, this);
    broadcastThread = std::thread(&NetworkEngine::broadcastLoop, this);
    messageProcessingThread = std::thread(&NetworkEngine::messageProcessingLoop, this);
    
    Logger::info("Network engine started on port " + std::to_string(config.listenPort));
    return true;
}

void NetworkEngine::stop() {
    if (!isRunning) {
        return;
    }
    
    isRunning = false;
    
    // Close listener socket
    if (listenerSocket >= 0) {
        NetworkUtils::closeSocket(listenerSocket);
        listenerSocket = -1;
    }
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        for (auto& connection : connections) {
            connection->disconnect();
        }
        connections.clear();
    }
    
    // Wait for threads to finish
    if (listenerThread.joinable()) {
        listenerThread.join();
    }
    if (discoveryThread.joinable()) {
        discoveryThread.join();
    }
    if (syncThread.joinable()) {
        syncThread.join();
    }
    if (broadcastThread.joinable()) {
        broadcastThread.join();
    }
    if (messageProcessingThread.joinable()) {
        messageProcessingThread.join();
    }
    
    Logger::info("Network engine stopped");
}

bool NetworkEngine::addPeer(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(peersMutex);
    
    std::string key = address + ":" + std::to_string(port);
    if (peers.find(key) != peers.end()) {
        return false;
    }
    
    PeerNode peer;
    peer.address = address;
    peer.port = port;
    peer.nodeId = NetworkUtils::generateNodeId();
    peer.version = "1.0.0";
    peer.lastSeen = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    peer.isConnected = false;
    
    peers[key] = peer;
    totalPeers++;
    
    Logger::info("Added peer: " + key);
    return true;
}

bool NetworkEngine::removePeer(const std::string& address) {
    std::lock_guard<std::mutex> lock(peersMutex);
    
    auto it = peers.find(address);
    if (it == peers.end()) {
        return false;
    }
    
    peers.erase(it);
    totalPeers--;
    
    Logger::info("Removed peer: " + address);
    return true;
}

std::vector<PeerNode> NetworkEngine::getPeers() const {
    std::lock_guard<std::mutex> lock(peersMutex);
    std::vector<PeerNode> result;
    for (const auto& pair : peers) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<PeerNode> NetworkEngine::getActivePeers() const {
    std::lock_guard<std::mutex> lock(peersMutex);
    std::vector<PeerNode> result;
    for (const auto& pair : peers) {
        if (pair.second.isActive()) {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool NetworkEngine::isPeerConnected(const std::string& address) const {
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = peers.find(address);
    return it != peers.end() && it->second.isConnected;
}

bool NetworkEngine::broadcastMessage(const NetworkMessage& message) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    
    bool success = false;
    for (auto& connection : connections) {
        if (connection->isConnected()) {
            if (connection->sendMessage(message)) {
                success = true;
            }
        }
    }
    
    if (success) {
        totalMessagesSent++;
    }
    
    return success;
}

bool NetworkEngine::sendMessageToPeer(const std::string& peerAddress, const NetworkMessage& message) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    
    for (auto& connection : connections) {
        if (connection->getRemoteAddress() == peerAddress && connection->isConnected()) {
            if (connection->sendMessage(message)) {
                totalMessagesSent++;
                return true;
            }
        }
    }
    
    return false;
}

bool NetworkEngine::broadcastBlock(const Block& block) {
    NetworkMessage message;
    message.type = MessageType::NEW_BLOCK;
    message.sender = generateNodeId();
    message.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    message.data = nlohmann::json::parse(block.serialize());
    
    return broadcastMessage(message);
}

bool NetworkEngine::broadcastTransaction(const Transaction& transaction) {
    NetworkMessage message;
    message.type = MessageType::NEW_TRANSACTION;
    message.sender = generateNodeId();
    message.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    message.data = nlohmann::json::parse(transaction.serialize());
    
    return broadcastMessage(message);
}

void NetworkEngine::discoverPeers() {
    // Query seed nodes
    for (const auto& seedNode : config.seedNodes) {
        auto discoveredPeers = NetworkUtils::queryDNSPeers(seedNode);
        for (const auto& peerAddress : discoveredPeers) {
            size_t colonPos = peerAddress.find(':');
            if (colonPos != std::string::npos) {
                std::string address = peerAddress.substr(0, colonPos);
                uint16_t port = std::stoi(peerAddress.substr(colonPos + 1));
                addPeer(address, port);
            }
        }
    }
    
    // Discover local peers
    auto localPeers = NetworkUtils::discoverLocalPeers(config.listenPort);
    for (const auto& peerAddress : localPeers) {
        size_t colonPos = peerAddress.find(':');
        if (colonPos != std::string::npos) {
            std::string address = peerAddress.substr(0, colonPos);
            uint16_t port = std::stoi(peerAddress.substr(colonPos + 1));
            addPeer(address, port);
        }
    }
}

void NetworkEngine::pingPeers() {
    NetworkMessage ping;
    ping.type = MessageType::PING;
    ping.sender = generateNodeId();
    ping.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    ping.data = {{"timestamp", ping.timestamp}};
    
    broadcastMessage(ping);
}

void NetworkEngine::syncWithPeers() {
    // Request blocks from peers
    NetworkMessage getBlocks;
    getBlocks.type = MessageType::GET_BLOCKS;
    getBlocks.sender = generateNodeId();
    getBlocks.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    getBlocks.data = {
        {"startHeight", blockchain.getChainHeight()},
        {"endHeight", blockchain.getChainHeight() + 100}
    };
    
    broadcastMessage(getBlocks);
}

void NetworkEngine::updateConfig(const NetworkConfig& newConfig) {
    config = newConfig;
    Logger::info("Network configuration updated");
}

nlohmann::json NetworkEngine::getNetworkStats() const {
    nlohmann::json stats;
    stats["isRunning"] = isRunning.load();
    stats["activeConnections"] = activeConnections;
    stats["totalPeers"] = totalPeers;
    stats["totalMessagesReceived"] = totalMessagesReceived;
    stats["totalMessagesSent"] = totalMessagesSent;
    stats["totalBytesReceived"] = totalBytesReceived;
    stats["totalBytesSent"] = totalBytesSent;
    stats["listenPort"] = config.listenPort;
    stats["bindAddress"] = config.bindAddress;
    stats["maxPeers"] = config.maxPeers;
    return stats;
}

void NetworkEngine::registerMessageHandler(MessageType type, std::function<void(const NetworkMessage&)> handler) {
    messageHandlers[type] = handler;
}

void NetworkEngine::unregisterMessageHandler(MessageType type) {
    messageHandlers.erase(type);
}

void NetworkEngine::listenerLoop() {
    while (isRunning) {
        if (!acceptConnection()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NetworkEngine::discoveryLoop() {
    while (isRunning) {
        discoverPeers();
        std::this_thread::sleep_for(std::chrono::seconds(config.peerDiscoveryInterval));
    }
}

void NetworkEngine::syncLoop() {
    while (isRunning) {
        syncWithPeers();
        std::this_thread::sleep_for(std::chrono::seconds(config.blockSyncInterval));
    }
}

void NetworkEngine::broadcastLoop() {
    while (isRunning) {
        pingPeers();
        std::this_thread::sleep_for(std::chrono::seconds(config.pingInterval));
    }
}

void NetworkEngine::messageProcessingLoop() {
    while (isRunning) {
        processMessageQueue();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool NetworkEngine::acceptConnection() {
    std::string clientAddress;
    uint16_t clientPort;
    
    int clientSocket = NetworkUtils::acceptConnection(listenerSocket, clientAddress, clientPort);
    if (clientSocket < 0) {
        return false;
    }
    
    handleNewConnection(clientSocket, clientAddress, clientPort);
    return true;
}

void NetworkEngine::handleNewConnection(int clientSocket, const std::string& clientAddress, uint16_t clientPort) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    
    if (connections.size() >= config.maxConnections) {
        Logger::warning("Max connections reached, rejecting connection from " + clientAddress);
        NetworkUtils::closeSocket(clientSocket);
        return;
    }
    
    auto connection = std::make_unique<NetworkConnection>(clientSocket, clientAddress, clientPort);
    if (connection->connect()) {
        connections.push_back(std::move(connection));
        activeConnections++;
        Logger::info("New connection from " + clientAddress + ":" + std::to_string(clientPort));
    }
}

void NetworkEngine::handleMessage(const NetworkMessage& message) {
    auto it = messageHandlers.find(message.type);
    if (it != messageHandlers.end()) {
        it->second(message);
    } else {
        Logger::warning("No handler for message type: " + std::to_string(static_cast<int>(message.type)));
    }
}

void NetworkEngine::processMessageQueue() {
    std::lock_guard<std::mutex> lock(messageMutex);
    
    while (!messageQueue.empty()) {
        NetworkMessage message = messageQueue.front();
        messageQueue.pop();
        
        handleMessage(message);
    }
}

bool NetworkEngine::validateMessage(const NetworkMessage& message) {
    return message.isValid() && !message.sender.empty();
}

std::string NetworkEngine::generateNodeId() {
    return NetworkUtils::generateNodeId();
}

// Message handlers implementation
void NetworkEngine::handleHandshake(const NetworkMessage& message) {
    Logger::info("Handshake from " + message.sender);
    // TODO: Implement handshake logic
}

void NetworkEngine::handlePing(const NetworkMessage& message) {
    Logger::debug("Ping from " + message.sender);
    // TODO: Implement ping response
}

void NetworkEngine::handlePong(const NetworkMessage& message) {
    Logger::debug("Pong from " + message.sender);
    // TODO: Update peer latency
}

void NetworkEngine::handleGetBlocks(const NetworkMessage& message) {
    Logger::info("Get blocks request from " + message.sender);
    // TODO: Send blocks to peer
}

void NetworkEngine::handleBlocks(const NetworkMessage& message) {
    Logger::info("Received blocks from " + message.sender);
    // TODO: Process received blocks
}

void NetworkEngine::handleGetTransactions(const NetworkMessage& message) {
    Logger::info("Get transactions request from " + message.sender);
    // TODO: Send transactions to peer
}

void NetworkEngine::handleTransactions(const NetworkMessage& message) {
    Logger::info("Received transactions from " + message.sender);
    // TODO: Process received transactions
}

void NetworkEngine::handleNewBlock(const NetworkMessage& message) {
    Logger::info("New block from " + message.sender);
    // TODO: Process new block
}

void NetworkEngine::handleNewTransaction(const NetworkMessage& message) {
    Logger::info("New transaction from " + message.sender);
    // TODO: Process new transaction
}

void NetworkEngine::handlePeerList(const NetworkMessage& message) {
    Logger::info("Peer list from " + message.sender);
    // TODO: Process peer list
}

void NetworkEngine::handleAddPeer(const NetworkMessage& message) {
    Logger::info("Add peer request from " + message.sender);
    // TODO: Add peer to network
}

void NetworkEngine::handleRemovePeer(const NetworkMessage& message) {
    Logger::info("Remove peer request from " + message.sender);
    // TODO: Remove peer from network
}

void NetworkEngine::handleMiningRequest(const NetworkMessage& message) {
    Logger::info("Mining request from " + message.sender);
    // TODO: Process mining request
}

void NetworkEngine::handleMiningResponse(const NetworkMessage& message) {
    Logger::info("Mining response from " + message.sender);
    // TODO: Process mining response
}

void NetworkEngine::handleConsensusRequest(const NetworkMessage& message) {
    Logger::info("Consensus request from " + message.sender);
    // TODO: Process consensus request
}

void NetworkEngine::handleConsensusResponse(const NetworkMessage& message) {
    Logger::info("Consensus response from " + message.sender);
    // TODO: Process consensus response
}

// NetworkUtils implementation
bool NetworkUtils::isValidAddress(const std::string& address) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) == 1;
}

bool NetworkUtils::isValidPort(uint16_t port) {
    return port > 0 && port < 65536;
}

std::string NetworkUtils::resolveHostname(const std::string& hostname) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(hostname.c_str(), nullptr, &hints, &result) != 0) {
        return "";
    }
    
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
    
    freeaddrinfo(result);
    return std::string(ip);
}

std::vector<std::string> NetworkUtils::getLocalAddresses() {
    std::vector<std::string> addresses;
    addresses.push_back("127.0.0.1");
    addresses.push_back("localhost");
    return addresses;
}

int NetworkUtils::createSocket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

bool NetworkUtils::bindSocket(int socket, const std::string& address, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    
    return ::bind(socket, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

bool NetworkUtils::listenSocket(int socket, int backlog) {
    return listen(socket, backlog) == 0;
}

int NetworkUtils::acceptConnection(int socket, std::string& clientAddress, uint16_t& clientPort) {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientSocket = accept(socket, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientSocket < 0) {
        return -1;
    }
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
    clientAddress = std::string(ip);
    clientPort = ntohs(clientAddr.sin_port);
    
    return clientSocket;
}

bool NetworkUtils::connectSocket(int socket, const std::string& address, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    
    return ::connect(socket, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

void NetworkUtils::closeSocket(int socket) {
    close(socket);
}

bool NetworkUtils::sendData(int socket, const std::string& data) {
    ssize_t bytesSent = send(socket, data.c_str(), data.length(), 0);
    return bytesSent == static_cast<ssize_t>(data.length());
}

bool NetworkUtils::receiveData(int socket, std::string& data, size_t maxSize) {
    char buffer[4096];
    ssize_t bytesReceived = recv(socket, buffer, std::min(sizeof(buffer), maxSize), 0);
    if (bytesReceived > 0) {
        data.assign(buffer, bytesReceived);
        return true;
    }
    return false;
}

bool NetworkUtils::sendMessage(int socket, const NetworkMessage& message) {
    std::string data = message.serialize();
    return sendData(socket, data);
}

bool NetworkUtils::receiveMessage(int socket, NetworkMessage& message) {
    std::string data;
    if (!receiveData(socket, data)) {
        return false;
    }
    
    message = NetworkMessage::deserialize(data);
    return message.isValid();
}

std::vector<std::string> NetworkUtils::discoverLocalPeers(uint16_t port) {
    std::vector<std::string> peers;
    // TODO: Implement local peer discovery
    return peers;
}

std::vector<std::string> NetworkUtils::queryDNSPeers(const std::string& domain) {
    std::vector<std::string> peers;
    // TODO: Implement DNS peer discovery
    return peers;
}

bool NetworkUtils::isPortOpen(const std::string& address, uint16_t port) {
    int socket = createSocket();
    if (socket < 0) {
        return false;
    }
    
    bool isOpen = connectSocket(socket, address, port);
    closeSocket(socket);
    return isOpen;
}

std::string NetworkUtils::generateNodeId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    
    return ss.str();
}

std::string NetworkUtils::signMessage(const std::string& message, const std::string& privateKey) {
    // TODO: Implement message signing
    return Utils::calculateSHA256(message + privateKey);
}

bool NetworkUtils::verifyMessage(const std::string& message, const std::string& signature, const std::string& publicKey) {
    // TODO: Implement message verification
    return true;
}

std::string NetworkUtils::encryptMessage(const std::string& message, const std::string& key) {
    // TODO: Implement message encryption
    return message;
}

std::string NetworkUtils::decryptMessage(const std::string& encryptedMessage, const std::string& key) {
    // TODO: Implement message decryption
    return encryptedMessage;
}

std::string NetworkUtils::compressData(const std::string& data) {
    // TODO: Implement data compression
    return data;
}

std::string NetworkUtils::decompressData(const std::string& compressedData) {
    // TODO: Implement data decompression
    return compressedData;
}

// Protocol utilities
std::string NetworkUtils::createHandshakeMessage(const std::string& nodeId, uint32_t version) {
    nlohmann::json data;
    data["nodeId"] = nodeId;
    data["version"] = version;
    data["capabilities"] = {"blocks", "transactions", "mining"};
    return data.dump();
}

bool NetworkUtils::validateHandshake(const NetworkMessage& message) {
    return message.type == MessageType::HANDSHAKE && message.data.contains("nodeId");
}

std::string NetworkUtils::createPingMessage() {
    nlohmann::json data;
    data["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return data.dump();
}

std::string NetworkUtils::createPongMessage(const std::string& pingData) {
    return pingData; // Echo the ping data
}

std::string NetworkUtils::createGetBlocksMessage(uint64_t startHeight, uint64_t endHeight) {
    nlohmann::json data;
    data["startHeight"] = startHeight;
    data["endHeight"] = endHeight;
    return data.dump();
}

std::string NetworkUtils::createBlocksMessage(const std::vector<Block>& blocks) {
    nlohmann::json data;
    data["blocks"] = nlohmann::json::array();
    for (const auto& block : blocks) {
        data["blocks"].push_back(nlohmann::json::parse(block.serialize()));
    }
    return data.dump();
}

std::string NetworkUtils::createGetTransactionsMessage(const std::vector<std::string>& transactionIds) {
    nlohmann::json data;
    data["transactionIds"] = transactionIds;
    return data.dump();
}

std::string NetworkUtils::createTransactionsMessage(const std::vector<Transaction>& transactions) {
    nlohmann::json data;
    data["transactions"] = nlohmann::json::array();
    for (const auto& tx : transactions) {
        data["transactions"].push_back(nlohmann::json::parse(tx.serialize()));
    }
    return data.dump();
}

std::string NetworkUtils::createNewBlockMessage(const Block& block) {
    nlohmann::json data;
    data["block"] = nlohmann::json::parse(block.serialize());
    return data.dump();
}

std::string NetworkUtils::createNewTransactionMessage(const Transaction& transaction) {
    nlohmann::json data;
    data["transaction"] = nlohmann::json::parse(transaction.serialize());
    return data.dump();
}

std::string NetworkUtils::createPeerListMessage(const std::vector<PeerNode>& peers) {
    nlohmann::json data;
    data["peers"] = nlohmann::json::array();
    for (const auto& peer : peers) {
        data["peers"].push_back(peer.toJson());
    }
    return data.dump();
}

std::string NetworkUtils::createAddPeerMessage(const PeerNode& peer) {
    nlohmann::json data;
    data["peer"] = peer.toJson();
    return data.dump();
}

std::string NetworkUtils::createRemovePeerMessage(const std::string& peerAddress) {
    nlohmann::json data;
    data["peerAddress"] = peerAddress;
    return data.dump();
}

std::string NetworkUtils::createMiningRequestMessage(const std::string& minerAddress, uint64_t difficulty) {
    nlohmann::json data;
    data["minerAddress"] = minerAddress;
    data["difficulty"] = difficulty;
    return data.dump();
}

std::string NetworkUtils::createMiningResponseMessage(const Block& block, bool success) {
    nlohmann::json data;
    data["success"] = success;
    if (success) {
        data["block"] = nlohmann::json::parse(block.serialize());
    }
    return data.dump();
}

std::string NetworkUtils::createConsensusRequestMessage(uint64_t blockHeight) {
    nlohmann::json data;
    data["blockHeight"] = blockHeight;
    return data.dump();
}

std::string NetworkUtils::createConsensusResponseMessage(uint64_t blockHeight, bool consensus) {
    nlohmann::json data;
    data["blockHeight"] = blockHeight;
    data["consensus"] = consensus;
    return data.dump();
} 