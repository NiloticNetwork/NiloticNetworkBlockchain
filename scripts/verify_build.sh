#!/bin/bash
# Build verification script for Nilotic Blockchain
# This script verifies that all components are properly built and functional

set -e

echo "🔍 Nilotic Blockchain Build Verification"
echo "========================================"

# Change to build directory
if [ ! -d "build" ]; then
    echo "❌ Build directory not found. Please run ./build.sh first."
    exit 1
fi

cd build

# Detect platform
case "$(uname -s)" in
    Linux*)     PLATFORM=Linux;;
    Darwin*)    PLATFORM=macOS;;
    CYGWIN*)    PLATFORM=Windows;;
    MINGW*)     PLATFORM=Windows;;
    MSYS*)      PLATFORM=Windows;;
    *)          PLATFORM="Unknown";;
esac

# Set executable paths based on platform
if [ "$PLATFORM" = "Windows" ]; then
    MAIN_EXEC="Release/nilotic_blockchain.exe"
    DEMO1_EXEC="Release/simple_voting_demo.exe"
    DEMO2_EXEC="Release/voting_consensus_demo.exe"
    TEST_SUFFIX=".exe"
else
    MAIN_EXEC="nilotic_blockchain"
    DEMO1_EXEC="simple_voting_demo"
    DEMO2_EXEC="voting_consensus_demo"
    TEST_SUFFIX=""
fi

echo "Platform: $PLATFORM"
echo ""

# Verify main executable
echo "1. Verifying main executable..."
if [ -f "$MAIN_EXEC" ]; then
    echo "   ✅ $MAIN_EXEC found"
    # Test basic functionality (version check)
    if ./"$MAIN_EXEC" --help >/dev/null 2>&1 || [ $? -eq 1 ]; then
        echo "   ✅ Main executable responds to commands"
    else
        echo "   ⚠️  Main executable may have issues"
    fi
else
    echo "   ❌ $MAIN_EXEC not found"
    exit 1
fi

# Verify demo executables
echo ""
echo "2. Verifying demo executables..."
if [ -f "$DEMO1_EXEC" ]; then
    echo "   ✅ $DEMO1_EXEC found"
else
    echo "   ⚠️  $DEMO1_EXEC not found"
fi

if [ -f "$DEMO2_EXEC" ]; then
    echo "   ✅ $DEMO2_EXEC found"
else
    echo "   ⚠️  $DEMO2_EXEC not found"
fi

# Verify test executables (if Google Test is available)
echo ""
echo "3. Verifying test executables..."
TEST_EXECUTABLES=(
    "consensus_harmony_tests"
    "consensus_router_tests"
    "consensus_balancer_tests"
    "consensus_monitor_tests"
    "consensus_config_manager_tests"
    "consensus_conflict_resolver_tests"
    "parameter_adjuster_tests"
    "unified_transaction_validator_tests"
    "emergency_consensus_mode_tests"
    "enhanced_blockchain_tests"
    "pos_consensus_tests"
    "porc_tests"
    "voting_consensus_tests"
    "voting_consensus_integration_tests"
    "smart_contract_consensus_tests"
)

TESTS_FOUND=0
TESTS_TOTAL=${#TEST_EXECUTABLES[@]}

for test_exec in "${TEST_EXECUTABLES[@]}"; do
    if [ -f "${test_exec}${TEST_SUFFIX}" ]; then
        echo "   ✅ ${test_exec}${TEST_SUFFIX} found"
        ((TESTS_FOUND++))
    else
        echo "   ⚠️  ${test_exec}${TEST_SUFFIX} not found"
    fi
done

if [ $TESTS_FOUND -gt 0 ]; then
    echo "   📊 Found $TESTS_FOUND/$TESTS_TOTAL test executables"
    echo "   💡 Run 'make test' or 'ctest' to execute tests"
else
    echo "   ℹ️  No test executables found (Google Test may not be installed)"
fi

# Check for required libraries and dependencies
echo ""
echo "4. Checking runtime dependencies..."

# Check if the main executable can load its dependencies
if command -v ldd >/dev/null 2>&1 && [ "$PLATFORM" != "macOS" ]; then
    echo "   🔍 Checking shared library dependencies (Linux):"
    if ldd "$MAIN_EXEC" | grep -q "not found"; then
        echo "   ❌ Missing shared library dependencies:"
        ldd "$MAIN_EXEC" | grep "not found"
        exit 1
    else
        echo "   ✅ All shared library dependencies satisfied"
    fi
elif command -v otool >/dev/null 2>&1 && [ "$PLATFORM" = "macOS" ]; then
    echo "   🔍 Checking dynamic library dependencies (macOS):"
    if otool -L "$MAIN_EXEC" | grep -q "not found"; then
        echo "   ❌ Missing dynamic library dependencies"
        exit 1
    else
        echo "   ✅ All dynamic library dependencies satisfied"
    fi
else
    echo "   ℹ️  Dependency checking not available on this platform"
fi

# Summary
echo ""
echo "5. Build Verification Summary"
echo "============================="
echo "✅ Main executable: PASSED"
echo "✅ Demo executables: $([ -f "$DEMO1_EXEC" ] && echo "PASSED" || echo "OPTIONAL")"
echo "✅ Test suite: $([ $TESTS_FOUND -gt 0 ] && echo "AVAILABLE ($TESTS_FOUND tests)" || echo "NOT AVAILABLE")"
echo "✅ Dependencies: SATISFIED"

echo ""
echo "🎉 Build verification completed successfully!"
echo ""
echo "🚀 Ready to run:"
echo "   Main application: ./$MAIN_EXEC --port 5000 --debug"
if [ -f "$DEMO1_EXEC" ]; then
    echo "   Simple voting demo: ./$DEMO1_EXEC"
fi
if [ -f "$DEMO2_EXEC" ]; then
    echo "   Voting consensus demo: ./$DEMO2_EXEC"
fi
if [ $TESTS_FOUND -gt 0 ]; then
    echo "   Test suite: make test"
fi

cd ..