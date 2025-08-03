import { NextRequest, NextResponse } from "next/server";
import { stakingAPI } from "../../../utils/blockchain";

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { amount, address, operation } = body;

    if (!amount || !address || !operation) {
      return NextResponse.json(
        { error: "Amount, address, and operation are required" },
        { status: 400 }
      );
    }

    let result;

    switch (operation) {
      case "stake":
        result = await stakingAPI.stakeTokens(amount, address);
        break;
      case "unstake":
        result = await stakingAPI.unstakeTokens(amount, address);
        break;
      case "claim":
        result = await stakingAPI.claimRewards(address);
        break;
      default:
        return NextResponse.json(
          { error: "Invalid operation. Use 'stake', 'unstake', or 'claim'" },
          { status: 400 }
        );
    }

    return NextResponse.json({
      success: true,
      message: `${operation} operation completed successfully`,
      transaction: result,
    });
  } catch (error) {
    console.error("Error performing staking operation:", error);
    return NextResponse.json(
      {
        error: "Failed to perform staking operation",
        details: error instanceof Error ? error.message : "Unknown error",
      },
      { status: 500 }
    );
  }
}
