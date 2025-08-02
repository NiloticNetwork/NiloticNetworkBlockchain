# Nilotic Blockchain Installation Guide

This document provides instructions for setting up and running the Nilotic Blockchain application on different operating systems.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Linux Installation](#linux-installation)
- [macOS Installation](#macos-installation)
- [Windows Installation](#windows-installation)
- [Running the Application](#running-the-application)
- [Configuration Options](#configuration-options)
- [Troubleshooting](#troubleshooting)

## Prerequisites

To build and run the Nilotic Blockchain, you'll need the following dependencies:

- C++ compiler with C++17 support (GCC 8+, Clang 6+, MSVC 2019+)
- CMake 3.10 or higher
- OpenSSL 1.1.1 or higher
- SQLite 3
- Git (for cloning the repository)
- Make (on Linux/macOS) or equivalent build tool
- nlohmann/json library (included in the repository's lib directory)

## Linux Installation

### Ubuntu/Debian

```bash
# Install required packages
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libsqlite3-dev git

# Clone the repository
git clone https://github.com/yourusername/nilotic-blockchain.git
cd nilotic-blockchain

# Build the project
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

### Fedora/CentOS/RHEL

```bash
# Install required packages
sudo dnf install -y gcc-c++ cmake openssl-devel sqlite-devel git make

# Clone the repository
git clone https://github.com/yourusername/nilotic-blockchain.git
cd nilotic-blockchain

# Build the project
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

### Arch Linux

```bash
# Install required packages
sudo pacman -S gcc cmake openssl sqlite git make

# Clone the repository
git clone https://github.com/yourusername/nilotic-blockchain.git
cd nilotic-blockchain

# Build the project
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

## macOS Installation

### Using Homebrew

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install cmake openssl sqlite

# Clone the repository
git clone https://github.com/yourusername/nilotic-blockchain.git
cd nilotic-blockchain

# Build the project (specifying OpenSSL path explicitly for M1 Macs)
mkdir -p build && cd build
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
make -j$(sysctl -n hw.ncpu)

# Optional: Install to /usr/local
sudo make install
```

### Notes for Apple Silicon (M1/M2 chips)

On Apple Silicon Macs, you may need to use Rosetta 2 for some dependencies or ensure that all dependencies are built for arm64 architecture.

```bash
# Ensure Homebrew is installed for Apple Silicon
arch -arm64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Use the arm64 version of Homebrew
arch -arm64 brew install cmake openssl sqlite

# Build specifying architecture and SSL path
mkdir -p build && cd build
arch -arm64 cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) -DCMAKE_OSX_ARCHITECTURES=arm64
arch -arm64 make -j$(sysctl -n hw.ncpu)
```

## Windows Installation

### Using Visual Studio

1. Install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) or newer with "Desktop development with C++" workload
2. Install [CMake](https://cmake.org/download/)
3. Install [Git](https://git-scm.com/download/win)
4. Install OpenSSL:
   - Download the installer from [OpenSSL for Windows](https://slproweb.com/products/Win32OpenSSL.html)
   - Choose the appropriate version (32 or 64 bit)
   - Run the installer

```powershell
# Clone the repository
git clone https://github.com/yourusername/nilotic-blockchain.git
cd nilotic-blockchain

# Create build directory and configure
mkdir build
cd build
cmake .. -A x64 -DOPENSSL_ROOT_DIR=C:/Path/To/OpenSSL

# Build the solution
cmake --build . --config Release
```

### Using MSYS2/MinGW

1. Install [MSYS2](https://www.msys2.org/)
2. Open MSYS2 terminal and install dependencies:

```bash
# Update package database
pacman -Syu

# Install required packages
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-openssl mingw-w64-x86_64-sqlite3 git make

# Clone the repository
git clone https://github.com/yourusername/nilotic-blockchain.git
cd nilotic-blockchain

# Build the project
mkdir -p build && cd build
cmake .. -G "MSYS Makefiles"
make -j$(nproc)
```

## Running the Application

After successful compilation, you can run the Nilotic Blockchain:

```bash
# From the build directory
./nilotic_blockchain --port 5000 --debug
```

### Basic Command Options

- `--port <port>`: Set the HTTP server port (default: 5000)
- `--debug`: Enable debug logging
- `--data-dir <path>`: Set data directory for blockchain storage
- `--peers <ip:port>`: Comma-separated list of peers to connect to
- `--no-mining`: Disable automatic mining
- `--help`: Display usage information

## Configuration Options

You can configure the Nilotic Blockchain using a configuration file:

```bash
# Run with custom config file
./nilotic_blockchain --config /path/to/config.json
```

Example configuration file (config.json):
```json
{
  "port": 5000,
  "dataDir": "./blockchain_data",
  "peers": ["192.168.1.100:5000", "192.168.1.101:5000"],
  "mining": true,
  "logLevel": "debug"
}
```

## Troubleshooting

### Common Issues

1. **OpenSSL Not Found**:
   - Linux: `sudo apt-get install libssl-dev` or equivalent for your distribution
   - macOS: `brew install openssl` and specify path with `-DOPENSSL_ROOT_DIR=$(brew --prefix openssl)`
   - Windows: Set environment variable `OPENSSL_ROOT_DIR` to OpenSSL installation directory

2. **Compilation Errors with C++17 Features**:
   - Ensure your compiler supports C++17
   - For GCC/Clang: Add `-std=c++17` to compiler flags
   - For MSVC: Use Visual Studio 2019 or newer

3. **SQLite Issues**:
   - Linux: `sudo apt-get install libsqlite3-dev` or equivalent
   - macOS: `brew install sqlite`
   - Windows: Set `SQLITE_ROOT_DIR` environment variable

4. **Port Already in Use**:
   - Change the port using `--port` option
   - Check if another instance is running: `lsof -i :5000` (Linux/macOS) or `netstat -ano | findstr 5000` (Windows)

### Getting Help

If you encounter issues not covered here, please:
1. Check the GitHub repository issues
2. Review the log file (logs/nilotic.log)
3. Submit a detailed bug report with your OS, compiler version, and exact error messages