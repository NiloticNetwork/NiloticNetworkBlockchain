"use client";

import { useState, useEffect, useCallback } from "react";
import {
  useAccount,
  useContractRead,
  useContractWrite,
  useWaitForTransaction,
} from "wagmi";
import { parseEther, formatEther } from "viem";
import toast from "react-hot-toast";
import {
  StakingContract,
  UserStakingInfo,
  RewardCalculation,
  TransactionStatus,
  StakingError,
} from "../contracts/StakingContract";

// Mock contract address - replace with actual deployed contract
const STAKING_CONTRACT_ADDRESS = "0x1234567890123456789012345678901234567890";

// Mock ABI - replace with actual contract ABI
const STAKING_ABI = [
  {
    inputs: [
      {
        internalType: "uint256",
        name: "amount",
        type: "uint256",
      },
    ],
    name: "stake",
    outputs: [],
    stateMutability: "nonpayable",
    type: "function",
  },
  {
    inputs: [
      {
        internalType: "uint256",
        name: "amount",
        type: "uint256",
      },
    ],
    name: "unstake",
    outputs: [],
    stateMutability: "nonpayable",
    type: "function",
  },
  {
    inputs: [],
    name: "claimRewards",
    outputs: [],
    stateMutability: "nonpayable",
    type: "function",
  },
  {
    inputs: [
      {
        internalType: "address",
        name: "user",
        type: "address",
      },
    ],
    name: "getStakedAmount",
    outputs: [
      {
        internalType: "uint256",
        name: "",
        type: "uint256",
      },
    ],
    stateMutability: "view",
    type: "function",
  },
  {
    inputs: [
      {
        internalType: "address",
        name: "user",
        type: "address",
      },
    ],
    name: "getRewards",
    outputs: [
      {
        internalType: "uint256",
        name: "",
        type: "uint256",
      },
    ],
    stateMutability: "view",
    type: "function",
  },
  {
    inputs: [],
    name: "getTotalStaked",
    outputs: [
      {
        internalType: "uint256",
        name: "",
        type: "uint256",
      },
    ],
    stateMutability: "view",
    type: "function",
  },
  {
    inputs: [],
    name: "getAPY",
    outputs: [
      {
        internalType: "uint256",
        name: "",
        type: "uint256",
      },
    ],
    stateMutability: "view",
    type: "function",
  },
];

