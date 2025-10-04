# Consensus API Documentation

## Overview

The Nilotic Blockchain Consensus API provides comprehensive endpoints for monitoring, managing, and interacting with the consensus harmony system. This API supports multiple consensus mechanisms including Proof of Work (PoW), Proof of Stake (PoS), Proof of Resource Contribution (PoRC), Voting Consensus, and Smart Contract Validation.

## Base URL

```
http://localhost:8080
```

## Authentication

Mutating operations (POST requests) require Bearer token authentication:

```
Authorization: Bearer <your_token>
```

## Endpoints

### 1. Consensus Status

Get the current status of all consensus mechanisms.

**Endpoint:** `GET /consensus/status`

**Response:**

```json
{
  "status": "success",
  "initialized": true,
  "running": true,
  "emergency_mode": false,
  "active_engines": [
    "PROOF_OF_WORK",
    "PROOF_OF_STAKE",
    "PROOF_OF_RESOURCE_CONTRIBUTION",
    "VOTING_CONSENSUS",
    "SMART_CONTRACT_VALIDATION"
  ],
  "detailed_status": {
    "total_validations": 1250,
    "successful_validations": 1248,
    "conflict_count": 2,
    "last_update": "2024-10-02T10:30:00Z"
  }
}
```

**Example Usage:**

```bash
curl -X GET http://localhost:8080/consensus/status
```

```javascript
// JavaScript
fetch("http://localhost:8080/consensus/status")
  .then((response) => response.json())
  .then((data) => console.log(data));
```

```python
# Python
import requests
response = requests.get('http://localhost:8080/consensus/status')
print(response.json())
```

### 2. Consensus Metrics

Get detailed performance metrics for all consensus mechanisms.

**Endpoint:** `GET /consensus/metrics`

**Response:**

```json
{
  "status": "success",
  "timestamp": 1696248600,
  "metrics": {
    "PROOF_OF_WORK": {
      "hash_rate": "125.5 TH/s",
      "difficulty": 6,
      "blocks_mined": 1024,
      "average_block_time": 598.2
    },
    "PROOF_OF_STAKE": {
      "total_stake": 50000000.0,
      "active_validators": 150,
      "stake_participation": 0.85,
      "validator_uptime": 0.99
    },
    "PROOF_OF_RESOURCE_CONTRIBUTION": {
      "total_contributors": 500,
      "compute_contributions": 1250.5,
      "storage_contributions": 2048.0,
      "bandwidth_contributions": 512.3
    },
    "VOTING_CONSENSUS": {
      "active_proposals": 3,
      "total_votes_cast": 25000,
      "participation_rate": 0.72,
      "governance_decisions": 45
    },
    "SMART_CONTRACT_VALIDATION": {
      "contracts_validated": 1500,
      "validation_success_rate": 0.998,
      "average_validation_time": 125.5
    }
  }
}
```

**Example Usage:**

```bash
curl -X GET http://localhost:8080/consensus/metrics
```

### 3. Active Consensus Engines

Get information about currently active consensus engines and their parameters.

**Endpoint:** `GET /consensus/engines`

**Response:**

```json
{
  "status": "success",
  "engines": [
    {
      "type": "PROOF_OF_WORK",
      "name": "Proof of Work",
      "parameters": {
        "difficulty": 6.0,
        "target_block_time": 600.0,
        "hash_algorithm": "SHA-256"
      }
    },
    {
      "type": "PROOF_OF_STAKE",
      "name": "Proof of Stake",
      "parameters": {
        "min_stake_amount": 1000.0,
        "staking_period": 86400.0,
        "validator_selection": "weighted_random"
      }
    },
    {
      "type": "PROOF_OF_RESOURCE_CONTRIBUTION",
      "name": "Proof of Resource Contribution",
      "parameters": {
        "min_resource_contribution": 100.0,
        "accepted_resource_types": ["COMPUTE", "STORAGE", "BANDWIDTH"],
        "quality_threshold": 0.8
      }
    }
  ]
}
```

### 4. Consensus Configuration

#### Get Configuration

**Endpoint:** `GET /consensus/config`

**Response:**

