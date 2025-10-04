#!/bin/bash

# Migration script for Consensus Harmony Integration
# This script helps migrate existing Nilotic Blockchain data to support the new consensus harmony system

set -e  # Exit on any error

# Configuration
BACKUP_DIR="backup_$(date +%Y%m%d_%H%M%S)"
CONFIG_DIR="config"
DATA_FILE="blockchain_data.json"
LOG_FILE="migration_$(date +%Y%m%d_%H%M%S).log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging function
log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

# Check if running as root (not recommended)
check_permissions() {
    if [[ $EUID -eq 0 ]]; then
        warning "Running as root is not recommended. Consider running as a regular user."
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# Create backup of existing data
create_backup() {
    log "Creating backup of existing data..."
    
    mkdir -p "$BACKUP_DIR"
    
    # Backup blockchain data
    if [[ -f "$DATA_FILE" ]]; then
        cp "$DATA_FILE" "$BACKUP_DIR/"
        success "Backed up blockchain data to $BACKUP_DIR/$DATA_FILE"
    else
        warning "No existing blockchain data file found"
    fi
    
    # Backup configuration files
    if [[ -d "$CONFIG_DIR" ]]; then
        cp -r "$CONFIG_DIR" "$BACKUP_DIR/"
        success "Backed up configuration directory to $BACKUP_DIR/$CONFIG_DIR"
    else
        warning "No existing configuration directory found"
    fi
    
    # Backup logs
    if ls logs/*.log 1> /dev/null 2>&1; then
        mkdir -p "$BACKUP_DIR/logs"
        cp logs/*.log "$BACKUP_DIR/logs/"
        success "Backed up log files to $BACKUP_DIR/logs"
    fi
    
    success "Backup completed in directory: $BACKUP_DIR"
}

# Create necessary directories
create_directories() {
    log "Creating necessary directories..."
    
    mkdir -p "$CONFIG_DIR"
    mkdir -p "logs"
    mkdir -p "scripts/deploy"
    mkdir -p "scripts/test"
    
    success "Directories created successfully"
}

# Create consensus harmony configuration
create_harmony_config() {
    log "Creating consensus harmony configuration..."
    
    cat > "$CONFIG_DIR/consensus_harmony.json" << EOF
{
    "consensus_harmony": {
        "enabled": true,
        "version": "1.0.0",
        "engines": {
            "proof_of_work": {
                "enabled": true,
                "difficulty": 4,
                "target_block_time": 600,
                "max_difficulty": 8,
                "min_difficulty": 2
            },
            "proof_of_stake": {
                "enabled": true,
                "min_stake": 100.0,
                "staking_period": 86400,
                "validator_selection": "weighted_random"
            },
            "proof_of_resource_contribution": {
                "enabled": true,
                "min_contribution": 10.0,
                "resource_types": ["compute", "storage", "bandwidth"],
                "quality_threshold": 0.8
            },
            "voting_consensus": {
                "enabled": true,
                "supermajority_threshold": 0.67,
                "voting_period": 604800,
                "min_voting_power": 1.0
            },
            "smart_contract_validation": {
                "enabled": true,
                "gas_limit": 1000000,
                "execution_timeout": 30
            }
        },
        "routing": {
            "block_validation": {
                "required": ["proof_of_work", "proof_of_stake"],
                "optional": ["proof_of_resource_contribution"],
                "strategy": "hierarchical"
            },
            "transaction_validation": {
                "required": ["proof_of_work"],
                "optional": ["proof_of_stake", "proof_of_resource_contribution"],
                "strategy": "most_restrictive"
            },
            "governance_proposal": {
                "required": ["voting_consensus"],
                "optional": ["proof_of_stake"],
                "strategy": "weighted_majority"
            },
            "smart_contract_execution": {
                "required": ["smart_contract_validation"],
                "optional": ["proof_of_work", "proof_of_stake"],
                "strategy": "unanimous"
            }
        },
        "security": {
            "cryptographic_validation": true,
            "attack_detection": true,
            "audit_logging": true,
            "rate_limiting": true,
            "threat_monitoring": true
        },
        "performance": {
            "parallel_validation": true,
            "result_caching": true,
            "computation_memoization": true,
            "memory_optimization": true
        },
        "emergency": {
            "enabled": true,
            "auto_activation_threshold": 0.8,
            "recovery_timeout": 3600,
            "backup_mechanisms": ["proof_of_work", "proof_of_stake"]
        }
    }
}
EOF
    
    success "Consensus harmony configuration created"
}

# Update security configuration
update_security_config() {
    log "Updating security configuration..."
    
    cat > "$CONFIG_DIR/consensus_security.json" << EOF
{
    "security": {
        "consensus_validation": {
            "enabled": true,
            "strict_mode": true,
            "signature_verification": true,
            "hash_validation": true
        },
        "attack_detection": {
            "enabled": true,
            "51_percent_attack": true,
            "double_spending": true,
            "sybil_attack": true,
            "eclipse_attack": true
        },
        "audit_logging": {
            "enabled": true,
            "log_level": "INFO",
            "log_rotation": true,
            "max_log_size": "100MB",
            "retention_days": 30
        },
        "rate_limiting": {
            "enabled": true,
            "requests_per_minute": 60,
            "burst_limit": 100,
            "ban_duration": 300
        },
        "encryption": {
            "enabled": true,
            "algorithm": "AES-256-GCM",
            "key_rotation_interval": 86400
        }
    }
}
EOF
    
    success "Security configuration updated"
}

# Validate existing blockchain data
validate_blockchain_data() {
    log "Validating existing blockchain data..."
    
    if [[ -f "$DATA_FILE" ]]; then
        # Check if the file is valid JSON
        if python3 -m json.tool "$DATA_FILE" > /dev/null 2>&1; then
            success "Blockchain data file is valid JSON"
        else
            error "Blockchain data file is not valid JSON"
            return 1
        fi
        
        # Check file size
        file_size=$(stat -f%z "$DATA_FILE" 2>/dev/null || stat -c%s "$DATA_FILE" 2>/dev/null)
        if [[ $file_size -gt 0 ]]; then
            success "Blockchain data file is not empty ($file_size bytes)"
        else
            warning "Blockchain data file is empty"
        fi
    else
        warning "No existing blockchain data file found - will start with genesis block"
    fi
}

# Build the consensus harmony integration
build_integration() {
    log "Building consensus harmony integration..."
    
    if [[ -f "CMakeLists.txt" ]]; then
        # Use CMake build
        mkdir -p build
        cd build
        cmake ..
        make -j$(nproc)
        cd ..
        success "Built using CMake"
    elif [[ -f "Makefile" ]]; then
        # Use Makefile
        make clean
        make -j$(nproc)
        success "Built using Makefile"
    else
        warning "No build system found - manual compilation may be required"
    fi
}

# Run integration tests
run_tests() {
    log "Running consensus harmony integration tests..."
    
    if [[ -f "tests/Makefile_consensus_harmony_integration" ]]; then
        cd tests
        make -f Makefile_consensus_harmony_integration clean
        make -f Makefile_consensus_harmony_integration test
        cd ..
        success "Integration tests completed successfully"
    else
        warning "Integration test Makefile not found - skipping tests"
    fi
}

# Create systemd service file (optional)
create_systemd_service() {
    if command -v systemctl &> /dev/null; then
        log "Creating systemd service file..."
        
        cat > "nilotic-blockchain-harmony.service" << EOF
[Unit]
Description=Nilotic Blockchain with Consensus Harmony
After=network.target

[Service]
Type=simple
User=nilotic
Group=nilotic
WorkingDirectory=$(pwd)
ExecStart=$(pwd)/build/nilotic_blockchain --port 5000
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF
        
        success "Systemd service file created: nilotic-blockchain-harmony.service"
        log "To install: sudo cp nilotic-blockchain-harmony.service /etc/systemd/system/"
        log "To enable: sudo systemctl enable nilotic-blockchain-harmony"
        log "To start: sudo systemctl start nilotic-blockchain-harmony"
    fi
}

# Main migration function
main() {
    log "Starting Consensus Harmony Migration"
    log "======================================"
    
    check_permissions
    create_backup
    create_directories
    create_harmony_config
    update_security_config
    validate_blockchain_data
    
    # Ask user if they want to build and test
    read -p "Do you want to build the consensus harmony integration? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        build_integration
    fi
    
    read -p "Do you want to run integration tests? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        run_tests
    fi
    
    read -p "Do you want to create a systemd service file? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        create_systemd_service
    fi
    
    success "Migration completed successfully!"
    log "======================================"
    log "Migration Summary:"
    log "- Backup created in: $BACKUP_DIR"
    log "- Configuration files created in: $CONFIG_DIR"
    log "- Migration log: $LOG_FILE"
    log ""
    log "Next steps:"
    log "1. Review the configuration files in $CONFIG_DIR"
    log "2. Start the blockchain with: ./build/nilotic_blockchain --port 5000"
    log "3. Check the consensus harmony status at: http://localhost:5000/consensus/harmony"
    log "4. Monitor logs for any issues"
    log ""
    log "For rollback, restore files from: $BACKUP_DIR"
}

# Handle script interruption
cleanup() {
    error "Migration interrupted"
    log "Partial migration completed. Check $LOG_FILE for details."
    exit 1
}

trap cleanup INT TERM

# Run main function
main "$@"