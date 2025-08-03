import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../utils/blockchain";

export async function POST(request: NextRequest) {
  try {
    const { fromAddress, toAddress, amount } = await request.json();

    console.log(
      `Testing transaction: ${amount} NIL from ${fromAddress} to ${toAddress}`
    );

    // Test blockchain API directly
    const transaction = await blockchainAPI.submitTransaction({
      from: fromAddress,
      to: toAddress,
      amount: amount,
    });

    console.log("Test transaction result:", transaction);

    return NextResponse.json({
      success: true,
      message: "Test transaction submitted successfully",
      transaction: {
        hash: transaction.hash,
        from: transaction.from,
        to: transaction.to,
        amount: transaction.amount,
        timestamp: transaction.timestamp,
      },
    });
  } catch (error) {
    console.error("Test transaction error:", error);
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
