import { NextRequest, NextResponse } from "next/server";
import { verifyJWTToken } from "../../../../lib/auth";
import { BlockchainSyncService } from "../../../../lib/blockchain-sync-service";

export async function POST(request: NextRequest) {
  try {
    // Verify authentication
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);
    const decoded = verifyJWTToken(token);
    if (!decoded) {
      return NextResponse.json({ error: "Invalid token" }, { status: 401 });
    }

    const { action, userId } = await request.json();

    if (!action || !userId) {
      return NextResponse.json(
        { error: "Action and userId are required" },
        { status: 400 }
      );
    }

    switch (action) {
      case "start":
        await BlockchainSyncService.startUserSync(userId);
        return NextResponse.json({
          message: "Sync started successfully",
          userId,
        });

      case "stop":
        BlockchainSyncService.stopUserSync(userId);
        return NextResponse.json({
          message: "Sync stopped successfully",
          userId,
        });

      case "force":
        await BlockchainSyncService.forceUserSync(userId);
        return NextResponse.json({
          message: "Force sync completed",
          userId,
        });

      default:
        return NextResponse.json(
          { error: "Invalid action. Use start, stop, or force" },
          { status: 400 }
        );
    }
  } catch (error) {
    console.error("Sync API error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

export async function GET(request: NextRequest) {
  try {
    // Verify authentication
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);
    const decoded = verifyJWTToken(token);
    if (!decoded) {
      return NextResponse.json({ error: "Invalid token" }, { status: 401 });
    }

    const { searchParams } = new URL(request.url);
    const userId = searchParams.get("userId");

    if (userId) {
      // Get sync status for specific user
      const status = BlockchainSyncService.getUserSyncStatus(userId);
      return NextResponse.json({ status });
    } else {
      // Get all sync statuses
      const allStatuses = BlockchainSyncService.getAllSyncStatuses();
      const statuses = Object.fromEntries(allStatuses);
      return NextResponse.json({ statuses });
    }
  } catch (error) {
    console.error("Sync status API error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
