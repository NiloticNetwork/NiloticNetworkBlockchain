import { blockchainAPI } from "../app/utils/blockchain";
import {
  createUserWallet,
  updateWalletBalance,
  createUserTransaction,
  updateUserStakingData,
  prisma,
} from "./database";

export class BlockchainService {
  static async syncUserWallets(userId: string) {
    try {
      // Get blockchain transactions
      const transactions = await blockchainAPI.getTransactions();

      // Get user's existing wallets
      const userWallets = await prisma.userWallet.findMany({
        where: { userId },
      });

      const walletAddresses = new Set<string>();
      const newWallets: Array<{ address: string; name: string; type: string }> =
        [];

      // Extract wallet addresses from transactions
      for (const tx of transactions) {
        // Check if transaction involves user's wallets
        const isUserTransaction = userWallets.some(
          (wallet) => wallet.address === tx.from || wallet.address === tx.to
        );

        if (isUserTransaction) {
          walletAddresses.add(tx.from);
          walletAddresses.add(tx.to);
        }
      }

      // Create new wallets for addresses not in database
      for (const address of walletAddresses) {
        const existingWallet = userWallets.find((w) => w.address === address);
        if (!existingWallet) {
          newWallets.push({
            address,
            name: `Wallet_${address.slice(-6)}`,
            type: "nilotic",
          });
        }
      }

      // Create new wallets in database
      for (const walletData of newWallets) {
        await createUserWallet(userId, walletData);
      }

      // Update wallet balances from blockchain
      const updatedWallets = await prisma.userWallet.findMany({
        where: { userId },
      });

      for (const wallet of updatedWallets) {
        try {
          const balance = await blockchainAPI.getBalance(wallet.address);
          await updateWalletBalance(
            wallet.id,
            balance,
            wallet.staked,
            wallet.rewards
          );
        } catch (error) {
          console.error(
            `Failed to update balance for wallet ${wallet.address}:`,
            error
          );
        }
      }

      return updatedWallets;
    } catch (error) {
      console.error("Error syncing user wallets:", error);
      throw error;
    }
  }

  static async syncUserTransactions(userId: string) {
    try {
      // Get blockchain transactions
      const transactions = await blockchainAPI.getTransactions();

      // Get user's wallets
      const userWallets = await prisma.userWallet.findMany({
        where: { userId },
      });

      const userWalletAddresses = userWallets.map((w) => w.address);
      const userTransactions: any[] = [];

      // Filter transactions that involve user's wallets
      for (const tx of transactions) {
        if (
          userWalletAddresses.includes(tx.from) ||
          userWalletAddresses.includes(tx.to)
        ) {
          const walletId = userWallets.find(
            (w) => w.address === tx.from || w.address === tx.to
          )?.id;

          userTransactions.push({
            hash: tx.hash,
            from: tx.from,
            to: tx.to,
            amount: tx.amount,
            type: this.categorizeTransaction(tx),
            status: "confirmed",
            timestamp: new Date(tx.timestamp),
            walletId,
          });
        }
      }

      // Check which transactions are new
      const existingTransactions = await prisma.userTransaction.findMany({
        where: { userId },
        select: { hash: true },
      });

      const existingHashes = new Set(existingTransactions.map((t) => t.hash));
      const newTransactions = userTransactions.filter(
        (tx) => !existingHashes.has(tx.hash)
      );

      // Create new transactions in database
      for (const txData of newTransactions) {
        await createUserTransaction(userId, txData);
      }

      return newTransactions;
    } catch (error) {
      console.error("Error syncing user transactions:", error);
      throw error;
    }
  }

