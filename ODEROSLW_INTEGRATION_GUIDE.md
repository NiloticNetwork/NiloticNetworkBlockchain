# OderoSLW Token System Integration Guide

## 🚀 Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+ (for full build system)
- OpenSSL library (for cryptographic features)
- nlohmann/json library (included in project)

### Basic Integration

#### 1. Include Headers

```cpp
#include "oderoslw.h"
#include "oderoslw_manager.h"
```

#### 2. Create Token Manager

```cpp
// Basic setup
OderoSLWManager tokenManager;

// With blockchain integration
OderoSLWManager tokenManager(&blockchain);
```

#### 3. Configure Security Settings

```cpp
tokenManager.setMaxRiskThreshold(0.8);
tokenManager.setMaxValidationAttempts(5);
tokenManager.setEnableFraudDetection(true);
```

## 📋 Integration Checklist

### ✅ Phase 1: Basic Integration

- [ ] Include OderoSLW headers in your project
- [ ] Initialize OderoSLWManager instance
- [ ] Test basic token creation and validation
- [ ] Verify token state transitions work correctly
- [ ] Test serialization/deserialization

### ✅ Phase 2: Security Configuration

- [ ] Configure appropriate security levels for your use case
- [ ] Set up fraud detection parameters
- [ ] Implement blacklisting mechanisms
- [ ] Configure rate limiting
- [ ] Set up audit logging

### ✅ Phase 3: Blockchain Integration

- [ ] Connect token manager to blockchain instance
- [ ] Test token operations create blockchain transactions
- [ ] Verify consensus mechanism compatibility
- [ ] Test transaction validation and mining

### ✅ Phase 4: API Integration

- [ ] Set up API endpoints for token operations
- [ ] Implement authentication and authorization
- [ ] Configure rate limiting per client
- [ ] Test all API endpoints
- [ ] Set up monitoring and logging

### ✅ Phase 5: Production Deployment

- [ ] Performance testing and optimization
- [ ] Security audit and penetration testing
- [ ] Backup and recovery procedures
- [ ] Monitoring and alerting setup
- [ ] Documentation and training

## 🔧 Configuration Examples

### Basic Configuration

```cpp
// Initialize manager
OderoSLWManager manager;

// Basic security settings
manager.setMaxRiskThreshold(0.7);
manager.setMaxValidationAttempts(3);

// Create basic token
std::string tokenId = manager.createToken("user123", 100.0, SecurityLevel::BASIC);
```

### Enhanced Security Configuration

```cpp
// Enhanced security settings
manager.setMaxRiskThreshold(0.8);
manager.setMaxValidationAttempts(5);
manager.setEnableFraudDetection(true);
manager.setRequireDeviceFingerprint(true);

// Create enhanced token
std::string tokenId = manager.createToken("enterprise_user", 1000.0,
                                         SecurityLevel::ENHANCED, "public_key");
```

### Enterprise Configuration

```cpp
// Enterprise security settings
manager.setMaxRiskThreshold(0.9);
manager.setMaxValidationAttempts(3);
manager.setEnableFraudDetection(true);
manager.setRequireDeviceFingerprint(true);
manager.setRequireLocationVerification(true);

// Create enterprise token with multi-sig
OderoSLW token("OSLW123", 10000.0, "enterprise_user",
               SecurityLevel::ENTERPRISE, "public_key");
```

## 🔌 API Integration

### REST API Endpoints

#### Token Creation

```http
POST /api/v2/tokens/create
Content-Type: application/json

{
    "creator": "user123",
    "amount": 100.0,
    "securityLevel": "BASIC",
    "publicKey": "optional_public_key"
}
```

#### Token Validation

```http
POST /api/v2/tokens/validate
Content-Type: application/json

{
    "tokenId": "OSLW123456789",
    "deviceFingerprint": "optional_device_fp",
    "locationHash": "optional_location"
}
```

#### Batch Operations

