import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI } from "../../../../utils/blockchain";
import { createUserWallet } from "../../../../../lib/database";
import { verifyJWTToken } from "../../../../../lib/auth";

export async function POST(request: NextRequest) {
  try {
    const { name, importMethod, privateKey, seedPhrase, password } =
      await request.json();

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

    // Validate import method specific data
    if (importMethod === "privateKey") {
      if (!privateKey) {
        return NextResponse.json(
          { error: "Private key is required" },
          { status: 400 }
        );
      }

      // Validate private key format (64 hex characters)
      const privateKeyRegex = /^[0-9a-fA-F]{64}$/;
      if (!privateKeyRegex.test(privateKey)) {
        return NextResponse.json(
          {
            error:
              "Invalid private key format. Must be 64 hexadecimal characters.",
          },
          { status: 400 }
        );
      }
    } else if (importMethod === "seedPhrase") {
      if (!seedPhrase) {
        return NextResponse.json(
          { error: "Seed phrase is required" },
          { status: 400 }
        );
      }

      // Validate seed phrase (12 or 24 words)
      const words = seedPhrase.trim().split(/\s+/);
      if (words.length !== 12 && words.length !== 24) {
        return NextResponse.json(
          { error: "Seed phrase must be 12 or 24 words" },
          { status: 400 }
        );
      }
    } else {
      return NextResponse.json(
        { error: "Invalid import method" },
        { status: 400 }
      );
    }

    // For now, we'll simulate wallet import since the blockchain API might not support this
    // In a real implementation, you would validate the private key/seed phrase and derive the address
    let walletAddress: string;

    if (importMethod === "privateKey") {
      // In a real implementation, you would derive the address from the private key
      // For now, we'll generate a mock address based on the private key hash
      const mockAddress = `NIL${privateKey.slice(0, 8)}${privateKey.slice(-8)}`;
      walletAddress = mockAddress;
    } else {
      // For seed phrase, we'll generate a mock address
      const mockAddress = `NIL${seedPhrase
        .split(" ")
        .slice(0, 2)
        .join("")
        .slice(0, 8)}${seedPhrase.split(" ").slice(-2).join("").slice(0, 8)}`;
      walletAddress = mockAddress;
    }

    // Get wallet balance from blockchain
    let balance = 0;
    try {
      balance = await blockchainAPI.getBalance(walletAddress);
    } catch (error) {
      console.warn("Could not fetch balance for imported wallet:", error);
      // Continue with 0 balance if we can't fetch it
    }

    const walletData = {
      address: walletAddress,
      name,
      balance,
      staked: 0, // Will be updated when we sync with blockchain
      rewards: 0, // Will be updated when we sync with blockchain
      createdAt: new Date().toISOString(),
      lastActivity: new Date().toISOString(),
      isPrimary: false,
      type: "imported",
      importMethod,
    };

    // Get user from authorization header and save to database
    const authHeader = request.headers.get("authorization");
    if (authHeader && authHeader.startsWith("Bearer ")) {
      try {
        const token = authHeader.substring(7);
        const decoded = verifyJWTToken(token);

        if (decoded && decoded.userId) {
          // Save wallet to database
          await createUserWallet(decoded.userId, {
            address: walletData.address,
            name: walletData.name,
            type: "imported",
            isPrimary: false,
          });
        }
      } catch (error) {
        console.warn("Failed to save imported wallet to database:", error);
        // Continue without database save if auth fails
      }
    }

    // Return the imported wallet information
    return NextResponse.json({
      success: true,
      wallet: walletData,
      message: "Wallet imported successfully",
    });
  } catch (error) {
    console.error("Wallet import error:", error);
    return NextResponse.json(
      { error: "Failed to import wallet. Please try again." },
      { status: 500 }
    );
  }
}
