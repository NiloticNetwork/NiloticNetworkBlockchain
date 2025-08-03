import { NextRequest, NextResponse } from "next/server";
import { getUserWallets } from "../../../../lib/database";
import { verifyJWTToken } from "../../../../lib/auth";

export async function GET(request: NextRequest) {
  try {
    // Get the authorization header
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json(
        { error: "Authorization header required" },
        { status: 401 }
      );
    }

    const token = authHeader.substring(7);
    const decoded = verifyJWTToken(token);

    if (!decoded || !decoded.userId) {
      return NextResponse.json(
        { error: "Invalid or expired token" },
        { status: 401 }
      );
    }

    // Get user wallets from database
    const wallets = await getUserWallets(decoded.userId);

    return NextResponse.json({
      success: true,
      wallets: wallets.map((wallet) => ({
        id: wallet.id,
        address: wallet.address,
        name: wallet.name,
        type: wallet.type,
        isPrimary: wallet.isPrimary,
        balance: wallet.balance,
        staked: wallet.staked,
        rewards: wallet.rewards,
        lastActivity:
          wallet.lastActivity?.toISOString() || wallet.updatedAt.toISOString(),
        createdAt: wallet.createdAt.toISOString(),
        updatedAt: wallet.updatedAt.toISOString(),
      })),
    });
  } catch (error) {
    console.error("Error fetching user wallets:", error);
    return NextResponse.json(
      { error: "Failed to fetch wallets" },
      { status: 500 }
    );
  }
}
