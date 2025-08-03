import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../utils/blockchain";

export async function GET(request: NextRequest) {
  try {
    // Get mining status
    const miningStatus = await blockchainAPI.getMiningStatus();

    return NextResponse.json(miningStatus);
  } catch (error) {
    console.error("Error fetching mining status:", error);
    return NextResponse.json(
      { error: "Failed to fetch mining status" },
      { status: 500 }
    );
  }
}

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { miner_address } = body;

    if (!miner_address) {
      return NextResponse.json(
        { error: "Miner address is required" },
        { status: 400 }
      );
    }

    // Mine a new block
    const miningResult = await blockchainAPI.mineBlock(miner_address);

    return NextResponse.json(miningResult);
  } catch (error) {
    console.error("Error mining block:", error);
    return NextResponse.json(
      { error: "Failed to mine block" },
      { status: 500 }
    );
  }
}
