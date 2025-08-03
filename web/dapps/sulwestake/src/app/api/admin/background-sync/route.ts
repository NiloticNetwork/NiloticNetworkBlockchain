import { NextRequest, NextResponse } from "next/server";
import { verifyJWTToken } from "../../../../lib/auth";
import { BackgroundSyncService } from "../../../../lib/background-sync-service";

export async function POST(request: NextRequest) {
  try {
    // Verify admin authentication
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);
    const decoded = verifyJWTToken(token);
    if (!decoded || decoded.role !== "admin") {
      return NextResponse.json(
        { error: "Admin access required" },
        { status: 403 }
      );
    }

    const { action, config } = await request.json();

    switch (action) {
      case "start":
        await BackgroundSyncService.start();
        return NextResponse.json({
          message: "Background sync service started",
          status: BackgroundSyncService.getStatus(),
        });

      case "stop":
        BackgroundSyncService.stop();
        return NextResponse.json({
          message: "Background sync service stopped",
          status: BackgroundSyncService.getStatus(),
        });

      case "force":
        await BackgroundSyncService.forceSync();
        return NextResponse.json({
          message: "Background sync forced",
          status: BackgroundSyncService.getStatus(),
        });

      case "config":
        if (config) {
          BackgroundSyncService.updateConfig(config);
          return NextResponse.json({
            message: "Configuration updated",
            config: BackgroundSyncService.getConfig(),
          });
        } else {
          return NextResponse.json({
            config: BackgroundSyncService.getConfig(),
          });
        }

      default:
        return NextResponse.json(
          { error: "Invalid action. Use start, stop, force, or config" },
          { status: 400 }
        );
    }
  } catch (error) {
    console.error("Background sync admin API error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

export async function GET(request: NextRequest) {
  try {
    // Verify admin authentication
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);
    const decoded = verifyJWTToken(token);
    if (!decoded || decoded.role !== "admin") {
      return NextResponse.json(
        { error: "Admin access required" },
        { status: 403 }
      );
    }

    const { searchParams } = new URL(request.url);
    const includeStats = searchParams.get("stats") === "true";

    const response: any = {
      status: BackgroundSyncService.getStatus(),
      config: BackgroundSyncService.getConfig(),
    };

    if (includeStats) {
      response.stats = await BackgroundSyncService.getSyncStats();
    }

    return NextResponse.json(response);
  } catch (error) {
    console.error("Background sync status API error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
