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

// Hash password with salt
function hashPassword(password: string): string {
  const salt = crypto.randomBytes(16).toString("hex");
  const hash = crypto
    .pbkdf2Sync(password, salt, 1000, 64, "sha512")
    .toString("hex");
  return `${salt}:${hash}`;
}

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
    const { email, password, walletAddress } = body;

    // Validate input
    if (!email || !password) {
      return NextResponse.json(
        { error: "Email and password are required" },
        { status: 400 }
      );
    }

    // Validate email format
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!emailRegex.test(email)) {
      return NextResponse.json(
        { error: "Invalid email format" },
        { status: 400 }
      );
    }

    // Validate password strength
    if (password.length < 8) {
      return NextResponse.json(
        { error: "Password must be at least 8 characters long" },
        { status: 400 }
      );
    }

    // Check if user already exists
    if (users.has(email)) {
      return NextResponse.json(
        { error: "User already exists with this email" },
        { status: 409 }
      );
    }

    // Create new user
    const userId = crypto.randomUUID();
    const passwordHash = hashPassword(password);

    const user: User = {
      id: userId,
      email,
      passwordHash,
      walletAddress,
      createdAt: new Date(),
      updatedAt: new Date(),
    };

    // Store user
    users.set(email, user);

    // Generate JWT token
    const token = generateToken(userId);

    // Return success response
    return NextResponse.json({
      success: true,
      message: "User registered successfully",
      token,
      user: {
        id: user.id,
        email: user.email,
        walletAddress: user.walletAddress,
        createdAt: user.createdAt,
      },
    });
  } catch (error) {
    console.error("Error during signup:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
