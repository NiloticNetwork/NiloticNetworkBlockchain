// Simple compilation test to verify ConsensusRouter header compiles correctly
// This test includes the actual header files to ensure they compile

#include <iostream>

// Mock the dependencies that ConsensusRouter needs
namespace nlohmann {
    class json {
    public:
        json() = default;
        json(const std::string&) {}
        
        template<typename T>
        T get() const { return T{}; }
        
        template<typename T>
        json& operator=(const T&) { return *this; }
        
        json& operator[](const std::string&) { return *this; }
        const json& operator[](const std::string&) const { return *this; }
        
        bool contains(const std::string&) const { return false; }
        
        std::string dump(int = 0) const { return "{}"; }
        
        static json parse(const std::string&) { return json{}; }
        
        class iterator {
        public:
            std::pair<std::string, json> operator*() const { return {"", json{}}; }
            iterator& operator++() { return *this; }
            bool operator!=(const iterator&) const { return false; }
        };
        
        iterator begin() const { return iterator{}; }
        iterator end() const { return iterator{}; }
        
        class items_iterator {
        public:
            std::pair<std::string, json> operator*() const { return {"", json{}}; }
            items_iterator& operator++() { return *this; }
            bool operator!=(const items_iterator&) const { return false; }
        };
        
        class items_type {
        public:
            items_iterator begin() const { return items_iterator{}; }
            items_iterator end() const { return items_iterator{}; }
        };
        
        items_type items() const { return items_type{}; }
    };
}

// Mock Logger
class Logger {
public:
    static void info(const std::string&) {}
    static void warning(const std::string&) {}
    static void error(const std::string&) {}
    static void debug(const std::string&) {}
};

// Mock Block and Transaction classes
class Block {
public:
    Block(uint64_t, const std::string&) {}
    std::string serialize() const { return "{}"; }
};

class Transaction {
public:
    Transaction(const std::string&, const std::string&, double) {}
    std::string serialize() const { return "{}"; }
};

// Now include the actual ConsensusRouter header
#include "../include/core/consensus_harmony.h"
#include "../include/core/consensus_router.h"

int main() {
    std::cout << "Testing ConsensusRouter header compilation..." << std::endl;
    
    // Test that we can create the basic types
    ConsensusType pow = ConsensusType::PROOF_OF_WORK;
    RequestType blockReq = RequestType::BLOCK_VALIDATION;
    AggregationStrategy strategy = AggregationStrategy::HIERARCHICAL;
    
    // Test that we can create structures
    ConsensusResult result(true, pow, 0.9, "Test");
    ConsensusRequest request(blockReq, "test_data");
    RoutingRule rule(blockReq, {pow}, {ConsensusType::PROOF_OF_STAKE});
    
    // Test that we can create a ConsensusRouter (but not initialize it without proper setup)
    ConsensusRouter router;
    
    std::cout << "✓ ConsensusRouter header compiles successfully!" << std::endl;
    std::cout << "✓ All types and structures are properly defined" << std::endl;
    
    return 0;
}