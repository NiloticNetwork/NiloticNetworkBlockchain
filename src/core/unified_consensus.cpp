#include "unified_consensus.h"
#include "logger.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <sqlite3.h>

// Note: Using utility functions from porc.cpp to avoid duplicate symbols

// ConsensusParticipant implementation
nlohmann::json ConsensusParticipant::toJson() const {
    nlohmann::json j;
    j["address"] = address;
    j["method"] = static_cast<int>(method);
    j["status"] = static_cast<int>(status);
    j["stake"] = stake;
    j["bandwidth"] = bandwidth;
    j["hashRate"] = hashRate;
    j["reputationScore"] = reputationScore;
    j["lastActivity"] = lastActivity;
    j["totalRewards"] = totalRewards;
    j["successfulValidations"] = successfulValidations;
    j["failedValidations"] = failedValidations;
    return j;
}

ConsensusParticipant ConsensusParticipant::fromJson(const nlohmann::json& json) {
    ConsensusParticipant participant;
    participant.address = json["address"];
    participant.method = static_cast<ConsensusMethod>(json["method"].get<int>());
    participant.status = static_cast<ParticipantStatus>(json["status"].get<int>());
    participant.stake = json["stake"];
    participant.bandwidth = json["bandwidth"];
    participant.hashRate = json["hashRate"];
    participant.reputationScore = json["reputationScore"];
    participant.lastActivity = json["lastActivity"];
    participant.totalRewards = json["totalRewards"];
    participant.successfulValidations = json["successfulValidations"];
    participant.failedValidations = json["failedValidations"];
    return participant;
}

// ValidationTask implementation
nlohmann::json ValidationTask::toJson() const {
    nlohmann::json j;
    j["taskId"] = taskId;
    j["transactionId"] = transactionId;
    j["assignedParticipants"] = assignedParticipants;
    
    nlohmann::json methodsJson;
    for (const auto& pair : participantMethods) {
        methodsJson[pair.first] = static_cast<int>(pair.second);
    }
    j["participantMethods"] = methodsJson;
    
    j["timestamp"] = timestamp;
    j["blockHeight"] = blockHeight;
    j["isCompleted"] = isCompleted;
    j["winningParticipant"] = winningParticipant;
    j["winningMethod"] = static_cast<int>(winningMethod);
    j["reward"] = reward;
    return j;
}

ValidationTask ValidationTask::fromJson(const nlohmann::json& json) {
    ValidationTask task;
    task.taskId = json["taskId"];
    task.transactionId = json["transactionId"];
    task.assignedParticipants = json["assignedParticipants"].get<std::vector<std::string>>();
    
    auto methodsJson = json["participantMethods"];
    for (auto it = methodsJson.begin(); it != methodsJson.end(); ++it) {
        task.participantMethods[it.key()] = static_cast<ConsensusMethod>(it.value().get<int>());
    }
    
    task.timestamp = json["timestamp"];
    task.blockHeight = json["blockHeight"];
    task.isCompleted = json["isCompleted"];
    task.winningParticipant = json["winningParticipant"];
    task.winningMethod = static_cast<ConsensusMethod>(json["winningMethod"].get<int>());
    task.reward = json["reward"];
    return task;
}

// ConsensusRound implementation
nlohmann::json ConsensusRound::toJson() const {
    nlohmann::json j;
    j["roundId"] = roundId;
    j["blockHeight"] = blockHeight;
    
    nlohmann::json tasksJson = nlohmann::json::array();
    for (const auto& task : tasks) {
        tasksJson.push_back(task.toJson());
    }
    j["tasks"] = tasksJson;
    
    nlohmann::json participantsJson;
    for (const auto& pair : participants) {
        participantsJson[std::to_string(static_cast<int>(pair.first))] = pair.second;
    }
    j["participants"] = participantsJson;
    
    nlohmann::json rewardsJson;
    for (const auto& pair : methodRewards) {
        rewardsJson[std::to_string(static_cast<int>(pair.first))] = pair.second;
    }
    j["methodRewards"] = rewardsJson;
    
    j["startTime"] = startTime;
    j["endTime"] = endTime;
    j["isCompleted"] = isCompleted;
    return j;
}

