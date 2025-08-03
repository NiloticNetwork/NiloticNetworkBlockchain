import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../../utils/blockchain";

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

    // Get wallet information
    const walletInfo = await blockchainAPI.getWalletInfo(address);

    return NextResponse.json(walletInfo);
  } catch (error) {
    console.error("Error fetching wallet info:", error);
    return NextResponse.json(
      { error: "Failed to fetch wallet information" },
      { status: 500 }
    );
  }
}