```json
{
  "status": "success",
  "config": {
    "pow": {
      "difficulty": 6,
      "target_block_time": 600
    },
    "pos": {
      "min_stake_amount": 1000.0,
      "staking_period": 86400
    },
    "porc": {
      "min_resource_contribution": 100.0,
      "accepted_resource_types": ["COMPUTE", "STORAGE", "BANDWIDTH"]
    },
    "voting": {
      "supermajority_threshold": 0.67,
      "voting_period": 604800
    },
    "balancing": {
      "max_dominance_ratio": 0.6,
      "rebalancing_interval": 3600
    }
  }
}
```

#### Update Configuration

**Endpoint:** `POST /consensus/config`

**Authentication:** Required

**Request Body:**

```json
{
  "pow": {
    "difficulty": 7,
    "target_block_time": 550
  },
  "pos": {
    "min_stake_amount": 1500.0
  }
}
```

**Response:**

```json
{
  "status": "success",
  "message": "Configuration updated successfully",
  "config": {
    // Updated configuration object
  }
}
```

**Example Usage:**

```bash
curl -X POST http://localhost:8080/consensus/config \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your_token" \
  -d '{"pow": {"difficulty": 7}}'
```

### 5. Manual Parameter Adjustment

Adjust specific parameters for individual consensus mechanisms.

**Endpoint:** `POST /consensus/parameters`

**Authentication:** Required

**Request Body:**

```json
{
  "consensus_type": "PROOF_OF_WORK",
  "parameter": "difficulty",
  "value": 7.0
}
```

**Response:**

```json
{
  "status": "success",
  "message": "Parameter adjusted successfully",
  "consensus_type": "PROOF_OF_WORK",
  "parameter": "difficulty",
  "value": 7.0
}
```

**Supported Parameters by Consensus Type:**

- **PROOF_OF_WORK**: `difficulty`, `target_block_time`
- **PROOF_OF_STAKE**: `min_stake_amount`, `staking_period`
- **PROOF_OF_RESOURCE_CONTRIBUTION**: `min_resource_contribution`, `quality_threshold`
- **VOTING_CONSENSUS**: `supermajority_threshold`, `voting_period`
- **SMART_CONTRACT_VALIDATION**: `validation_timeout`, `gas_limit`

**Example Usage:**

```bash
curl -X POST http://localhost:8080/consensus/parameters \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your_token" \
  -d '{
    "consensus_type": "PROOF_OF_STAKE",
    "parameter": "min_stake_amount",
    "value": 2000.0
  }'
```

### 6. Emergency Mode Management

#### Get Emergency Status

**Endpoint:** `GET /consensus/emergency`

**Response:**

```json
{
  "status": "success",
  "emergency_mode": false,
  "emergency_details": null
}
```

Or when in emergency mode:

```json
{
  "status": "success",
  "emergency_mode": true,
  "emergency_details": {
    "type": "CONSENSUS_CONFLICT",
    "severity": "HIGH",
    "activated_at": "2024-10-02T10:15:00Z",
    "description": "Multiple consensus mechanisms reporting conflicting results",
    "affected_mechanisms": ["PROOF_OF_WORK", "PROOF_OF_STAKE"],
    "recovery_strategy": "MOST_RESTRICTIVE_VALIDATION"
  }
}
```

#### Enter Emergency Mode

**Endpoint:** `POST /consensus/emergency/enter`

**Authentication:** Required

**Response:**

```json
{
  "status": "success",
  "message": "Emergency mode activated"
}
```

#### Exit Emergency Mode

**Endpoint:** `POST /consensus/emergency/exit`

**Authentication:** Required

**Response:**

```json
{
  "status": "success",
  "message": "Emergency mode deactivated"
}
```

## WebSocket Real-Time Monitoring

For real-time consensus monitoring, connect to the WebSocket endpoint.

**Endpoint:** `ws://localhost:8080/consensus/monitor`

**Connection Example:**

```javascript
// JavaScript WebSocket connection
const ws = new WebSocket("ws://localhost:8080/consensus/monitor");

ws.onopen = function (event) {
  console.log("Connected to consensus monitoring");
};

ws.onmessage = function (event) {
  const data = JSON.parse(event.data);
  console.log("Consensus update:", data);
};

ws.onclose = function (event) {
  console.log("Disconnected from consensus monitoring");
};
```

**Real-time Data Format:**

```json
{
  "type": "consensus_update",
  "timestamp": 1696248600,
  "status": {
    "initialized": true,
    "running": true,
    "emergency_mode": false,
    "active_engines": ["PROOF_OF_WORK", "PROOF_OF_STAKE"]
  },
  "metrics": {
    "PROOF_OF_WORK": {
      "current_difficulty": 6,
      "hash_rate": "125.5 TH/s",
      "last_block_time": 595.2
    },
    "PROOF_OF_STAKE": {
      "active_validators": 150,
      "current_stake": 50000000.0
    }
  }
}
```

