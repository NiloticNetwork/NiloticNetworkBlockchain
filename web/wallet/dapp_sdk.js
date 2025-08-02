/**
 * Nilotic Blockchain dApp SDK
 * A comprehensive SDK for building decentralized applications
 */

class NiloticDApp {
    constructor(blockchainUrl = 'http://localhost:8080') {
        this.blockchainUrl = blockchainUrl;
        this.contracts = new Map();
        this.eventListeners = new Map();
        this.wallet = null;
    }

    /**
     * Connect to a wallet
     * @param {string} address - Wallet address
     * @param {string} privateKey - Private key (for signing)
     */
    async connectWallet(address, privateKey = null) {
        this.wallet = {
            address: address,
            privateKey: privateKey
        };
        
        // Verify connection by checking balance
        const balance = await this.getBalance(address);
        console.log(`Connected to wallet: ${address}, Balance: ${balance.balance}`);
        
        return this.wallet;
    }

    /**
     * Deploy a smart contract
     * @param {string} contractCode - Contract source code
     * @param {Array} constructorArgs - Constructor arguments
     * @param {Object} options - Deployment options
     */
    async deployContract(contractCode, constructorArgs = [], options = {}) {
        if (!this.wallet) {
            throw new Error('Wallet not connected');
        }

        const contractData = {
            type: 'contract_deployment',
            sender: this.wallet.address,
            contractCode: contractCode,
            constructorArgs: constructorArgs,
            gasLimit: options.gasLimit || 1000000,
            gasPrice: options.gasPrice || 1
        };

        try {
            const response = await fetch(`${this.blockchainUrl}/transaction`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(contractData)
            });

            const result = await response.json();
            
            if (result.success) {
                const contractAddress = this.generateContractAddress(contractCode, this.wallet.address);
                this.contracts.set(contractAddress, {
                    address: contractAddress,
                    code: contractCode,
                    abi: this.parseABI(contractCode)
                });
                
                return {
                    success: true,
                    contractAddress: contractAddress,
                    transactionHash: result.transaction?.hash
                };
            } else {
                throw new Error(result.message || 'Contract deployment failed');
            }
        } catch (error) {
            throw new Error(`Failed to deploy contract: ${error.message}`);
        }
    }

    /**
     * Call a contract function
     * @param {string} contractAddress - Contract address
     * @param {string} functionName - Function name
     * @param {Array} args - Function arguments
     * @param {Object} options - Call options
     */
    async callContract(contractAddress, functionName, args = [], options = {}) {
        if (!this.wallet) {
            throw new Error('Wallet not connected');
        }

        const callData = {
            type: 'contract_call',
            sender: this.wallet.address,
            contractAddress: contractAddress,
            functionName: functionName,
            args: args,
            gasLimit: options.gasLimit || 100000,
            gasPrice: options.gasPrice || 1
        };

        try {
            const response = await fetch(`${this.blockchainUrl}/transaction`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(callData)
            });

            const result = await response.json();
            
            if (result.success) {
                return {
                    success: true,
                    result: result.returnValue,
                    gasUsed: result.gasUsed,
                    transactionHash: result.transaction?.hash
                };
            } else {
                throw new Error(result.message || 'Contract call failed');
            }
        } catch (error) {
            throw new Error(`Failed to call contract: ${error.message}`);
        }
    }

    /**
     * Get contract state
     * @param {string} contractAddress - Contract address
     * @param {string} key - Storage key
     */
    async getContractState(contractAddress, key = null) {
        try {
            const response = await fetch(`${this.blockchainUrl}/contract/${contractAddress}/state${key ? `?key=${key}` : ''}`);
            return await response.json();
        } catch (error) {
            throw new Error(`Failed to get contract state: ${error.message}`);
        }
    }

    /**
     * Listen to contract events
     * @param {string} contractAddress - Contract address
     * @param {string} eventName - Event name
     * @param {Function} callback - Event callback
     */
    async listenToEvents(contractAddress, eventName, callback) {
        const eventKey = `${contractAddress}:${eventName}`;
        this.eventListeners.set(eventKey, callback);

        // Start polling for events
        this.startEventPolling(contractAddress, eventName);
    }

    /**
     * Send a transaction
     * @param {string} to - Recipient address
     * @param {number} amount - Amount to send
     * @param {Object} options - Transaction options
     */
    async sendTransaction(to, amount, options = {}) {
        if (!this.wallet) {
            throw new Error('Wallet not connected');
        }

        const transactionData = {
            type: 'transaction',
            sender: this.wallet.address,
            recipient: to,
            amount: amount,
            gasLimit: options.gasLimit || 21000,
            gasPrice: options.gasPrice || 1
        };

        try {
            const response = await fetch(`${this.blockchainUrl}/transaction`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(transactionData)
            });

            const result = await response.json();
            
            if (result.success) {
                return {
                    success: true,
                    transactionHash: result.transaction?.hash,
                    gasUsed: result.gasUsed
                };
            } else {
                throw new Error(result.message || 'Transaction failed');
            }
        } catch (error) {
            throw new Error(`Failed to send transaction: ${error.message}`);
        }
    }

    /**
     * Get wallet balance
     * @param {string} address - Wallet address
     */
    async getBalance(address) {
        try {
            const response = await fetch(`${this.blockchainUrl}/balance?address=${encodeURIComponent(address)}`);
            return await response.json();
        } catch (error) {
            throw new Error(`Failed to get balance: ${error.message}`);
        }
    }

    /**
     * Mine a block
     * @param {string} address - Miner address
     */
    async mineBlock(address) {
        try {
            const response = await fetch(`${this.blockchainUrl}/mine`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ address: address })
            });
            return await response.json();
        } catch (error) {
            throw new Error(`Failed to mine block: ${error.message}`);
        }
    }

    /**
     * Get blockchain info
     */
    async getBlockchainInfo() {
        try {
            const response = await fetch(`${this.blockchainUrl}/`);
            return await response.json();
        } catch (error) {
            throw new Error(`Failed to get blockchain info: ${error.message}`);
        }
    }

    /**
     * Get all transactions
     */
    async getAllTransactions() {
        try {
            const response = await fetch(`${this.blockchainUrl}/chain`);
            const chain = await response.json();
            
            const transactions = [];
            chain.forEach((block, blockIndex) => {
                try {
                    if (block.data && typeof block.data === 'string' && block.data !== 'Genesis Block') {
                        const data = JSON.parse(block.data);
                        if (data.type === 'transaction' || data.type === 'contract_deployment' || data.type === 'contract_call') {
                            transactions.push({
                                blockIndex: blockIndex,
                                blockHash: block.hash,
                                ...data
                            });
                        }
                    }
                } catch (err) {
                    console.error('Error parsing block data:', err);
                }
            });
            
            return transactions;
        } catch (error) {
            throw new Error(`Failed to get transactions: ${error.message}`);
        }
    }

    // Helper methods

    /**
     * Generate contract address
     */
    generateContractAddress(contractCode, sender) {
        // Simple hash-based address generation
        const data = contractCode + sender + Date.now();
        let hash = 0;
        for (let i = 0; i < data.length; i++) {
            const char = data.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash; // Convert to 32-bit integer
        }
        return '0x' + Math.abs(hash).toString(16).padStart(40, '0');
    }

    /**
     * Parse ABI from contract code (simplified)
     */
    parseABI(contractCode) {
        const abi = [];
        
        // Simple regex-based ABI parsing
        const functionRegex = /function\s+(\w+)\s*\(([^)]*)\)/g;
        const eventRegex = /event\s+(\w+)\s*\(([^)]*)\)/g;
        
        let match;
        while ((match = functionRegex.exec(contractCode)) !== null) {
            abi.push({
                type: 'function',
                name: match[1],
                inputs: this.parseParameters(match[2])
            });
        }
        
        while ((match = eventRegex.exec(contractCode)) !== null) {
            abi.push({
                type: 'event',
                name: match[1],
                inputs: this.parseParameters(match[2])
            });
        }
        
        return abi;
    }

    /**
     * Parse function parameters
     */
    parseParameters(paramString) {
        if (!paramString.trim()) return [];
        
        return paramString.split(',').map(param => {
            const parts = param.trim().split(/\s+/);
            return {
                type: parts[0] || 'string',
                name: parts[1] || ''
            };
        });
    }

    /**
     * Start event polling
     */
    startEventPolling(contractAddress, eventName) {
        const pollInterval = setInterval(async () => {
            try {
                const events = await this.getContractEvents(contractAddress, eventName);
                const eventKey = `${contractAddress}:${eventName}`;
                const callback = this.eventListeners.get(eventKey);
                
                if (callback && events.length > 0) {
                    events.forEach(event => callback(event));
                }
            } catch (error) {
                console.error('Event polling error:', error);
            }
        }, 5000); // Poll every 5 seconds
        
        // Store interval for cleanup
        this.eventListeners.set(`${contractAddress}:${eventName}:interval`, pollInterval);
    }

    /**
     * Get contract events
     */
    async getContractEvents(contractAddress, eventName) {
        try {
            const response = await fetch(`${this.blockchainUrl}/contract/${contractAddress}/events?event=${eventName}`);
            return await response.json();
        } catch (error) {
            console.error('Failed to get contract events:', error);
            return [];
        }
    }

    /**
     * Disconnect from wallet
     */
    disconnect() {
        this.wallet = null;
        
        // Clear event polling intervals
        for (const [key, interval] of this.eventListeners.entries()) {
            if (key.includes(':interval')) {
                clearInterval(interval);
            }
        }
        
        this.eventListeners.clear();
        this.contracts.clear();
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = NiloticDApp;
} 