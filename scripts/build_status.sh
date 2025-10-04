#!/bin/bash
# Build status script - shows current build configuration and available components

echo "🏗️  Nilotic Blockchain Build Status"
echo "===================================="

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "❌ No build directory found"
    echo "💡 Run './build.sh' to build the project"
    exit 1
fi

echo "✅ Build directory exists"

# Check CMakeLists.txt for configured components
echo ""
echo "📋 Configured Components:"
echo "========================"

# Extract source files from CMakeLists.txt
if [ -f "CMakeLists.txt" ]; then
    echo "Core Components:"
    grep -A 50 "add_executable(nilotic_blockchain" CMakeLists.txt | grep "src/core/" | sed 's/.*src\/core\///g' | sed 's/\.cpp.*//g' | sort | while read component; do
        if [ ! -z "$component" ]; then
            echo "   ✓ $component"
        fi
    done
    
    echo ""
    echo "Test Components:"
    grep "_tests" CMakeLists.txt | grep "add_executable" | sed 's/.*add_executable(//g' | sed 's/ .*//g' | sort | while read test; do
        echo "   🧪 $test"
    done
else
    echo "❌ CMakeLists.txt not found"
fi

# Check actual build artifacts
echo ""
echo "🔍 Built Artifacts:"
echo "=================="

cd build 2>/dev/null || { echo "❌ Cannot access build directory"; exit 1; }

# Detect platform for executable extensions
case "$(uname -s)" in
    Windows*|CYGWIN*|MINGW*|MSYS*) 
        EXEC_EXT=".exe"
        BUILD_DIR="Release"
        ;;
    *) 
        EXEC_EXT=""
        BUILD_DIR="."
        ;;
esac

# Check main executable
if [ -f "${BUILD_DIR}/nilotic_blockchain${EXEC_EXT}" ]; then
    echo "✅ Main executable: nilotic_blockchain${EXEC_EXT}"
    SIZE=$(ls -lh "${BUILD_DIR}/nilotic_blockchain${EXEC_EXT}" | awk '{print $5}')
    echo "   📏 Size: $SIZE"
else
    echo "❌ Main executable not found"
fi

# Check demo executables
echo ""
echo "Demo Executables:"
for demo in "simple_voting_demo" "voting_consensus_demo"; do
    if [ -f "${BUILD_DIR}/${demo}${EXEC_EXT}" ]; then
        echo "✅ ${demo}${EXEC_EXT}"
    else
        echo "⚠️  ${demo}${EXEC_EXT} (not built)"
    fi
done

# Check test executables
echo ""
echo "Test Executables:"
TEST_COUNT=0
for test_file in ${BUILD_DIR}/*_tests${EXEC_EXT}; do
    if [ -f "$test_file" ]; then
        basename "$test_file" | sed "s/${EXEC_EXT}$//" | sed 's/^/✅ /'
        ((TEST_COUNT++))
    fi
done

if [ $TEST_COUNT -eq 0 ]; then
    echo "⚠️  No test executables found (Google Test may not be installed)"
else
    echo "📊 Total test executables: $TEST_COUNT"
fi

cd ..

# Check for configuration files
echo ""
echo "⚙️  Configuration Files:"
echo "======================="
for config in "config/security.json" "config/consensus_security.json"; do
    if [ -f "$config" ]; then
        echo "✅ $config"
    else
        echo "⚠️  $config (optional)"
    fi
done

# Check for documentation
echo ""
echo "📚 Documentation:"
echo "================"
for doc in "README.md" "API_DOCUMENTATION.md" "docs/CONSENSUS_API.md"; do
    if [ -f "$doc" ]; then
        echo "✅ $doc"
    else
        echo "⚠️  $doc"
    fi
done

echo ""
echo "🎯 Quick Actions:"
echo "================"
echo "• Build project: ./build.sh"
echo "• Verify build: ./scripts/verify_build.sh"
echo "• Run main app: ./build/nilotic_blockchain --port 5000"
echo "• Run tests: cd build && make test"
echo "• Clean build: rm -rf build"

echo ""
echo "📈 Build Status: $([ -f "build/nilotic_blockchain" ] || [ -f "build/Release/nilotic_blockchain.exe" ] && echo "READY" || echo "NEEDS BUILD")"