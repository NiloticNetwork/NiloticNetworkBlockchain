// Staking Contract Interface
export interface StakingContract {
  // Core staking functions
  stake(amount: bigint): Promise<void>;
  unstake(amount: bigint): Promise<void>;
  claimRewards(): Promise<void>;

  // View functions
  getStakedAmount(user: string): Promise<bigint>;
  getRewards(user: string): Promise<bigint>;
  getTotalStaked(): Promise<bigint>;
  getAPY(): Promise<number>;

  // Pool functions
  joinPool(poolId: number, amount: bigint): Promise<void>;
  leavePool(poolId: number, amount: bigint): Promise<void>;

  // Governance functions
  vote(proposalId: number, support: boolean): Promise<void>;

  // Events
  on(event: "Staked", listener: (user: string, amount: bigint) => void): void;
  on(event: "Unstaked", listener: (user: string, amount: bigint) => void): void;
  on(
    event: "RewardsClaimed",
    listener: (user: string, amount: bigint) => void
  ): void;
}

// Staking Pool Interface
export interface StakingPool {
  id: number;
  name: string;
  description: string;
  totalStaked: bigint;
  apy: number;
  minStake: bigint;
  maxStake: bigint;
  lockPeriod: number;
  isActive: boolean;
  poolType: "community" | "charity" | "governance";
}

// User Staking Info
export interface UserStakingInfo {
  totalStaked: bigint;
  totalRewards: bigint;
  availableRewards: bigint;
  stakingLevel: "Bronze" | "Silver" | "Gold" | "Platinum" | "Diamond";
  stakingDuration: number;
  lastClaimTime: number;
  poolMemberships: number[];
}

// Reward Calculation
export interface RewardCalculation {
  currentAPY: number;
  projectedRewards: bigint;
  timeToNextReward: number;
  totalEarned: bigint;
}

// Governance Proposal
export interface GovernanceProposal {
  id: number;
  title: string;
  description: string;
  creator: string;
  forVotes: bigint;
  againstVotes: bigint;
  startTime: number;
  endTime: number;
  executed: boolean;
  canceled: boolean;
}

// Charity Pool Info
export interface CharityPool {
  id: number;
  name: string;
  description: string;
  beneficiary: string;
  totalDonated: bigint;
  totalStaked: bigint;
  apy: number;
  donationPercentage: number;
}

// Challenge/Competition
export interface Challenge {
  id: number;
  name: string;
  description: string;
  startTime: number;
  endTime: number;
  rewardPool: bigint;
  participants: number;
  requirements: string[];
  isActive: boolean;
}

// Leaderboard Entry
export interface LeaderboardEntry {
  rank: number;
  address: string;
  name: string;
  totalStaked: bigint;
  totalRewards: bigint;
  level: string;
  badge: string;
}

// Transaction Status
export enum TransactionStatus {
  PENDING = "pending",
  SUCCESS = "success",
  FAILED = "failed",
  CANCELLED = "cancelled",
}

// Error Types
export enum StakingError {
  INSUFFICIENT_BALANCE = "Insufficient balance",
  INSUFFICIENT_STAKED = "Insufficient staked amount",
  LOCK_PERIOD_NOT_MET = "Lock period not met",
  POOL_FULL = "Pool is full",
  POOL_INACTIVE = "Pool is inactive",
  INVALID_AMOUNT = "Invalid amount",
  TRANSACTION_FAILED = "Transaction failed",
  NETWORK_ERROR = "Network error",
  USER_REJECTED = "User rejected transaction",
}

// Configuration
export interface StakingConfig {
  minStakeAmount: bigint;
  maxStakeAmount: bigint;
  defaultAPY: number;
  lockPeriod: number;
  rewardDistributionInterval: number;
  governanceThreshold: bigint;
  charityDonationPercentage: number;
  referralRewardPercentage: number;
}
