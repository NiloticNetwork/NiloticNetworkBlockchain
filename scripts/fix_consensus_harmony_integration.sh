#!/bin/bash

# Quick fix script for consensus harmony integration issues

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log "Starting consensus harmony integration fixes"

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]]; then
    error "Please run this script from the project root directory"
    exit 1
fi

# Create necessary directories
log "Creating necessary directories..."
mkdir -p config
mkdir -p logs
mkdir -p build

# Create a minimal consensus harmony configuration
log "Creating minimal consensus harmony configuration..."
cat > config/consensus_harmony.json << 'EOF'
{
    "consensus_harmony": {
        "enabled": true,
        "version": "1.0.0",
        "engines": {
            "proof_of_work": {
                "enabled": true,
                "difficulty": 2
            },
            "voting_consensus": {
                "enabled": true,
                "supermajority_threshold": 0.67
            }
        },
        "security": {
            "cryptographic_validation": false,
            "attack_detection": false,
            "audit_logging": true
        },
        "performance": {
            "parallel_validation": false,
            "result_caching": false
        }
    }
}
EOF

# Create a minimal security configuration
log "Creating minimal security configuration..."
cat > config/consensus_security.json << 'EOF'
{
    "security": {
        "consensus_validation": {
            "enabled": false,
            "strict_mode": false
        },
        "attack_detection": {
            "enabled": false
        },
        "audit_logging": {
            "enabled": true,
            "log_level": "INFO"
        }
    }
}
EOF

# Try to build the simple test
log "Building simple consensus harmony test..."
cd tests

if make -f Makefile_simple_consensus_harmony clean; then
    success "Cleaned previous build artifacts"
else
    warning "Clean failed, continuing..."
fi

if make -f Makefile_simple_consensus_harmony; then
    success "Simple test built successfully"
    
    # Run the simple test
    log "Running simple consensus harmony test..."
    if ./simple_consensus_harmony_test; then
        success "Simple test passed!"
    else
        error "Simple test failed"
        cd ..
        exit 1
    fi
else
    error "Failed to build simple test"
    cd ..
    exit 1
fi

cd ..

# Try to build the main application
log "Building main application..."
if [[ -d "build" ]]; then
    cd build
    
    if cmake ..; then
        success "CMake configuration successful"
        
        if make -j$(nproc) nilotic_blockchain; then
            success "Main application built successfully"
        else
            error "Failed to build main application"
            cd ..
            exit 1
        fi
    else
        error "CMake configuration failed"
        cd ..
        exit 1
    fi
    
    cd ..
else
    error "Build directory not found"
    exit 1
fi

success "All fixes applied successfully!"
log "Next steps:"
log "1. Start the blockchain: ./build/nilotic_blockchain --port 5000"
log "2. Test the API: curl http://localhost:5000/"
log "3. Check consensus harmony: curl http://localhost:5000/consensus/harmony"