// ConsensusStats implementation
nlohmann::json ConsensusStats::toJson() const {
    nlohmann::json j;
    j["totalRounds"] = totalRounds;
    j["totalValidations"] = totalValidations;
    
    nlohmann::json winsJson;
    for (const auto& pair : methodWins) {
        winsJson[std::to_string(static_cast<int>(pair.first))] = pair.second;
    }
    j["methodWins"] = winsJson;
    
    nlohmann::json rewardsJson;
    for (const auto& pair : methodRewards) {
        rewardsJson[std::to_string(static_cast<int>(pair.first))] = pair.second;
    }
    j["methodRewards"] = rewardsJson;
    
    j["totalRewardsDistributed"] = totalRewardsDistributed;
    j["averageRoundTime"] = averageRoundTime;
    j["activeParticipants"] = activeParticipants;
    return j;
}

// UnifiedConsensusSystem implementation
UnifiedConsensusSystem::UnifiedConsensusSystem(Blockchain& blockchain, MiningEngine& miningEngine, PoRCSystem& porcSystem)
    : blockchain(blockchain), miningEngine(miningEngine), porcSystem(porcSystem), 
      isRunning(false), currentRoundId(0), gen(rd()), dis(0.0, 1.0) {
    
    Logger::info("Unified Consensus System initialized");
}

UnifiedConsensusSystem::~UnifiedConsensusSystem() {
    stop();
}

bool UnifiedConsensusSystem::start() {
    if (isRunning) {
        return true;
    }
    
    isRunning = true;
    
    // Start worker threads
    consensusLoopThread = std::thread(&UnifiedConsensusSystem::consensusLoop, this);
    participantManagementThread = std::thread(&UnifiedConsensusSystem::participantManagementLoop, this);
    rewardDistributionThread = std::thread(&UnifiedConsensusSystem::rewardDistributionLoop, this);
    
    Logger::info("Unified Consensus System started successfully");
    return true;
}

void UnifiedConsensusSystem::stop() {
    if (!isRunning) {
        return;
    }
    
    isRunning = false;
    
    // Wait for threads to finish
    if (consensusLoopThread.joinable()) {
        consensusLoopThread.join();
    }
    if (participantManagementThread.joinable()) {
        participantManagementThread.join();
    }
    if (rewardDistributionThread.joinable()) {
        rewardDistributionThread.join();
    }
    
    Logger::info("Unified Consensus System stopped");
}

// Participant management
bool UnifiedConsensusSystem::joinAsPoW(const std::string& address) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    ConsensusParticipant participant;
    participant.address = address;
    participant.method = ConsensusMethod::PROOF_OF_WORK;
    participant.status = ParticipantStatus::ACTIVE;
    participant.lastActivity = getCurrentTimestamp();
    participant.reputationScore = 1.0; // Start with neutral reputation
    
    participants[address] = participant;
    saveParticipant(participant);
    
    Logger::info("Participant joined as PoW: " + address);
    return true;
}

