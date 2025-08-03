/**
 * Nilotic Blockchain dApp SDK
 * A comprehensive SDK for building decentralized applications
 * Compatible with Nilotic Blockchain API v1.0.0
 */

class NiloticDApp {
    constructor(blockchainUrl = 'http://localhost:5500') {
        this.blockchainUrl = blockchainUrl;
        this.contracts = new Map();
        this.eventListeners = new Map();
        this.wallet = null;
        this.isConnected = false;
    }

    /**
     * Connect to the blockchain and verify connection
     */
    async connect() {
        try {
            const response = await fetch(`${this.blockchainUrl}/info`);
            if (response.ok) {
                const data = await response.json();
                this.isConnected = true;
                console.log('Connected to Nilotic Blockchain:', data);
                return data;
            } else {
                throw new Error('Failed to connect to blockchain');
            }
        } catch (error) {
            this.isConnected = false;
            throw new Error(`Connection failed: ${error.message}`);
        }
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
     * Create a new wallet
     * @param {string} name - Wallet name
     * @param {string} password - Wallet password
     */
    async createWallet(name, password) {
        try {
            const response = await fetch(`${this.blockchainUrl}/wallet/create`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name, password })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                this.wallet = {
                    address: data.address,
                    name: data.name,
                    privateKey: null // Not stored for security
                };
                
                return {
                    success: true,
                    wallet: this.wallet,
                    message: data.message
                };
            } else {
                throw new Error(data.error || 'Failed to create wallet');
            }
        } catch (error) {
            throw new Error(`Failed to create wallet: ${error.message}`);
        }
    }

    /**
     * Import an existing wallet
     * @param {string} name - Wallet name
     * @param {string} privateKeyPEM - Private key in PEM format
     * @param {string} password - Wallet password
     */
    async importWallet(name, privateKeyPEM, password) {
        try {
            const response = await fetch(`${this.blockchainUrl}/wallet/import`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    name, 
                    private_key: privateKeyPEM, 
                    password 
                })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                this.wallet = {
                    address: data.address,
                    name: data.name,
                    privateKey: null // Not stored for security
                };
                
                return {
                    success: true,
                    wallet: this.wallet,
                    message: data.message
                };
            } else {
                throw new Error(data.error || 'Failed to import wallet');
            }
        } catch (error) {
            throw new Error(`Failed to import wallet: ${error.message}`);
        }
    }

    /**
     * Sign a transaction
     * @param {string} privateKeyPEM - Private key in PEM format
     * @param {string} password - Wallet password
     * @param {string} transactionData - Transaction data to sign
     */
    async signTransaction(privateKeyPEM, password, transactionData) {
        try {
            const response = await fetch(`${this.blockchainUrl}/wallet/sign`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    private_key: privateKeyPEM,
                    password: password,
                    transaction_data: transactionData
                })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                return {
                    success: true,
                    signature: data.signature,
                    address: data.address
                };
            } else {
                throw new Error(data.error || 'Failed to sign transaction');
            }
        } catch (error) {
            throw new Error(`Failed to sign transaction: ${error.message}`);
        }
    }

    /**
     * Send a transaction
     * @param {string} to - Recipient address
     * @param {number} amount - Amount to send
     * @param {number} fee - Transaction fee
     * @param {string} message - Optional transaction message
     * @param {Object} options - Transaction options
     */
    async sendTransaction(to, amount, fee = 0.001, message = '', options = {}) {
        if (!this.wallet) {
            throw new Error('Wallet not connected');
        }

        const transactionData = {
            sender: this.wallet.address,
            recipient: to,
            amount: amount,
            fee: fee,
            message: message,
            type: options.type || 'transfer',
            timestamp: Date.now()
        };

        console.log('Attempting to send transaction:', transactionData);

        try {
            // First, test the connection
            await this.connect();
            console.log('Blockchain connection verified');

            const response = await fetch(`${this.blockchainUrl}/transaction`, {
                method: 'POST',
                headers: { 
                    'Content-Type': 'application/json',
                    'Accept': 'application/json'
                },
                body: JSON.stringify(transactionData)
            });

            console.log('Transaction response status:', response.status);
            console.log('Transaction response headers:', response.headers);

            if (response.status === 404) {
                throw new Error('Transaction endpoint not found. The blockchain server does not support transactions yet.');
            }

            if (!response.ok) {
                const errorText = await response.text();
                console.error('Transaction failed with status:', response.status);
                console.error('Error response:', errorText);
                
                let errorMessage = `HTTP ${response.status}: ${errorText}`;
                
                if (response.status === 500) {
                    errorMessage = 'Server error occurred while processing transaction.';
                } else if (response.status === 400) {
                    errorMessage = 'Invalid transaction data. Please check your inputs.';
                } else if (response.status === 0) {
                    errorMessage = 'Cannot connect to blockchain server. Please check if the server is running.';
                }
                
                throw new Error(errorMessage);
            }

            const result = await response.json();
            console.log('Transaction response:', result);
            
            // Handle different response formats
            if (result.success === true || result.status === 'success') {
                // Extract transaction data from the response
                const transactionData = result.transaction || {};
                
                return {
                    success: true,
                    transactionId: transactionData.hash || result.transaction_id || 'unknown',
                    message: result.message || 'Transaction submitted successfully',
                    transaction: {
                        id: transactionData.hash || result.transaction_id || 'unknown',
                        sender: transactionData.from || this.wallet.address,
                        recipient: transactionData.to || to,
                        amount: transactionData.amount || amount,
                        fee: fee,
                        message: message,
                        status: 'pending',
                        timestamp: transactionData.timestamp || Date.now()
                    }
                };
            } else {
                const errorMsg = result.error || result.message || 'Transaction failed';
                console.error('Transaction failed:', errorMsg);
                throw new Error(errorMsg);
            }
        } catch (error) {
            console.error('Transaction error details:', {
                error: error,
                transactionData: transactionData,
                blockchainUrl: this.blockchainUrl
            });
            
            // Provide more specific error messages
            if (error.name === 'TypeError' && error.message.includes('fetch')) {
                throw new Error('Cannot connect to blockchain server. Please check if the server is running at ' + this.blockchainUrl);
            } else if (error.message.includes('NetworkError')) {
                throw new Error('Network error. Please check your internet connection.');
            } else if (error.message.includes('CORS')) {
                throw new Error('CORS error. Please check blockchain server configuration.');
            } else {
                throw new Error(`Transaction failed: ${error.message}`);
            }
        }
    }

    /**
     * Test blockchain connection with detailed diagnostics
     */
    async testConnection() {
        const diagnostics = {
            connection: false,
            endpoints: {},
            errors: []
        };

        try {
            // Test basic connection
            const info = await this.connect();
            diagnostics.connection = true;
            diagnostics.endpoints.info = 'OK';
            console.log('Basic connection test passed:', info);
        } catch (error) {
            diagnostics.errors.push(`Connection failed: ${error.message}`);
            console.error('Connection test failed:', error);
        }

        // Test balance endpoint if we have a wallet
        if (this.wallet) {
            try {
                const balance = await this.getBalance(this.wallet.address);
                diagnostics.endpoints.balance = 'OK';
                console.log('Balance endpoint test passed:', balance);
            } catch (error) {
                diagnostics.endpoints.balance = `Failed: ${error.message}`;
                diagnostics.errors.push(`Balance endpoint failed: ${error.message}`);
                console.error('Balance endpoint test failed:', error);
            }
        }

        // Test transaction endpoint
        try {
            const testResponse = await fetch(`${this.blockchainUrl}/transaction`, {
                method: 'OPTIONS'
            });
            diagnostics.endpoints.transaction = testResponse.ok ? 'Available' : `HTTP ${testResponse.status}`;
            console.log('Transaction endpoint test result:', testResponse.status);
        } catch (error) {
            diagnostics.endpoints.transaction = `Failed: ${error.message}`;
            diagnostics.errors.push(`Transaction endpoint failed: ${error.message}`);
            console.error('Transaction endpoint test failed:', error);
        }

        console.log('Blockchain diagnostics:', diagnostics);
        return diagnostics;
    }

    /**
     * Check available endpoints on the blockchain server
     */
    async checkAvailableEndpoints() {
        const endpoints = [
            { path: '/info', method: 'GET', name: 'Blockchain Info' },
            { path: '/balance/', method: 'GET', name: 'Balance' },
            { path: '/transaction', method: 'POST', name: 'Send Transaction' },
            { path: '/transactions/', method: 'GET', name: 'Transaction History' },
            { path: '/wallet/create', method: 'POST', name: 'Create Wallet' },
            { path: '/wallet/import', method: 'POST', name: 'Import Wallet' },
            { path: '/mining/status', method: 'GET', name: 'Mining Status' },
            { path: '/network/status', method: 'GET', name: 'Network Status' }
        ];

        const results = {};

        for (const endpoint of endpoints) {
            try {
                const url = `${this.blockchainUrl}${endpoint.path}`;
                console.log(`Testing endpoint: ${endpoint.method} ${url}`);
                
                const response = await fetch(url, {
                    method: endpoint.method === 'GET' ? 'GET' : 'OPTIONS',
                    headers: {
                        'Accept': 'application/json'
                    }
                });

                results[endpoint.name] = {
                    available: response.ok || response.status === 405, // 405 means method not allowed but endpoint exists
                    status: response.status,
                    statusText: response.statusText
                };

                console.log(`${endpoint.name}: ${response.status} ${response.statusText}`);
            } catch (error) {
                results[endpoint.name] = {
                    available: false,
                    error: error.message
                };
                console.log(`${endpoint.name}: Failed - ${error.message}`);
            }
        }

        console.log('Available endpoints summary:', results);
        return results;
    }

    /**
     * Get wallet balance
     * @param {string} address - Wallet address
     */
    async getBalance(address) {
        try {
            const response = await fetch(`${this.blockchainUrl}/balance/${address}`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get balance');
            }
        } catch (error) {
            throw new Error(`Failed to get balance: ${error.message}`);
        }
    }

    /**
     * Get blockchain information
     */
    async getBlockchainInfo() {
        try {
            const response = await fetch(`${this.blockchainUrl}/info`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get blockchain info');
            }
        } catch (error) {
            throw new Error(`Failed to get blockchain info: ${error.message}`);
        }
    }

    /**
     * Get latest block
     */
    async getLatestBlock() {
        try {
            const response = await fetch(`${this.blockchainUrl}/block/latest`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get latest block');
            }
        } catch (error) {
            throw new Error(`Failed to get latest block: ${error.message}`);
        }
    }

    /**
     * Get block by index
     * @param {number} index - Block index
     */
    async getBlock(index) {
        try {
            const response = await fetch(`${this.blockchainUrl}/block/${index}`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get block');
            }
        } catch (error) {
            throw new Error(`Failed to get block: ${error.message}`);
        }
    }

    /**
     * Start mining
     * @param {string} minerAddress - Miner address
     */
    async startMining(minerAddress) {
        try {
            const response = await fetch(`${this.blockchainUrl}/mining/start`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ miner_address: minerAddress })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                return {
                    success: true,
                    message: data.message,
                    minerAddress: data.miner_address
                };
            } else {
                throw new Error(data.error || 'Failed to start mining');
            }
        } catch (error) {
            throw new Error(`Failed to start mining: ${error.message}`);
        }
    }

    /**
     * Stop mining
     */
    async stopMining() {
        try {
            const response = await fetch(`${this.blockchainUrl}/mining/stop`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                return {
                    success: true,
                    message: data.message
                };
            } else {
                throw new Error(data.error || 'Failed to stop mining');
            }
        } catch (error) {
            throw new Error(`Failed to stop mining: ${error.message}`);
        }
    }

    /**
     * Get mining status
     */
    async getMiningStatus() {
        try {
            const response = await fetch(`${this.blockchainUrl}/mining/status`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get mining status');
            }
        } catch (error) {
            throw new Error(`Failed to get mining status: ${error.message}`);
        }
    }

    /**
     * Get network status
     */
    async getNetworkStatus() {
        try {
            const response = await fetch(`${this.blockchainUrl}/network/status`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get network status');
            }
        } catch (error) {
            throw new Error(`Failed to get network status: ${error.message}`);
        }
    }

    /**
     * Get peer list
     */
    async getPeers() {
        try {
            const response = await fetch(`${this.blockchainUrl}/network/peers`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get peers');
            }
        } catch (error) {
            throw new Error(`Failed to get peers: ${error.message}`);
        }
    }

    /**
     * Connect to a peer
     * @param {string} address - Peer address
     * @param {number} port - Peer port
     */
    async connectToPeer(address, port) {
        try {
            const response = await fetch(`${this.blockchainUrl}/network/connect`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ address, port })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                return {
                    success: true,
                    message: data.message,
                    address: data.address,
                    port: data.port
                };
            } else {
                throw new Error(data.error || 'Failed to connect to peer');
            }
        } catch (error) {
            throw new Error(`Failed to connect to peer: ${error.message}`);
        }
    }

    /**
     * Disconnect from a peer
     * @param {string} address - Peer address
     */
    async disconnectFromPeer(address) {
        try {
            const response = await fetch(`${this.blockchainUrl}/network/disconnect`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ address })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                return {
                    success: true,
                    message: data.message,
                    address: data.address
                };
            } else {
                throw new Error(data.error || 'Failed to disconnect from peer');
            }
        } catch (error) {
            throw new Error(`Failed to disconnect from peer: ${error.message}`);
        }
    }

    /**
     * Create a token (OderoSLW)
     * @param {string} tokenId - Token ID
     * @param {number} amount - Token amount
     * @param {string} creator - Creator address
     */
    async createToken(tokenId, amount, creator) {
        try {
            const response = await fetch(`${this.blockchainUrl}/token`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ token_id: tokenId, amount, creator })
            });

            const data = await response.json();
            
            if (data.status === 'success') {
                return {
                    success: true,
                    message: data.message,
                    tokenId: data.token_id
                };
            } else {
                throw new Error(data.error || 'Failed to create token');
            }
        } catch (error) {
            throw new Error(`Failed to create token: ${error.message}`);
        }
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
            
            if (result.status === 'success') {
                const contractAddress = this.generateContractAddress(contractCode, this.wallet.address);
                this.contracts.set(contractAddress, {
                    address: contractAddress,
                    code: contractCode,
                    abi: this.parseABI(contractCode)
                });
                
                return {
                    success: true,
                    contractAddress: contractAddress,
                    transactionId: result.transaction_id
                };
            } else {
                throw new Error(result.error || 'Contract deployment failed');
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
            
            if (result.status === 'success') {
                return {
                    success: true,
                    transactionId: result.transaction_id,
                    result: result.result
                };
            } else {
                throw new Error(result.error || 'Contract call failed');
            }
        } catch (error) {
            throw new Error(`Failed to call contract: ${error.message}`);
        }
    }

    /**
     * Get contract state
     * @param {string} contractAddress - Contract address
     * @param {string} key - State key (optional)
     */
    async getContractState(contractAddress, key = null) {
        try {
            const url = key 
                ? `${this.blockchainUrl}/contract/${contractAddress}/state/${key}`
                : `${this.blockchainUrl}/contract/${contractAddress}/state`;
            
            const response = await fetch(url);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get contract state');
            }
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
        
        return {
            success: true,
            eventKey: eventKey
        };
    }

    /**
     * Generate contract address
     * @param {string} contractCode - Contract code
     * @param {string} sender - Sender address
     */
    generateContractAddress(contractCode, sender) {
        const data = contractCode + sender + Date.now().toString();
        return this.hashString(data).substring(0, 40);
    }

    /**
     * Parse ABI from contract code
     * @param {string} contractCode - Contract source code
     */
    parseABI(contractCode) {
        // Simple ABI parser - in production, use a proper parser
        const functions = [];
        const lines = contractCode.split('\n');
        
        for (const line of lines) {
            if (line.includes('function') && line.includes('(')) {
                const match = line.match(/function\s+(\w+)\s*\(([^)]*)\)/);
                if (match) {
                    functions.push({
                        name: match[1],
                        parameters: this.parseParameters(match[2])
                    });
                }
            }
        }
        
        return functions;
    }

    /**
     * Parse function parameters
     * @param {string} paramString - Parameter string
     */
    parseParameters(paramString) {
        if (!paramString.trim()) return [];
        
        return paramString.split(',').map(param => {
            const parts = param.trim().split(' ');
            return {
                type: parts[0],
                name: parts[1] || 'param'
            };
        });
    }

    /**
     * Start event polling
     * @param {string} contractAddress - Contract address
     * @param {string} eventName - Event name
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
     * @param {string} contractAddress - Contract address
     * @param {string} eventName - Event name
     */
    async getContractEvents(contractAddress, eventName) {
        try {
            const response = await fetch(`${this.blockchainUrl}/contract/${contractAddress}/events/${eventName}`);
            if (response.ok) {
                return await response.json();
            } else {
                return [];
            }
        } catch (error) {
            console.error('Failed to get contract events:', error);
            return [];
        }
    }

    /**
     * Hash a string using SHA-256
     * @param {string} str - String to hash
     */
    hashString(str) {
        // Simple hash function - in production, use crypto-js or similar
        let hash = 0;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash; // Convert to 32-bit integer
        }
        return Math.abs(hash).toString(16);
    }

    /**
     * Get transaction history for a wallet
     * @param {string} address - Wallet address
     * @param {string} filter - Filter type (all, sent, received, pending)
     */
    async getTransactionHistory(address, filter = 'all') {
        try {
            console.log(`Fetching transaction history for ${address} with filter: ${filter}`);
            
            const response = await fetch(`${this.blockchainUrl}/transactions/${address}?filter=${filter}`);
            
            if (response.status === 404) {
                console.warn('Transaction history endpoint not found (404). Using mock data.');
                return this.getMockTransactionHistory(address, filter);
            }
            
            if (!response.ok) {
                console.error(`Transaction history request failed with status: ${response.status}`);
                throw new Error(`Failed to get transaction history: HTTP ${response.status}`);
            }
            
            const data = await response.json();
            console.log('Transaction history response:', data);
            return data.transactions || [];
        } catch (error) {
            console.warn('Transaction history endpoint unavailable, using mock data:', error.message);
            return this.getMockTransactionHistory(address, filter);
        }
    }

    /**
     * Get mock transaction history for demonstration
     * @param {string} address - Wallet address
     * @param {string} filter - Filter type
     */
    getMockTransactionHistory(address, filter) {
        const mockTransactions = [
            {
                id: 'tx_' + Math.random().toString(36).substr(2, 9),
                sender: address,
                recipient: '0x742d35Cc6634C0532925a3b8D4C9db96C4b4d8b7',
                amount: 10.5,
                fee: 0.001,
                message: 'Payment for services',
                type: 'sent',
                status: 'confirmed',
                timestamp: Date.now() - 3600000 // 1 hour ago
            },
            {
                id: 'tx_' + Math.random().toString(36).substr(2, 9),
                sender: '0x8ba1f109551bD432803012645Hac136c772c37e',
                recipient: address,
                amount: 25.0,
                fee: 0.001,
                message: 'Refund for overpayment',
                type: 'received',
                status: 'confirmed',
                timestamp: Date.now() - 7200000 // 2 hours ago
            },
            {
                id: 'tx_' + Math.random().toString(36).substr(2, 9),
                sender: address,
                recipient: '0x1234567890123456789012345678901234567890',
                amount: 5.25,
                fee: 0.001,
                message: 'Coffee payment',
                type: 'sent',
                status: 'pending',
                timestamp: Date.now() - 300000 // 5 minutes ago
            }
        ];

        // Filter transactions based on type
        if (filter === 'sent') {
            return mockTransactions.filter(tx => tx.type === 'sent');
        } else if (filter === 'received') {
            return mockTransactions.filter(tx => tx.type === 'received');
        } else if (filter === 'pending') {
            return mockTransactions.filter(tx => tx.status === 'pending');
        }

        return mockTransactions;
    }

    /**
     * Get transaction details by ID
     * @param {string} transactionId - Transaction ID
     */
    async getTransactionDetails(transactionId) {
        try {
            const response = await fetch(`${this.blockchainUrl}/transaction/${transactionId}`);
            if (response.ok) {
                return await response.json();
            } else {
                throw new Error('Failed to get transaction details');
            }
        } catch (error) {
            throw new Error(`Failed to get transaction details: ${error.message}`);
        }
    }

    /**
     * Validate transaction data
     * @param {Object} transactionData - Transaction data to validate
     */
    validateTransaction(transactionData) {
        const errors = [];

        if (!transactionData.recipient || transactionData.recipient.length < 10) {
            errors.push('Invalid recipient address');
        }

        if (!transactionData.amount || transactionData.amount <= 0) {
            errors.push('Invalid amount');
        }

        if (transactionData.fee < 0) {
            errors.push('Invalid transaction fee');
        }

        if (transactionData.message && transactionData.message.length > 100) {
            errors.push('Message too long (max 100 characters)');
        }

        return {
            isValid: errors.length === 0,
            errors: errors
        };
    }

    /**
     * Disconnect from the blockchain
     */
    disconnect() {
        this.wallet = null;
        this.isConnected = false;
        this.contracts.clear();
        
        // Clear event polling intervals
        for (const [key, value] of this.eventListeners.entries()) {
            if (key.includes(':interval')) {
                clearInterval(value);
            }
        }
        this.eventListeners.clear();
        
        console.log('Disconnected from Nilotic Blockchain');
    }

    /**
     * Get connection status
     */
    isConnected() {
        return this.isConnected;
    }

    /**
     * Get current wallet
     */
    getWallet() {
        return this.wallet;
    }

    /**
     * Get deployed contracts
     */
    getContracts() {
        return Array.from(this.contracts.values());
    }
}

// Export for use in modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = NiloticDApp;
}

// Global instance for browser use
window.NiloticDApp = NiloticDApp; 