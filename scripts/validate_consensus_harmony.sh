#!/bin/bash

# Consensus Harmony System Validation Script
# This script validates the consensus harmony integration and system health

set -e

# Configuration
LOG_FILE="validation_$(date +%Y%m%d_%H%M%S).log"
API_BASE_URL="http://localhost:5000"
TIMEOUT=30

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
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

# Check if blockchain server is running
check_server_status() {
    log "Checking blockchain server status..."
    
    if curl -s --connect-timeout $TIMEOUT "$API_BASE_URL/" > /dev/null; then
        success "Blockchain server is running"
        return 0
    else
        error "Blockchain server is not responding"
        return 1
    fi
}

# Validate basic API endpoints
validate_basic_api() {
    log "Validating basic API endpoints..."
    
    # Test root endpoint
    response=$(curl -s --connect-timeout $TIMEOUT "$API_BASE_URL/")
    if echo "$response" | grep -q "Nilotic Blockchain API"; then
        success "Root API endpoint is working"
    else
        error "Root API endpoint failed"
        return 1
    fi
    
    # Test chain endpoint
    response=$(curl -s --connect-timeout $TIMEOUT "$API_BASE_URL/chain")
    if echo "$response" | grep -q "chain_height"; then
        success "Chain API endpoint is working"
    else
        error "Chain API endpoint failed"
        return 1
    fi
    
    return 0
}

# Validate consensus harmony API
validate_consensus_harmony_api() {
    log "Validating consensus harmony API..."
    
    # Test consensus harmony status endpoint
    response=$(curl -s --connect-timeout $TIMEOUT "$API_BASE_URL/consensus/harmony")
    if echo "$response" | grep -q "enabled"; then
        if echo "$response" | grep -q '"enabled":true'; then
            success "Consensus harmony is enabled"
            
            # Check for active engines
            if echo "$response" | grep -q "manager_status"; then
                success "Harmony manager is active"
            else
                warning "Harmony manager status not found"
            fi
            
            # Check for metrics
            if echo "$response" | grep -q "metrics"; then
                success "Consensus metrics are available"
            else
                warning "Consensus metrics not found"
            fi
            
        else
            warning "Consensus harmony is disabled"
        fi
    else
        error "Consensus harmony API endpoint failed"
        return 1
    fi
    
    return 0
}

# Test emergency mode functionality
test_emergency_mode() {
    log "Testing emergency mode functionality..."
    
    # Test emergency mode status
    response=$(curl -s --connect-timeout $TIMEOUT -X POST \
        -H "Content-Type: application/json" \
        -d '{"action":"status"}' \
        "$API_BASE_URL/consensus/harmony/emergency")
    
    if echo "$response" | grep -q "emergency_active"; then
        success "Emergency mode status endpoint is working"
        
        # Check if emergency mode is currently active
        if echo "$response" | grep -q '"emergency_active":true'; then
            warning "Emergency mode is currently active"
        else
            log "Emergency mode is currently inactive (normal)"
        fi
    else
        error "Emergency mode status endpoint failed"
        return 1
    fi
    
    return 0
}

# Validate configuration files
validate_configuration() {
    log "Validating configuration files..."
    
    # Check consensus harmony configuration
    if [[ -f "config/consensus_harmony.json" ]]; then
        if python3 -m json.tool "config/consensus_harmony.json" > /dev/null 2>&1; then
            success "Consensus harmony configuration is valid JSON"
        else
            error "Consensus harmony configuration is invalid JSON"
            return 1
        fi
    else
        warning "Consensus harmony configuration file not found"
    fi
    
    # Check security configuration
    if [[ -f "config/consensus_security.json" ]]; then
        if python3 -m json.tool "config/consensus_security.json" > /dev/null 2>&1; then
            success "Security configuration is valid JSON"
        else
            error "Security configuration is invalid JSON"
            return 1
        fi
    else
        warning "Security configuration file not found"
    fi
    
    return 0
}

# Validate blockchain data integrity
validate_blockchain_data() {
    log "Validating blockchain data integrity..."
    
    if [[ -f "blockchain_data.json" ]]; then
        # Check if file is valid JSON
        if python3 -m json.tool "blockchain_data.json" > /dev/null 2>&1; then
            success "Blockchain data file is valid JSON"
            
            # Check file size
            file_size=$(stat -f%z "blockchain_data.json" 2>/dev/null || stat -c%s "blockchain_data.json" 2>/dev/null)
            if [[ $file_size -gt 0 ]]; then
                success "Blockchain data file is not empty ($file_size bytes)"
            else
                warning "Blockchain data file is empty"
            fi
        else
            error "Blockchain data file is invalid JSON"
            return 1
        fi
    else
        warning "Blockchain data file not found"
    fi
    
    return 0
}

# Test transaction creation and validation
test_transaction_validation() {
    log "Testing transaction validation..."
    
    # Create a test transaction
    response=$(curl -s --connect-timeout $TIMEOUT -X POST \
        -H "Content-Type: application/json" \
        -d '{"sender":"test_sender","recipient":"test_recipient","amount":1.0}' \
        "$API_BASE_URL/transaction")
    
    if echo "$response" | grep -q "success"; then
        if echo "$response" | grep -q '"success":true'; then
            success "Transaction validation is working"
        else
            warning "Transaction was rejected (may be expected)"
        fi
    else
        error "Transaction validation endpoint failed"
        return 1
    fi
    
    return 0
}

