import { PrismaClient } from "@prisma/client";
import { hashPassword } from "../src/lib/auth";

const prisma = new PrismaClient();

async function main() {
  console.log("🌱 Seeding database...");

  // Create demo user
  const demoPasswordHash = await hashPassword("password123");

  const demoUser = await prisma.user.upsert({
    where: { email: "demo@nilotic.com" },
    update: {},
    create: {
      email: "demo@nilotic.com",
      username: "demo_user",
      firstName: "Demo",
      lastName: "User",
      passwordHash: demoPasswordHash,
      avatar:
        "https://images.unsplash.com/photo-1472099645785-5658abf4ff4e?w=150&h=150&fit=crop&crop=face",
      isActive: true,
      role: "user",
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
      stakingData: true,
    },
  });

  console.log("✅ Demo user created:", demoUser.email);

  // Create sample wallets for demo user
  const sampleWallets = [
    {
      address: "NIL8af1f08b74eb0abe3e2b4fdf4d3a020f21",
      name: "Primary Wallet",
      type: "nilotic",
      isPrimary: true,
      balance: 1250.5,
      staked: 500.0,
      rewards: 75.25,
    },
    {
      address: "NIL9bf2f19c85fc1cbf3e3c5gdf5e4b131f32",
      name: "Staking Wallet",
      type: "nilotic",
      isPrimary: false,
      balance: 750.25,
      staked: 300.0,
      rewards: 45.5,
    },
  ];

  for (const walletData of sampleWallets) {
    await prisma.userWallet.upsert({
      where: { address: walletData.address },
      update: {},
      create: {
        userId: demoUser.id,
        address: walletData.address,
        name: walletData.name,
        type: walletData.type,
        isPrimary: walletData.isPrimary,
        balance: walletData.balance,
        staked: walletData.staked,
        rewards: walletData.rewards,
      },
    });
  }

  console.log("✅ Sample wallets created");

  // Create sample transactions
  const sampleTransactions = [
    {
      hash: "0x1234567890abcdef",
      from: "NIL8af1f08b74eb0abe3e2b4fdf4d3a020f21",
      to: "staking_pool",
      amount: 100.0,
      type: "stake",
      status: "confirmed",
      timestamp: new Date(Date.now() - 2 * 60 * 60 * 1000), // 2 hours ago
    },
    {
      hash: "0xabcdef1234567890",
      from: "mining_reward",
      to: "NIL8af1f08b74eb0abe3e2b4fdf4d3a020f21",
      amount: 25.25,
      type: "reward",
      status: "confirmed",
      timestamp: new Date(Date.now() - 24 * 60 * 60 * 1000), // 1 day ago
    },
    {
      hash: "0x9876543210fedcba",
      from: "NIL9bf2f19c85fc1cbf3e3c5gdf5e4b131f32",
      to: "staking_pool",
      amount: 150.0,
      type: "stake",
      status: "confirmed",
      timestamp: new Date(Date.now() - 3 * 60 * 60 * 1000), // 3 hours ago
    },
  ];

  for (const txData of sampleTransactions) {
    await prisma.userTransaction.upsert({
      where: { hash: txData.hash },
      update: {},
      create: {
        userId: demoUser.id,
        hash: txData.hash,
        from: txData.from,
        to: txData.to,
        amount: txData.amount,
        type: txData.type,
        status: txData.status,
        timestamp: txData.timestamp,
      },
    });
  }

  console.log("✅ Sample transactions created");

  // Update staking data based on transactions
  const stakingTransactions = sampleTransactions.filter(
    (tx) => tx.type === "stake"
  );
  const rewardTransactions = sampleTransactions.filter(
    (tx) => tx.type === "reward"
  );

  const totalStaked = stakingTransactions.reduce(
    (sum, tx) => sum + tx.amount,
    0
  );
  const totalRewards = rewardTransactions.reduce(
    (sum, tx) => sum + tx.amount,
    0
  );

  await prisma.userStakingData.update({
    where: { userId: demoUser.id },
    data: {
      totalStaked,
      totalRewards,
      stakingLevel:
        totalStaked >= 1000
          ? "platinum"
          : totalStaked >= 500
          ? "gold"
          : totalStaked >= 100
          ? "silver"
          : "bronze",
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
      nextRewardEstimate: (totalStaked * 0.125) / 365, // Daily reward estimate
    },
  });

  console.log("✅ Staking data updated");

  console.log("🎉 Database seeding completed successfully!");
}

main()
  .catch((e) => {
    console.error("❌ Error seeding database:", e);
    process.exit(1);
  })
  .finally(async () => {
    await prisma.$disconnect();
  });
