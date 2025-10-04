cmake_minimum_required(VERSION 3.10)
project(nilotic_blockchain)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Set paths for external dependencies
# Note: We're now using nlohmann/json.hpp for JSON handling

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/api
    ${CMAKE_CURRENT_SOURCE_DIR}/include/persistence
    ${CMAKE_CURRENT_SOURCE_DIR}/include/wallet
    ${CMAKE_CURRENT_SOURCE_DIR}  # Add this to resolve paths in json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib  # Add lib directory for Crow
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/crow  # Add Crow headers
    ${OPENSSL_INCLUDE_DIR}
)

# Add all source files including new security and consensus harmony components
add_executable(nilotic_blockchain 
    src/core/main.cpp
    src/core/blockchain.cpp
    src/core/block.cpp
    src/core/transaction.cpp
    src/core/oderoslw.cpp
    src/core/oderoslw_manager.cpp
    src/core/wallet.cpp
    src/core/utils.cpp
    src/core/persistence.cpp
    src/core/logger.cpp
    src/core/api.cpp
    src/core/optimized_api.cpp
    src/core/mining.cpp
    src/core/networking.cpp
    src/core/rate_limiter.cpp
    src/core/security_middleware.cpp
    src/core/smart_contract_vm.cpp
    src/core/consensus_harmony.cpp
    src/core/consensus_harmony_manager.cpp
    src/core/consensus_harmony_integration.cpp
    src/core/consensus_router.cpp
    src/core/consensus_balancer.cpp
    src/core/consensus_monitor.cpp
    src/core/consensus_config_manager.cpp
    src/core/consensus_conflict_resolver.cpp
    src/core/consensus_security_validator.cpp
    src/core/consensus_security_auditor.cpp
    src/core/consensus_performance_optimizer.cpp
    src/core/emergency_consensus_mode.cpp
    src/core/parameter_adjuster.cpp
    src/core/pos_consensus_engine.cpp
    src/core/proof_of_resource_contribution.cpp
    src/core/voting_consensus_engine.cpp
    src/core/smart_contract_consensus_engine.cpp
    src/core/unified_transaction_validator.cpp
    src/core/enhanced_blockchain.cpp
)