**Python WebSocket Example:**

```python
import asyncio
import websockets
import json

async def monitor_consensus():
    uri = "ws://localhost:8080/consensus/monitor"
    async with websockets.connect(uri) as websocket:
        while True:
            message = await websocket.recv()
            data = json.loads(message)
            print(f"Consensus update: {data}")

# Run the monitoring
asyncio.run(monitor_consensus())
```

## Error Responses

All endpoints return consistent error responses:

```json
{
  "error": "Error description",
  "code": "ERROR_CODE",
  "timestamp": 1696248600
}
```

**Common Error Codes:**

- `401 Unauthorized`: Missing or invalid authentication token
- `400 Bad Request`: Invalid request parameters
- `404 Not Found`: Endpoint not found
- `500 Internal Server Error`: Server-side error
- `503 Service Unavailable`: Consensus manager not available

## Rate Limiting

API endpoints are rate-limited to prevent abuse:

- **GET requests**: 100 requests per minute
- **POST requests**: 20 requests per minute
- **WebSocket connections**: 5 concurrent connections per IP

## Security Considerations

1. **Authentication**: All mutating operations require valid Bearer tokens
2. **HTTPS**: Use HTTPS in production environments
3. **Input Validation**: All inputs are validated and sanitized
4. **Rate Limiting**: Prevents API abuse and DoS attacks
5. **CORS**: Configured for cross-origin requests

## Integration Examples

### Monitoring Dashboard

```javascript
class ConsensusDashboard {
  constructor() {
    this.ws = null;
    this.reconnectInterval = 5000;
  }

  connect() {
    this.ws = new WebSocket("ws://localhost:8080/consensus/monitor");

    this.ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      this.updateDashboard(data);
    };

    this.ws.onclose = () => {
      setTimeout(() => this.connect(), this.reconnectInterval);
    };
  }

  updateDashboard(data) {
    // Update UI with real-time consensus data
    document.getElementById("consensus-status").textContent = data.status
      .running
      ? "Running"
      : "Stopped";

    // Update metrics charts
    this.updateMetricsCharts(data.metrics);
  }

  async adjustParameter(consensusType, parameter, value) {
    const response = await fetch("/consensus/parameters", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${this.token}`,
      },
      body: JSON.stringify({
        consensus_type: consensusType,
        parameter: parameter,
        value: value,
      }),
    });

    return response.json();
  }
}
```

### Health Check Service

```python
import requests
import time
import logging

class ConsensusHealthChecker:
    def __init__(self, api_base_url):
        self.api_base_url = api_base_url
        self.logger = logging.getLogger(__name__)

    def check_consensus_health(self):
        try:
            response = requests.get(f"{self.api_base_url}/consensus/status")
            data = response.json()

            if not data.get('running', False):
                self.logger.warning("Consensus system is not running")
                return False

            if data.get('emergency_mode', False):
                self.logger.error("Consensus system is in emergency mode")
                return False

            return True
        except Exception as e:
            self.logger.error(f"Health check failed: {e}")
            return False

    def monitor_continuously(self, interval=30):
        while True:
            if not self.check_consensus_health():
                # Send alert to administrators
                self.send_alert("Consensus system health check failed")

            time.sleep(interval)
```

## Testing

The API includes comprehensive test suites:

1. **Unit Tests**: `tests/consensus_api_test.cpp`
2. **WebSocket Tests**: `tests/consensus_websocket_test.cpp`
3. **Integration Tests**: `tests/simple_consensus_api_test.cpp`

Run tests with:

```bash
# HTTP API tests
make -f tests/Makefile_consensus_api test

# WebSocket tests
make -f tests/Makefile_consensus_websocket test

# Simple implementation verification
g++ -o simple_test tests/simple_consensus_api_test.cpp && ./simple_test
```

## Changelog

### Version 1.0.0

- Initial release with all consensus API endpoints
- WebSocket support for real-time monitoring
- Comprehensive authentication and security features
- Full test suite and documentation

## Support

For issues and questions:

- GitHub Issues: [Nilotic Blockchain Repository]
- Documentation: `/docs/CONSENSUS_API.md`
- API Tests: `/tests/consensus_*_test.cpp`
