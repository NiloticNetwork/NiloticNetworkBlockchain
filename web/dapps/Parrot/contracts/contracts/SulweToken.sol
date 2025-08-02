// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "@openzeppelin/contracts/token/ERC20/extensions/ERC20Burnable.sol";
import "@openzeppelin/contracts/token/ERC20/extensions/ERC20Pausable.sol";
import "@openzeppelin/contracts/access/Ownable.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/utils/math/SafeMath.sol";

/**
 * @title SulweToken
 * @dev Sulwe (SLW) token for the Parrot SocialFi platform
 * Inspired by the Nilotic heritage and "star" concept
 */
contract SulweToken is ERC20, ERC20Burnable, ERC20Pausable, Ownable, ReentrancyGuard {
    using SafeMath for uint256;

    // Staking structures
    struct StakingInfo {
        uint256 stakedAmount;
        uint256 stakingStartTime;
        uint256 lastRewardTime;
        uint256 totalRewardsEarned;
    }

    // Reward structures
    struct RewardRate {
        uint256 rate; // Rewards per second per token
        uint256 lastUpdateTime;
        uint256 totalStaked;
    }

    // Constants
    uint256 public constant REWARD_PRECISION = 1e18;
    uint256 public constant MIN_STAKE_AMOUNT = 100 * 1e18; // 100 SLW
    uint256 public constant MAX_STAKE_AMOUNT = 1000000 * 1e18; // 1M SLW
    uint256 public constant STAKING_LOCK_PERIOD = 7 days;
    uint256 public constant REWARD_DISTRIBUTION_PERIOD = 30 days;

    // State variables
    mapping(address => StakingInfo) public stakingInfo;
    mapping(address => bool) public isStaking;
    RewardRate public rewardRate;
    
    uint256 public totalStaked;
    uint256 public totalRewardsDistributed;
    uint256 public stakingRewardPool;
    
    // Events
    event TokensStaked(address indexed user, uint256 amount, uint256 timestamp);
    event TokensUnstaked(address indexed user, uint256 amount, uint256 rewards, uint256 timestamp);
    event RewardsClaimed(address indexed user, uint256 amount, uint256 timestamp);
    event RewardPoolUpdated(uint256 newPool, uint256 timestamp);
    event StakingPaused(bool paused, uint256 timestamp);

    // Modifiers
    modifier onlyStaking() {
        require(isStaking[msg.sender], "SulweToken: User is not staking");
        _;
    }

    modifier notStaking() {
        require(!isStaking[msg.sender], "SulweToken: User is already staking");
        _;
    }

    /**
     * @dev Constructor - Initializes the Sulwe token
     * @param initialSupply Initial token supply
     * @param initialRewardPool Initial reward pool for staking
     */
    constructor(
        uint256 initialSupply,
        uint256 initialRewardPool
    ) ERC20("Sulwe", "SLW") {
        _mint(msg.sender, initialSupply);
        stakingRewardPool = initialRewardPool;
        rewardRate = RewardRate({
            rate: 0,
            lastUpdateTime: block.timestamp,
            totalStaked: 0
        });
    }

    /**
     * @dev Stake SLW tokens to earn rewards
     * @param amount Amount of tokens to stake
     */
    function stake(uint256 amount) external notStaking nonReentrant {
        require(amount >= MIN_STAKE_AMOUNT, "SulweToken: Amount below minimum");
        require(amount <= MAX_STAKE_AMOUNT, "SulweToken: Amount above maximum");
        require(balanceOf(msg.sender) >= amount, "SulweToken: Insufficient balance");

        _transfer(msg.sender, address(this), amount);
        
        stakingInfo[msg.sender] = StakingInfo({
            stakedAmount: amount,
            stakingStartTime: block.timestamp,
            lastRewardTime: block.timestamp,
            totalRewardsEarned: 0
        });

        isStaking[msg.sender] = true;
        totalStaked = totalStaked.add(amount);
        rewardRate.totalStaked = rewardRate.totalStaked.add(amount);

        _updateRewardRate();

        emit TokensStaked(msg.sender, amount, block.timestamp);
    }

    /**
     * @dev Unstake SLW tokens and claim rewards
     */
    function unstake() external onlyStaking nonReentrant {
        StakingInfo storage userStaking = stakingInfo[msg.sender];
        
        require(
            block.timestamp >= userStaking.stakingStartTime.add(STAKING_LOCK_PERIOD),
            "SulweToken: Staking lock period not met"
        );

        uint256 stakedAmount = userStaking.stakedAmount;
        uint256 rewards = _calculateRewards(msg.sender);

        // Transfer staked tokens back to user
        _transfer(address(this), msg.sender, stakedAmount);
        
        // Mint rewards to user
        if (rewards > 0) {
            _mint(msg.sender, rewards);
            totalRewardsDistributed = totalRewardsDistributed.add(rewards);
        }

        // Update state
        totalStaked = totalStaked.sub(stakedAmount);
        rewardRate.totalStaked = rewardRate.totalStaked.sub(stakedAmount);
        isStaking[msg.sender] = false;
        
        delete stakingInfo[msg.sender];

        _updateRewardRate();

        emit TokensUnstaked(msg.sender, stakedAmount, rewards, block.timestamp);
    }

    /**
     * @dev Claim accumulated rewards without unstaking
     */
    function claimRewards() external onlyStaking nonReentrant {
        uint256 rewards = _calculateRewards(msg.sender);
        require(rewards > 0, "SulweToken: No rewards to claim");

        StakingInfo storage userStaking = stakingInfo[msg.sender];
        userStaking.lastRewardTime = block.timestamp;
        userStaking.totalRewardsEarned = userStaking.totalRewardsEarned.add(rewards);

        _mint(msg.sender, rewards);
        totalRewardsDistributed = totalRewardsDistributed.add(rewards);

        emit RewardsClaimed(msg.sender, rewards, block.timestamp);
    }

    /**
     * @dev Calculate pending rewards for a user
     * @param user Address of the user
     * @return Pending rewards
     */
    function calculateRewards(address user) external view returns (uint256) {
        if (!isStaking[user]) return 0;
        return _calculateRewards(user);
    }

    /**
     * @dev Get staking information for a user
     * @param user Address of the user
     * @return Staking information
     */
    function getStakingInfo(address user) external view returns (StakingInfo memory) {
        return stakingInfo[user];
    }

    /**
     * @dev Update reward pool (only owner)
     * @param newPool New reward pool amount
     */
    function updateRewardPool(uint256 newPool) external onlyOwner {
        stakingRewardPool = newPool;
        _updateRewardRate();
        emit RewardPoolUpdated(newPool, block.timestamp);
    }

    /**
     * @dev Pause/unpause staking (only owner)
     * @param paused Whether to pause staking
     */
    function setStakingPaused(bool paused) external onlyOwner {
        if (paused) {
            _pause();
        } else {
            _unpause();
        }
        emit StakingPaused(paused, block.timestamp);
    }

    /**
     * @dev Internal function to calculate rewards
     * @param user Address of the user
     * @return Calculated rewards
     */
    function _calculateRewards(address user) internal view returns (uint256) {
        if (!isStaking[user]) return 0;

        StakingInfo memory userStaking = stakingInfo[user];
        uint256 timeElapsed = block.timestamp.sub(userStaking.lastRewardTime);
        
        if (timeElapsed == 0 || rewardRate.totalStaked == 0) return 0;

        uint256 userShare = userStaking.stakedAmount.mul(REWARD_PRECISION).div(rewardRate.totalStaked);
        uint256 rewards = timeElapsed.mul(rewardRate.rate).mul(userShare).div(REWARD_PRECISION);

        return rewards;
    }

    /**
     * @dev Internal function to update reward rate
     */
    function _updateRewardRate() internal {
        if (rewardRate.totalStaked > 0) {
            rewardRate.rate = stakingRewardPool.div(REWARD_DISTRIBUTION_PERIOD).div(rewardRate.totalStaked);
        } else {
            rewardRate.rate = 0;
        }
        rewardRate.lastUpdateTime = block.timestamp;
    }

    /**
     * @dev Override required functions
     */
    function _beforeTokenTransfer(
        address from,
        address to,
        uint256 amount
    ) internal virtual override(ERC20, ERC20Pausable) {
        super._beforeTokenTransfer(from, to, amount);
    }

    /**
     * @dev Emergency function to recover stuck tokens (only owner)
     * @param token Address of the token to recover
     * @param amount Amount to recover
     */
    function emergencyRecover(address token, uint256 amount) external onlyOwner {
        require(token != address(this), "SulweToken: Cannot recover SLW tokens");
        IERC20(token).transfer(owner(), amount);
    }
} 