# Check system resources
check_system_resources() {
    log "Checking system resources..."
    
    # Check memory usage
    if command -v free &> /dev/null; then
        memory_info=$(free -h | grep "Mem:")
        log "Memory usage: $memory_info"
    fi
    
    # Check disk space
    if command -v df &> /dev/null; then
        disk_info=$(df -h . | tail -1)
        log "Disk usage: $disk_info"
    fi
    
    # Check CPU load
    if command -v uptime &> /dev/null; then
        load_info=$(uptime)
        log "System load: $load_info"
    fi
    
    success "System resource check completed"
}

# Check log files for errors
check_log_files() {
    log "Checking log files for errors..."
    
    if [[ -d "logs" ]]; then
        error_count=0
        
        # Check for recent error messages
        if ls logs/*.log 1> /dev/null 2>&1; then
            for log_file in logs/*.log; do
                if [[ -f "$log_file" ]]; then
                    recent_errors=$(tail -100 "$log_file" | grep -i "error" | wc -l)
                    if [[ $recent_errors -gt 0 ]]; then
                        warning "Found $recent_errors recent errors in $log_file"
                        error_count=$((error_count + recent_errors))
                    fi
                fi
            done
        fi
        
        if [[ $error_count -eq 0 ]]; then
            success "No recent errors found in log files"
        else
            warning "Found $error_count total recent errors in log files"
        fi
    else
        warning "Logs directory not found"
    fi
}

# Validate consensus engine health
validate_consensus_engines() {
    log "Validating consensus engine health..."
    
    response=$(curl -s --connect-timeout $TIMEOUT "$API_BASE_URL/consensus/harmony")
    
    if echo "$response" | grep -q "engines"; then
        # Check each engine type
        engines=("mining_engine" "voting_engine" "porc_engine" "smart_contract_engine")
        
        for engine in "${engines[@]}"; do
            if echo "$response" | grep -q "$engine"; then
                if echo "$response" | grep -A 10 "$engine" | grep -q '"healthy":true'; then
                    success "$engine is healthy"
                else
                    warning "$engine may not be healthy"
                fi
            else
                warning "$engine status not found"
            fi
        done
    else
        warning "Engine status information not available"
    fi
}

# Generate validation report
generate_report() {
    log "Generating validation report..."
    
    report_file="validation_report_$(date +%Y%m%d_%H%M%S).json"
    
    cat > "$report_file" << EOF
{
    "validation_report": {
        "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
        "system_info": {
            "hostname": "$(hostname)",
            "os": "$(uname -s)",
            "architecture": "$(uname -m)",
            "kernel": "$(uname -r)"
        },
        "validation_results": {
            "server_status": "$(check_server_status && echo "PASS" || echo "FAIL")",
            "basic_api": "$(validate_basic_api && echo "PASS" || echo "FAIL")",
            "consensus_harmony_api": "$(validate_consensus_harmony_api && echo "PASS" || echo "FAIL")",
            "configuration": "$(validate_configuration && echo "PASS" || echo "FAIL")",
            "blockchain_data": "$(validate_blockchain_data && echo "PASS" || echo "FAIL")"
        },
        "recommendations": [
            "Monitor log files regularly for errors",
            "Keep configuration files backed up",
            "Ensure adequate system resources",
            "Test emergency mode procedures periodically"
        ]
    }
}
EOF
    
    success "Validation report generated: $report_file"
}

# Main validation function
main() {
    log "Starting Consensus Harmony System Validation"
    log "============================================="
    
    local exit_code=0
    
    # Run all validation checks
    check_server_status || exit_code=1
    validate_basic_api || exit_code=1
    validate_consensus_harmony_api || exit_code=1
    test_emergency_mode || exit_code=1
    validate_configuration || exit_code=1
    validate_blockchain_data || exit_code=1
    test_transaction_validation || exit_code=1
    validate_consensus_engines
    check_system_resources
    check_log_files
    
    # Generate report
    generate_report
    
    log "============================================="
    if [[ $exit_code -eq 0 ]]; then
        success "All critical validations passed!"
        log "System appears to be healthy and functioning correctly."
    else
        error "Some critical validations failed!"
        log "Please review the errors above and take corrective action."
    fi
    
    log "Validation log saved to: $LOG_FILE"
    
    return $exit_code
}

# Handle script interruption
cleanup() {
    error "Validation interrupted"
    exit 1
}

trap cleanup INT TERM

# Show help
show_help() {
    echo "Consensus Harmony System Validation Script"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -u, --url URL  Set API base URL (default: http://localhost:5000)"
    echo "  -t, --timeout  Set request timeout in seconds (default: 30)"
    echo ""
    echo "Examples:"
    echo "  $0                           # Run with default settings"
    echo "  $0 -u http://localhost:8080  # Use custom API URL"
    echo "  $0 -t 60                     # Use 60 second timeout"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -u|--url)
            API_BASE_URL="$2"
            shift 2
            ;;
        -t|--timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        *)
            error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Run main function
main "$@"