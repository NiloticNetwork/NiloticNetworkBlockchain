import { blockchainAPI, stakingAPI } from "../app/utils/blockchain";
import {
  createUserWallet,
  updateWalletBalance,
  createUserTransaction,
  updateUserStakingData,
  prisma,
  findUserById,
  getUserWallets,
} from "./database";

interface SyncStatus {
  lastSync: Date;
  isSyncing: boolean;
  error?: string;
  walletCount: number;
  transactionCount: number;
}

interface WalletSyncResult {
  walletId: string;
  address: string;
  oldBalance: number;
  newBalance: number;
  oldStaked: number;
  newStaked: number;
  oldRewards: number;
  newRewards: number;
  updated: boolean;
}

interface TransactionSyncResult {
  transactionId: string;
  hash: string;
  status: "new" | "updated" | "unchanged";
  blockNumber?: number;
  gasUsed?: number;
  fee?: number;
}

export class BlockchainSyncService {
  private static syncStatus: Map<string, SyncStatus> = new Map();
  private static syncIntervals: Map<string, NodeJS.Timeout> = new Map();
  private static readonly SYNC_INTERVAL = 30000; // 30 seconds
  private static readonly MAX_RETRIES = 3;
  private static readonly RETRY_DELAY = 5000; // 5 seconds

  /**
   * Start real-time sync for a user
   */
  static async startUserSync(userId: string): Promise<void> {
    try {
      // Check if sync is already running
      if (this.syncIntervals.has(userId)) {
        console.log(`Sync already running for user ${userId}`);
        return;
      }

      // Initialize sync status
      this.syncStatus.set(userId, {
        lastSync: new Date(),
        isSyncing: false,
        walletCount: 0,
        transactionCount: 0,
      });

      // Perform initial sync
      await this.performUserSync(userId);

      // Start periodic sync
      const interval = setInterval(async () => {
        await this.performUserSync(userId);
      }, this.SYNC_INTERVAL);

      this.syncIntervals.set(userId, interval);

      console.log(`Started real-time sync for user ${userId}`);
    } catch (error) {
      console.error(`Failed to start sync for user ${userId}:`, error);
      throw error;
    }
  }

  /**
   * Stop real-time sync for a user
   */
  static stopUserSync(userId: string): void {
    const interval = this.syncIntervals.get(userId);
    if (interval) {
      clearInterval(interval);
      this.syncIntervals.delete(userId);
      this.syncStatus.delete(userId);
      console.log(`Stopped real-time sync for user ${userId}`);
    }
  }

  /**
   * Get sync status for a user
   */
  static getUserSyncStatus(userId: string): SyncStatus | null {
    return this.syncStatus.get(userId) || null;
  }

  /**
   * Perform comprehensive user sync
   */
  private static async performUserSync(userId: string): Promise<void> {
    const status = this.syncStatus.get(userId);
    if (!status || status.isSyncing) {
      return;
    }

    status.isSyncing = true;
    status.error = undefined;

    try {
      // Sync wallets and balances
      const walletResults = await this.syncUserWallets(userId);

      // Sync transactions
      const transactionResults = await this.syncUserTransactions(userId);

      // Sync staking data
      await this.syncUserStakingData(userId);

      // Update sync status
      status.lastSync = new Date();
      status.walletCount = walletResults.length;
      status.transactionCount = transactionResults.length;

      console.log(
        `Sync completed for user ${userId}: ${walletResults.length} wallets, ${transactionResults.length} transactions`
      );
    } catch (error) {
      console.error(`Sync failed for user ${userId}:`, error);
      status.error = error instanceof Error ? error.message : "Unknown error";
    } finally {
      status.isSyncing = false;
    }
  }

