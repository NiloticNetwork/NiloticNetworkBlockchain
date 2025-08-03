export interface User {
  id: string;
  email: string;
  username: string;
  firstName: string;
  lastName: string;
  avatar?: string;
  createdAt: string;
  lastLogin: string;
  isActive: boolean;
  role: "user" | "admin" | "moderator";
  preferences: UserPreferences;
  walletAddresses: string[];
  primaryWalletAddress?: string;
}

export interface UserPreferences {
  theme: "light" | "dark" | "auto";
  notifications: {
    email: boolean;
    push: boolean;
    sms: boolean;
  };
  privacy: {
    showBalance: boolean;
    showTransactions: boolean;
    allowAnalytics: boolean;
  };
  language: string;
  timezone: string;
}

export interface UserProfile {
  user: User;
  stats: UserStats;
  wallets: WalletInfo[];
  recentTransactions: Transaction[];
  stakingData: StakingData;
}

export interface UserStats {
  totalBalance: number;
  totalStaked: number;
  totalRewards: number;
  totalTransactions: number;
  accountAge: number; // in days
  lastActivity: string;
}

export interface StakingData {
  totalStaked: number;
  totalRewards: number;
  stakingStartDate: string;
  lastRewardDate: string;
  apy: number;
  stakingLevel: "bronze" | "silver" | "gold" | "platinum";
  nextRewardEstimate: number;
}

export interface AuthState {
  isAuthenticated: boolean;
  user: User | null;
  token: string | null;
  loading: boolean;
  error: string | null;
}

export interface LoginCredentials {
  email: string;
  password: string;
  rememberMe?: boolean;
}

export interface RegisterData {
  email: string;
  username: string;
  password: string;
  confirmPassword: string;
  firstName: string;
  lastName: string;
  acceptTerms: boolean;
}

export interface PasswordResetRequest {
  email: string;
}

export interface PasswordReset {
  token: string;
  newPassword: string;
  confirmPassword: string;
}

export interface ProfileUpdateData {
  firstName?: string;
  lastName?: string;
  username?: string;
  avatar?: string;
  preferences?: Partial<UserPreferences>;
}

export interface WalletInfo {
  address: string;
  name: string;
  balance: number;
  staked: number;
  rewards: number;
  createdAt: string;
  lastActivity: string;
  isPrimary: boolean;
  type: "nilotic" | "metamask" | "imported";
}

export interface Transaction {
  hash: string;
  from: string;
  to: string;
  amount: number;
  type: "transfer" | "stake" | "unstake" | "reward" | "mining";
  status: "pending" | "confirmed" | "failed";
  timestamp: string;
  blockNumber?: number;
  gasUsed?: number;
  fee?: number;
}
