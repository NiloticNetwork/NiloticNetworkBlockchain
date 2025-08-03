import { prisma } from "./database";
import { BlockchainSyncService } from "./blockchain-sync-service";

interface BackgroundSyncConfig {
  enabled: boolean;
  interval: number; // milliseconds
  maxConcurrentUsers: number;
  retryAttempts: number;
  retryDelay: number; // milliseconds
}

export class BackgroundSyncService {
  private static isRunning = false;
  private static interval: NodeJS.Timeout | null = null;
  private static config: BackgroundSyncConfig = {
    enabled: true,
    interval: 60000, // 1 minute
    maxConcurrentUsers: 10,
    retryAttempts: 3,
    retryDelay: 5000, // 5 seconds
  };

  /**
   * Start the background sync service
   */
  static async start(): Promise<void> {
    if (this.isRunning) {
      console.log("Background sync service is already running");
      return;
    }

    console.log("Starting background sync service...");
    this.isRunning = true;

    // Perform initial sync
    await this.performBackgroundSync();

    // Set up periodic sync
    this.interval = setInterval(async () => {
      await this.performBackgroundSync();
    }, this.config.interval);

    console.log(
      `Background sync service started with ${this.config.interval}ms interval`
    );
  }

  /**
   * Stop the background sync service
   */
  static stop(): void {
    if (!this.isRunning) {
      console.log("Background sync service is not running");
      return;
    }

    console.log("Stopping background sync service...");
    this.isRunning = false;

    if (this.interval) {
      clearInterval(this.interval);
      this.interval = null;
    }

    console.log("Background sync service stopped");
  }

  /**
   * Update configuration
   */
  static updateConfig(newConfig: Partial<BackgroundSyncConfig>): void {
    this.config = { ...this.config, ...newConfig };
    console.log("Background sync configuration updated:", this.config);
  }

  /**
   * Get current configuration
   */
  static getConfig(): BackgroundSyncConfig {
    return { ...this.config };
  }

  /**
   * Get service status
   */
  static getStatus(): {
    isRunning: boolean;
    lastSync?: Date;
    config: BackgroundSyncConfig;
  } {
    return {
      isRunning: this.isRunning,
      config: this.config,
    };
  }

  /**
   * Perform background sync for all active users
   */
  private static async performBackgroundSync(): Promise<void> {
    try {
      console.log("Starting background sync for all active users...");

      // Get all active users
      const activeUsers = await prisma.user.findMany({
        where: {
          isActive: true,
          lastLogin: {
            gte: new Date(Date.now() - 24 * 60 * 60 * 1000), // Active in last 24 hours
          },
        },
        select: {
          id: true,
          email: true,
          lastLogin: true,
        },
        take: this.config.maxConcurrentUsers,
        orderBy: {
          lastLogin: "desc",
        },
      });

      console.log(
        `Found ${activeUsers.length} active users for background sync`
      );

      // Process users in batches to avoid overwhelming the system
      const batchSize = 3;
      for (let i = 0; i < activeUsers.length; i += batchSize) {
        const batch = activeUsers.slice(i, i + batchSize);

        await Promise.allSettled(
          batch.map(async (user) => {
            try {
              await this.syncUserWithRetry(user.id);
            } catch (error) {
              console.error(
                `Background sync failed for user ${user.id}:`,
                error
              );
            }
          })
        );

        // Small delay between batches to prevent overwhelming the blockchain
        if (i + batchSize < activeUsers.length) {
          await new Promise((resolve) => setTimeout(resolve, 1000));
        }
      }

      console.log("Background sync completed successfully");
    } catch (error) {
      console.error("Background sync failed:", error);
    }
  }

  /**
   * Sync user with retry logic
   */
  private static async syncUserWithRetry(userId: string): Promise<void> {
    let lastError: Error | null = null;

    for (let attempt = 1; attempt <= this.config.retryAttempts; attempt++) {
      try {
        await BlockchainSyncService.forceUserSync(userId);
        console.log(
          `Background sync successful for user ${userId} (attempt ${attempt})`
        );
        return;
      } catch (error) {
        lastError = error instanceof Error ? error : new Error("Unknown error");
        console.warn(
          `Background sync attempt ${attempt} failed for user ${userId}:`,
          error
        );

        if (attempt < this.config.retryAttempts) {
          await new Promise((resolve) =>
            setTimeout(resolve, this.config.retryDelay)
          );
        }
      }
    }

    console.error(
      `Background sync failed for user ${userId} after ${this.config.retryAttempts} attempts:`,
      lastError
    );
    throw lastError;
  }

  /**
   * Force immediate background sync
   */
  static async forceSync(): Promise<void> {
    console.log("Forcing immediate background sync...");
    await this.performBackgroundSync();
  }

  /**
   * Get sync statistics
   */
  static async getSyncStats(): Promise<{
    totalUsers: number;
    activeUsers: number;
    syncedUsers: number;
    failedUsers: number;
    lastSyncTime?: Date;
  }> {
    try {
      const totalUsers = await prisma.user.count();
      const activeUsers = await prisma.user.count({
        where: {
          isActive: true,
          lastLogin: {
            gte: new Date(Date.now() - 24 * 60 * 60 * 1000),
          },
        },
      });

      const allSyncStatuses = BlockchainSyncService.getAllSyncStatuses();
      const syncedUsers = Array.from(allSyncStatuses.values()).filter(
        (status) => !status.error
      ).length;
      const failedUsers = Array.from(allSyncStatuses.values()).filter(
        (status) => status.error
      ).length;

      return {
        totalUsers,
        activeUsers,
        syncedUsers,
        failedUsers,
        lastSyncTime: this.isRunning ? new Date() : undefined,
      };
    } catch (error) {
      console.error("Failed to get sync stats:", error);
      return {
        totalUsers: 0,
        activeUsers: 0,
        syncedUsers: 0,
        failedUsers: 0,
      };
    }
  }
}

// Auto-start background sync service when this module is loaded
if (process.env.NODE_ENV === "production") {
  BackgroundSyncService.start().catch((error) => {
    console.error("Failed to start background sync service:", error);
  });
}