  /**
   * Enhanced wallet sync with detailed tracking
   */
  private static async syncUserWallets(
    userId: string
  ): Promise<WalletSyncResult[]> {
    const results: WalletSyncResult[] = [];

    try {
      // Get user's wallets from database
      const userWallets = await getUserWallets(userId);

      // Get blockchain transactions to discover new wallets
      const blockchainTransactions = await blockchainAPI.getTransactions();

      // Discover new wallets from transactions
      const discoveredAddresses = new Set<string>();
      for (const tx of blockchainTransactions) {
        if (
          userWallets.some((w) => w.address === tx.from || w.address === tx.to)
        ) {
          discoveredAddresses.add(tx.from);
          discoveredAddresses.add(tx.to);
        }
      }

      // Create new wallets for discovered addresses
      for (const address of discoveredAddresses) {
        const existingWallet = userWallets.find((w) => w.address === address);
        if (!existingWallet) {
          await createUserWallet(userId, {
            address,
            name: `Wallet_${address.slice(-6)}`,
            type: "nilotic",
          });
          console.log(`Created new wallet ${address} for user ${userId}`);
        }
      }

      // Refresh wallet list after creating new ones
      const updatedWallets = await getUserWallets(userId);

      // Update balances for all wallets
      for (const wallet of updatedWallets) {
        try {
          const oldBalance = wallet.balance;
          const oldStaked = wallet.staked;
          const oldRewards = wallet.rewards;

          // Get current blockchain balance
          const newBalance = await blockchainAPI.getBalance(wallet.address);

          // Get staking data for this wallet
          const stakingData = await stakingAPI.getUserStakingData(
            wallet.address
          );
          const newStaked = stakingData.totalStaked;
          const newRewards = stakingData.totalRewards;

          // Update database if values changed
          const hasChanges =
            oldBalance !== newBalance ||
            oldStaked !== newStaked ||
            oldRewards !== newRewards;

          if (hasChanges) {
            await updateWalletBalance(
              wallet.id,
              newBalance,
              newStaked,
              newRewards
            );

            results.push({
              walletId: wallet.id,
              address: wallet.address,
              oldBalance,
              newBalance,
              oldStaked,
              newStaked,
              oldRewards,
              newRewards,
              updated: true,
            });

            console.log(
              `Updated wallet ${wallet.address}: balance ${oldBalance}→${newBalance}, staked ${oldStaked}→${newStaked}, rewards ${oldRewards}→${newRewards}`
            );
          } else {
            results.push({
              walletId: wallet.id,
              address: wallet.address,
              oldBalance,
              newBalance,
              oldStaked,
              newStaked,
              oldRewards,
              newRewards,
              updated: false,
            });
          }
        } catch (error) {
          console.error(`Failed to sync wallet ${wallet.address}:`, error);
        }
      }

      return results;
    } catch (error) {
      console.error(`Failed to sync wallets for user ${userId}:`, error);
      throw error;
    }
  }

