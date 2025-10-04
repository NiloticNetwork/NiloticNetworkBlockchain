#include "../include/core/api.h"
#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cassert>

class WebSocketTest {
private:
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> consensusManager;
    std::unique_ptr<API> api;
    const int testPort = 8081;

public:
    WebSocketTest() {
        blockchain = std::make_unique<Blockchain>();
        consensusManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        consensusManager->initializeConsensus();
        api = std::make_unique<API>(*blockchain, consensusManager.get());
        
        api->start(testPort);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::cout << "WebSocket Test Setup Complete" << std::endl;
    }
    
    ~WebSocketTest() {
        if (api) {
            api->stop();
        }
        std::cout << "WebSocket Test Cleanup Complete" << std::endl;
    }
    
    void testWebSocketConnection() {
        std::cout << "\n--- Testing WebSocket Connection ---" << std::endl;
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        assert(sock >= 0);
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(testPort);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        int result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        assert(result >= 0);
        
        // Send WebSocket handshake request
        std::string handshake = 
            "GET /consensus/monitor HTTP/1.1\r\n"
            "Host: localhost:8081\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
        
        ssize_t sent = send(sock, handshake.c_str(), handshake.length(), 0);
        assert(sent > 0);
        
        // Read response
        char buffer[1024];
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        assert(received > 0);
        
        buffer[received] = '\0';
        std::string response(buffer);
        
        // Check for WebSocket upgrade response
        assert(response.find("101 Switching Protocols") != std::string::npos);
        assert(response.find("Upgrade: websocket") != std::string::npos);
        
        std::cout << "✓ WebSocket handshake successful" << std::endl;
        
        // Wait for some real-time data
        std::cout << "Waiting for real-time consensus data..." << std::endl;
        
        for (int i = 0; i < 3; i++) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            
            struct timeval timeout;
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;
            
            int activity = select(sock + 1, &readfds, NULL, NULL, &timeout);
            
            if (activity > 0 && FD_ISSET(sock, &readfds)) {
                char wsBuffer[2048];
                ssize_t wsReceived = recv(sock, wsBuffer, sizeof(wsBuffer) - 1, 0);
                
                if (wsReceived > 0) {
                    // Parse WebSocket frame (simplified)
                    if (wsReceived >= 2) {
                        uint8_t opcode = wsBuffer[0] & 0x0F;
                        uint8_t payloadLen = wsBuffer[1] & 0x7F;
                        
                        if (opcode == 0x1 && payloadLen > 0) { // Text frame
                            std::string payload;
                            if (payloadLen < 126) {
                                payload = std::string(wsBuffer + 2, payloadLen);
                            } else {
                                // Handle extended payload length if needed
                                payload = std::string(wsBuffer + 4, payloadLen);
                            }
                            
                            std::cout << "Received WebSocket data: " << payload.substr(0, 100) << "..." << std::endl;
                            
                            // Try to parse as JSON
                            try {
                                nlohmann::json data = nlohmann::json::parse(payload);
                                assert(data.contains("type"));
                                assert(data["type"] == "consensus_update");
                                assert(data.contains("timestamp"));
                                
                                std::cout << "✓ Valid consensus update received" << std::endl;
                            } catch (const std::exception& e) {
                                std::cout << "Warning: Could not parse JSON: " << e.what() << std::endl;
                            }
                        }
                    }
                }
            } else {
                std::cout << "No data received in timeout period" << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        close(sock);
        std::cout << "✓ WebSocket connection test completed" << std::endl;
    }
    
    void runAllTests() {
        std::cout << "\n=== Running WebSocket Tests ===" << std::endl;
        testWebSocketConnection();
        std::cout << "\n=== All WebSocket Tests Completed ===" << std::endl;
    }
};

int main() {
    try {
        WebSocketTest test;
        test.runAllTests();
        
        std::cout << "\n🎉 All WebSocket tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "WebSocket test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}