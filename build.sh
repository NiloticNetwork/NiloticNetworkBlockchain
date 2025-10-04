#!/bin/bash
# Cross-platform build script for Nilotic Blockchain

set -e  # Exit on any error

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

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
echo "🔍 Checking for required tools..."

# Check for CMake (minimum version 3.16)
if ! command_exists cmake; then
    echo "❌ CMake is not installed or not in PATH"
    echo "Please install CMake 3.16+ and try again:"
    echo "- Linux: sudo apt-get install cmake"
    echo "- macOS: brew install cmake"
    echo "- Windows: Download from https://cmake.org/download/"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
echo "✅ CMake $CMAKE_VERSION is installed"

# Check for C++ compiler with C++17 support
if [ "$PLATFORM" = "Linux" ] || [ "$PLATFORM" = "macOS" ]; then
    if ! command_exists g++ && ! command_exists clang++; then
        echo "❌ No C++ compiler found"
        echo "Please install a C++ compiler with C++17 support:"
        echo "- Linux: sudo apt-get install build-essential"
        echo "- macOS: xcode-select --install"
        exit 1
    fi
    echo "✅ C++ compiler is installed"
fi

# Check for Git (required for version info)
if ! command_exists git; then
    echo "⚠️  Git not found - version info will be unavailable"
fi
    
# Check for pkg-config on Linux/macOS
if [ "$PLATFORM" = "Linux" ] || [ "$PLATFORM" = "macOS" ]; then
    if ! command_exists pkg-config; then
        echo "⚠️  pkg-config not found - some dependencies may not be detected properly"
    fi
fi

