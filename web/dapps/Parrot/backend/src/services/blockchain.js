const { ethers } = require('ethers');
const logger = require('../utils/logger');

class BlockchainService {
  constructor() {
    this.provider = null;
    this.signer = null;
    this.contracts = {};
    this.isInitialized = false;
  }

  async initialize() {
    try {
      // Initialize provider
      const rpcUrl = process.env.NILOTIC_RPC_URL || 'https://testnet.nilotic.chain';
      this.provider = new ethers.providers.JsonRpcProvider(rpcUrl);
      
      // Initialize signer for admin operations
      const privateKey = process.env.PRIVATE_KEY;
      if (privateKey) {
        this.signer = new ethers.Wallet(privateKey, this.provider);
      }

      // Load contract addresses
      const contractAddresses = this.loadContractAddresses();
      
      // Initialize contract instances
      await this.initializeContracts(contractAddresses);
      
      this.isInitialized = true;
      logger.info('Blockchain service initialized successfully');
    } catch (error) {
      logger.error('Failed to initialize blockchain service:', error);
      throw error;
    }
  }

  loadContractAddresses() {
    // Load from environment variables or deployment file
    const addresses = {
      sulweToken: process.env.SULWE_TOKEN_ADDRESS,
      parrotProfile: process.env.PARROT_PROFILE_ADDRESS,
      parrotPosts: process.env.PARROT_POSTS_ADDRESS,
      parrotNFT: process.env.PARROT_NFT_ADDRESS,
      parrotDAO: process.env.PARROT_DAO_ADDRESS,
    };

    // Validate addresses
    for (const [name, address] of Object.entries(addresses)) {
      if (!address || !ethers.utils.isAddress(address)) {
        throw new Error(`Invalid ${name} address: ${address}`);
      }
    }

    return addresses;
  }

  async initializeContracts(addresses) {
    try {
      // Load contract ABIs
      const abis = await this.loadContractABIs();
      
      // Initialize contract instances
      this.contracts.sulweToken = new ethers.Contract(
        addresses.sulweToken,
        abis.sulweToken,
        this.provider
      );

      this.contracts.parrotProfile = new ethers.Contract(
        addresses.parrotProfile,
        abis.parrotProfile,
        this.provider
      );

      this.contracts.parrotPosts = new ethers.Contract(
        addresses.parrotPosts,
        abis.parrotPosts,
        this.provider
      );

      this.contracts.parrotNFT = new ethers.Contract(
        addresses.parrotNFT,
        abis.parrotNFT,
        this.provider
      );

      this.contracts.parrotDAO = new ethers.Contract(
        addresses.parrotDAO,
        abis.parrotDAO,
        this.provider
      );

      logger.info('Contract instances initialized');
    } catch (error) {
      logger.error('Failed to initialize contracts:', error);
      throw error;
    }
  }

