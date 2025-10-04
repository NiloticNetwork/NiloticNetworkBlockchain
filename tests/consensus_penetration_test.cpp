#include "../include/core/consensus_security_validator.h"
#include "../include/core/consensus_security_auditor.h"
#include "../include/core/consensus_harmony_manager.h"
#include "../include/core/block.h"
#include "../include/core/transaction.h"
#include "../include/core/utils.h"
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <cassert>

class ConsensusPenetrationTest {
private:
    std::unique_ptr<ConsensusSecurityValidator> validator;
    std::unique_ptr<ConsensusSecurityAuditor> auditor;
    std::unique_ptr<ConsensusHarmonyManager> harmonyManager;
    std::mt19937 rng;
    
public:
    ConsensusPenetrationTest() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        validator = std::make_unique<ConsensusSecurityValidator>();
        
        AuditConfig auditConfig;
        auditConfig.enableFileLogging = true;
        auditConfig.logFilePath = "penetration_test_audit.log";
        auditor = std::make_unique<ConsensusSecurityAuditor>(auditConfig);
        
        harmonyManager = std::make_unique<ConsensusHarmonyManager>();
    }
    
    bool runPenetrationTests() {
        std::cout << "=== CONSENSUS PENETRATION TESTING SUITE ===" << std::endl;
        std::cout << "WARNING: This test simulates real attack scenarios" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        // Initialize components
        if (!validator->initialize()) {
            std::cout << "FAILED: Security validator initialization" << std::endl;
            return false;
        }
        
        if (!auditor->initialize()) {
            std::cout << "FAILED: Security auditor initialization" << std::endl;
            return false;
        }
        
        if (!harmonyManager->initializeConsensus()) {
            std::cout << "FAILED: Harmony manager initialization" << std::endl;
            return false;
        }
        
        bool allTestsPassed = true;
        
        // Run penetration tests
        allTestsPassed &= testDDoSAttack();
        allTestsPassed &= testTimestampManipulationAttack();
        allTestsPassed &= testSybilAttackSimulation();
        allTestsPassed &= test51PercentAttackSimulation();
        allTestsPassed &= testDoubleSpendingAttack();
        allTestsPassed &= testCryptographicAttacks();
        allTestsPassed &= testConsensusFloodingAttack();
        allTestsPassed &= testMalformedDataAttacks();
        allTestsPassed &= testReplayAttacks();
        allTestsPassed &= testEclipseAttackSimulation();
        allTestsPassed &= testLongRangeAttack();
        allTestsPassed &= testNothingAtStakeAttack();
        allTestsPassed &= testSelfishMiningAttack();
        allTestsPassed &= testConsensusDisruptionAttack();
        
        // Cleanup
        harmonyManager->shutdown();
        auditor->shutdown();
        validator->shutdown();
        
        std::cout << "\n=== PENETRATION TEST RESULTS ===" << std::endl;
        std::cout << "Overall Result: " << (allTestsPassed ? "SYSTEM RESILIENT" : "VULNERABILITIES DETECTED") << std::endl;
        
        return allTestsPassed;
    }
    
