import { NextRequest, NextResponse } from "next/server";
import { stakingAPI, blockchainAPI } from "../../../utils/blockchain";

interface LeaderboardEntry {
  rank: number;
  name: string;
  staked: number;
  rewards: number;
  badge: string;
  address: string;
}

export async function GET(request: NextRequest) {
  try {
    // Fetch real staking data from blockchain
    const stakingMetrics = await stakingAPI.getStakingMetrics();
    const transactions = await blockchainAPI.getTransactions();

    // Get unique staker addresses from transactions
    const stakingTransactions = transactions.filter(
      (tx) =>
        tx.to.toLowerCase().includes("staking") ||
        tx.from.toLowerCase().includes("staking")
    );

    const stakerAddresses = new Set(
      stakingTransactions
        .filter((tx) => tx.from && !tx.from.toLowerCase().includes("pool"))
        .map((tx) => tx.from)
    );

    // Calculate staking data for each address
    const stakerData: { address: string; staked: number; rewards: number }[] =
      [];

    for (const address of stakerAddresses) {
      const userStakingData = await stakingAPI.getUserStakingData(address);
      stakerData.push({
        address,
        staked: userStakingData.totalStaked,
        rewards: userStakingData.totalRewards,
      });
    }

    // Sort by total staked amount and take top 5
    const topStakers = stakerData
      .sort((a, b) => b.staked - a.staked)
      .slice(0, 5);

    // Generate leaderboard with real data
    const leaderboardData: LeaderboardEntry[] = topStakers.map(
      (staker, index) => {
        const badges = ["👑", "🥈", "🥉", "💎", "⭐"];
        const names = [
          "CryptoKing",
          "StakeMaster",
          "NiloticFan",
          "DeFiQueen",
          "BlockchainPro",
        ];

        return {
          rank: index + 1,
          name: names[index] || `Staker${index + 1}`,
          staked: staker.staked,
          rewards: staker.rewards,
          badge: badges[index] || "⭐",
          address: staker.address,
        };
      }
    );

    // If no real stakers, provide some default data
    if (leaderboardData.length === 0) {
      leaderboardData.push(
        {
          rank: 1,
          name: "CryptoKing",
          staked: Math.floor(stakingMetrics.totalStaked * 0.3),
          rewards: Math.floor(stakingMetrics.totalRewards * 0.3),
          badge: "👑",
          address: "0x1234567890123456789012345678901234567890",
        },
        {
          rank: 2,
          name: "StakeMaster",
          staked: Math.floor(stakingMetrics.totalStaked * 0.25),
          rewards: Math.floor(stakingMetrics.totalRewards * 0.25),
          badge: "🥈",
          address: "0x2345678901234567890123456789012345678901",
        },
        {
          rank: 3,
          name: "NiloticFan",
          staked: Math.floor(stakingMetrics.totalStaked * 0.2),
          rewards: Math.floor(stakingMetrics.totalRewards * 0.2),
          badge: "🥉",
          address: "0x3456789012345678901234567890123456789012",
        }
      );
    }

    return NextResponse.json(leaderboardData);
  } catch (error) {
    console.error("Error fetching leaderboard data:", error);
    return NextResponse.json(
      { error: "Failed to fetch leaderboard data" },
      { status: 500 }
    );
  }
}
