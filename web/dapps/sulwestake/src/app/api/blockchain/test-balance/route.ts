import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../utils/blockchain";

export async function GET(request: NextRequest) {
  try {
    const { searchParams } = new URL(request.url);
    const address =
      searchParams.get("address") || "NILda9879380c1efaff4aede80339f2e35fac";

    console.log(`Testing balance fetch for address: ${address}`);

    // Test direct balance fetch
    const balance = await blockchainAPI.getBalance(address);

    // Test wallet info fetch
    const walletInfo = await blockchainAPI.getWalletInfo(address);

    // Test staking data fetch
    const stakingData = await blockchainAPI.getUserStakingData(address);

    console.log("Balance fetch result:", {
      address,
      balance,
      walletInfo,
      stakingData,
    });

    return NextResponse.json({
      success: true,
      address,
      balance,
      walletInfo,
      stakingData,
      timestamp: new Date().toISOString(),
    });
  } catch (error) {
    console.error("Test balance fetch error:", error);
    return NextResponse.json(
      {
        success: false,
        error: error instanceof Error ? error.message : "Unknown error",
        address: "NILda9879380c1efaff4aede80339f2e35fac",
        timestamp: new Date().toISOString(),
      },
      { status: 500 }
    );
  }
}