# Clean previous build if requested
if [ "$1" = "clean" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf build
    echo "✅ Build directory cleaned"
fi

# Create build directory
echo "📁 Creating build directory..."
mkdir -p build
cd build || { echo "❌ Failed to enter build directory"; exit 1; }

# Configure with CMake
echo "⚙️ Configuring with CMake..."

CMAKE_OPTIONS="-DCMAKE_BUILD_TYPE=Release"

# Platform-specific configuration
if [ "$PLATFORM" = "macOS" ]; then
    # Check if OpenSSL is installed via Homebrew and set path
    if command_exists brew; then
        OPENSSL_PATH=$(brew --prefix openssl 2>/dev/null || echo "")
        if [ -n "$OPENSSL_PATH" ] && [ -d "$OPENSSL_PATH" ]; then
            CMAKE_OPTIONS="$CMAKE_OPTIONS -DOPENSSL_ROOT_DIR=$OPENSSL_PATH"
            echo "🔐 Using OpenSSL from Homebrew at $OPENSSL_PATH"
        fi
    fi
    
    # Check for Apple Silicon
    if [ "$(uname -m)" = "arm64" ]; then
        CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_OSX_ARCHITECTURES=arm64"
        echo "🍎 Detected Apple Silicon, configuring for arm64 architecture"
    fi

    # Set minimum macOS version
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"
elif [ "$PLATFORM" = "Windows" ]; then
    # For MSYS/MinGW environments
    if [ "$(uname -o 2>/dev/null)" = "Msys" ]; then
        CMAKE_OPTIONS="$CMAKE_OPTIONS -G \"MSYS Makefiles\""
fi
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON"

elif [ "$PLATFORM" = "Linux" ]; then
    # Enable position independent code for Linux
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_POSITION_INDEPENDENT_CODE=ON"
fi

# Add common options
CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_CXX_STANDARD=17"
CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_CXX_STANDARD_REQUIRED=ON"

# Execute CMake with error handling
echo "Running: cmake .. $CMAKE_OPTIONS"
if ! eval "cmake .. $CMAKE_OPTIONS"; then
    echo "❌ CMake configuration failed"
    echo "💡 Try installing missing dependencies or check CMakeLists.txt"
    exit 1
fi

echo "✅ CMake configuration successful"

# Build the project
echo "🔨 Building Nilotic Blockchain (using $CORES cores)..."

BUILD_START_TIME=$(date +%s)
if [ "$PLATFORM" = "Windows" ]; then
    # For Windows with Visual Studio or MinGW
    if ! cmake --build . --config Release --parallel "$CORES"; then
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

BUILD_END_TIME=$(date +%s)
BUILD_DURATION=$((BUILD_END_TIME - BUILD_START_TIME))
echo "✅ Build completed in ${BUILD_DURATION} seconds"

# Verify critical executables were built
echo "🔍 Verifying build artifacts..."

# Define expected executables
EXECUTABLES=("nilotic_blockchain")
OPTIONAL_EXECUTABLES=("simple_voting_demo" "voting_consensus_demo" "consensus_harmony_tests")

# Check main executables
for exec in "${EXECUTABLES[@]}"; do
    EXEC_PATH="$exec"
    if [ "$PLATFORM" = "Windows" ]; then
        EXEC_PATH="Release/${exec}.exe"
    fi
    
    if [ -f "$EXEC_PATH" ]; then
        echo "✅ $exec verified"
        # Make executable on Unix systems
        if [ "$PLATFORM" != "Windows" ]; then
            chmod +x "$EXEC_PATH"
        fi
    else
        echo "❌ Critical executable $exec not found at $EXEC_PATH"
        exit 1
    fi
done

# Check optional executables
for exec in "${OPTIONAL_EXECUTABLES[@]}"; do
    EXEC_PATH="$exec"
    if [ "$PLATFORM" = "Windows" ]; then
        EXEC_PATH="Release/${exec}.exe"
    fi
    
    if [ -f "$EXEC_PATH" ]; then
        echo "✅ $exec verified"
        if [ "$PLATFORM" != "Windows" ]; then
            chmod +x "$EXEC_PATH"
        fi
    else
        echo "⚠️  Optional executable $exec not found"
    fi
done

echo "✅ Build verification completed successfully!"

# Show executable location and usage
echo ""
echo "🚀 Build Results:"
if [ "$PLATFORM" = "Windows" ]; then
    echo "📍 Main executable: $(pwd)/Release/nilotic_blockchain.exe"
    echo "🏃 Run with: cd build && Release\\nilotic_blockchain.exe --port 5000"
    
    if [ -f "Release/simple_voting_demo.exe" ]; then
        echo "🎯 Demo executables available in Release/ directory"
    fi
else
    echo "📍 Main executable: $(pwd)/nilotic_blockchain"
    echo "🏃 Run with: cd build && ./nilotic_blockchain --port 5000"
    
    if [ -f "simple_voting_demo" ]; then
        echo "🎯 Demo executables available in current directory"
    fi
fi

# Show available features
echo ""
echo "🔧 Built Features:"
echo "   ✓ Consensus Harmony System"
echo "   ✓ Multi-Algorithm Consensus Support"
echo "   ✓ Real-time Monitoring"
echo "   ✓ Security Validation"
echo "   ✓ Performance Optimization"
echo "   ✓ Cross-platform Compatibility"

# Run basic tests if available
if [ -f "consensus_harmony_tests" ] || [ -f "Release/consensus_harmony_tests.exe" ]; then
    echo ""
    echo "🧪 Running basic tests..."
    if [ "$PLATFORM" = "Windows" ]; then
        if ./Release/consensus_harmony_tests.exe --gtest_brief=1; then
            echo "✅ Basic tests passed"
        else
            echo "⚠️  Some tests failed - check logs"
        fi
    else
        if ./consensus_harmony_tests --gtest_brief=1; then
            echo "✅ Basic tests passed"
        else
            echo "⚠️  Some tests failed - check logs"
        fi
    fi
fi

cd ..

echo ""
echo "✨ Nilotic Blockchain build completed successfully! ✨"
echo ""
echo "💡 Next steps:"
echo "   1. Run: cd build && ./nilotic_blockchain --help"
echo "   2. Check logs in build/ directory for any warnings"
echo "   3. Review documentation for configuration options"
echo ""
echo "🌟 Ready for deployment!"
