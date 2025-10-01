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
    src/core/wallet.cpp
    src/core/utils.cpp
    src/core/persistence.cpp
    src/core/logger.cpp
    src/core/api.cpp
    src/core/mining.cpp
    src/core/networking.cpp
    src/core/porc.cpp
    src/core/unified_consensus.cpp
    src/core/rate_limiter.cpp
    src/core/security_middleware.cpp
    src/core/smart_contract_vm.cpp
    src/core/consensus_harmony.cpp
    src/core/consensus_harmony_manager.cpp
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

# Install
install(TARGETS nilotic_blockchain DESTINATION bin)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest QUIET)

if(GTest_FOUND)
    # Add test executable
    add_executable(consensus_harmony_tests
        tests/consensus_harmony_test.cpp
        src/core/consensus_harmony.cpp
        src/core/consensus_harmony_manager.cpp
        src/core/blockchain.cpp
        src/core/block.cpp
        src/core/transaction.cpp
        src/core/utils.cpp
        src/core/logger.cpp
    )
    
    # Link test libraries
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
    
    # Add test include directories
    target_include_directories(consensus_harmony_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${SQLITE3_INCLUDE_DIRS}
    )
    
    # Register test
    add_test(NAME ConsensusHarmonyTests COMMAND consensus_harmony_tests)
    
    message(STATUS "Google Test found - consensus harmony tests enabled")
else()
    message(WARNING "Google Test not found - consensus harmony tests disabled")
endif()