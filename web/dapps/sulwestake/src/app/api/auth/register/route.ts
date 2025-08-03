import { NextRequest, NextResponse } from "next/server";
import {
  createUserWithDefaults,
  findUserByEmail,
  findUserByUsername,
  createUserSession,
} from "../../../../lib/database";
import {
  hashPassword,
  generateJWTToken,
  generateSessionToken,
  calculateTokenExpiry,
  sanitizeUserData,
  validateEmail,
  validatePassword,
  validateUsername,
} from "../../../../lib/auth";
import { BlockchainService } from "../../../../lib/blockchain-service";

export async function POST(request: NextRequest) {
  try {
    const { email, username, password, firstName, lastName } =
      await request.json();

    // Validate input
    if (!email || !username || !password || !firstName || !lastName) {
      return NextResponse.json(
        { error: "All fields are required" },
        { status: 400 }
      );
    }

    // Validate email format
    if (!validateEmail(email)) {
      return NextResponse.json(
        { error: "Invalid email format" },
        { status: 400 }
      );
    }

    // Validate password strength
    const passwordValidation = validatePassword(password);
    if (!passwordValidation.isValid) {
      return NextResponse.json(
        {
          error: "Password requirements not met",
          details: passwordValidation.errors,
        },
        { status: 400 }
      );
    }

    // Validate username
    const usernameValidation = validateUsername(username);
    if (!usernameValidation.isValid) {
      return NextResponse.json(
        {
          error: "Username requirements not met",
          details: usernameValidation.errors,
        },
        { status: 400 }
      );
    }

    // Check if email already exists
    const existingUserByEmail = await findUserByEmail(email);
    if (existingUserByEmail) {
      return NextResponse.json(
        { error: "Email already registered" },
        { status: 409 }
      );
    }

    // Check if username already exists
    const existingUserByUsername = await findUserByUsername(username);
    if (existingUserByUsername) {
      return NextResponse.json(
        { error: "Username already taken" },
        { status: 409 }
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

    // Hash password
    const passwordHash = await hashPassword(password);

    // Create user avatar
    const avatar = `https://ui-avatars.com/api/?name=${firstName}+${lastName}&background=6366f1&color=fff`;

    // Create user in database with default preferences and staking data
    const newUser = await createUserWithDefaults({
      email,
      username,
      firstName,
      lastName,
      passwordHash,
      avatar,
    });

    // Generate JWT token
    const jwtToken = generateJWTToken({
      userId: newUser.id,
      email: newUser.email,
      role: newUser.role,
    });

    // Create session in database
    const sessionToken = generateSessionToken();
    const expiresAt = calculateTokenExpiry();
    await createUserSession(newUser.id, sessionToken, expiresAt);

    // Sync user data from blockchain
    try {
      await BlockchainService.syncUserProfile(newUser.id);
    } catch (error) {
      console.error("Error syncing user profile:", error);
      // Don't fail registration if blockchain sync fails
    }

    // Sanitize user data (remove password hash)
    const sanitizedUser = sanitizeUserData(newUser);

    return NextResponse.json({
      user: sanitizedUser,
      token: jwtToken,
      sessionToken,
      message: "Registration successful",
      blockchainStatus,
    });
  } catch (error) {
    console.error("Registration error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
