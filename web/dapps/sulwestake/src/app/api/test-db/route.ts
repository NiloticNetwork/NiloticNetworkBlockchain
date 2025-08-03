import { NextRequest, NextResponse } from "next/server";
import { prisma } from "../../lib/database";

export async function GET(request: NextRequest) {
  try {
    console.log("Test database endpoint called");

    // Test database connection
    const userCount = await prisma.user.count();
    console.log("Total users in database:", userCount);

    // Test wallet query
    const walletCount = await prisma.userWallet.count();
    console.log("Total wallets in database:", walletCount);

    // Test transaction query
    const transactionCount = await prisma.userTransaction.count();
    console.log("Total transactions in database:", transactionCount);

    return NextResponse.json({
      success: true,
      message: "Database connection successful",
      stats: {
        users: userCount,
        wallets: walletCount,
        transactions: transactionCount,
      },
    });
  } catch (error) {
    console.error("Test database error:", error);
    return NextResponse.json(
      {
        success: false,
        error: error instanceof Error ? error.message : "Unknown error",
        details: error instanceof Error ? error.stack : undefined,
      },
      { status: 500 }
    );
  }
}
