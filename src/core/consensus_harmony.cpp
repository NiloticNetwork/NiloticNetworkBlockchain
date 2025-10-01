#include "consensus_harmony.h"
#include "utils.h"
#include <sstream>
#include <iomanip>

// ConsensusRequest implementation
std::string ConsensusRequest::generateRequestId() const {
    std::stringstream ss;
    ss << std::hex << timestamp << "_" << static_cast<int>(type);
    return Utils::calculateSHA256(ss.str()).substr(0, 16);
}

// ConsensusConfig implementation
nlohmann::json ConsensusConfig::toJson() const {
    nlohmann::json j;
    
    // PoW Configuration
    j["pow"]["difficulty"] = powDifficulty;
    j["pow"]["targetBlockTime"] = powTargetBlockTime;
    
    // PoS Configuration
    j["pos"]["minStakeAmount"] = minStakeAmount;
    j["pos"]["stakingPeriod"] = stakingPeriod;
    
    // PoRC Configuration
    j["porc"]["minResourceContribution"] = minResourceContribution;
    j["porc"]["acceptedResourceTypes"] = acceptedResourceTypes;
    
    // Voting Configuration
    j["voting"]["supermajorityThreshold"] = supermajorityThreshold;
    j["voting"]["votingPeriod"] = votingPeriod;
    
    // Balancing Configuration
    j["balancing"]["maxDominanceRatio"] = maxDominanceRatio;
    j["balancing"]["rebalancingInterval"] = rebalancingInterval;
    
    // Consensus Priority
    nlohmann::json priority;
    for (const auto& [type, prio] : consensusPriority) {
        std::string typeStr;
        switch (type) {
            case ConsensusType::PROOF_OF_WORK:
                typeStr = "PROOF_OF_WORK";
                break;
            case ConsensusType::PROOF_OF_STAKE:
                typeStr = "PROOF_OF_STAKE";
                break;
            case ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION:
                typeStr = "PROOF_OF_RESOURCE_CONTRIBUTION";
                break;
            case ConsensusType::VOTING_CONSENSUS:
                typeStr = "VOTING_CONSENSUS";
                break;
            case ConsensusType::SMART_CONTRACT_VALIDATION:
                typeStr = "SMART_CONTRACT_VALIDATION";
                break;
        }
        priority[typeStr] = prio;
    }
    j["priority"] = priority;
    
    return j;
}

void ConsensusConfig::fromJson(const nlohmann::json& j) {
    // PoW Configuration
    if (j.contains("pow")) {
        if (j["pow"].contains("difficulty")) {
            powDifficulty = j["pow"]["difficulty"];
        }
        if (j["pow"].contains("targetBlockTime")) {
            powTargetBlockTime = j["pow"]["targetBlockTime"];
        }
    }
    
    // PoS Configuration
    if (j.contains("pos")) {
        if (j["pos"].contains("minStakeAmount")) {
            minStakeAmount = j["pos"]["minStakeAmount"];
        }
        if (j["pos"].contains("stakingPeriod")) {
            stakingPeriod = j["pos"]["stakingPeriod"];
        }
    }
    
    // PoRC Configuration
    if (j.contains("porc")) {
        if (j["porc"].contains("minResourceContribution")) {
            minResourceContribution = j["porc"]["minResourceContribution"];
        }
        if (j["porc"].contains("acceptedResourceTypes")) {
            acceptedResourceTypes = j["porc"]["acceptedResourceTypes"];
        }
    }
    
    // Voting Configuration
    if (j.contains("voting")) {
        if (j["voting"].contains("supermajorityThreshold")) {
            supermajorityThreshold = j["voting"]["supermajorityThreshold"];
        }
        if (j["voting"].contains("votingPeriod")) {
            votingPeriod = j["voting"]["votingPeriod"];
        }
    }
    
    // Balancing Configuration
    if (j.contains("balancing")) {
        if (j["balancing"].contains("maxDominanceRatio")) {
            maxDominanceRatio = j["balancing"]["maxDominanceRatio"];
        }
        if (j["balancing"].contains("rebalancingInterval")) {
            rebalancingInterval = j["balancing"]["rebalancingInterval"];
        }
    }
    
    // Consensus Priority
    if (j.contains("priority")) {
        consensusPriority.clear();
        for (const auto& [typeStr, prio] : j["priority"].items()) {
            ConsensusType type;
            if (typeStr == "PROOF_OF_WORK") {
                type = ConsensusType::PROOF_OF_WORK;
            } else if (typeStr == "PROOF_OF_STAKE") {
                type = ConsensusType::PROOF_OF_STAKE;
            } else if (typeStr == "PROOF_OF_RESOURCE_CONTRIBUTION") {
                type = ConsensusType::PROOF_OF_RESOURCE_CONTRIBUTION;
            } else if (typeStr == "VOTING_CONSENSUS") {
                type = ConsensusType::VOTING_CONSENSUS;
            } else if (typeStr == "SMART_CONTRACT_VALIDATION") {
                type = ConsensusType::SMART_CONTRACT_VALIDATION;
            } else {
                continue; // Skip unknown types
            }
            consensusPriority[type] = prio;
        }
    }
}