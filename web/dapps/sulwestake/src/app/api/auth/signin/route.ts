import { NextRequest, NextResponse } from "next/server";
import crypto from "crypto";
import jwt from "jsonwebtoken";

interface User {
  id: string;
  email: string;
  passwordHash: string;
  walletAddress?: string;
  createdAt: Date;
  updatedAt: Date;
}

// In-memory storage (replace with database in production)
const users: Map<string, User> = new Map();

// Verify password
function verifyPassword(password: string, hashedPassword: string): boolean {
  const [salt, hash] = hashedPassword.split(":");
  const verifyHash = crypto
    .pbkdf2Sync(password, salt, 1000, 64, "sha512")
    .toString("hex");
  return hash === verifyHash;
}

// Generate JWT token
function generateToken(userId: string): string {
  const secret = process.env.JWT_SECRET || "your-secret-key";
  return jwt.sign({ userId }, secret, { expiresIn: "7d" });
}

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { email, password } = body;

    // Validate input
    if (!email || !password) {
      return NextResponse.json(
        { error: "Email and password are required" },
        { status: 400 }
      );
    }

    // Find user by email
    const user = users.get(email);
    if (!user) {
      return NextResponse.json(
        { error: "Invalid email or password" },
        { status: 401 }
      );
    }

    // Verify password
    if (!verifyPassword(password, user.passwordHash)) {
      return NextResponse.json(
        { error: "Invalid email or password" },
        { status: 401 }
      );
    }

    // Generate JWT token
    const token = generateToken(user.id);

    // Update last login
    user.updatedAt = new Date();

    // Return success response
    return NextResponse.json({
      success: true,
      message: "Signed in successfully",
      token,
      user: {
        id: user.id,
        email: user.email,
        walletAddress: user.walletAddress,
        createdAt: user.createdAt,
        updatedAt: user.updatedAt,
      },
    });
  } catch (error) {
    console.error("Error during signin:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
