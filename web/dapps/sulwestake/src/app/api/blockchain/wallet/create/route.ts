import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../../utils/blockchain";
import { createUserWallet } from "../../../../../lib/database";
import { verifyJWTToken } from "../../../../../lib/auth";

export async function POST(request: NextRequest) {
  try {
    const { name, password } = await request.json();

    // Validate input
    if (!name || !password) {
      return NextResponse.json(
        { error: "Wallet name and password are required" },
        { status: 400 }
      );
    }

    if (name.length < 3) {
      return NextResponse.json(
        { error: "Wallet name must be at least 3 characters long" },
        { status: 400 }
      );
    }

    if (password.length < 6) {
      return NextResponse.json(
        { error: "Password must be at least 6 characters long" },
        { status: 400 }
      );
    }

    // Create wallet on blockchain
    const walletInfo = await blockchainAPI.createWallet(name, password);

    if (!walletInfo || !walletInfo.address) {
      return NextResponse.json(
        { error: "Failed to create wallet on blockchain" },
        { status: 500 }
      );
    }

    // For now, we'll create the wallet without user association
    // In a real implementation, you would get the user ID from the authenticated session
    const walletData = {
      address: walletInfo.address,
      name: walletInfo.name,
      balance: walletInfo.balance || 0,
      staked: walletInfo.staked || 0,
      rewards: walletInfo.rewards || 0,
      createdAt: new Date().toISOString(),
      lastActivity: new Date().toISOString(),
      isPrimary: false, // New wallets are not primary by default
      type: "blockchain",
    };

    // Get user from authorization header
    const authHeader = request.headers.get("authorization");
    if (authHeader && authHeader.startsWith("Bearer ")) {
      try {
        const token = authHeader.substring(7);
        const decoded = verifyJWTToken(token);

        if (decoded && decoded.userId) {
          // Save wallet to database
          await createUserWallet(decoded.userId, {
            address: walletInfo.address,
            name: walletInfo.name,
            type: "blockchain",
            isPrimary: false,
          });
        }
      } catch (error) {
        console.warn("Failed to save wallet to database:", error);
        // Continue without database save if auth fails
      }
    }

    // Return the created wallet information
    return NextResponse.json({
      success: true,
      wallet: {
        ...walletData,
        seedPhrase: walletInfo.seedPhrase, // Include the seed phrase from blockchain
      },
      message: "Wallet created successfully",
    });
  } catch (error) {
    console.error("Wallet creation error:", error);
    return NextResponse.json(
      { error: "Failed to create wallet. Please try again." },
      { status: 500 }
    );
  }
}
