#!/bin/bash
# Cross-platform build script for Nilotic Blockchain

# Detect operating system
case "$(uname -s)" in
    Linux*)     PLATFORM=Linux;;
    Darwin*)    PLATFORM=macOS;;
    CYGWIN*)    PLATFORM=Windows;;
    MINGW*)     PLATFORM=Windows;;
    MSYS*)      PLATFORM=Windows;;
    *)          PLATFORM="Unknown";;
esac

echo "🔄 Detected platform: $PLATFORM"

# Determine number of CPU cores for parallel build
if [ "$PLATFORM" = "Linux" ]; then
    CORES=$(nproc)
elif [ "$PLATFORM" = "macOS" ]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4  # Default for Windows or unknown
fi

# Check for required tools
echo "🔍 Checking for required tools..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake is not installed or not in PATH"
    echo "Please install CMake and try again:"
    echo "- Linux: sudo apt-get install cmake"
    echo "- macOS: brew install cmake"
    echo "- Windows: Download from https://cmake.org/download/"
    exit 1
fi
echo "✅ CMake is installed"

# Check for C++ compiler
if [ "$PLATFORM" = "Linux" ] || [ "$PLATFORM" = "macOS" ]; then
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        echo "❌ No C++ compiler found"
        echo "Please install a C++ compiler and try again:"
        echo "- Linux: sudo apt-get install g++"
        echo "- macOS: xcode-select --install"
        exit 1
    fi
    echo "✅ C++ compiler is installed"
fi

# Create build directory
echo "📁 Creating build directory..."
mkdir -p build
cd build || { echo "❌ Failed to enter build directory"; exit 1; }

# Configure with CMake
echo "⚙️ Configuring with CMake..."

CMAKE_OPTIONS=""

# Platform-specific configuration
if [ "$PLATFORM" = "macOS" ]; then
    # Check if OpenSSL is installed via Homebrew and set path
    if command -v brew &> /dev/null && [ -d "$(brew --prefix openssl)" ]; then
        CMAKE_OPTIONS="-DOPENSSL_ROOT_DIR=$(brew --prefix openssl)"
        echo "🔐 Using OpenSSL from Homebrew at $(brew --prefix openssl)"
    fi
    
    # Check for Apple Silicon
    if [ "$(uname -m)" = "arm64" ]; then
        CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_OSX_ARCHITECTURES=arm64"
        echo "🍎 Detected Apple Silicon, configuring for arm64 architecture"
    fi
elif [ "$PLATFORM" = "Windows" ]; then
    # For MSYS/MinGW environments
    if [ "$(uname -o 2>/dev/null)" = "Msys" ]; then
        CMAKE_OPTIONS="-G \"MSYS Makefiles\""
    fi
fi

# Execute CMake
eval "cmake .. $CMAKE_OPTIONS"
if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

# Build the project
echo "🔨 Building Nilotic Blockchain (using $CORES cores)..."
if [ "$PLATFORM" = "Windows" ]; then
    # For Windows with Visual Studio
    if ! eval "cmake --build . --config Release"; then
        echo "❌ Build failed"
        exit 1
    fi
else
    # For Linux/macOS
    if ! make -j "$CORES"; then
        echo "❌ Build failed"
        exit 1
    fi
fi

echo "✅ Build completed successfully!"

# Show executable location
if [ "$PLATFORM" = "Windows" ]; then
    echo "🚀 Executable built at: $(pwd)/Release/nilotic_blockchain.exe"
    echo "📝 Run with: Release\\nilotic_blockchain.exe --port 5000 --debug"
else
    echo "🚀 Executable built at: $(pwd)/nilotic_blockchain"
    echo "📝 Run with: ./nilotic_blockchain --port 5000 --debug"
fi

cd ..

echo "✨ Nilotic Blockchain build process complete! ✨"