  async loadContractABIs() {
    // In a real implementation, these would be loaded from compiled contract artifacts
    // For now, we'll use simplified ABIs
    return {
      sulweToken: [
        'function balanceOf(address owner) view returns (uint256)',
        'function transfer(address to, uint256 amount) returns (bool)',
        'function transferFrom(address from, address to, uint256 amount) returns (bool)',
        'function mint(address to, uint256 amount)',
        'function stake(uint256 amount)',
        'function unstake()',
        'function claimRewards()',
        'function calculateRewards(address user) view returns (uint256)',
        'function getStakingInfo(address user) view returns (uint256, uint256, uint256, uint256)',
      ],
      parrotProfile: [
        'function createProfile(string username, string displayName, string bio, string avatarHash, string[] tags)',
        'function updateProfile(string displayName, string bio, string avatarHash, string coverHash)',
        'function getProfile(address user) view returns (string, string, string, string, string, uint256, uint256, bool, uint256, uint256, uint256, uint256, uint256)',
        'function followUser(address userToFollow)',
        'function unfollowUser(address userToUnfollow)',
        'function isFollowing(address follower, address following) view returns (bool)',
        'function hasProfile(address user) view returns (bool)',
      ],
      parrotPosts: [
        'function createPost(string contentHash, string mediaHash, string[] tags)',
        'function likePost(uint256 postId)',
        'function unlikePost(uint256 postId)',
        'function addComment(uint256 postId, string content)',
        'function getPost(uint256 postId) view returns (uint256, address, string, string, uint256, uint256, bool, uint256, uint256, uint256, uint256)',
        'function getUserPosts(address user) view returns (uint256[])',
        'function isPostLiked(uint256 postId, address user) view returns (bool)',
      ],
      parrotNFT: [
        'function mintNFT(string name, string description, string imageHash, string animationHash, string category, string[] tags, uint256 royaltyPercentage)',
        'function listNFT(uint256 tokenId, uint256 price)',
        'function buyNFT(uint256 tokenId)',
        'function createAuction(uint256 tokenId, uint256 startPrice, uint256 duration)',
        'function placeBid(uint256 tokenId, uint256 bidAmount)',
        'function getNFTMetadata(uint256 tokenId) view returns (uint256, address, string, string, string, string, uint256, uint256, bool, bool, uint256, uint256, address, string, uint256)',
        'function getCreatorNFTs(address creator) view returns (uint256[])',
      ],
      parrotDAO: [
        'function createProposal(string title, string description, string action)',
        'function vote(uint256 proposalId, bool support)',
        'function executeProposal(uint256 proposalId)',
        'function getProposal(uint256 proposalId) view returns (uint256, address, string, string, string, uint256, uint256, uint256, uint256, uint256, bool, bool)',
        'function getVotingPower(address account) view returns (uint256)',
        'function getProposalState(uint256 proposalId) view returns (string)',
      ],
    };
  }

  // Token methods
  async getTokenBalance(address) {
    try {
      const balance = await this.contracts.sulweToken.balanceOf(address);
      return ethers.utils.formatEther(balance);
    } catch (error) {
      logger.error('Error getting token balance:', error);
      throw error;
    }
  }

  async getStakingInfo(address) {
    try {
      const stakingInfo = await this.contracts.sulweToken.getStakingInfo(address);
      return {
        stakedAmount: ethers.utils.formatEther(stakingInfo[0]),
        stakingStartTime: stakingInfo[1].toNumber(),
        lastRewardTime: stakingInfo[2].toNumber(),
        totalRewardsEarned: ethers.utils.formatEther(stakingInfo[3]),
      };
    } catch (error) {
      logger.error('Error getting staking info:', error);
      throw error;
    }
  }

  async calculateRewards(address) {
    try {
      const rewards = await this.contracts.sulweToken.calculateRewards(address);
      return ethers.utils.formatEther(rewards);
    } catch (error) {
      logger.error('Error calculating rewards:', error);
      throw error;
    }
  }

  // Profile methods
  async createProfile(userAddress, profileData) {
    try {
      const tx = await this.contracts.parrotProfile.connect(this.signer).createProfile(
        profileData.username,
        profileData.displayName,
        profileData.bio,
        profileData.avatarHash,
        profileData.tags
      );
      return await tx.wait();
    } catch (error) {
      logger.error('Error creating profile:', error);
      throw error;
    }
  }

  async getProfile(address) {
    try {
      const profile = await this.contracts.parrotProfile.getProfile(address);
      return {
        username: profile[0],
        displayName: profile[1],
        bio: profile[2],
        avatarHash: profile[3],
        coverHash: profile[4],
        createdAt: profile[5].toNumber(),
        lastActive: profile[6].toNumber(),
        isVerified: profile[7],
        followerCount: profile[8].toNumber(),
        followingCount: profile[9].toNumber(),
        postCount: profile[10].toNumber(),
        totalLikes: profile[11].toNumber(),
        totalRewards: ethers.utils.formatEther(profile[12]),
      };
    } catch (error) {
      logger.error('Error getting profile:', error);
      throw error;
    }
  }

  // Post methods
  async createPost(userAddress, postData) {
    try {
      const tx = await this.contracts.parrotPosts.connect(this.signer).createPost(
        postData.contentHash,
        postData.mediaHash || '',
        postData.tags || []
      );
      return await tx.wait();
    } catch (error) {
      logger.error('Error creating post:', error);
      throw error;
    }
  }