export function useStaking() {
  const { address, isConnected } = useAccount();
  const [userInfo, setUserInfo] = useState<UserStakingInfo | null>(null);
  const [rewardCalculation, setRewardCalculation] =
    useState<RewardCalculation | null>(null);
  const [transactionStatus, setTransactionStatus] =
    useState<TransactionStatus | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  // Contract reads
  const { data: stakedAmount, refetch: refetchStakedAmount } = useContractRead({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "getStakedAmount",
    args: [address as `0x${string}`],
    enabled: !!address,
  });

  const { data: rewards, refetch: refetchRewards } = useContractRead({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "getRewards",
    args: [address as `0x${string}`],
    enabled: !!address,
  });

  const { data: totalStaked, refetch: refetchTotalStaked } = useContractRead({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "getTotalStaked",
  });

  const { data: apy, refetch: refetchAPY } = useContractRead({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "getAPY",
  });

  // Contract writes
  const { write: stake, data: stakeData } = useContractWrite({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "stake",
  });

  const { write: unstake, data: unstakeData } = useContractWrite({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "unstake",
  });

  const { write: claimRewards, data: claimData } = useContractWrite({
    address: STAKING_CONTRACT_ADDRESS as `0x${string}`,
    abi: STAKING_ABI,
    functionName: "claimRewards",
  });

  // Wait for transactions
  const { isLoading: isStakingLoading, isSuccess: isStakingSuccess } =
    useWaitForTransaction({
      hash: stakeData?.hash,
    });

  const { isLoading: isUnstakingLoading, isSuccess: isUnstakingSuccess } =
    useWaitForTransaction({
      hash: unstakeData?.hash,
    });

  const { isLoading: isClaimingLoading, isSuccess: isClaimingSuccess } =
    useWaitForTransaction({
      hash: claimData?.hash,
    });

  // Update user info when data changes
  useEffect(() => {
    if (stakedAmount !== undefined && rewards !== undefined) {
      const staked = stakedAmount ? parseFloat(formatEther(stakedAmount)) : 0;
      const reward = rewards ? parseFloat(formatEther(rewards)) : 0;

      // Calculate staking level based on amount
      let level: UserStakingInfo["stakingLevel"] = "Bronze";
      if (staked >= 10000) level = "Diamond";
      else if (staked >= 5000) level = "Platinum";
      else if (staked >= 1000) level = "Gold";
      else if (staked >= 100) level = "Silver";

      setUserInfo({
        totalStaked: stakedAmount || BigInt(0),
        totalRewards: rewards || BigInt(0),
        availableRewards: rewards || BigInt(0),
        stakingLevel: level,
        stakingDuration: 0, // Calculate based on first stake time
        lastClaimTime: Date.now(),
        poolMemberships: [],
      });
    }
  }, [stakedAmount, rewards]);

  // Update reward calculation
  useEffect(() => {
    if (apy && stakedAmount) {
      const apyValue = apy ? parseFloat(formatEther(apy)) : 12.5;
      const staked = stakedAmount ? parseFloat(formatEther(stakedAmount)) : 0;
      const projectedRewards = (staked * apyValue) / 100;

      setRewardCalculation({
        currentAPY: apyValue,
        projectedRewards: parseEther(projectedRewards.toString()),
        timeToNextReward: 7200, // 2 hours in seconds
        totalEarned: rewards || BigInt(0),
      });
    }
  }, [apy, stakedAmount, rewards]);

  // Handle transaction success/failure
  useEffect(() => {
    if (isStakingSuccess) {
      toast.success("Successfully staked tokens!");
      refetchStakedAmount();
      refetchTotalStaked();
      setTransactionStatus(TransactionStatus.SUCCESS);
    }
  }, [isStakingSuccess, refetchStakedAmount, refetchTotalStaked]);

  useEffect(() => {
    if (isUnstakingSuccess) {
      toast.success("Successfully unstaked tokens!");
      refetchStakedAmount();
      refetchTotalStaked();
      setTransactionStatus(TransactionStatus.SUCCESS);
    }
  }, [isUnstakingSuccess, refetchStakedAmount, refetchTotalStaked]);

  useEffect(() => {
    if (isClaimingSuccess) {
      toast.success("Successfully claimed rewards!");
      refetchRewards();
      setTransactionStatus(TransactionStatus.SUCCESS);
    }
  }, [isClaimingSuccess, refetchRewards]);

  // Staking functions
  const handleStake = useCallback(
    async (amount: string) => {
      if (!isConnected || !address) {
        toast.error("Please connect your wallet first");
        return;
      }

      try {
        setIsLoading(true);
        setTransactionStatus(TransactionStatus.PENDING);

        const amountWei = parseEther(amount);
        stake({ args: [amountWei] });

        toast.success("Staking transaction submitted...");
      } catch (error) {
        console.error("Staking error:", error);
        toast.error("Failed to stake tokens");
        setTransactionStatus(TransactionStatus.FAILED);
      } finally {
        setIsLoading(false);
      }
    },
    [isConnected, address, stake]
  );

  const handleUnstake = useCallback(
    async (amount: string) => {
      if (!isConnected || !address) {
        toast.error("Please connect your wallet first");
        return;
      }

      try {
        setIsLoading(true);
        setTransactionStatus(TransactionStatus.PENDING);

        const amountWei = parseEther(amount);
        unstake({ args: [amountWei] });

        toast.success("Unstaking transaction submitted...");
      } catch (error) {
        console.error("Unstaking error:", error);
        toast.error("Failed to unstake tokens");
        setTransactionStatus(TransactionStatus.FAILED);
      } finally {
        setIsLoading(false);
      }
    },
    [isConnected, address, unstake]
  );

  const handleClaimRewards = useCallback(async () => {
    if (!isConnected || !address) {
      toast.error("Please connect your wallet first");
      return;
    }

    try {
      setIsLoading(true);
      setTransactionStatus(TransactionStatus.PENDING);

      claimRewards();

      toast.success("Claiming rewards...");
    } catch (error) {
      console.error("Claim rewards error:", error);
      toast.error("Failed to claim rewards");
      setTransactionStatus(TransactionStatus.FAILED);
    } finally {
      setIsLoading(false);
    }
  }, [isConnected, address, claimRewards]);

  // Refresh data
  const refreshData = useCallback(() => {
    refetchStakedAmount();
    refetchRewards();
    refetchTotalStaked();
    refetchAPY();
  }, [refetchStakedAmount, refetchRewards, refetchTotalStaked, refetchAPY]);

  return {
    // State
    userInfo,
    rewardCalculation,
    transactionStatus,
    isLoading:
      isLoading || isStakingLoading || isUnstakingLoading || isClaimingLoading,

    // Data
    stakedAmount: stakedAmount ? formatEther(stakedAmount) : "0",
    rewards: rewards ? formatEther(rewards) : "0",
    totalStaked: totalStaked ? formatEther(totalStaked) : "0",
    apy: apy ? parseFloat(formatEther(apy)) : 12.5,

    // Functions
    handleStake,
    handleUnstake,
    handleClaimRewards,
    refreshData,
  };
}
