import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../utils/blockchain";

export async function GET(request: NextRequest) {
  try {
    // Fetch comprehensive blockchain analytics
    const analytics = await blockchainAPI.getBlockchainAnalytics();

    return NextResponse.json(analytics);
  } catch (error) {
    console.error("Error fetching blockchain analytics:", error);
    return NextResponse.json(
      { error: "Failed to fetch blockchain analytics" },
      { status: 500 }
    );
  }
}
