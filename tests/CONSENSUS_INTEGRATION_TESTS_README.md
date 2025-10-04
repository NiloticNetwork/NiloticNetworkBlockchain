# Consensus Integration Tests

This directory contains comprehensive integration tests for the Nilotic Blockchain consensus harmony system. These tests verify that all consensus mechanisms work together seamlessly to ensure network security, performance, and democratic governance.

## Requirements Coverage

These tests address the following requirements from the consensus harmony specification:

- **Requirement 1.1**: All consensus mechanisms work together seamlessly
- **Requirement 1.2**: Multiple consensus mechanisms are active and prevent conflicts
- **Requirement 1.3**: Clear resolution hierarchy for consensus disagreements
- **Requirement 1.4**: Graceful fallback to alternative mechanisms on failure
- **Requirement 1.5**: Automatic adjustment of consensus parameters

## Test Suites

### 1. Comprehensive Consensus Integration Test (`comprehensive_consensus_integration_test.cpp`)

**Purpose**: Tests all consensus mechanisms working together in harmony.

**Coverage**:

- PoW, PoS, PoRC, Voting, and Smart Contract validation integration
- Multi-consensus block and transaction validation
- Consensus engine coordination and parameter adjustment
- Conflict resolution and emergency mode handling
- Performance under various load conditions

**Build & Run**:

```bash
make -f Makefile_comprehensive_consensus_integration all
make -f Makefile_comprehensive_consensus_integration test
```

**Expected Duration**: 2-3 minutes

### 2. Consensus Stress Test (`consensus_stress_test.cpp`)

**Purpose**: Tests consensus system under extreme load conditions.

**Coverage**:

- High-volume transaction validation (5,000+ transactions)
- Concurrent consensus requests (100+ threads)
- Massive conflict resolution (500+ conflicts)
- Extreme load scenarios (100+ concurrent threads)
- System stability under sustained load

**Build & Run**:

```bash
make -f Makefile_consensus_stress all
make -f Makefile_consensus_stress test
```

**Expected Duration**: 3-5 minutes
**Warning**: This test will stress your system with high CPU and memory usage.

### 3. End-to-End Workflow Test (`consensus_end_to_end_test.cpp`)

**Purpose**: Tests complete consensus workflows from start to finish.

**Coverage**:

- Complete block mining and validation workflow
- Multi-consensus transaction processing
- Governance proposal lifecycle
- Smart contract deployment and execution
- Resource contribution workflows

**Build & Run**:

```bash
make -f Makefile_consensus_end_to_end all
make -f Makefile_consensus_end_to_end test
```

**Expected Duration**: 1-2 minutes

### 4. Automated Test Suite (`automated_consensus_test_suite.cpp`)

**Purpose**: Comprehensive automated testing framework for continuous integration.

**Coverage**:

- Core functionality tests
- Integration tests
- Performance benchmarks
- Stress tests
- Regression testing
- Detailed reporting

**Build & Run**:

```bash
make -f Makefile_automated_consensus_suite all
make -f Makefile_automated_consensus_suite test
```

**Expected Duration**: 5-10 minutes
**Output**: Generates detailed HTML and text reports in `test_reports/`

## Running All Tests

### Quick Run

```bash
./run_all_consensus_integration_tests.sh
```

### Manual Run

```bash
# Run each test suite individually
make -f Makefile_comprehensive_consensus_integration test
make -f Makefile_consensus_end_to_end test
make -f Makefile_consensus_stress test
make -f Makefile_automated_consensus_suite test
```

## Test Reports

Tests generate various reports in the `test_reports/` directory:

- `test_summary.txt` - Overall test results summary
- `detailed_test_report.html` - Detailed HTML report with test results
- `test_execution.log` - Complete execution log
- `previous_results.txt` - Results for regression testing

## Prerequisites

### System Requirements

- **OS**: macOS, Linux, or Windows with WSL
- **Compiler**: g++ with C++17 support
- **Memory**: At least 2GB RAM (4GB recommended for stress tests)
- **CPU**: Multi-core processor recommended

### Dependencies

- **Build Tools**: make, g++
- **Libraries**:
  - OpenSSL (libssl-dev, libcrypto-dev)
  - SQLite3 (libsqlite3-dev)
  - pthread