```http
POST /api/v2/tokens/batch
Content-Type: application/json

{
    "creator": "batch_user",
    "tokens": [
        {"name": "token1", "amount": 100.0},
        {"name": "token2", "amount": 200.0}
    ],
    "securityLevel": "ENHANCED"
}
```

### Response Format

```json
{
  "statusCode": 200,
  "status": "success",
  "data": {
    "tokenId": "OSLW123456789",
    "amount": 100.0,
    "state": "CREATED",
    "securityLevel": "BASIC"
  },
  "errors": [],
  "warnings": []
}
```

## 🔒 Security Best Practices

### 1. Security Level Selection

- **Basic**: For low-value, high-volume transactions
- **Enhanced**: For medium-value transactions requiring signatures
- **Enterprise**: For high-value transactions requiring multi-sig

### 2. Key Management

```cpp
// Generate secure keys
std::string privateKey = generateSecurePrivateKey();
std::string publicKey = derivePublicKey(privateKey);

// Sign token
token.signToken(privateKey);

// Verify with public key
bool isValid = token.verifyWithKey(publicKey);
```

### 3. Fraud Detection

```cpp
// Configure fraud detection
manager.setMaxRiskThreshold(0.8);
manager.setEnableFraudDetection(true);

// Validate with context
TokenValidationResult result = manager.validateTokenWithContext(
    tokenId, deviceFingerprint, locationHash);

if (result.riskScore > 0.8) {
    // Handle high-risk token
    manager.addToBlacklist(tokenId);
}
```

### 4. Rate Limiting

```cpp
// API rate limiting
api.setRateLimit(100); // 100 requests per minute per IP

// Validation attempt limiting
manager.setMaxValidationAttempts(5);
```

## 📊 Monitoring and Analytics

### Health Monitoring

```cpp
// Perform health check
bool isHealthy = manager.performHealthCheck();

// Get health status
nlohmann::json healthStatus = manager.getHealthStatus();
```

### Statistics Collection

```cpp
// Get basic statistics
TokenStatistics stats = manager.getStatistics();
std::cout << "Total tokens: " << stats.totalTokens << std::endl;
std::cout << "Active tokens: " << stats.activeTokens << std::endl;

// Get detailed statistics
nlohmann::json detailedStats = manager.getDetailedStatistics();
```

### Performance Metrics

```cpp
// Get operation counts
std::map<std::string, int> operations = manager.getOperationCounts();
std::cout << "Token creations: " << operations["CREATE"] << std::endl;
std::cout << "Validations: " << operations["VALIDATE"] << std::endl;
```

## 🔄 Blockchain Integration

### Setup

```cpp
// Initialize blockchain
Blockchain blockchain;

// Connect token manager
tokenManager.setBlockchain(&blockchain);

// Token operations automatically create blockchain transactions
std::string tokenId = tokenManager.createToken("user", 100.0);
```

### Transaction Integration

```cpp
// Token operations create corresponding blockchain transactions
bool redeemed = tokenManager.redeemToken(tokenId, "redeemer", "signature");

// Check blockchain for transaction record
std::vector<Transaction> pendingTxs = blockchain.getPendingTransactions();
```

## 🧪 Testing Integration

### Unit Tests

```cpp
// Test basic functionality
void testTokenCreation() {
    OderoSLWManager manager;
    std::string tokenId = manager.createToken("test_user", 100.0);
    assert(!tokenId.empty());

    OderoSLW* token = manager.getToken(tokenId);
    assert(token != nullptr);
    assert(token->getAmount() == 100.0);
}
```

### Integration Tests

```cpp
// Test blockchain integration
void testBlockchainIntegration() {
    Blockchain blockchain;
    OderoSLWManager manager(&blockchain);

    std::string tokenId = manager.createToken("user", 100.0);
    manager.redeemToken(tokenId, "redeemer", "sig");

    // Verify blockchain transaction was created
    auto pendingTxs = blockchain.getPendingTransactions();
    assert(!pendingTxs.empty());
}
```

### Performance Tests

