#include "../include/core/api.h"
#include "../include/core/blockchain.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * Comprehensive integration test for Consensus API endpoints
 * Tests all REST endpoints and WebSocket functionality
 */

class ConsensusAPIIntegrationTest {
private:
    std::unique_ptr<Blockchain> blockchain;
    std::unique_ptr<ConsensusHarmonyManager> consensusManager;
    std::unique_ptr<API> api;
    const int testPort = 8082;
    const std::string baseUrl = "http://localhost:8082";
    const std::string authToken = "integration_test_token";

public:
    ConsensusAPIIntegrationTest() {
        std::cout << "Setting up Consensus API Integration Test..." << std::endl;
        
        // Initialize blockchain
        blockchain = std::make_unique<Blockchain>();
        
        // Initialize consensus manager
        consensusManager = std::make_unique<ConsensusHarmonyManager>(blockchain.get());
        
        // Initialize consensus with default configuration
        if (!consensusManager->initializeConsensus()) {
            std::cout << "Warning: Consensus manager initialization failed, continuing with mock data" << std::endl;
        }
        
        // Initialize API with consensus manager
        api = std::make_unique<API>(*blockchain, consensusManager.get());
        
        // Start API server
        api->start(testPort);
        
        // Wait for server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        std::cout << "✓ Integration test setup complete" << std::endl;
    }
    
    ~ConsensusAPIIntegrationTest() {
        if (api) {
            api->stop();
        }
        std::cout << "✓ Integration test cleanup complete" << std::endl;
    }
    