bool UnifiedConsensusSystem::joinAsPoS(const std::string& address, double stake) {
    if (stake < config.minStake || stake > config.maxStake) {
        Logger::error("Invalid stake amount for PoS participant: " + address);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    ConsensusParticipant participant;
    participant.address = address;
    participant.method = ConsensusMethod::PROOF_OF_STAKE;
    participant.status = ParticipantStatus::ACTIVE;
    participant.stake = stake;
    participant.lastActivity = getCurrentTimestamp();
    participant.reputationScore = 1.0;
    
    participants[address] = participant;
    saveParticipant(participant);
    
    Logger::info("Participant joined as PoS: " + address + " with stake: " + std::to_string(stake));
    return true;
}

bool UnifiedConsensusSystem::joinAsPoRC(const std::string& address, uint64_t bandwidth) {
    if (bandwidth < config.minBandwidth || bandwidth > config.maxBandwidth) {
        Logger::error("Invalid bandwidth for PoRC participant: " + address);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    ConsensusParticipant participant;
    participant.address = address;
    participant.method = ConsensusMethod::PROOF_OF_RESOURCE_CONTRIBUTION;
    participant.status = ParticipantStatus::ACTIVE;
    participant.bandwidth = bandwidth;
    participant.lastActivity = getCurrentTimestamp();
    participant.reputationScore = 1.0;
    
    participants[address] = participant;
    saveParticipant(participant);
    
    Logger::info("Participant joined as PoRC: " + address + " with bandwidth: " + std::to_string(bandwidth));
    return true;
}

bool UnifiedConsensusSystem::leave(const std::string& address) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(address);
    if (it != participants.end()) {
        it->second.status = ParticipantStatus::IDLE;
        saveParticipant(it->second);
        Logger::info("Participant left: " + address);
        return true;
    }
    
    return false;
}

bool UnifiedConsensusSystem::isParticipant(const std::string& address) const {
    std::lock_guard<std::mutex> lock(participantsMutex);
    auto it = participants.find(address);
    return it != participants.end() && it->second.status != ParticipantStatus::IDLE;
}

ConsensusParticipant UnifiedConsensusSystem::getParticipant(const std::string& address) const {
    std::lock_guard<std::mutex> lock(participantsMutex);
    auto it = participants.find(address);
    if (it != participants.end()) {
        return it->second;
    }
    return ConsensusParticipant();
}

// Task submission
bool UnifiedConsensusSystem::submitTransactionForValidation(const std::string& transactionId) {
    std::lock_guard<std::mutex> lock(tasksMutex);
    
    ValidationTask task = createValidationTask(transactionId, blockchain.getChainHeight());
    pendingTasks.push(task);
    saveTask(task);
    
    Logger::info("Transaction submitted for validation: " + transactionId);
    return true;
}

bool UnifiedConsensusSystem::submitValidationResult(const std::string& taskId, const std::string& participantAddress, bool success) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(participantAddress);
    if (it == participants.end()) {
        Logger::error("Unknown participant submitted result: " + participantAddress);
        return false;
    }
    
    ConsensusParticipant& participant = it->second;
    updateParticipantReputation(participantAddress, success);
    
    if (success) {
        participant.successfulValidations++;
    } else {
        participant.failedValidations++;
    }
    
    participant.lastActivity = getCurrentTimestamp();
    saveParticipant(participant);
    
    Logger::info("Validation result submitted by " + participantAddress + ": " + (success ? "SUCCESS" : "FAILURE"));
    return true;
}

// Statistics
ConsensusStats UnifiedConsensusSystem::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

std::vector<ConsensusRound> UnifiedConsensusSystem::getActiveRounds() const {
    std::lock_guard<std::mutex> lock(roundsMutex);
    std::vector<ConsensusRound> rounds;
    for (const auto& pair : activeRounds) {
        rounds.push_back(pair.second);
    }
    return rounds;
}

std::vector<ConsensusParticipant> UnifiedConsensusSystem::getActiveParticipants() const {
    std::lock_guard<std::mutex> lock(participantsMutex);
    std::vector<ConsensusParticipant> activeParticipants;
    for (const auto& pair : participants) {
        if (pair.second.status == ParticipantStatus::ACTIVE || 
            pair.second.status == ParticipantStatus::VALIDATING) {
            activeParticipants.push_back(pair.second);
        }
    }
    return activeParticipants;
}

std::map<ConsensusMethod, std::vector<std::string>> UnifiedConsensusSystem::getParticipantsByMethod() const {
    std::lock_guard<std::mutex> lock(participantsMutex);
    std::map<ConsensusMethod, std::vector<std::string>> participantsByMethod;
    
    for (const auto& pair : participants) {
        if (pair.second.status == ParticipantStatus::ACTIVE || 
            pair.second.status == ParticipantStatus::VALIDATING) {
            participantsByMethod[pair.second.method].push_back(pair.first);
        }
    }
    
    return participantsByMethod;
}

