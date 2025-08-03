import { NextRequest, NextResponse } from "next/server";
import {
  findUserByEmail,
  createUserSession,
  updateUserLastLogin,
  findValidSession,
} from "../../../../lib/database";
import {
  comparePassword,
  generateJWTToken,
  generateSessionToken,
  calculateTokenExpiry,
  sanitizeUserData,
} from "../../../../lib/auth";
import { BlockchainService } from "../../../../lib/blockchain-service";

export async function POST(request: NextRequest) {
  try {
    const { email, password } = await request.json();

    // Validate input
    if (!email || !password) {
      return NextResponse.json(
        { error: "Email and password are required" },
        { status: 400 }
      );
    }

    // Find user in database
    const user = await findUserByEmail(email);
    if (!user) {
      return NextResponse.json(
        { error: "Invalid credentials" },
        { status: 401 }
      );
    }

    // Verify password
    const isPasswordValid = await comparePassword(password, user.passwordHash);
    if (!isPasswordValid) {
      return NextResponse.json(
        { error: "Invalid credentials" },
        { status: 401 }
      );
    }

    // Check if user is active
    if (!user.isActive) {
      return NextResponse.json(
        { error: "Account is deactivated" },
        { status: 403 }
      );
    }

    // Verify blockchain connection
    const blockchainStatus = await BlockchainService.getBlockchainStatus();
    if (!blockchainStatus.connected) {
      return NextResponse.json(
        {
          error:
            "Blockchain not available. Please ensure the blockchain server is running.",
        },
        { status: 503 }
      );
    }

    // Generate JWT token
    const jwtToken = generateJWTToken({
      userId: user.id,
      email: user.email,
      role: user.role,
    });

    // Create session in database
    const sessionToken = generateSessionToken();
    const expiresAt = calculateTokenExpiry();
    await createUserSession(user.id, sessionToken, expiresAt);

    // Update last login
    await updateUserLastLogin(user.id);

    // Sync user data from blockchain
    try {
      await BlockchainService.syncUserProfile(user.id);
    } catch (error) {
      console.error("Error syncing user profile:", error);
      // Don't fail login if blockchain sync fails
    }

    // Sanitize user data (remove password hash)
    const sanitizedUser = sanitizeUserData(user);

    return NextResponse.json({
      user: sanitizedUser,
      token: jwtToken,
      sessionToken,
      message: "Login successful",
      blockchainStatus,
    });
  } catch (error) {
    console.error("Login error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