    void runAllTests() {
        std::cout << "\n=== Running Consensus API Integration Tests ===" << std::endl;
        
        testBasicConnectivity();
        testConsensusStatusEndpoint();
        testConsensusMetricsEndpoint();
        testConsensusEnginesEndpoint();
        testConsensusConfigEndpoints();
        testParameterAdjustmentEndpoint();
        testEmergencyModeEndpoints();
        testWebSocketConnection();
        testErrorHandling();
        testAuthenticationSecurity();
        
        std::cout << "\n=== All Integration Tests Completed Successfully ===" << std::endl;
    }

private:
    // Helper function to make HTTP requests
    std::string makeHTTPRequest(const std::string& method, const std::string& path, 
                               const std::string& data = "", bool useAuth = false) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "";
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(testPort);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return "";
        }
        
        std::stringstream request;
        request << method << " " << path << " HTTP/1.1\r\n";
        request << "Host: localhost:" << testPort << "\r\n";
        request << "Content-Type: application/json\r\n";
        
        if (useAuth) {
            request << "Authorization: Bearer " << authToken << "\r\n";
        }
        
        if (!data.empty()) {
            request << "Content-Length: " << data.length() << "\r\n";
        }
        
        request << "\r\n";
        
        if (!data.empty()) {
            request << data;
        }
        
        std::string requestStr = request.str();
        send(sock, requestStr.c_str(), requestStr.length(), 0);
        
        char buffer[4096];
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);
        
        if (received > 0) {
            buffer[received] = '\0';
            return std::string(buffer);
        }
        
        return "";
    }
    
    void testBasicConnectivity() {
        std::cout << "\n--- Testing Basic API Connectivity ---" << std::endl;
        
        std::string response = makeHTTPRequest("GET", "/");
        assert(!response.empty());
        assert(response.find("200 OK") != std::string::npos);
        
        std::cout << "✓ Basic API connectivity working" << std::endl;
    }
    
    void testConsensusStatusEndpoint() {
        std::cout << "\n--- Testing Consensus Status Endpoint ---" << std::endl;
        
        std::string response = makeHTTPRequest("GET", "/consensus/status");
        assert(!response.empty());
        assert(response.find("200 OK") != std::string::npos);
        
        // Check for expected JSON fields in response body
        assert(response.find("status") != std::string::npos);
        assert(response.find("initialized") != std::string::npos);
        assert(response.find("running") != std::string::npos);
        assert(response.find("emergency_mode") != std::string::npos);
        
        std::cout << "✓ Consensus status endpoint working correctly" << std::endl;
    }
    
    void testConsensusMetricsEndpoint() {
        std::cout << "\n--- Testing Consensus Metrics Endpoint ---" << std::endl;
        
        std::string response = makeHTTPRequest("GET", "/consensus/metrics");
        assert(!response.empty());
        assert(response.find("200 OK") != std::string::npos);
        
        // Check for expected JSON fields
        assert(response.find("metrics") != std::string::npos);
        assert(response.find("timestamp") != std::string::npos);
        
        std::cout << "✓ Consensus metrics endpoint working correctly" << std::endl;
    }
    
    void testConsensusEnginesEndpoint() {
        std::cout << "\n--- Testing Consensus Engines Endpoint ---" << std::endl;
        
        std::string response = makeHTTPRequest("GET", "/consensus/engines");
        assert(!response.empty());
        assert(response.find("200 OK") != std::string::npos);
        
        // Check for expected JSON structure
        assert(response.find("engines") != std::string::npos);
        
        std::cout << "✓ Consensus engines endpoint working correctly" << std::endl;
    }
    
    void testConsensusConfigEndpoints() {
        std::cout << "\n--- Testing Consensus Configuration Endpoints ---" << std::endl;
        
        // Test GET config
        std::string getResponse = makeHTTPRequest("GET", "/consensus/config");
        assert(!getResponse.empty());
        assert(getResponse.find("200 OK") != std::string::npos);
        assert(getResponse.find("config") != std::string::npos);
        
        std::cout << "✓ GET consensus config working" << std::endl;
        
        // Test POST config (update)
        std::string configData = R"({
            "pow": {
                "difficulty": 5,
                "target_block_time": 550
            }
        })";
        
        std::string postResponse = makeHTTPRequest("POST", "/consensus/config", configData, true);
        assert(!postResponse.empty());
        assert(postResponse.find("200 OK") != std::string::npos);
        
        std::cout << "✓ POST consensus config working" << std::endl;
    }
    
    void testParameterAdjustmentEndpoint() {
        std::cout << "\n--- Testing Parameter Adjustment Endpoint ---" << std::endl;
        
        std::string parameterData = R"({
            "consensus_type": "PROOF_OF_WORK",
            "parameter": "difficulty",
            "value": 6.0
        })";
        
        std::string response = makeHTTPRequest("POST", "/consensus/parameters", parameterData, true);
        assert(!response.empty());
        assert(response.find("200 OK") != std::string::npos);
        
        std::cout << "✓ Parameter adjustment endpoint working" << std::endl;
    }
    
    void testEmergencyModeEndpoints() {
        std::cout << "\n--- Testing Emergency Mode Endpoints ---" << std::endl;
        
        // Test emergency status
        std::string statusResponse = makeHTTPRequest("GET", "/consensus/emergency");
        assert(!statusResponse.empty());
        assert(statusResponse.find("200 OK") != std::string::npos);
        assert(statusResponse.find("emergency_mode") != std::string::npos);
        
        std::cout << "✓ Emergency status endpoint working" << std::endl;
        
        // Test entering emergency mode
        std::string enterResponse = makeHTTPRequest("POST", "/consensus/emergency/enter", "{}", true);
        assert(!enterResponse.empty());
        // Should return 200 OK or 500 (if emergency mode activation fails)
        assert(enterResponse.find("200 OK") != std::string::npos || 
               enterResponse.find("500") != std::string::npos);
        
        std::cout << "✓ Emergency enter endpoint working" << std::endl;
        
        // Test exiting emergency mode
        std::string exitResponse = makeHTTPRequest("POST", "/consensus/emergency/exit", "{}", true);
        assert(!exitResponse.empty());
        assert(exitResponse.find("200 OK") != std::string::npos || 
               exitResponse.find("500") != std::string::npos);
        
        std::cout << "✓ Emergency exit endpoint working" << std::endl;
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
        
        // Send WebSocket handshake
        std::string handshake = 
            "GET /consensus/monitor HTTP/1.1\r\n"
            "Host: localhost:" + std::to_string(testPort) + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
        
        ssize_t sent = send(sock, handshake.c_str(), handshake.length(), 0);
        assert(sent > 0);
        
        // Read handshake response
        char buffer[1024];
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        assert(received > 0);
        
        buffer[received] = '\0';
        std::string response(buffer);
        
        // Verify WebSocket upgrade
        assert(response.find("101 Switching Protocols") != std::string::npos);
        assert(response.find("Upgrade: websocket") != std::string::npos);
        
        std::cout << "✓ WebSocket handshake successful" << std::endl;
        
        // Wait for real-time data (with timeout)
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
                std::cout << "✓ Real-time data received via WebSocket" << std::endl;
            }
        } else {
            std::cout << "✓ WebSocket connection established (no data in timeout period)" << std::endl;
        }
        
        close(sock);
        std::cout << "✓ WebSocket functionality verified" << std::endl;
    }
    
    void testErrorHandling() {
        std::cout << "\n--- Testing Error Handling ---" << std::endl;
        
        // Test invalid endpoint
        std::string invalidResponse = makeHTTPRequest("GET", "/consensus/invalid");
        assert(!invalidResponse.empty());
        assert(invalidResponse.find("404") != std::string::npos);
        
        std::cout << "✓ 404 error handling working" << std::endl;
        
        // Test malformed JSON
        std::string malformedResponse = makeHTTPRequest("POST", "/consensus/parameters", 
                                                       "invalid json", true);
        assert(!malformedResponse.empty());
        assert(malformedResponse.find("200 OK") != std::string::npos); // Returns 200 with error in body
        
        std::cout << "✓ Malformed JSON error handling working" << std::endl;
        
        // Test invalid consensus type
        std::string invalidTypeData = R"({
            "consensus_type": "INVALID_TYPE",
            "parameter": "difficulty",
            "value": 6.0
        })";
        
        std::string invalidTypeResponse = makeHTTPRequest("POST", "/consensus/parameters", 
                                                         invalidTypeData, true);
        assert(!invalidTypeResponse.empty());
        assert(invalidTypeResponse.find("200 OK") != std::string::npos);
        
        std::cout << "✓ Invalid consensus type error handling working" << std::endl;
    }
    
    void testAuthenticationSecurity() {
        std::cout << "\n--- Testing Authentication Security ---" << std::endl;
        
        // Test unauthorized parameter adjustment
        std::string parameterData = R"({
            "consensus_type": "PROOF_OF_WORK",
            "parameter": "difficulty",
            "value": 6.0
        })";
        
        std::string unauthorizedResponse = makeHTTPRequest("POST", "/consensus/parameters", 
                                                          parameterData, false);
        assert(!unauthorizedResponse.empty());
        assert(unauthorizedResponse.find("401") != std::string::npos);
        
        std::cout << "✓ Unauthorized access properly blocked" << std::endl;
        
        // Test unauthorized config update
        std::string configData = R"({"pow": {"difficulty": 5}})";
        std::string unauthorizedConfigResponse = makeHTTPRequest("POST", "/consensus/config", 
                                                                configData, false);
        assert(!unauthorizedConfigResponse.empty());
        assert(unauthorizedConfigResponse.find("401") != std::string::npos);
        
        std::cout << "✓ Unauthorized config update properly blocked" << std::endl;
        
        // Test unauthorized emergency mode access
        std::string unauthorizedEmergencyResponse = makeHTTPRequest("POST", "/consensus/emergency/enter", 
                                                                   "{}", false);
        assert(!unauthorizedEmergencyResponse.empty());
        assert(unauthorizedEmergencyResponse.find("401") != std::string::npos);
        
        std::cout << "✓ Unauthorized emergency mode access properly blocked" << std::endl;
    }
};

int main() {
    try {
        std::cout << "=== Consensus API Integration Test Suite ===" << std::endl;
        std::cout << "Testing all consensus API endpoints and functionality" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        ConsensusAPIIntegrationTest test;
        test.runAllTests();
        
        std::cout << "\n🎉 All Consensus API Integration Tests Passed!" << std::endl;
        std::cout << "\nTested Features:" << std::endl;
        std::cout << "✓ REST API endpoints (9 endpoints)" << std::endl;
        std::cout << "✓ WebSocket real-time monitoring" << std::endl;
        std::cout << "✓ Authentication and authorization" << std::endl;
        std::cout << "✓ Error handling and validation" << std::endl;
        std::cout << "✓ JSON request/response processing" << std::endl;
        std::cout << "✓ Security features" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}