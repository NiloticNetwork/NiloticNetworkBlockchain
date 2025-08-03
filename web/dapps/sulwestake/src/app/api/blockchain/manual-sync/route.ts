import { NextRequest, NextResponse } from "next/server";
import { verifyJWTToken } from "../../../../lib/auth";
import { blockchainAPI } from "../../../utils/blockchain";
import { prisma } from "../../../../lib/database";

export async function POST(request: NextRequest) {
  try {
    // Verify authentication
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);
    const decoded = verifyJWTToken(token);
    if (!decoded) {
      return NextResponse.json({ error: "Invalid token" }, { status: 401 });
    }

    const { address, userId } = await request.json();

    if (!address) {
      return NextResponse.json(
        { error: "Address is required" },
        { status: 400 }
      );
    }

    console.log(
      `Manual sync requested for address: ${address}, user: ${userId}`
    );

    // Get blockchain data
    const blockchainBalance = await blockchainAPI.getBalance(address);
    const stakingData = await blockchainAPI.getUserStakingData(address);

    console.log("Blockchain data:", {
      address,
      balance: blockchainBalance,
      stakingData,
    });

    // Find wallet in database
    let wallet = await prisma.userWallet.findFirst({
      where: { address },
    });

    if (!wallet) {
      // Create wallet if it doesn't exist
      if (!userId) {
        return NextResponse.json(
          { error: "UserId required to create new wallet" },
          { status: 400 }
        );
      }

      wallet = await prisma.userWallet.create({
        data: {
          address,
          name: `Wallet_${address.slice(-6)}`,
          type: "nilotic",
          balance: blockchainBalance,
          staked: stakingData.totalStaked,
          rewards: stakingData.totalRewards,
          userId,
        },
      });

      console.log(`Created new wallet in database:`, wallet);
    } else {
      // Update existing wallet
      const oldBalance = wallet.balance;
      const oldStaked = wallet.staked;
      const oldRewards = wallet.rewards;

      wallet = await prisma.userWallet.update({
        where: { id: wallet.id },
        data: {
          balance: blockchainBalance,
          staked: stakingData.totalStaked,
          rewards: stakingData.totalRewards,
          updatedAt: new Date(),
        },
      });

      console.log(
        `Updated wallet balance: ${oldBalance}→${blockchainBalance}, staked: ${oldStaked}→${stakingData.totalStaked}, rewards: ${oldRewards}→${stakingData.totalRewards}`
      );
    }

    // Get transactions for this wallet
    const transactions = await blockchainAPI.getTransactions();
    const walletTransactions = transactions.filter(
      (tx) => tx.from === address || tx.to === address
    );

    console.log(
      `Found ${walletTransactions.length} transactions for wallet ${address}`
    );

    // Sync transactions
    let newTransactions = 0;
    let updatedTransactions = 0;

    for (const tx of walletTransactions) {
      const existingTx = await prisma.userTransaction.findFirst({
        where: { hash: tx.hash },
      });

      if (!existingTx) {
        // Create new transaction
        await prisma.userTransaction.create({
          data: {
            hash: tx.hash,
            from: tx.from,
            to: tx.to,
            amount: tx.amount,
            type: "transfer",
            status: "confirmed",
            timestamp: new Date(tx.timestamp * 1000),
            userId: wallet.userId,
            blockNumber: 0, // Will be updated when available
            gasUsed: 0,
            fee: 0,
          },
        });
        newTransactions++;
      } else if (existingTx.status !== "confirmed") {
        // Update transaction status
        await prisma.userTransaction.update({
          where: { id: existingTx.id },
          data: {
            status: "confirmed",
            updatedAt: new Date(),
          },
        });
        updatedTransactions++;
      }
    }

    // Get updated wallet info
    const updatedWallet = await prisma.userWallet.findUnique({
      where: { id: wallet.id },
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

    const result = {
      success: true,
      wallet: updatedWallet,
      syncResults: {
        balanceUpdated: true,
        stakingDataUpdated: true,
        newTransactions,
        updatedTransactions,
        totalTransactions: walletTransactions.length,
      },
      blockchainData: {
        balance: blockchainBalance,
        stakingData,
      },
      timestamp: new Date().toISOString(),
    };

    console.log("Manual sync completed:", result);

    return NextResponse.json(result);
  } catch (error) {
    console.error("Manual sync error:", error);
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