  static async syncUserStakingData(userId: string) {
    try {
      // Get user's transactions
      const userTransactions = await prisma.userTransaction.findMany({
        where: { userId },
      });

      // Calculate staking data from transactions
      const stakingTransactions = userTransactions.filter(
        (tx) => tx.type === "stake" || tx.to.toLowerCase().includes("staking")
      );

      const rewardTransactions = userTransactions.filter(
        (tx) => tx.type === "reward" || tx.from.toLowerCase().includes("reward")
      );

      const totalStaked = stakingTransactions.reduce(
        (sum, tx) => sum + tx.amount,
        0
      );
      const totalRewards = rewardTransactions.reduce(
        (sum, tx) => sum + tx.amount,
        0
      );

      // Calculate staking level
      let stakingLevel = "bronze";
      if (totalStaked >= 1000) stakingLevel = "platinum";
      else if (totalStaked >= 500) stakingLevel = "gold";
      else if (totalStaked >= 100) stakingLevel = "silver";

      // Calculate next reward estimate
      const apy = 12.5;
      const nextRewardEstimate = (totalStaked * (apy / 100)) / 365; // Daily estimate

      // Update staking data
      await updateUserStakingData(userId, {
        totalStaked,
        totalRewards,
        stakingLevel,
        stakingStartDate:
          stakingTransactions.length > 0
            ? new Date(
                Math.min(
                  ...stakingTransactions.map((tx) => tx.timestamp.getTime())
                )
              )
            : undefined,
        lastRewardDate:
          rewardTransactions.length > 0
            ? new Date(
                Math.max(
                  ...rewardTransactions.map((tx) => tx.timestamp.getTime())
                )
              )
            : undefined,
        nextRewardEstimate,
      });

      return {
        totalStaked,
        totalRewards,
        stakingLevel,
        apy,
        nextRewardEstimate,
      };
    } catch (error) {
      console.error("Error syncing user staking data:", error);
      throw error;
    }
  }

  static async syncUserProfile(userId: string) {
    try {
      // Sync all user data from blockchain
      await this.syncUserWallets(userId);
      await this.syncUserTransactions(userId);
      await this.syncUserStakingData(userId);

      // Get updated user profile
      const user = await prisma.user.findUnique({
        where: { id: userId },
        include: {
          preferences: true,
          wallets: true,
          stakingData: true,
          transactions: {
            orderBy: { timestamp: "desc" },
            take: 10,
          },
        },
      });

      return user;
    } catch (error) {
      console.error("Error syncing user profile:", error);
      throw error;
    }
  }

  static async createBlockchainWallet(
    userId: string,
    walletName: string,
    password: string
  ) {
    try {
      // Create wallet on blockchain
      const walletResult = await blockchainAPI.createWallet(
        walletName,
        password
      );

      if (!walletResult.address) {
        throw new Error("Failed to create wallet on blockchain");
      }

      // Create wallet in database
      const wallet = await createUserWallet(userId, {
        address: walletResult.address,
        name: walletName,
        type: "nilotic",
        isPrimary: false,
      });

      return wallet;
    } catch (error) {
      console.error("Error creating blockchain wallet:", error);
      throw error;
    }
  }

  static async getBlockchainStatus() {
    try {
      const status = await blockchainAPI.getStatus();
      const metrics = await blockchainAPI.getMetrics();

      return {
        connected: true,
        chainHeight: status.chainHeight,
        difficulty: status.difficulty,
        pendingTransactions: status.pendingTransactions,
        totalSupply: metrics.totalSupply,
        circulatingSupply: metrics.circulatingSupply,
        availableForMining: metrics.availableForMining,
        totalMined: metrics.totalMined,
        miningRewards: metrics.miningRewards,
      };
    } catch (error) {
      console.error("Error getting blockchain status:", error);
      return {
        connected: false,
        error: error instanceof Error ? error.message : "Unknown error",
      };
    }
  }

  private static categorizeTransaction(tx: any): string {
    const to = tx.to.toLowerCase();
    const from = tx.from.toLowerCase();

    if (to.includes("staking") || from.includes("staking")) {
      return "stake";
    }

    if (from.includes("reward") || to.includes("reward")) {
      return "reward";
    }

    if (from.includes("mining") || to.includes("mining")) {
      return "mining";
    }

    return "transfer";
  }
}
