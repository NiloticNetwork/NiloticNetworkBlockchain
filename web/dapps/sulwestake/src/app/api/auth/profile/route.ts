import { NextRequest, NextResponse } from "next/server";
import {
  findUserById,
  updateUserPreferences,
  findValidSession,
} from "../../../../lib/database";
import { verifyJWTToken } from "../../../../lib/auth";
import { BlockchainService } from "../../../../lib/blockchain-service";

export async function GET(request: NextRequest) {
  try {
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);

    // Verify JWT token
    const decoded = verifyJWTToken(token);
    if (!decoded) {
      return NextResponse.json({ error: "Invalid token" }, { status: 401 });
    }

    // Get user from database
    const user = await findUserById(decoded.userId);
    if (!user) {
      return NextResponse.json({ error: "User not found" }, { status: 404 });
    }

    // Sync user data from blockchain
    try {
      await BlockchainService.syncUserProfile(user.id);
    } catch (error) {
      console.error("Error syncing user profile:", error);
      // Continue with cached data if blockchain sync fails
    }

    // Get updated user data
    const updatedUser = await findUserById(user.id);
    if (!updatedUser) {
      return NextResponse.json({ error: "User not found" }, { status: 404 });
    }

    // Calculate user statistics
    const totalBalance = updatedUser.wallets.reduce(
      (sum, wallet) => sum + wallet.balance,
      0
    );
    const totalStaked = updatedUser.stakingData?.totalStaked || 0;
    const totalRewards = updatedUser.stakingData?.totalRewards || 0;
    const totalTransactions = updatedUser.transactions.length;
    const accountAge = Math.floor(
      (Date.now() - new Date(updatedUser.createdAt).getTime()) /
        (1000 * 60 * 60 * 24)
    );
    const lastActivity =
      updatedUser.transactions.length > 0
        ? updatedUser.transactions[0].timestamp
        : updatedUser.lastLogin || updatedUser.createdAt;

    // Build user profile response
    const userProfile = {
      user: {
        id: updatedUser.id,
        email: updatedUser.email,
        username: updatedUser.username,
        firstName: updatedUser.firstName,
        lastName: updatedUser.lastName,
        avatar: updatedUser.avatar,
        createdAt: updatedUser.createdAt.toISOString(),
        lastLogin: updatedUser.lastLogin?.toISOString(),
        isActive: updatedUser.isActive,
        role: updatedUser.role,
        preferences: updatedUser.preferences
          ? {
              theme: updatedUser.preferences.theme,
              language: updatedUser.preferences.language,
              timezone: updatedUser.preferences.timezone,
              notifications: {
                email: updatedUser.preferences.emailNotifications,
                push: updatedUser.preferences.pushNotifications,
                sms: updatedUser.preferences.smsNotifications,
              },
              privacy: {
                showBalance: updatedUser.preferences.showBalance,
                showTransactions: updatedUser.preferences.showTransactions,
                allowAnalytics: updatedUser.preferences.allowAnalytics,
              },
            }
          : null,
        walletAddresses: updatedUser.wallets.map((w) => w.address),
        primaryWalletAddress: updatedUser.wallets.find((w) => w.isPrimary)
          ?.address,
      },
      stats: {
        totalBalance,
        totalStaked,
        totalRewards,
        totalTransactions,
        accountAge,
        lastActivity: lastActivity.toISOString(),
      },
      wallets: updatedUser.wallets.map((wallet) => ({
        address: wallet.address,
        name: wallet.name,
        balance: wallet.balance,
        staked: wallet.staked,
        rewards: wallet.rewards,
        createdAt: wallet.createdAt.toISOString(),
        lastActivity: wallet.lastActivity.toISOString(),
        isPrimary: wallet.isPrimary,
        type: wallet.type,
      })),
      recentTransactions: updatedUser.transactions.map((tx) => ({
        hash: tx.hash,
        from: tx.from,
        to: tx.to,
        amount: tx.amount,
        type: tx.type,
        status: tx.status,
        timestamp: tx.timestamp.toISOString(),
        blockNumber: tx.blockNumber,
        gasUsed: tx.gasUsed,
        fee: tx.fee,
      })),
      stakingData: updatedUser.stakingData
        ? {
            totalStaked: updatedUser.stakingData.totalStaked,
            totalRewards: updatedUser.stakingData.totalRewards,
            stakingStartDate:
              updatedUser.stakingData.stakingStartDate?.toISOString(),
            lastRewardDate:
              updatedUser.stakingData.lastRewardDate?.toISOString(),
            apy: updatedUser.stakingData.apy,
            stakingLevel: updatedUser.stakingData.stakingLevel,
            nextRewardEstimate: updatedUser.stakingData.nextRewardEstimate,
          }
        : null,
    };

    return NextResponse.json(userProfile);
  } catch (error) {
    console.error("Profile GET error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

export async function PUT(request: NextRequest) {
  try {
    const authHeader = request.headers.get("authorization");
    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
    }

    const token = authHeader.substring(7);

    // Verify JWT token
    const decoded = verifyJWTToken(token);
    if (!decoded) {
      return NextResponse.json({ error: "Invalid token" }, { status: 401 });
    }

    const updateData = await request.json();

    // Update user preferences in database
    if (updateData.preferences) {
      await updateUserPreferences(decoded.userId, {
        theme: updateData.preferences.theme,
        language: updateData.preferences.language,
        timezone: updateData.preferences.timezone,
        emailNotifications: updateData.preferences.notifications?.email,
        pushNotifications: updateData.preferences.notifications?.push,
        smsNotifications: updateData.preferences.notifications?.sms,
        showBalance: updateData.preferences.privacy?.showBalance,
        showTransactions: updateData.preferences.privacy?.showTransactions,
        allowAnalytics: updateData.preferences.privacy?.allowAnalytics,
      });
    }

    // Get updated user data
    const updatedUser = await findUserById(decoded.userId);
    if (!updatedUser) {
      return NextResponse.json({ error: "User not found" }, { status: 404 });
    }

    return NextResponse.json({
      user: {
        id: updatedUser.id,
        email: updatedUser.email,
        username: updatedUser.username,
        firstName: updatedUser.firstName,
        lastName: updatedUser.lastName,
        avatar: updatedUser.avatar,
        createdAt: updatedUser.createdAt.toISOString(),
        lastLogin: updatedUser.lastLogin?.toISOString(),
        isActive: updatedUser.isActive,
        role: updatedUser.role,
        preferences: updatedUser.preferences
          ? {
              theme: updatedUser.preferences.theme,
              language: updatedUser.preferences.language,
              timezone: updatedUser.preferences.timezone,
              notifications: {
                email: updatedUser.preferences.emailNotifications,
                push: updatedUser.preferences.pushNotifications,
                sms: updatedUser.preferences.smsNotifications,
              },
              privacy: {
                showBalance: updatedUser.preferences.showBalance,
                showTransactions: updatedUser.preferences.showTransactions,
                allowAnalytics: updatedUser.preferences.allowAnalytics,
              },
            }
          : null,
      },
      message: "Profile updated successfully",
    });
  } catch (error) {
    console.error("Profile PUT error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
