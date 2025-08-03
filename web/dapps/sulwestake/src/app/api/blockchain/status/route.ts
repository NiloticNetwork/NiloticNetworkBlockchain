import { NextRequest, NextResponse } from "next/server";
import { blockchainAPI, stakingAPI } from "../../../utils/blockchain";

export async function GET(request: NextRequest) {
  try {
    // Fetch real blockchain status and metrics
    const [status, metrics, stakingMetrics] = await Promise.all([
      blockchainAPI.getStatus(),
      blockchainAPI.getMetrics(),
      stakingAPI.getStakingMetrics(),
    ]);

    const blockchainStatus = {
      // Blockchain status
      status: status.status,
      chainHeight: status.chainHeight,
      totalBlocks: status.totalBlocks,
      totalTransactions: status.totalTransactions,
      difficulty: status.difficulty,
      miningRate: metrics.miningRate,
      lastBlockHash: metrics.lastBlockHash,

      // Staking metrics
      totalStaked: stakingMetrics.totalStaked,
      totalStakers: stakingMetrics.totalStakers,
      averageAPY: stakingMetrics.averageAPY,
      totalRewards: stakingMetrics.totalRewards,

      // Connection status
      connected: true,
      timestamp: new Date().toISOString(),
    };

    return NextResponse.json(blockchainStatus);
  } catch (error) {
    console.error("Error fetching blockchain status:", error);

    // Return fallback status if blockchain is not available
    return NextResponse.json({
      status: "disconnected",
      chainHeight: 0,
      totalBlocks: 0,
      totalTransactions: 0,
      difficulty: 1,
      miningRate: 0,
      lastBlockHash: "0x0",
      totalStaked: 0,
      totalStakers: 0,
      averageAPY: 12.5,
      totalRewards: 0,
      connected: false,
      timestamp: new Date().toISOString(),
      error: "Blockchain connection failed",
    });
  }
}
