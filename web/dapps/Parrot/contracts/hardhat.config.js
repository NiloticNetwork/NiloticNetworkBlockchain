require("@nomicfoundation/hardhat-toolbox");
require("@openzeppelin/hardhat-upgrades");
require("hardhat-gas-reporter");
require("solidity-coverage");

require("dotenv").config();

const PRIVATE_KEY = process.env.PRIVATE_KEY || "0x0000000000000000000000000000000000000000000000000000000000000000";

module.exports = {
  solidity: {
    version: "0.8.19",
    settings: {
      optimizer: {
        enabled: true,
        runs: 200,
      },
    },
  },
  networks: {
    hardhat: {
      chainId: 1337,
    },
    localhost: {
      url: "http://127.0.0.1:8545",
    },
    "nilotic-testnet": {
      url: process.env.NILOTIC_TESTNET_RPC_URL || "https://testnet.nilotic.chain",
      accounts: [PRIVATE_KEY],
      chainId: parseInt(process.env.NILOTIC_TESTNET_CHAIN_ID || "1234"),
      gasPrice: 20000000000, // 20 gwei
    },
    "nilotic-mainnet": {
      url: process.env.NILOTIC_MAINNET_RPC_URL || "https://mainnet.nilotic.chain",
      accounts: [PRIVATE_KEY],
      chainId: parseInt(process.env.NILOTIC_MAINNET_CHAIN_ID || "1235"),
      gasPrice: 5000000000, // 5 gwei
    },
  },
  gasReporter: {
    enabled: process.env.REPORT_GAS !== undefined,
    currency: "USD",
  },
  etherscan: {
    apiKey: {
      "nilotic-testnet": process.env.NILOTIC_EXPLORER_API_KEY || "",
      "nilotic-mainnet": process.env.NILOTIC_EXPLORER_API_KEY || "",
    },
    customChains: [
      {
        network: "nilotic-testnet",
        chainId: parseInt(process.env.NILOTIC_TESTNET_CHAIN_ID || "1234"),
        urls: {
          apiURL: process.env.NILOTIC_TESTNET_EXPLORER_API_URL || "https://testnet-explorer.nilotic.chain/api",
          browserURL: process.env.NILOTIC_TESTNET_EXPLORER_URL || "https://testnet-explorer.nilotic.chain",
        },
      },
      {
        network: "nilotic-mainnet",
        chainId: parseInt(process.env.NILOTIC_MAINNET_CHAIN_ID || "1235"),
        urls: {
          apiURL: process.env.NILOTIC_MAINNET_EXPLORER_API_URL || "https://explorer.nilotic.chain/api",
          browserURL: process.env.NILOTIC_MAINNET_EXPLORER_URL || "https://explorer.nilotic.chain",
        },
      },
    ],
  },
  paths: {
    sources: "./contracts",
    tests: "./test",
    cache: "./cache",
    artifacts: "./artifacts",
  },
}; 