```cpp
// Test performance under load
void testPerformance() {
    OderoSLWManager manager;

    auto start = std::chrono::high_resolution_clock::now();

    // Create 10,000 tokens
    for (int i = 0; i < 10000; ++i) {
        manager.createToken("user" + std::to_string(i), 100.0);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Created 10,000 tokens in " << duration.count() << "ms" << std::endl;
}
```

## 🚀 Deployment Guide

### Development Environment

```bash
# Clone repository
git clone <repository_url>
cd blockchain_project

# Build with enhanced OderoSLW
mkdir build && cd build
cmake .. -DENABLE_ODEROSLW_ENHANCED=ON
make -j4

# Run tests
make test
```

### Production Deployment

```bash
# Build optimized version
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_ODEROSLW_ENHANCED=ON
make -j4

# Install system-wide
sudo make install

# Configure service
sudo systemctl enable blockchain-service
sudo systemctl start blockchain-service
```

### Docker Deployment

```dockerfile
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcrypto++-dev

# Copy source code
COPY . /app
WORKDIR /app

# Build application
RUN mkdir build && cd build && \
    cmake .. -DENABLE_ODEROSLW_ENHANCED=ON && \
    make -j4

# Run application
CMD ["./build/blockchain_app"]
```

## 🔧 Troubleshooting

### Common Issues

#### 1. Compilation Errors

```bash
# Missing OpenSSL
sudo apt-get install libssl-dev

# Missing C++17 support
export CXX=g++-8  # Use newer compiler
```

#### 2. Runtime Issues

```cpp
// Check token manager initialization
if (!manager.performHealthCheck()) {
    std::cerr << "Token manager health check failed" << std::endl;
}

// Verify blockchain connection
if (!manager.getBlockchain()) {
    std::cerr << "Blockchain not connected" << std::endl;
}
```

#### 3. Performance Issues

```cpp
// Monitor memory usage
TokenStatistics stats = manager.getStatistics();
if (stats.totalTokens > 1000000) {
    manager.cleanupExpiredTokens();
    manager.archiveOldTokens(30);
}
```

### Debug Mode

```cpp
// Enable debug logging
Logger::setLevel(LogLevel::DEBUG);

// Detailed token information
std::string debugInfo = token.getEnhancedMetadata();
std::cout << debugInfo << std::endl;
```

## 📈 Performance Optimization

### Memory Optimization

```cpp
// Regular cleanup
manager.cleanupExpiredTokens();
manager.archiveOldTokens(30);
manager.optimizeStorage();
```

### Batch Processing

```cpp
// Use batch operations for better performance
std::vector<std::pair<std::string, double>> tokenData;
for (int i = 0; i < 1000; ++i) {
    tokenData.push_back({"token" + std::to_string(i), 100.0});
}

std::vector<std::string> tokenIds = manager.createTokenBatch(
    tokenData, "batch_user", SecurityLevel::BASIC);
```

### Caching

```cpp
// Cache frequently accessed tokens
std::unordered_map<std::string, OderoSLW*> tokenCache;

OderoSLW* getTokenCached(const std::string& tokenId) {
    auto it = tokenCache.find(tokenId);
    if (it != tokenCache.end()) {
        return it->second;
    }

    OderoSLW* token = manager.getToken(tokenId);
    if (token) {
        tokenCache[tokenId] = token;
    }
    return token;
}
```

## 🔮 Future Enhancements

### Planned Features

- Hardware Security Module (HSM) integration
- Zero-knowledge proof validation
- Cross-chain compatibility
- Advanced machine learning fraud detection
- Mobile SDK development
- Web-based administration interface

### Migration Path

The enhanced OderoSLW system is designed to be backward compatible with existing implementations while providing a clear upgrade path for advanced features.

## 📞 Support

For technical support, integration assistance, or questions about the OderoSLW Token System:

1. Check the comprehensive documentation
2. Review the test examples
3. Consult the API reference
4. Contact the development team

The enhanced OderoSLW Token System provides a robust foundation for secure, scalable offline payment solutions in blockchain applications.
