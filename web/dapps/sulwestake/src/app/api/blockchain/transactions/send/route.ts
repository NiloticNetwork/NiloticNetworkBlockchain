import { NextRequest, NextResponse } from "next/server";
import { verifyJWTToken } from "../../../../../lib/auth";
import { blockchainAPI } from "../../../../utils/blockchain";
import { prisma } from "../../../../../lib/database";

export async function POST(request: NextRequest) {
  try {
    console.log("Send transaction API called");

    // Verify authentication
    const authHeader = request.headers.get("authorization");
    console.log("Auth header present:", !!authHeader);

    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      console.log("Missing or invalid auth header");
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);
    console.log("Token extracted, length:", token.length);

    const decoded = verifyJWTToken(token);
    console.log("Token verification result:", !!decoded);

    if (!decoded) {
      console.log("Token verification failed");
      return NextResponse.json({ error: "Invalid token" }, { status: 401 });
    }

    const { fromAddress, toAddress, amount, description } =
      await request.json();

    // Validate required fields
    if (!fromAddress || !toAddress || !amount) {
      return NextResponse.json(
        { error: "Missing required fields: fromAddress, toAddress, amount" },
        { status: 400 }
      );
    }

    // Validate amount
    if (amount <= 0) {
      return NextResponse.json(
        { error: "Amount must be greater than 0" },
        { status: 400 }
      );
    }

    // Validate addresses
    if (!fromAddress.startsWith("NIL") || !toAddress.startsWith("NIL")) {
      return NextResponse.json(
        { error: "Invalid address format" },
        { status: 400 }
      );
    }

    // Check if user owns the fromAddress
    console.log("Looking for wallet:", {
      address: fromAddress,
      userId: decoded.userId,
    });

    const userWallet = await prisma.userWallet.findFirst({
      where: {
        address: fromAddress,
        userId: decoded.userId,
      },
    });

    console.log("Wallet found:", !!userWallet);
    if (userWallet) {
      console.log(
        "Wallet balance:",
        userWallet.balance,
        "Requested amount:",
        amount
      );
    }

    if (!userWallet) {
      console.log("Wallet not found or not owned by user");
      return NextResponse.json(
        { error: "Wallet not found or not owned by user" },
        { status: 404 }
      );
    }

    // Check balance
    if (userWallet.balance < amount) {
      return NextResponse.json(
        { error: "Insufficient balance" },
        { status: 400 }
      );
    }

    console.log(
      `Sending transaction: ${amount} NIL from ${fromAddress} to ${toAddress}`
    );

    // Submit transaction to blockchain
    let blockchainTransaction;
    try {
      blockchainTransaction = await blockchainAPI.submitTransaction({
        from: fromAddress,
        to: toAddress,
        amount: amount,
      });
      console.log(
        "Blockchain transaction submitted successfully:",
        blockchainTransaction
      );
    } catch (blockchainError) {
      console.error(
        "Blockchain transaction submission failed:",
        blockchainError
      );
      throw new Error(
        `Blockchain error: ${
          blockchainError instanceof Error
            ? blockchainError.message
            : "Unknown error"
        }`
      );
    }

    // Create transaction record in database
    let dbTransaction;
    try {
      dbTransaction = await prisma.userTransaction.create({
        data: {
          hash: blockchainTransaction.hash,
          from: fromAddress,
          to: toAddress,
          amount: amount,
          type: "transfer",
          status: "pending",
          timestamp: new Date(blockchainTransaction.timestamp * 1000),
          userId: decoded.userId,
          // description: description || null, // Commented out until database schema is updated
          blockNumber: null, // Will be updated when confirmed
          gasUsed: null,
          fee: null,
        },
      });
      console.log("Database transaction record created:", dbTransaction.id);
    } catch (dbError) {
      console.error("Failed to create database transaction record:", dbError);
      throw new Error("Failed to save transaction to database");
    }

    // Update wallet balance in database
    try {
      await prisma.userWallet.update({
        where: { id: userWallet.id },
        data: {
          balance: userWallet.balance - amount,
          lastActivity: new Date(),
        },
      });
      console.log("Wallet balance updated successfully");
    } catch (balanceError) {
      console.error("Failed to update wallet balance:", balanceError);
      // Don't throw here as the transaction was already submitted to blockchain
      // Just log the error for debugging
    }

    // Get updated transaction with user info
    const updatedTransaction = await prisma.userTransaction.findUnique({
      where: { id: dbTransaction.id },
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

    return NextResponse.json({
      success: true,
      message: "Transaction submitted successfully",
      transaction: {
        id: updatedTransaction?.id,
        hash: blockchainTransaction.hash,
        from: fromAddress,
        to: toAddress,
        amount: amount,
        type: "transfer",
        status: "pending",
        timestamp: updatedTransaction?.timestamp,
        description: description,
        user: updatedTransaction?.user,
      },
    });
  } catch (error) {
    console.error("Send transaction error:", error);

    // Handle specific blockchain errors
    if (error instanceof Error) {
      if (error.message.includes("insufficient balance")) {
        return NextResponse.json(
          { error: "Insufficient balance in blockchain" },
          { status: 400 }
        );
      }
      if (error.message.includes("invalid address")) {
        return NextResponse.json(
          { error: "Invalid recipient address" },
          { status: 400 }
        );
      }
    }

    return NextResponse.json(
      { error: "Failed to send transaction. Please try again." },
      { status: 500 }
    );
  }
}