private:
    bool testDDoSAttack() {
        std::cout << "\n[PENETRATION TEST] DDoS Attack Simulation" << std::endl;
        std::cout << "Simulating high-volume request flooding..." << std::endl;
        
        try {
            const int ATTACK_REQUESTS = 1000;
            const int ATTACK_THREADS = 10;
            std::atomic<int> detectedAttacks(0);
            std::atomic<int> blockedRequests(0);
            
            auto attackWorker = [&](int threadId) {
                for (int i = 0; i < ATTACK_REQUESTS / ATTACK_THREADS; i++) {
                    ConsensusRequest request(RequestType::BLOCK_VALIDATION, 
                                           "malicious_data_" + std::to_string(threadId) + "_" + std::to_string(i));
                    request.metadata["source"] = "attacker_" + std::to_string(threadId);
                    
                    // Rapid-fire requests
                    if (validator->detectConsensusAttack(request, request.metadata["source"])) {
                        detectedAttacks++;
                    }
                    
                    auto result = validator->validateConsensusRequest(request);
                    if (!result.isSecure) {
                        blockedRequests++;
                    }
                    
                    // Small delay to simulate network latency
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            };
            
            auto startTime = std::chrono::high_resolution_clock::now();
            
            std::vector<std::thread> attackThreads;
            for (int i = 0; i < ATTACK_THREADS; i++) {
                attackThreads.emplace_back(attackWorker, i);
            }
            
            for (auto& thread : attackThreads) {
                thread.join();
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            std::cout << "Attack completed in " << duration.count() << "ms" << std::endl;
            std::cout << "Attacks detected: " << detectedAttacks.load() << "/" << ATTACK_REQUESTS << std::endl;
            std::cout << "Requests blocked: " << blockedRequests.load() << "/" << ATTACK_REQUESTS << std::endl;
            
            // Log the attack attempt
            auditor->logAttackDetection("DDoS_simulation", "penetration_test", 
                                       {{"requests", ATTACK_REQUESTS}, {"detected", detectedAttacks.load()}});
            
            // Consider test passed if at least some attacks were detected
            bool testPassed = detectedAttacks.load() > 0 || blockedRequests.load() > (ATTACK_REQUESTS * 0.1);
            std::cout << "DDoS Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "DDoS Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testTimestampManipulationAttack() {
        std::cout << "\n[PENETRATION TEST] Timestamp Manipulation Attack" << std::endl;
        std::cout << "Testing various timestamp manipulation techniques..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            // Attack 1: Far future timestamps
            std::vector<uint64_t> futureTimestamps = {
                currentTime + 3600,    // 1 hour future
                currentTime + 86400,   // 1 day future
                currentTime + 31536000 // 1 year future
            };
            
            for (auto timestamp : futureTimestamps) {
                totalAttacks++;
                if (validator->detectTimestampManipulation(timestamp)) {
                    attacksDetected++;
                }
                
                // Create malicious block with future timestamp
                Block maliciousBlock(1, "prev_hash", "merkle_root");
                
                ConsensusRequest request(RequestType::BLOCK_VALIDATION, maliciousBlock.serialize());
                request.timestamp = timestamp;
                request.metadata["source"] = "timestamp_attacker";
                
                auto result = validator->validateConsensusRequest(request);
                if (!result.isSecure) {
                    attacksDetected++;
                }
            }
            
            // Attack 2: Far past timestamps
            std::vector<uint64_t> pastTimestamps = {
                currentTime - 90000,   // 25 hours past
                currentTime - 604800,  // 1 week past
                currentTime - 31536000 // 1 year past
            };
            
            for (auto timestamp : pastTimestamps) {
                totalAttacks++;
                if (validator->detectTimestampManipulation(timestamp)) {
                    attacksDetected++;
                }
            }
            
            std::cout << "Timestamp attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("timestamp_manipulation", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.5); // At least 50% detection rate
            std::cout << "Timestamp Manipulation Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Timestamp Manipulation Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testSybilAttackSimulation() {
        std::cout << "\n[PENETRATION TEST] Sybil Attack Simulation" << std::endl;
        std::cout << "Creating multiple fake identities..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Attack 1: Many identities from single source
            std::vector<std::string> manyIdentities;
            for (int i = 0; i < 50; i++) {
                manyIdentities.push_back("fake_identity_" + std::to_string(i));
            }
            
            totalAttacks++;
            if (validator->detectSybilAttack("sybil_attacker_1", manyIdentities)) {
                attacksDetected++;
            }
            
            // Attack 2: Similar identity patterns
            std::vector<std::string> similarIdentities;
            std::string baseId = "abcdef123456";
            for (int i = 0; i < 20; i++) {
                similarIdentities.push_back(baseId + std::to_string(i));
            }
            
            totalAttacks++;
            if (validator->detectSybilAttack("sybil_attacker_2", similarIdentities)) {
                attacksDetected++;
            }
            
            std::cout << "Sybil attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("sybil_attack", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.3); // At least 30% detection rate
            std::cout << "Sybil Attack Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Sybil Attack Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test51PercentAttackSimulation() {
        std::cout << "\n[PENETRATION TEST] 51% Attack Simulation" << std::endl;
        std::cout << "Simulating consensus mechanism dominance..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Attack 1: Single mechanism dominance (75%)
            std::vector<ConsensusResult> dominatedResults;
            for (int i = 0; i < 15; i++) {
                dominatedResults.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_WORK));
            }
            for (int i = 0; i < 5; i++) {
                dominatedResults.push_back(ConsensusResult(true, ConsensusType::PROOF_OF_STAKE));
            }
            
            totalAttacks++;
            if (validator->detect51PercentAttack(dominatedResults)) {
                attacksDetected++;
            }
            
            std::cout << "51% attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("51_percent_attack", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.6); // At least 60% detection rate
            std::cout << "51% Attack Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "51% Attack Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testDoubleSpendingAttack() {
        std::cout << "\n[PENETRATION TEST] Double Spending Attack" << std::endl;
        std::cout << "Attempting to spend the same funds multiple times..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Attack 1: Negative amount transactions
            Transaction negativeTx("attacker", "victim", -500.0);
            totalAttacks++;
            if (validator->detectDoubleSpendingAttack(negativeTx)) {
                attacksDetected++;
            }
            
            // Attack 2: Zero amount transactions
            Transaction zeroTx("attacker", "victim", 0.0);
            totalAttacks++;
            if (validator->detectDoubleSpendingAttack(zeroTx)) {
                attacksDetected++;
            }
            
            std::cout << "Double spending attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("double_spending", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.2); // At least 20% detection rate
            std::cout << "Double Spending Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Double Spending Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testCryptographicAttacks() {
        std::cout << "\n[PENETRATION TEST] Cryptographic Attacks" << std::endl;
        std::cout << "Testing weak cryptography and signature attacks..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Attack 1: Weak algorithms
            std::vector<std::string> weakAlgorithms = {"MD5", "SHA1", "DES", "RC4"};
            
            for (const auto& algorithm : weakAlgorithms) {
                CryptoValidationContext weakContext;
                weakContext.algorithm = algorithm;
                weakContext.data = "test_data";
                weakContext.signature = "weak_signature_1234567890abcdef1234567890abcdef12345678";
                weakContext.publicKey = "weak_public_key";
                weakContext.requireStrongCrypto = true;
                
                totalAttacks++;
                if (!validator->validateCryptographicSignature(weakContext)) {
                    attacksDetected++;
                }
            }
            
            std::cout << "Cryptographic attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("cryptographic_attack", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.8); // At least 80% detection rate
            std::cout << "Cryptographic Attack Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Cryptographic Attack Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testConsensusFloodingAttack() {
        std::cout << "\n[PENETRATION TEST] Consensus Flooding Attack" << std::endl;
        std::cout << "Flooding consensus with invalid requests..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            const int FLOOD_COUNT = 500;
            
            // Generate random malicious data
            std::uniform_int_distribution<> dis(0, 255);
            
            for (int i = 0; i < FLOOD_COUNT; i++) {
                // Create malicious consensus request
                std::string maliciousData;
                for (int j = 0; j < 1000; j++) { // Large payload
                    maliciousData += static_cast<char>(dis(rng));
                }
                
                ConsensusRequest floodRequest(RequestType::BLOCK_VALIDATION, maliciousData);
                floodRequest.metadata["source"] = "flood_attacker";
                floodRequest.metadata["attack_id"] = std::to_string(i);
                
                totalAttacks++;
                auto result = validator->validateConsensusRequest(floodRequest);
                if (!result.isSecure) {
                    attacksDetected++;
                }
                
                // Check for attack detection
                if (validator->detectConsensusAttack(floodRequest, "flood_attacker")) {
                    attacksDetected++;
                }
                
                // Minimal delay to simulate rapid flooding
                if (i % 100 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            
            std::cout << "Flooding attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("consensus_flooding", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.1); // At least 10% detection rate
            std::cout << "Consensus Flooding Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Consensus Flooding Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testMalformedDataAttacks() {
        std::cout << "\n[PENETRATION TEST] Malformed Data Attacks" << std::endl;
        std::cout << "Testing system resilience to malformed inputs..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Attack 1: Empty and null data
            std::vector<std::string> malformedData = {
                "",                    // Empty
                std::string(1, '\0'),  // Null character
                std::string(1000, 'A'), // Very long string
                "\\x00\\x01\\x02",     // Binary data
                "{invalid_json}",      // Invalid JSON
                "<script>alert('xss')</script>", // XSS attempt
                "'; DROP TABLE blocks; --", // SQL injection attempt
            };
            
            for (const auto& data : malformedData) {
                ConsensusRequest malformedRequest(RequestType::BLOCK_VALIDATION, data);
                malformedRequest.metadata["source"] = "malformed_attacker";
                
                totalAttacks++;
                auto result = validator->validateConsensusRequest(malformedRequest);
                if (!result.isSecure) {
                    attacksDetected++;
                }
            }
            
            std::cout << "Malformed data attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("malformed_data", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.7); // At least 70% detection rate
            std::cout << "Malformed Data Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Malformed Data Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testReplayAttacks() {
        std::cout << "\n[PENETRATION TEST] Replay Attacks" << std::endl;
        std::cout << "Testing replay attack detection..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Create a legitimate request
            ConsensusRequest originalRequest(RequestType::TRANSACTION_VALIDATION, "legitimate_transaction");
            originalRequest.metadata["source"] = "legitimate_user";
            
            // Attack 1: Exact replay
            for (int i = 0; i < 10; i++) {
                ConsensusRequest replayRequest = originalRequest;
                replayRequest.metadata["replay_attempt"] = std::to_string(i);
                
                totalAttacks++;
                auto result = validator->validateConsensusRequest(replayRequest);
                if (!result.isSecure) {
                    attacksDetected++;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            std::cout << "Replay attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("replay_attack", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.2); // At least 20% detection rate
            std::cout << "Replay Attack Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Replay Attack Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testEclipseAttackSimulation() {
        std::cout << "\n[PENETRATION TEST] Eclipse Attack Simulation" << std::endl;
        std::cout << "Simulating network isolation attacks..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Simulate eclipse attack by creating requests from isolated network
            std::vector<std::string> isolatedSources = {
                "isolated_node_1", "isolated_node_2", "isolated_node_3"
            };
            
            // Attack 1: Coordinated requests from isolated nodes
            for (const auto& source : isolatedSources) {
                for (int i = 0; i < 10; i++) {
                    ConsensusRequest isolatedRequest(RequestType::BLOCK_VALIDATION, 
                                                   "isolated_block_" + std::to_string(i));
                    isolatedRequest.metadata["source"] = source;
                    isolatedRequest.metadata["network_id"] = "isolated_network";
                    
                    totalAttacks++;
                    if (validator->detectConsensusAttack(isolatedRequest, source)) {
                        attacksDetected++;
                    }
                    
                    auto result = validator->validateConsensusRequest(isolatedRequest);
                    if (!result.isSecure) {
                        attacksDetected++;
                    }
                }
            }
            
            std::cout << "Eclipse attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("eclipse_attack", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.1); // At least 10% detection rate
            std::cout << "Eclipse Attack Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Eclipse Attack Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testLongRangeAttack() {
        std::cout << "\n[PENETRATION TEST] Long Range Attack" << std::endl;
        std::cout << "Testing historical blockchain rewrite attempts..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Simulate long range attack with old timestamps
            uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            std::vector<uint64_t> historicalTimestamps = {
                currentTime - 86400,   // 1 day ago
                currentTime - 604800,  // 1 week ago
                currentTime - 2592000, // 1 month ago
                currentTime - 31536000 // 1 year ago
            };
            
            for (auto timestamp : historicalTimestamps) {
                // Create block with historical timestamp
                Block historicalBlock(1, "old_prev_hash", "old_merkle_root");
                
                ConsensusRequest longRangeRequest(RequestType::BLOCK_VALIDATION, historicalBlock.serialize());
                longRangeRequest.timestamp = timestamp;
                longRangeRequest.metadata["source"] = "long_range_attacker";
                longRangeRequest.metadata["attack_type"] = "historical_rewrite";
                
                totalAttacks++;
                if (validator->detectTimestampManipulation(timestamp)) {
                    attacksDetected++;
                }
                
                auto result = validator->validateConsensusRequest(longRangeRequest);
                if (!result.isSecure) {
                    attacksDetected++;
                }
            }
            
            std::cout << "Long range attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("long_range_attack", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.5); // At least 50% detection rate
            std::cout << "Long Range Attack Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Long Range Attack Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testNothingAtStakeAttack() {
        std::cout << "\n[PENETRATION TEST] Nothing at Stake Attack" << std::endl;
        std::cout << "Testing multiple chain validation attempts..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Simulate validator trying to validate multiple competing chains
            std::vector<std::string> competingChains = {
                "chain_A_block_data", "chain_B_block_data", "chain_C_block_data"
            };
            
            std::string attackerSource = "nothing_at_stake_attacker";
            
            // Rapid validation attempts on multiple chains
            for (int round = 0; round < 5; round++) {
                for (const auto& chainData : competingChains) {
                    ConsensusRequest chainRequest(RequestType::BLOCK_VALIDATION, chainData);
                    chainRequest.metadata["source"] = attackerSource;
                    chainRequest.metadata["chain_id"] = chainData.substr(0, 7);
                    chainRequest.metadata["round"] = std::to_string(round);
                    
                    totalAttacks++;
                    if (validator->detectConsensusAttack(chainRequest, attackerSource)) {
                        attacksDetected++;
                    }
                    
                    auto result = validator->validateConsensusRequest(chainRequest);
                    if (!result.isSecure) {
                        attacksDetected++;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            std::cout << "Nothing at stake attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("nothing_at_stake", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.2); // At least 20% detection rate
            std::cout << "Nothing at Stake Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Nothing at Stake Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testSelfishMiningAttack() {
        std::cout << "\n[PENETRATION TEST] Selfish Mining Attack" << std::endl;
        std::cout << "Testing private chain mining and strategic revelation..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Simulate selfish miner creating private chain
            std::string selfishMiner = "selfish_miner";
            
            // Phase 1: Private mining (rapid block creation)
            for (int i = 0; i < 10; i++) {
                Block privateBlock(i + 1, "private_prev_hash_" + std::to_string(i), 
                                 "private_merkle_" + std::to_string(i));
                
                ConsensusRequest privateRequest(RequestType::BLOCK_VALIDATION, privateBlock.serialize());
                privateRequest.metadata["source"] = selfishMiner;
                privateRequest.metadata["mining_type"] = "private";
                privateRequest.metadata["block_height"] = std::to_string(i + 1);
                
                totalAttacks++;
                if (validator->detectConsensusAttack(privateRequest, selfishMiner)) {
                    attacksDetected++;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Rapid mining
            }
            
            std::cout << "Selfish mining attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("selfish_mining", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.3); // At least 30% detection rate
            std::cout << "Selfish Mining Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Selfish Mining Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testConsensusDisruptionAttack() {
        std::cout << "\n[PENETRATION TEST] Consensus Disruption Attack" << std::endl;
        std::cout << "Testing systematic consensus mechanism disruption..." << std::endl;
        
        try {
            int attacksDetected = 0;
            int totalAttacks = 0;
            
            // Attack 1: Conflicting consensus results
            std::vector<ConsensusResult> conflictingResults = {
                ConsensusResult(true, ConsensusType::PROOF_OF_WORK, 1.0, "PoW says valid"),
                ConsensusResult(false, ConsensusType::PROOF_OF_STAKE, 1.0, "PoS says invalid"),
                ConsensusResult(true, ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION, 0.5, "PoRC uncertain"),
                ConsensusResult(false, ConsensusType::VOTING_CONSENSUS, 0.8, "Voting says invalid")
            };
            
            totalAttacks++;
            if (validator->detect51PercentAttack(conflictingResults)) {
                attacksDetected++;
            }
            
            std::cout << "Consensus disruption attacks detected: " << attacksDetected << "/" << totalAttacks << std::endl;
            
            // Log the attack
            auditor->logAttackDetection("consensus_disruption", "penetration_test",
                                       {{"total_attacks", totalAttacks}, {"detected", attacksDetected}});
            
            bool testPassed = attacksDetected >= (totalAttacks * 0.15); // At least 15% detection rate
            std::cout << "Consensus Disruption Test: " << (testPassed ? "PASSED" : "FAILED") << std::endl;
            
            return testPassed;
            
        } catch (const std::exception& e) {
            std::cout << "Consensus Disruption Test FAILED: " << e.what() << std::endl;
            return false;
        }
    }
};

int main() {
    std::cout << "NILOTIC BLOCKCHAIN CONSENSUS SECURITY PENETRATION TESTING" << std::endl;
    std::cout << "=========================================================" << std::endl;
    std::cout << "This test suite simulates real-world attacks against the" << std::endl;
    std::cout << "consensus harmony system to validate security measures." << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    ConsensusPenetrationTest penetrationTest;
    
    bool allTestsPassed = penetrationTest.runPenetrationTests();
    
    std::cout << "\n=========================================================" << std::endl;
    if (allTestsPassed) {
        std::cout << "PENETRATION TEST RESULT: SYSTEM SHOWS GOOD RESILIENCE" << std::endl;
        std::cout << "The consensus harmony system successfully detected and" << std::endl;
        std::cout << "mitigated most simulated attack scenarios." << std::endl;
        return 0;
    } else {
        std::cout << "PENETRATION TEST RESULT: VULNERABILITIES DETECTED" << std::endl;
        std::cout << "Some attack scenarios were not properly detected." << std::endl;
        std::cout << "Review the test output and strengthen security measures." << std::endl;
        return 1;
    }
}