# Find SQLite3 - use pkg-config approach for better compatibility
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Link libraries
target_link_libraries(nilotic_blockchain
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
    ${SQLITE3_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Add SQLite3 include directories
target_include_directories(nilotic_blockchain PRIVATE ${SQLITE3_INCLUDE_DIRS})
target_link_directories(nilotic_blockchain PRIVATE ${SQLITE3_LIBRARY_DIRS})

# Add simple voting consensus demo executable
add_executable(simple_voting_demo
    examples/simple_voting_demo.cpp
    src/core/voting_consensus_engine.cpp
    src/core/consensus_harmony.cpp
    src/core/pos_consensus_engine.cpp
    src/core/blockchain.cpp
    src/core/block.cpp
    src/core/transaction.cpp
    src/core/utils.cpp
    src/core/logger.cpp
)

# Add voting consensus demo executable (optional)
add_executable(voting_consensus_demo
    examples/voting_consensus_demo.cpp
    src/core/voting_consensus_engine.cpp
    src/core/consensus_harmony.cpp
    src/core/consensus_harmony_manager.cpp
    src/core/consensus_router.cpp
    src/core/consensus_balancer.cpp
    src/core/pos_consensus_engine.cpp
    src/core/blockchain.cpp
    src/core/block.cpp
    src/core/transaction.cpp
    src/core/utils.cpp
    src/core/logger.cpp
)

# Link libraries for simple voting demo
target_link_libraries(simple_voting_demo
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
    ${SQLITE3_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Add include directories for simple voting demo
target_include_directories(simple_voting_demo PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${SQLITE3_INCLUDE_DIRS}
)

# Link libraries for voting consensus demo
target_link_libraries(voting_consensus_demo
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
    ${SQLITE3_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Add include directories for voting consensus demo
target_include_directories(voting_consensus_demo PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${SQLITE3_INCLUDE_DIRS}
)

# Install
install(TARGETS nilotic_blockchain DESTINATION bin)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest QUIET)

if(GTest_FOUND)
    # Add consensus harmony test executable
    add_executable(consensus_harmony_tests
        tests/consensus_harmony_test.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_harmony_manager.cpp
        src/core/consensus_router.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Add consensus router test executable
    add_executable(consensus_router_tests
        tests/consensus_router_test.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_router.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for consensus harmony tests
    target_link_libraries(consensus_harmony_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Link test libraries for consensus router tests
    target_link_libraries(consensus_router_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for consensus harmony tests
    target_include_directories(consensus_harmony_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add test include directories for consensus router tests
    target_include_directories(consensus_router_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add PoS consensus engine test executable
    add_executable(pos_consensus_tests
        tests/pos_consensus_engine_test.cpp
        src/core/pos_consensus_engine.cpp
        src/core/consensus_harmony.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for PoS consensus tests
    target_link_libraries(pos_consensus_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for PoS consensus tests
    target_include_directories(pos_consensus_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add PoRC engine test executable
    add_executable(porc_tests
        tests/proof_of_resource_contribution_test.cpp
        src/core/proof_of_resource_contribution.cpp
        src/core/consensus_harmony.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for PoRC tests
    target_link_libraries(porc_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for PoRC tests
    target_include_directories(porc_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add consensus balancer test executable
    add_executable(consensus_balancer_tests
        tests/consensus_balancer_test.cpp
        src/core/consensus_balancer.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_router.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for consensus balancer tests
    target_link_libraries(consensus_balancer_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for consensus balancer tests
    target_include_directories(consensus_balancer_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add voting consensus engine test executable
    add_executable(voting_consensus_tests
        tests/voting_consensus_engine_test.cpp
        src/core/voting_consensus_engine.cpp
        src/core/consensus_harmony.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for voting consensus tests
    target_link_libraries(voting_consensus_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for voting consensus tests
    target_include_directories(voting_consensus_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add voting consensus integration test executable
    add_executable(voting_consensus_integration_tests
        tests/voting_consensus_integration_test.cpp
        src/core/voting_consensus_engine.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_harmony_manager.cpp
        src/core/consensus_router.cpp
        src/core/consensus_balancer.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for voting consensus integration tests
    target_link_libraries(voting_consensus_integration_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for voting consensus integration tests
    target_include_directories(voting_consensus_integration_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add smart contract consensus engine test executable
    add_executable(smart_contract_consensus_tests
        tests/smart_contract_consensus_engine_test.cpp
        src/core/smart_contract_consensus_engine.cpp
        src/core/smart_contract_vm.cpp
        src/core/voting_consensus_engine.cpp
        src/core/consensus_harmony.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for smart contract consensus tests
    target_link_libraries(smart_contract_consensus_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for smart contract consensus tests
    target_include_directories(smart_contract_consensus_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add consensus monitor test executable
    add_executable(consensus_monitor_tests
        tests/consensus_monitor_test.cpp
        src/core/consensus_monitor.cpp
        src/core/consensus_harmony.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for consensus monitor tests
    target_link_libraries(consensus_monitor_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for consensus monitor tests
    target_include_directories(consensus_monitor_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add consensus config manager test executable
    add_executable(consensus_config_manager_tests
        tests/consensus_config_manager_test.cpp
        src/core/consensus_config_manager.cpp
        src/core/consensus_harmony.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for consensus config manager tests
    target_link_libraries(consensus_config_manager_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for consensus config manager tests
    target_include_directories(consensus_config_manager_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add consensus conflict resolver test executable
    add_executable(consensus_conflict_resolver_tests
        tests/consensus_conflict_resolver_test.cpp
        src/core/consensus_conflict_resolver.cpp
        src/core/consensus_harmony.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for consensus conflict resolver tests
    target_link_libraries(consensus_conflict_resolver_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for consensus conflict resolver tests
    target_include_directories(consensus_conflict_resolver_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add parameter adjuster test executable
    add_executable(parameter_adjuster_tests
        tests/parameter_adjuster_test.cpp
        src/core/parameter_adjuster.cpp
        src/core/consensus_harmony.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for parameter adjuster tests
    target_link_libraries(parameter_adjuster_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for parameter adjuster tests
    target_include_directories(parameter_adjuster_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add unified transaction validator test executable
    add_executable(unified_transaction_validator_tests
        tests/unified_transaction_validator_test.cpp
        src/core/unified_transaction_validator.cpp
        src/core/transaction.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for unified transaction validator tests
    target_link_libraries(unified_transaction_validator_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for unified transaction validator tests
    target_include_directories(unified_transaction_validator_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add emergency consensus mode test executable
    add_executable(emergency_consensus_mode_tests
        tests/emergency_consensus_mode_test.cpp
        src/core/emergency_consensus_mode.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_monitor.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for emergency consensus mode tests
    target_link_libraries(emergency_consensus_mode_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for emergency consensus mode tests
    target_include_directories(emergency_consensus_mode_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Add enhanced blockchain test executable
    add_executable(enhanced_blockchain_tests
        tests/enhanced_blockchain_integration_test.cpp
        src/core/enhanced_blockchain.cpp
        src/core/blockchain.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_harmony_manager.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries for enhanced blockchain tests
    target_link_libraries(enhanced_blockchain_tests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        OpenSSL::Crypto
        OpenSSL::SSL
        ${SQLITE3_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )
    
    # Add test include directories for enhanced blockchain tests
    target_include_directories(enhanced_blockchain_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Register tests
    add_test(NAME ConsensusHarmonyTests COMMAND consensus_harmony_tests)
    add_test(NAME ConsensusRouterTests COMMAND consensus_router_tests)
    add_test(NAME ConsensusBalancerTests COMMAND consensus_balancer_tests)
    add_test(NAME ConsensusMonitorTests COMMAND consensus_monitor_tests)
    add_test(NAME ConsensusConfigManagerTests COMMAND consensus_config_manager_tests)
    add_test(NAME ConsensusConflictResolverTests COMMAND consensus_conflict_resolver_tests)
    add_test(NAME ParameterAdjusterTests COMMAND parameter_adjuster_tests)
    add_test(NAME UnifiedTransactionValidatorTests COMMAND unified_transaction_validator_tests)
    add_test(NAME EmergencyConsensusModeTests COMMAND emergency_consensus_mode_tests)
    add_test(NAME EnhancedBlockchainTests COMMAND enhanced_blockchain_tests)
    add_test(NAME PoSConsensusTests COMMAND pos_consensus_tests)
    add_test(NAME PoRCTests COMMAND porc_tests)
    add_test(NAME VotingConsensusTests COMMAND voting_consensus_tests)
    add_test(NAME VotingConsensusIntegrationTests COMMAND voting_consensus_integration_tests)
    add_test(NAME SmartContractConsensusTests COMMAND smart_contract_consensus_tests)
    
    message(STATUS "Google Test found - comprehensive consensus test suite enabled including harmony, router, balancer, monitor, config manager, conflict resolver, parameter adjuster, unified validator, emergency mode, enhanced blockchain, PoS, PoRC, voting consensus, smart contract consensus, and integration tests")
else()
    message(WARNING "Google Test not found - consensus harmony and router tests disabled")
endif()