  async getPost(postId) {
    try {
      const post = await this.contracts.parrotPosts.getPost(postId);
      return {
        id: post[0].toNumber(),
        author: post[1],
        contentHash: post[2],
        mediaHash: post[3],
        createdAt: post[4].toNumber(),
        lastEdited: post[5].toNumber(),
        isEdited: post[6],
        likeCount: post[7].toNumber(),
        commentCount: post[8].toNumber(),
        shareCount: post[9].toNumber(),
        rewardAmount: ethers.utils.formatEther(post[10]),
      };
    } catch (error) {
      logger.error('Error getting post:', error);
      throw error;
    }
  }

  async likePost(userAddress, postId) {
    try {
      const tx = await this.contracts.parrotPosts.connect(this.signer).likePost(postId);
      return await tx.wait();
    } catch (error) {
      logger.error('Error liking post:', error);
      throw error;
    }
  }

  // NFT methods
  async mintNFT(userAddress, nftData) {
    try {
      const tx = await this.contracts.parrotNFT.connect(this.signer).mintNFT(
        nftData.name,
        nftData.description,
        nftData.imageHash,
        nftData.animationHash || '',
        nftData.category,
        nftData.tags || [],
        nftData.royaltyPercentage
      );
      return await tx.wait();
    } catch (error) {
      logger.error('Error minting NFT:', error);
      throw error;
    }
  }

  async getNFTMetadata(tokenId) {
    try {
      const metadata = await this.contracts.parrotNFT.getNFTMetadata(tokenId);
      return {
        id: metadata[0].toNumber(),
        creator: metadata[1],
        name: metadata[2],
        description: metadata[3],
        imageHash: metadata[4],
        animationHash: metadata[5],
        createdAt: metadata[6].toNumber(),
        price: ethers.utils.formatEther(metadata[7]),
        isForSale: metadata[8],
        isAuction: metadata[9],
        auctionEndTime: metadata[10].toNumber(),
        highestBid: ethers.utils.formatEther(metadata[11]),
        highestBidder: metadata[12],
        category: metadata[13],
        royaltyPercentage: metadata[14].toNumber(),
      };
    } catch (error) {
      logger.error('Error getting NFT metadata:', error);
      throw error;
    }
  }

  // DAO methods
  async createProposal(userAddress, proposalData) {
    try {
      const tx = await this.contracts.parrotDAO.connect(this.signer).createProposal(
        proposalData.title,
        proposalData.description,
        proposalData.action
      );
      return await tx.wait();
    } catch (error) {
      logger.error('Error creating proposal:', error);
      throw error;
    }
  }

  async getProposal(proposalId) {
    try {
      const proposal = await this.contracts.parrotDAO.getProposal(proposalId);
      return {
        id: proposal[0].toNumber(),
        proposer: proposal[1],
        title: proposal[2],
        description: proposal[3],
        action: proposal[4],
        forVotes: ethers.utils.formatEther(proposal[5]),
        againstVotes: ethers.utils.formatEther(proposal[6]),
        abstainVotes: ethers.utils.formatEther(proposal[7]),
        startTime: proposal[8].toNumber(),
        endTime: proposal[9].toNumber(),
        executed: proposal[10],
        canceled: proposal[11],
      };
    } catch (error) {
      logger.error('Error getting proposal:', error);
      throw error;
    }
  }

  async getVotingPower(address) {
    try {
      const power = await this.contracts.parrotDAO.getVotingPower(address);
      return ethers.utils.formatEther(power);
    } catch (error) {
      logger.error('Error getting voting power:', error);
      throw error;
    }
  }

  // Utility methods
  async getNetworkInfo() {
    try {
      const network = await this.provider.getNetwork();
      const blockNumber = await this.provider.getBlockNumber();
      const gasPrice = await this.provider.getGasPrice();
      
      return {
        chainId: network.chainId,
        name: network.name,
        blockNumber,
        gasPrice: ethers.utils.formatUnits(gasPrice, 'gwei'),
      };
    } catch (error) {
      logger.error('Error getting network info:', error);
      throw error;
    }
  }

  async estimateGas(contract, method, ...args) {
    try {
      const gasEstimate = await contract.estimateGas[method](...args);
      return gasEstimate.toString();
    } catch (error) {
      logger.error('Error estimating gas:', error);
      throw error;
    }
  }

  isInitialized() {
    return this.isInitialized;
  }

  getProvider() {
    return this.provider;
  }

  getContracts() {
    return this.contracts;
  }
}

module.exports = new BlockchainService(); 