  /**
   * Enhanced transaction sync with status tracking
   */
  private static async syncUserTransactions(
    userId: string
  ): Promise<TransactionSyncResult[]> {
    const results: TransactionSyncResult[] = [];

    try {
      // Get user's wallets
      const userWallets = await getUserWallets(userId);
      const userWalletAddresses = userWallets.map((w) => w.address);

      // Get blockchain transactions
      const blockchainTransactions = await blockchainAPI.getTransactions();

      // Get existing transactions from database
      const existingTransactions = await prisma.userTransaction.findMany({
        where: { userId },
        orderBy: { timestamp: "desc" },
        take: 100, // Limit to recent transactions
      });

      const existingTxHashes = new Set(
        existingTransactions.map((tx) => tx.hash)
      );

      // Process new transactions
      for (const tx of blockchainTransactions) {
        const isUserTransaction =
          userWalletAddresses.includes(tx.from) ||
          userWalletAddresses.includes(tx.to);

        if (isUserTransaction) {
          const isNewTransaction = !existingTxHashes.has(tx.hash);

          if (isNewTransaction) {
            // Create new transaction in database
            const newTransaction = await createUserTransaction(userId, {
              hash: tx.hash,
              from: tx.from,
              to: tx.to,
              amount: tx.amount,
              type: this.categorizeTransaction(tx),
              status: "confirmed",
              timestamp: new Date(tx.timestamp * 1000),
              blockNumber: tx.blockNumber,
              gasUsed: tx.gasUsed,
              fee: tx.fee,
            });

            results.push({
              transactionId: newTransaction.id,
              hash: tx.hash,
              status: "new",
              blockNumber: tx.blockNumber,
              gasUsed: tx.gasUsed,
              fee: tx.fee,
            });

            console.log(
              `Created new transaction ${tx.hash} for user ${userId}`
            );
          } else {
            // Check if existing transaction needs updating
            const existingTx = existingTransactions.find(
              (t) => t.hash === tx.hash
            );
            if (existingTx && existingTx.status !== "confirmed") {
              await prisma.userTransaction.update({
                where: { id: existingTx.id },
                data: {
                  status: "confirmed",
                  blockNumber: tx.blockNumber,
                  gasUsed: tx.gasUsed,
                  fee: tx.fee,
                },
              });

              results.push({
                transactionId: existingTx.id,
                hash: tx.hash,
                status: "updated",
                blockNumber: tx.blockNumber,
                gasUsed: tx.gasUsed,
                fee: tx.fee,
              });

              console.log(`Updated transaction ${tx.hash} for user ${userId}`);
            } else {
              results.push({
                transactionId: existingTx?.id || "",
                hash: tx.hash,
                status: "unchanged",
              });
            }
          }
        }
      }

      return results;
    } catch (error) {
      console.error(`Failed to sync transactions for user ${userId}:`, error);
      throw error;
    }
  }

  /**
   * Enhanced staking data sync
   */
  private static async syncUserStakingData(userId: string): Promise<void> {
    try {
      const user = await findUserById(userId);
      if (!user) {
        throw new Error(`User ${userId} not found`);
      }

      // Get user's wallets
      const userWallets = await getUserWallets(userId);

      let totalStaked = 0;
      let totalRewards = 0;

      // Aggregate staking data from all wallets
      for (const wallet of userWallets) {
        try {
          const stakingData = await stakingAPI.getUserStakingData(
            wallet.address
          );
          totalStaked += stakingData.totalStaked;
          totalRewards += stakingData.totalRewards;
        } catch (error) {
          console.error(
            `Failed to get staking data for wallet ${wallet.address}:`,
            error
          );
        }
      }

      // Update user's staking data
      await updateUserStakingData(userId, {
        totalStaked,
        totalRewards,
        stakingStartDate: user.stakingData?.stakingStartDate || new Date(),
        lastRewardDate: new Date(),
        nextRewardEstimate: (totalStaked * 0.125) / 365, // 12.5% APY daily estimate
      });

      console.log(
        `Updated staking data for user ${userId}: staked=${totalStaked}, rewards=${totalRewards}`
      );
    } catch (error) {
      console.error(`Failed to sync staking data for user ${userId}:`, error);
      throw error;
    }
  }

  /**
   * Categorize transaction type based on blockchain data
   */
  private static categorizeTransaction(tx: any): string {
    // Determine transaction type based on blockchain data
    if (tx.type) {
      return tx.type;
    }

    // Default categorization logic
    if (tx.amount > 0 && tx.to && tx.from) {
      return "transfer";
    }

    return "unknown";
  }

  /**
   * Force immediate sync for a user
   */
  static async forceUserSync(userId: string): Promise<void> {
    console.log(`Forcing immediate sync for user ${userId}`);
    await this.performUserSync(userId);
  }

  /**
   * Get all active sync statuses
   */
  static getAllSyncStatuses(): Map<string, SyncStatus> {
    return new Map(this.syncStatus);
  }

  /**
   * Stop all active syncs
   */
  static stopAllSyncs(): void {
    for (const [userId, interval] of this.syncIntervals) {
      clearInterval(interval);
    }
    this.syncIntervals.clear();
    this.syncStatus.clear();
    console.log("Stopped all active syncs");
  }
}