- **Project Libraries**:
  - nlohmann/json (included in lib/)
  - SQLiteCpp (included in lib/)

### Installation (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev libcrypto++-dev libsqlite3-dev
```

### Installation (macOS)

```bash
brew install openssl sqlite3
```

## Test Configuration

### Environment Variables

- `CONSENSUS_TEST_MODE`: Set to "CI" for continuous integration mode
- `CONSENSUS_LOG_LEVEL`: Control logging verbosity (DEBUG, INFO, WARNING, ERROR)
- `CONSENSUS_PERFORMANCE_MODE`: Set to "BENCHMARK" for performance testing

### Performance Thresholds

- Transaction validation: < 10ms per transaction
- Block mining: < 1000ms per block
- Conflict resolution: < 100ms per conflict
- Emergency mode activation: < 500ms

## Continuous Integration

### GitHub Actions Example

```yaml
name: Consensus Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt-get install build-essential libssl-dev libsqlite3-dev
      - name: Run consensus integration tests
        run: ./tests/run_all_consensus_integration_tests.sh
      - name: Upload test reports
        uses: actions/upload-artifact@v2
        with:
          name: test-reports
          path: test_reports/
```

### GitLab CI Example

```yaml
consensus_tests:
  stage: test
  script:
    - apt-get update && apt-get install -y build-essential libssl-dev libsqlite3-dev
    - ./tests/run_all_consensus_integration_tests.sh
  artifacts:
    reports:
      junit: test_reports/junit.xml
    paths:
      - test_reports/
    expire_in: 30 days
```

## Troubleshooting

### Common Issues

1. **Build Failures**

   - Ensure all dependencies are installed
   - Check that you're running from the project root directory
   - Verify g++ supports C++17

2. **Test Timeouts**

   - Stress tests may take longer on slower systems
   - Increase timeout values in CI configuration
   - Consider running tests with fewer concurrent threads

3. **Memory Issues**

   - Stress tests require significant memory
   - Close other applications during testing
   - Consider reducing test parameters for resource-constrained environments

4. **Permission Errors**
   - Ensure test script is executable: `chmod +x run_all_consensus_integration_tests.sh`
   - Check write permissions for test_reports directory

### Debug Mode

Run tests with debug logging:

```bash
CONSENSUS_LOG_LEVEL=DEBUG ./run_all_consensus_integration_tests.sh
```

### Selective Testing

Run individual test categories:

```bash
# Only run core integration tests
make -f Makefile_comprehensive_consensus_integration test

# Only run performance tests
CONSENSUS_PERFORMANCE_MODE=BENCHMARK make -f Makefile_automated_consensus_suite test
```

## Performance Benchmarks

### Expected Performance (on modern hardware)

- **Transaction Validation**: 1,000+ tx/sec
- **Block Mining**: < 500ms per block
- **Conflict Resolution**: < 50ms per conflict
- **Concurrent Requests**: 90%+ success rate with 100+ threads

### Performance Monitoring

The automated test suite includes performance monitoring that tracks:

- Transaction throughput
- Memory usage patterns
- CPU utilization
- Response time percentiles
- System stability metrics

## Contributing

### Adding New Tests

1. Create test file in `tests/` directory
2. Follow existing naming convention: `consensus_*_test.cpp`
3. Create corresponding Makefile: `Makefile_consensus_*`
4. Add test to `run_all_consensus_integration_tests.sh`
5. Update this README with test description

### Test Guidelines

- Each test should be self-contained and not depend on external state
- Use descriptive test names and include requirement references
- Add appropriate assertions and error handling
- Include performance measurements where relevant
- Generate meaningful log output for debugging

### Code Style

- Follow existing C++ style conventions
- Use meaningful variable and function names
- Include comprehensive error handling
- Add comments for complex test logic
- Use consistent formatting and indentation

## Support

For issues with the consensus integration tests:

1. Check the troubleshooting section above
2. Review test logs in `test_reports/test_execution.log`
3. Run individual tests to isolate issues
4. Check system resources and dependencies
5. Create an issue with detailed error information

## License

These tests are part of the Nilotic Blockchain project and are subject to the same license terms.
