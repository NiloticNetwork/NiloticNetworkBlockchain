#include <iostream>
#include <cassert>
#include "../include/core/unified_transaction_validator.h"
#include "../include/core/transaction.h"
#include "../include/core/logger.h"

int main() {
    try {
        Logger::info("Starting simple unified transaction validator test");
        
        // Create validator
        UnifiedTransactionValidator validator;
        
        // Initialize validator
        assert(validator.initialize());
        assert(validator.isInitialized());
        
        // Create a simple transaction
        Transaction tx("Alice", "Bob", 10.0);
        
        // Validate transaction
        TransactionValidationResult result = validator.validateTransaction(tx);
        
        // Check result
        assert(result.calculatedFee > 0.0);
        assert(result.priority > 0);
        
        // Calculate fee
        double fee = validator.calculateTransactionFee(tx);
        assert(fee > 0.0);
        
        // Calculate priority
        uint32_t priority = validator.calculateTransactionPriority(tx);
        assert(priority > 0);
        
        // Test statistics
        nlohmann::json stats = validator.getStatistics();
        assert(stats.contains("totalValidations"));
        
        Logger::info("Simple unified transaction validator test passed!");
        std::cout << "Simple test passed successfully!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}