// Thread functions
void UnifiedConsensusSystem::consensusLoop() {
    while (isRunning) {
        try {
            // Create new round if needed
            std::lock_guard<std::mutex> lock(roundsMutex);
            
            if (activeRounds.size() < config.maxConcurrentRounds) {
                ConsensusRound round = createNewRound(blockchain.getChainHeight());
                startRound(round);
                activeRounds[round.roundId] = round;
                
                Logger::info("Started consensus round: " + std::to_string(round.roundId));
            }
            
            // Process existing rounds
            for (auto it = activeRounds.begin(); it != activeRounds.end();) {
                if (isRoundComplete(it->second)) {
                    endRound(it->second);
                    Logger::info("Completed consensus round: " + std::to_string(it->second.roundId));
                    it = activeRounds.erase(it);
                } else {
                    ++it;
                }
            }
            
        } catch (const std::exception& e) {
            Logger::error("Error in consensus loop: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void UnifiedConsensusSystem::participantManagementLoop() {
    while (isRunning) {
        try {
            std::lock_guard<std::mutex> lock(participantsMutex);
            
            uint64_t currentTime = getCurrentTimestamp();
            
            for (auto& pair : participants) {
                ConsensusParticipant& participant = pair.second;
                
                // Update reputation decay
                participant.reputationScore *= config.reputationDecayRate;
                if (participant.reputationScore < config.minReputationScore) {
                    participant.reputationScore = config.minReputationScore;
                }
                
                // Check for inactivity
                if (currentTime - participant.lastActivity > config.minActivityTime) {
                    participant.status = ParticipantStatus::IDLE;
                }
                
                // Check for ban conditions
                if (participant.failedValidations > config.maxFailuresBeforeBan) {
                    participant.status = ParticipantStatus::DISQUALIFIED;
                    Logger::warning("Participant banned due to failures: " + participant.address);
                }
                
                saveParticipant(participant);
            }
            
        } catch (const std::exception& e) {
            Logger::error("Error in participant management loop: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void UnifiedConsensusSystem::rewardDistributionLoop() {
    while (isRunning) {
        try {
            std::lock_guard<std::mutex> lock(roundsMutex);
            
            for (auto& pair : activeRounds) {
                ConsensusRound& round = pair.second;
                if (round.isCompleted && !round.methodRewards.empty()) {
                    distributeRewards(round);
                    round.methodRewards.clear(); // Clear to prevent double distribution
                }
            }
            
        } catch (const std::exception& e) {
            Logger::error("Error in reward distribution loop: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

// Round management
ConsensusRound UnifiedConsensusSystem::createNewRound(uint64_t blockHeight) {
    ConsensusRound round;
    round.roundId = ++currentRoundId;
    round.blockHeight = blockHeight;
    round.startTime = getCurrentTimestamp();
    round.endTime = round.startTime + config.roundDuration;
    round.isCompleted = false;
    
    // Get participants by method
    auto participantsByMethod = getParticipantsByMethod();
    
    // Assign participants to round
    for (const auto& pair : participantsByMethod) {
        ConsensusMethod method = pair.first;
        const std::vector<std::string>& addresses = pair.second;
        
        // Limit participants per method
        size_t maxParticipants = config.maxParticipantsPerRound / 3; // Equal distribution
        size_t participantCount = std::min(addresses.size(), maxParticipants);
        
        for (size_t i = 0; i < participantCount; ++i) {
            round.participants[method].push_back(addresses[i]);
        }
    }
    
    // Create validation tasks from pending transactions
    std::lock_guard<std::mutex> lock(tasksMutex);
    while (!pendingTasks.empty() && round.tasks.size() < 10) { // Limit tasks per round
        ValidationTask task = pendingTasks.front();
        pendingTasks.pop();
        assignTaskToParticipants(task, round);
        round.tasks.push_back(task);
    }
    
    return round;
}

void UnifiedConsensusSystem::startRound(ConsensusRound& round) {
    Logger::info("Starting consensus round " + std::to_string(round.roundId) + 
                " with " + std::to_string(round.tasks.size()) + " tasks");
    
    // Update participant statuses
    std::lock_guard<std::mutex> lock(participantsMutex);
    for (const auto& pair : round.participants) {
        for (const auto& address : pair.second) {
            auto it = participants.find(address);
            if (it != participants.end()) {
                it->second.status = ParticipantStatus::VALIDATING;
                it->second.lastActivity = getCurrentTimestamp();
            }
        }
    }
}

void UnifiedConsensusSystem::endRound(ConsensusRound& round) {
    round.isCompleted = true;
    round.endTime = getCurrentTimestamp();
    
    // Calculate method rewards
    std::map<ConsensusMethod, uint64_t> methodWins;
    for (const auto& task : round.tasks) {
        if (task.isCompleted) {
            methodWins[task.winningMethod]++;
        }
    }
    
    // Distribute rewards based on wins
    double totalReward = config.totalRewardPerRound;
    for (const auto& pair : methodWins) {
        ConsensusMethod method = pair.first;
        uint64_t wins = pair.second;
        double methodReward = (wins * totalReward) / round.tasks.size() * getMethodWeight(method);
        round.methodRewards[method] = methodReward;
    }
    
    // Update statistics
    std::lock_guard<std::mutex> lock(statsMutex);
    stats.totalRounds++;
    stats.averageRoundTime = (stats.averageRoundTime + (round.endTime - round.startTime)) / 2;
    
    for (const auto& pair : methodWins) {
        stats.methodWins[pair.first] += pair.second;
    }
    
    for (const auto& pair : round.methodRewards) {
        stats.methodRewards[pair.first] += pair.second;
    }
    
    Logger::info("Completed consensus round " + std::to_string(round.roundId) + 
                " with " + std::to_string(methodWins.size()) + " method wins");
}

bool UnifiedConsensusSystem::isRoundComplete(const ConsensusRound& round) const {
    return round.isCompleted || getCurrentTimestamp() >= round.endTime;
}

// Task management
ValidationTask UnifiedConsensusSystem::createValidationTask(const std::string& transactionId, uint64_t blockHeight) {
    ValidationTask task;
    task.taskId = generateTaskId();
    task.transactionId = transactionId;
    task.blockHeight = blockHeight;
    task.timestamp = getCurrentTimestamp();
    task.isCompleted = false;
    return task;
}

void UnifiedConsensusSystem::assignTaskToParticipants(ValidationTask& task, const ConsensusRound& round) {
    // Assign participants from each method to the task
    for (const auto& pair : round.participants) {
        ConsensusMethod method = pair.first;
        const std::vector<std::string>& addresses = pair.second;
        
        // Assign up to 3 participants per method
        size_t participantCount = std::min(addresses.size(), size_t(3));
        for (size_t i = 0; i < participantCount; ++i) {
            task.assignedParticipants.push_back(addresses[i]);
            task.participantMethods[addresses[i]] = method;
        }
    }
}

// Method-specific validation
bool UnifiedConsensusSystem::validateWithPoW(const std::string& participantAddress, const ValidationTask& task) {
    // Simulate PoW validation
    std::string data = task.transactionId + participantAddress + std::to_string(task.timestamp);
    std::string hash = Utils::calculateSHA256(data);
    
    // Check if hash meets difficulty requirement
    std::string target(config.powDifficulty, '0');
    return hash.substr(0, config.powDifficulty) == target;
}

bool UnifiedConsensusSystem::validateWithPoS(const std::string& participantAddress, const ValidationTask& task) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(participantAddress);
    if (it == participants.end()) {
        return false;
    }
    
    const ConsensusParticipant& participant = it->second;
    
    // PoS validation based on stake weight
    double stakeWeight = participant.stake / config.maxStake;
    double randomValue = dis(gen);
    
    return randomValue <= stakeWeight;
}

bool UnifiedConsensusSystem::validateWithPoRC(const std::string& participantAddress, const ValidationTask& task) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(participantAddress);
    if (it == participants.end()) {
        return false;
    }
    
    const ConsensusParticipant& participant = it->second;
    
    // PoRC validation based on bandwidth contribution
    double bandwidthWeight = static_cast<double>(participant.bandwidth) / config.maxBandwidth;
    double randomValue = dis(gen);
    
    return randomValue <= bandwidthWeight;
}

// Reward calculation
double UnifiedConsensusSystem::calculateReward(const std::string& participantAddress, ConsensusMethod method) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(participantAddress);
    if (it == participants.end()) {
        return 0.0;
    }
    
    const ConsensusParticipant& participant = it->second;
    double baseReward = config.totalRewardPerRound * getMethodWeight(method);
    
    // Adjust reward based on reputation
    double reputationMultiplier = participant.reputationScore / config.maxReputationScore;
    
    return baseReward * reputationMultiplier;
}

void UnifiedConsensusSystem::distributeRewards(const ConsensusRound& round) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    for (const auto& pair : round.methodRewards) {
        ConsensusMethod method = pair.first;
        double totalMethodReward = pair.second;
        
        // Find participants for this method
        std::vector<std::string> methodParticipants;
        for (const auto& participantPair : participants) {
            if (participantPair.second.method == method && 
                participantPair.second.status == ParticipantStatus::VALIDATING) {
                methodParticipants.push_back(participantPair.first);
            }
        }
        
        if (methodParticipants.empty()) {
            continue;
        }
        
        // Distribute reward equally among participants
        double rewardPerParticipant = totalMethodReward / methodParticipants.size();
        
        for (const auto& address : methodParticipants) {
            auto it = participants.find(address);
            if (it != participants.end()) {
                it->second.totalRewards += static_cast<uint64_t>(rewardPerParticipant * 1000000); // Convert to micro units
                it->second.status = ParticipantStatus::REWARDED;
                saveParticipant(it->second);
                
                Logger::info("Rewarded participant " + address + " with " + std::to_string(rewardPerParticipant) + " for method " + std::to_string(static_cast<int>(method)));
            }
        }
    }
    
    stats.totalRewardsDistributed += config.totalRewardPerRound;
}

double UnifiedConsensusSystem::getMethodWeight(ConsensusMethod method) const {
    switch (method) {
        case ConsensusMethod::PROOF_OF_WORK:
            return config.powWeight;
        case ConsensusMethod::PROOF_OF_STAKE:
            return config.posWeight;
        case ConsensusMethod::PROOF_OF_RESOURCE_CONTRIBUTION:
            return config.porcWeight;
        default:
            return 0.0;
    }
}

// Utility functions
std::string UnifiedConsensusSystem::generateTaskId() {
    static std::atomic<uint64_t> taskCounter(0);
    return "task_" + std::to_string(getCurrentTimestamp()) + "_" + std::to_string(++taskCounter);
}

uint64_t UnifiedConsensusSystem::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool UnifiedConsensusSystem::isParticipantEligible(const ConsensusParticipant& participant) const {
    return participant.status == ParticipantStatus::ACTIVE || 
           participant.status == ParticipantStatus::VALIDATING;
}

double UnifiedConsensusSystem::calculateReputationScore(const ConsensusParticipant& participant) const {
    if (participant.successfulValidations + participant.failedValidations == 0) {
        return config.minReputationScore;
    }
    
    double successRate = static_cast<double>(participant.successfulValidations) / 
                        (participant.successfulValidations + participant.failedValidations);
    
    return config.minReputationScore + (config.maxReputationScore - config.minReputationScore) * successRate;
}

// Participant management
void UnifiedConsensusSystem::updateParticipantReputation(const std::string& address, bool success) {
    auto it = participants.find(address);
    if (it == participants.end()) {
        return;
    }
    
    ConsensusParticipant& participant = it->second;
    
    if (success) {
        participant.reputationScore = std::min(config.maxReputationScore, 
                                              participant.reputationScore + 0.1);
    } else {
        participant.reputationScore = std::max(config.minReputationScore, 
                                              participant.reputationScore - 0.2);
    }
}

// Database operations (placeholder implementations)
bool UnifiedConsensusSystem::saveParticipant(const ConsensusParticipant& participant) {
    // TODO: Implement SQLite storage
    return true;
}

bool UnifiedConsensusSystem::loadParticipant(const std::string& address, ConsensusParticipant& participant) {
    // TODO: Implement SQLite loading
    return false;
}

bool UnifiedConsensusSystem::saveRound(const ConsensusRound& round) {
    // TODO: Implement SQLite storage
    return true;
}

bool UnifiedConsensusSystem::loadRound(uint64_t roundId, ConsensusRound& round) {
    // TODO: Implement SQLite loading
    return false;
}

bool UnifiedConsensusSystem::saveTask(const ValidationTask& task) {
    // TODO: Implement SQLite storage
    return true;
}

bool UnifiedConsensusSystem::loadTask(const std::string& taskId, ValidationTask& task) {
    // TODO: Implement SQLite loading
    return false;
}

// Validation and security methods
bool UnifiedConsensusSystem::validateParticipant(const ConsensusParticipant& participant) {
    // Check if participant meets minimum requirements
    if (participant.address.empty()) {
        return false;
    }
    
    // Check method-specific requirements
    switch (participant.method) {
        case ConsensusMethod::PROOF_OF_WORK:
            return participant.hashRate > 0;
        case ConsensusMethod::PROOF_OF_STAKE:
            return participant.stake >= config.minStake && participant.stake <= config.maxStake;
        case ConsensusMethod::PROOF_OF_RESOURCE_CONTRIBUTION:
            return participant.bandwidth >= config.minBandwidth && participant.bandwidth <= config.maxBandwidth;
        default:
            return false;
    }
}

bool UnifiedConsensusSystem::validateTask(const ValidationTask& task) {
    // Check if task has required fields
    if (task.taskId.empty() || task.transactionId.empty()) {
        return false;
    }
    
    // Check if task has assigned participants
    if (task.assignedParticipants.empty()) {
        return false;
    }
    
    // Check if participant methods are valid
    for (const auto& pair : task.participantMethods) {
        if (pair.first.empty()) {
            return false;
        }
    }
    
    return true;
}

bool UnifiedConsensusSystem::validateRound(const ConsensusRound& round) {
    // Check if round has required fields
    if (round.roundId == 0) {
        return false;
    }
    
    // Check if round has participants
    if (round.participants.empty()) {
        return false;
    }
    
    // Check if round has tasks
    if (round.tasks.empty()) {
        return false;
    }
    
    // Validate all tasks in the round
    for (const auto& task : round.tasks) {
        if (!validateTask(task)) {
            return false;
        }
    }
    
    return true;
}

bool UnifiedConsensusSystem::isParticipantBanned(const std::string& address) const {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(address);
    if (it == participants.end()) {
        return false;
    }
    
    return it->second.status == ParticipantStatus::DISQUALIFIED;
}

void UnifiedConsensusSystem::banParticipant(const std::string& address, uint64_t duration) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(address);
    if (it != participants.end()) {
        it->second.status = ParticipantStatus::DISQUALIFIED;
        it->second.lastActivity = getCurrentTimestamp();
        
        Logger::warning("Participant banned: " + address + " for " + std::to_string(duration) + " seconds");
        
        // Schedule unban after duration
        std::thread([this, address, duration]() {
            std::this_thread::sleep_for(std::chrono::seconds(duration));
            this->unbanParticipant(address);
        }).detach();
    }
}

void UnifiedConsensusSystem::unbanParticipant(const std::string& address) {
    std::lock_guard<std::mutex> lock(participantsMutex);
    
    auto it = participants.find(address);
    if (it != participants.end()) {
        it->second.status = ParticipantStatus::IDLE;
        it->second.failedValidations = 0; // Reset failure count
        it->second.lastActivity = getCurrentTimestamp();
        
        Logger::info("Participant unbanned: " + address);
    }
}

// API endpoint handlers
nlohmann::json UnifiedConsensusSystem::handleJoinRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        std::string address = request["address"];
        std::string method = request["method"];
        
        bool success = false;
        if (method == "pow") {
            success = joinAsPoW(address);
        } else if (method == "pos") {
            double stake = request["stake"];
            success = joinAsPoS(address, stake);
        } else if (method == "porc") {
            uint64_t bandwidth = request["bandwidth"];
            success = joinAsPoRC(address, bandwidth);
        } else {
            response["success"] = false;
            response["message"] = "Invalid consensus method";
            return response;
        }
        
        response["success"] = success;
        response["message"] = success ? "Successfully joined consensus" : "Failed to join consensus";
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

nlohmann::json UnifiedConsensusSystem::handleLeaveRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        std::string address = request["address"];
        bool success = leave(address);
        
        response["success"] = success;
        response["message"] = success ? "Successfully left consensus" : "Failed to leave consensus";
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

nlohmann::json UnifiedConsensusSystem::handleStatsRequest(const nlohmann::json& request) {
    nlohmann::json response;
    response["success"] = true;
    response["stats"] = getStats().toJson();
    return response;
}

nlohmann::json UnifiedConsensusSystem::handleRoundsRequest(const nlohmann::json& request) {
    nlohmann::json response;
    response["success"] = true;
    
    auto rounds = getActiveRounds();
    nlohmann::json roundsJson = nlohmann::json::array();
    for (const auto& round : rounds) {
        roundsJson.push_back(round.toJson());
    }
    response["rounds"] = roundsJson;
    
    return response;
}

nlohmann::json UnifiedConsensusSystem::handleParticipantsRequest(const nlohmann::json& request) {
    nlohmann::json response;
    response["success"] = true;
    
    auto participants = getActiveParticipants();
    nlohmann::json participantsJson = nlohmann::json::array();
    for (const auto& participant : participants) {
        participantsJson.push_back(participant.toJson());
    }
    response["participants"] = participantsJson;
    
    return response;
}

nlohmann::json UnifiedConsensusSystem::handleSubmitTransactionRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        std::string transactionId = request["transaction_id"];
        bool success = submitTransactionForValidation(transactionId);
        
        response["success"] = success;
        response["message"] = success ? "Transaction submitted for validation" : "Failed to submit transaction";
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

nlohmann::json UnifiedConsensusSystem::handleSubmitResultRequest(const nlohmann::json& request) {
    nlohmann::json response;
    
    try {
        std::string taskId = request["task_id"];
        std::string participantAddress = request["participant_address"];
        bool success = request["success"];
        
        bool result = submitValidationResult(taskId, participantAddress, success);
        
        response["success"] = result;
        response["message"] = result ? "Validation result submitted" : "Failed to submit validation result";
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = "Invalid request format";
    }
    
    return response;
}

// Integration with blockchain
void UnifiedConsensusSystem::onBlockMined(uint64_t blockHeight) {
    // Trigger new consensus round when block is mined
    Logger::info("Block mined at height " + std::to_string(blockHeight) + ", triggering consensus round");
}

void UnifiedConsensusSystem::onTransactionCreated(const std::string& transactionId) {
    // Automatically submit new transactions for validation
    submitTransactionForValidation(transactionId);
}

void UnifiedConsensusSystem::onTransactionValidated(const std::string& transactionId, bool success) {
    Logger::info("Transaction validated: " + transactionId + " - " + (success ? "SUCCESS" : "FAILURE"));
}
