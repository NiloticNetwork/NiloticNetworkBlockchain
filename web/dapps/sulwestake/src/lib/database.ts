import { PrismaClient } from "@prisma/client";

declare global {
  var __prisma: PrismaClient | undefined;
}

export const prisma =
  globalThis.__prisma ||
  new PrismaClient({
    log:
      process.env.NODE_ENV === "development"
        ? ["query", "error", "warn"]
        : ["error"],
  });

if (process.env.NODE_ENV !== "production") {
  globalThis.__prisma = prisma;
}

export async function connectDatabase() {
  try {
    await prisma.$connect();
    console.log("✅ Database connected successfully");
  } catch (error) {
    console.error("❌ Database connection failed:", error);
    throw error;
  }
}

export async function disconnectDatabase() {
  try {
    await prisma.$disconnect();
    console.log("✅ Database disconnected successfully");
  } catch (error) {
    console.error("❌ Database disconnection failed:", error);
  }
}

// Database utilities
export async function createUserWithDefaults(data: {
  email: string;
  username: string;
  firstName: string;
  lastName: string;
  passwordHash: string;
  avatar?: string;
}) {
  return await prisma.user.create({
    data: {
      ...data,
      preferences: {
        create: {
          theme: "dark",
          language: "en",
          timezone: "UTC",
          emailNotifications: true,
          pushNotifications: true,
          smsNotifications: false,
          showBalance: true,
          showTransactions: true,
          allowAnalytics: true,
        },
      },
      stakingData: {
        create: {
          totalStaked: 0,
          totalRewards: 0,
          apy: 12.5,
          stakingLevel: "bronze",
          nextRewardEstimate: 0,
        },
      },
    },
    include: {
      preferences: true,
      wallets: true,
      stakingData: true,
    },
  });
}

export async function findUserByEmail(email: string) {
  return await prisma.user.findUnique({
    where: { email },
    include: {
      preferences: true,
      wallets: true,
      stakingData: true,
    },
  });
}

export async function findUserByUsername(username: string) {
  return await prisma.user.findUnique({
    where: { username },
    include: {
      preferences: true,
      wallets: true,
      stakingData: true,
    },
  });
}

export async function findUserById(id: string) {
  return await prisma.user.findUnique({
    where: { id },
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
}

export async function createUserSession(
  userId: string,
  token: string,
  expiresAt: Date
) {
  return await prisma.userSession.create({
    data: {
      userId,
      token,
      expiresAt,
      isActive: true,
    },
  });
}

export async function findValidSession(token: string) {
  return await prisma.userSession.findFirst({
    where: {
      token,
      isActive: true,
      expiresAt: {
        gt: new Date(),
      },
    },
    include: {
      user: {
        include: {
          preferences: true,
          wallets: true,
          stakingData: true,
        },
      },
    },
  });
}

export async function invalidateSession(token: string) {
  return await prisma.userSession.updateMany({
    where: { token },
    data: { isActive: false },
  });
}

export async function updateUserLastLogin(userId: string) {
  return await prisma.user.update({
    where: { id: userId },
    data: { lastLogin: new Date() },
  });
}

export async function createUserWallet(
  userId: string,
  walletData: {
    address: string;
    name: string;
    type?: string;
    isPrimary?: boolean;
  }
) {
  return await prisma.userWallet.create({
    data: {
      userId,
      address: walletData.address,
      name: walletData.name,
      type: walletData.type || "nilotic",
      isPrimary: walletData.isPrimary || false,
    },
  });
}

export async function updateWalletBalance(
  walletId: string,
  balance: number,
  staked: number,
  rewards: number
) {
  return await prisma.userWallet.update({
    where: { id: walletId },
    data: {
      balance,
      staked,
      rewards,
      lastActivity: new Date(),
    },
  });
}

export async function createUserTransaction(
  userId: string,
  transactionData: {
    hash: string;
    from: string;
    to: string;
    amount: number;
    type: string;
    status?: string;
    blockNumber?: number;
    gasUsed?: number;
    fee?: number;
    timestamp: Date;
    walletId?: string;
  }
) {
  return await prisma.userTransaction.create({
    data: {
      userId,
      walletId: transactionData.walletId,
      hash: transactionData.hash,
      from: transactionData.from,
      to: transactionData.to,
      amount: transactionData.amount,
      type: transactionData.type,
      status: transactionData.status || "pending",
      blockNumber: transactionData.blockNumber,
      gasUsed: transactionData.gasUsed,
      fee: transactionData.fee,
      timestamp: transactionData.timestamp,
    },
  });
}

export async function updateUserStakingData(
  userId: string,
  stakingData: {
    totalStaked: number;
    totalRewards: number;
    stakingLevel?: string;
    stakingStartDate?: Date;
    lastRewardDate?: Date;
    nextRewardEstimate?: number;
  }
) {
  return await prisma.userStakingData.upsert({
    where: { userId },
    update: {
      totalStaked: stakingData.totalStaked,
      totalRewards: stakingData.totalRewards,
      stakingLevel: stakingData.stakingLevel,
      stakingStartDate: stakingData.stakingStartDate,
      lastRewardDate: stakingData.lastRewardDate,
      nextRewardEstimate: stakingData.nextRewardEstimate,
    },
    create: {
      userId,
      totalStaked: stakingData.totalStaked,
      totalRewards: stakingData.totalRewards,
      stakingLevel: stakingData.stakingLevel || "bronze",
      stakingStartDate: stakingData.stakingStartDate,
      lastRewardDate: stakingData.lastRewardDate,
      nextRewardEstimate: stakingData.nextRewardEstimate || 0,
    },
  });
}

export async function updateUserPreferences(
  userId: string,
  preferences: {
    theme?: string;
    language?: string;
    timezone?: string;
    emailNotifications?: boolean;
    pushNotifications?: boolean;
    smsNotifications?: boolean;
    showBalance?: boolean;
    showTransactions?: boolean;
    allowAnalytics?: boolean;
  }
) {
  return await prisma.userPreferences.update({
    where: { userId },
    data: preferences,
  });
}

export async function getUserWallets(userId: string) {
  return await prisma.userWallet.findMany({
    where: { userId },
    orderBy: { createdAt: "desc" },
  });
}

export async function getWalletById(walletId: string) {
  return await prisma.userWallet.findUnique({
    where: { id: walletId },
  });
}

export async function deleteUserWallet(walletId: string, userId: string) {
  return await prisma.userWallet.delete({
    where: {
      id: walletId,
      userId, // Ensure user owns the wallet
    },
  });
}
