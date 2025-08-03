import { NextRequest, NextResponse } from "next/server";
import { verifyJWTToken } from "../../lib/auth";

export async function GET(request: NextRequest) {
  try {
    console.log("Test auth endpoint called");

    // Check for auth header
    const authHeader = request.headers.get("authorization");
    console.log("Auth header present:", !!authHeader);

    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json(
        {
          error: "No auth header",
          authHeader: authHeader ? "present but invalid" : "missing",
        },
        { status: 401 }
      );
    }

    const token = authHeader.substring(7);
    console.log("Token length:", token.length);
    console.log("Token preview:", token.substring(0, 20) + "...");

    // Verify token
    const decoded = verifyJWTToken(token);
    console.log("Token verification result:", !!decoded);

    if (!decoded) {
      return NextResponse.json(
        {
          error: "Invalid token",
          tokenLength: token.length,
        },
        { status: 401 }
      );
    }

    return NextResponse.json({
      success: true,
      message: "Authentication successful",
      userId: decoded.userId,
      tokenLength: token.length,
    });
  } catch (error) {
    console.error("Test auth error:", error);
    return NextResponse.json(
      {
        success: false,
        error: error instanceof Error ? error.message : "Unknown error",
      },
      { status: 500 }
    );
  }
}
