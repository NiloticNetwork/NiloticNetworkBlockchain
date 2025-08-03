import { NextRequest, NextResponse } from "next/server";
import { stakingAPI } from "../../../../utils/blockchain";

export async function GET(
  request: NextRequest,
  { params }: { params: { address: string } }
) {
  try {
    const { address } = params;

    if (!address) {
      return NextResponse.json(
        { error: "Wallet address is required" },
        { status: 400 }
      );
    }

    // Fetch real staking data from blockchain
    const userStakingData = await stakingAPI.getUserStakingData(address);

    return NextResponse.json(userStakingData);
  } catch (error) {
    console.error("Error fetching user staking data:", error);
    return NextResponse.json(
      { error: "Failed to fetch user staking data" },
      { status: 500 }
    );
  }
}
