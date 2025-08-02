#ifndef TRANSACTION_TYPES_H
#define TRANSACTION_TYPES_H

// Transaction types enumeration
enum class TransactionType {
    REGULAR,
    STAKE,
    UNSTAKE,
    CREATE_ODERO,
    REDEEM_ODERO,
    CONTRACT
};

// Network types enumeration
enum class NetworkType {
    LIVEWIRE,
    TESTWIRE,
    PIPE
};

#endif // TRANSACTION_TYPES_H