import { NextRequest, NextResponse } from "next/server";
import { prisma } from "../../../../lib/database";
import { blockchainAPI } from "../../../utils/blockchain";

export async function GET(request: NextRequest) {
  try {
    const { searchParams } = new URL(request.url);
    const address =
      searchParams.get("address") || "NILda9879380c1efaff4aede80339f2e35fac";
    const userId = searchParams.get("userId");

    console.log(`Debugging wallet state for address: ${address}`);

    // Get wallet from database
    const dbWallet = await prisma.userWallet.findFirst({
      where: { address },
      include: {
        user: {
          select: {
            id: true,
            email: true,
            firstName: true,
            lastName: true,
          },
        },
      },
    });

    // Get blockchain data
    let blockchainBalance = 0;
    let blockchainStakingData = null;
    let blockchainError = null;

    try {
      blockchainBalance = await blockchainAPI.getBalance(address);
      blockchainStakingData = await blockchainAPI.getUserStakingData(address);
    } catch (error) {
      blockchainError =
        error instanceof Error ? error.message : "Unknown error";
    }

    // Get user's transactions
    const userTransactions = await prisma.userTransaction.findMany({
      where: {
        OR: [{ from: address }, { to: address }],
      },
      orderBy: { timestamp: "desc" },
      take: 10,
    });

    // Get all wallets for the user if userId provided
    let userWallets = null;
    if (userId) {
      userWallets = await prisma.userWallet.findMany({
        where: { userId },
        orderBy: { createdAt: "desc" },
      });
    }

    const debugData = {
      address,
      database: {
        wallet: dbWallet,
        transactions: userTransactions,
        userWallets,
      },
      blockchain: {
        balance: blockchainBalance,
        stakingData: blockchainStakingData,
        error: blockchainError,
      },
      comparison: {
        balanceMatch: dbWallet
          ? Math.abs(dbWallet.balance - blockchainBalance) < 0.01
          : false,
        stakedMatch:
          dbWallet && blockchainStakingData
            ? Math.abs(dbWallet.staked - blockchainStakingData.totalStaked) <
              0.01
            : false,
        rewardsMatch:
          dbWallet && blockchainStakingData
            ? Math.abs(dbWallet.rewards - blockchainStakingData.totalRewards) <
              0.01
            : false,
      },
      timestamp: new Date().toISOString(),
    };

    console.log("Debug wallet state result:", debugData);

    return NextResponse.json(debugData);
  } catch (error) {
    console.error("Debug wallet state error:", error);
    return NextResponse.json(
      {
        success: false,
        error: error instanceof Error ? error.message : "Unknown error",
        timestamp: new Date().toISOString(),
      },
      { status: 500 }
    );
  }
}
