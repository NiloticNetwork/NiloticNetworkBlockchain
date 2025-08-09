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

# Add all source files (excluding problematic files for now)
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