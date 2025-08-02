// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "@openzeppelin/contracts/access/Ownable.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/utils/Strings.sol";
import "@openzeppelin/contracts/utils/Counters.sol";

/**
 * @title ParrotProfile
 * @dev Manages user profiles with Nilotic-themed usernames and decentralized identity
 */
contract ParrotProfile is Ownable, ReentrancyGuard {
    using Counters for Counters.Counter;
    using Strings for string;

    // Profile structure
    struct Profile {
        string username;
        string displayName;
        string bio;
        string avatarHash; // IPFS hash for avatar
        string coverHash;  // IPFS hash for cover image
        uint256 createdAt;
        uint256 lastActive;
        bool isVerified;
        uint256 followerCount;
        uint256 followingCount;
        uint256 postCount;
        uint256 totalLikes;
        uint256 totalRewards;
        string[] tags; // Cultural tags, interests
        mapping(address => bool) followers;
        mapping(address => bool) following;
    }

    // Nilotic-themed username prefixes
    string[] public niloticPrefixes = [
        "Sulwe", "Nyota", "Jua", "Mwezi", "Nuru", "Mwanga",
        "Kipenzi", "Mpenzi", "Rafiki", "Ndugu", "Mama", "Baba",
        "Mwanamke", "Mwanamume", "Mtoto", "Mzee", "Mchanga",
        "Mlima", "Mto", "Ziwa", "Nyika", "Msitu", "Shamba"
    ];

    // Star-themed suffixes
    string[] public starSuffixes = [
        "Star", "Light", "Glow", "Shine", "Spark", "Flame",
        "Fire", "Sun", "Moon", "Dawn", "Dusk", "Twilight",
        "Aurora", "Nova", "Comet", "Meteor", "Galaxy", "Cosmos"
    ];

    // State variables
    mapping(address => Profile) public profiles;
    mapping(string => address) public usernameToAddress;
    mapping(address => bool) public hasProfile;
    
    Counters.Counter private _profileIdCounter;
    uint256 public totalProfiles;
    uint256 public verificationFee = 0.1 ether; // Fee for verification

    // Events
    event ProfileCreated(address indexed user, string username, uint256 timestamp);
    event ProfileUpdated(address indexed user, string field, uint256 timestamp);
    event UsernameChanged(address indexed user, string oldUsername, string newUsername, uint256 timestamp);
    event UserFollowed(address indexed follower, address indexed following, uint256 timestamp);
    event UserUnfollowed(address indexed follower, address indexed following, uint256 timestamp);
    event UserVerified(address indexed user, uint256 timestamp);
    event VerificationFeeUpdated(uint256 newFee, uint256 timestamp);

    // Modifiers
    modifier onlyProfileOwner(address user) {
        require(hasProfile[user], "ParrotProfile: Profile does not exist");
        require(msg.sender == user, "ParrotProfile: Not profile owner");
        _;
    }

    modifier profileExists(address user) {
        require(hasProfile[user], "ParrotProfile: Profile does not exist");
        _;
    }

    modifier usernameAvailable(string memory username) {
        require(usernameToAddress[username] == address(0), "ParrotProfile: Username taken");
        require(bytes(username).length >= 3, "ParrotProfile: Username too short");
        require(bytes(username).length <= 20, "ParrotProfile: Username too long");
        _;
    }

    /**
     * @dev Create a new profile with Nilotic-themed username
     * @param username Custom username
     * @param displayName Display name
     * @param bio User bio
     * @param avatarHash IPFS hash for avatar
     * @param tags Cultural tags and interests
     */
    function createProfile(
        string memory username,
        string memory displayName,
        string memory bio,
        string memory avatarHash,
        string[] memory tags
    ) external nonReentrant usernameAvailable(username) {
        require(!hasProfile[msg.sender], "ParrotProfile: Profile already exists");
        require(bytes(displayName).length > 0, "ParrotProfile: Display name required");

        // Create profile
        Profile storage profile = profiles[msg.sender];
        profile.username = username;
        profile.displayName = displayName;
        profile.bio = bio;
        profile.avatarHash = avatarHash;
        profile.createdAt = block.timestamp;
        profile.lastActive = block.timestamp;
        profile.isVerified = false;
        profile.followerCount = 0;
        profile.followingCount = 0;
        profile.postCount = 0;
        profile.totalLikes = 0;
        profile.totalRewards = 0;

        // Add tags
        for (uint i = 0; i < tags.length; i++) {
            profile.tags.push(tags[i]);
        }

        // Update state
        hasProfile[msg.sender] = true;
        usernameToAddress[username] = msg.sender;
        totalProfiles++;

        emit ProfileCreated(msg.sender, username, block.timestamp);
    }

    /**
     * @dev Generate a Nilotic-themed username
     * @return Generated username
     */
    function generateNiloticUsername() external view returns (string memory) {
        uint256 prefixIndex = uint256(keccak256(abi.encodePacked(msg.sender, block.timestamp))) % niloticPrefixes.length;
        uint256 suffixIndex = uint256(keccak256(abi.encodePacked(msg.sender, block.number))) % starSuffixes.length;
        
        string memory prefix = niloticPrefixes[prefixIndex];
        string memory suffix = starSuffixes[suffixIndex];
        
        return string(abi.encodePacked(prefix, suffix));
    }

    /**
     * @dev Update profile information
     * @param displayName New display name
     * @param bio New bio
     * @param avatarHash New avatar IPFS hash
     * @param coverHash New cover IPFS hash
     */
    function updateProfile(
        string memory displayName,
        string memory bio,
        string memory avatarHash,
        string memory coverHash
    ) external onlyProfileOwner(msg.sender) {
        Profile storage profile = profiles[msg.sender];
        
        if (bytes(displayName).length > 0) {
            profile.displayName = displayName;
            emit ProfileUpdated(msg.sender, "displayName", block.timestamp);
        }
        
        profile.bio = bio;
        profile.avatarHash = avatarHash;
        profile.coverHash = coverHash;
        profile.lastActive = block.timestamp;

        emit ProfileUpdated(msg.sender, "profile", block.timestamp);
    }

    /**
     * @dev Change username
     * @param newUsername New username
     */
    function changeUsername(string memory newUsername) external onlyProfileOwner(msg.sender) usernameAvailable(newUsername) {
        Profile storage profile = profiles[msg.sender];
        string memory oldUsername = profile.username;
        
        // Remove old username mapping
        delete usernameToAddress[oldUsername];
        
        // Set new username
        profile.username = newUsername;
        usernameToAddress[newUsername] = msg.sender;
        profile.lastActive = block.timestamp;

        emit UsernameChanged(msg.sender, oldUsername, newUsername, block.timestamp);
    }

    /**
     * @dev Follow another user
     * @param userToFollow Address of user to follow
     */
    function followUser(address userToFollow) external profileExists(msg.sender) profileExists(userToFollow) {
        require(msg.sender != userToFollow, "ParrotProfile: Cannot follow yourself");
        require(!profiles[msg.sender].following[userToFollow], "ParrotProfile: Already following");

        Profile storage followerProfile = profiles[msg.sender];
        Profile storage followingProfile = profiles[userToFollow];

        followerProfile.following[userToFollow] = true;
        followingProfile.followers[msg.sender] = true;

        followerProfile.followingCount++;
        followingProfile.followerCount++;

        emit UserFollowed(msg.sender, userToFollow, block.timestamp);
    }

    /**
     * @dev Unfollow another user
     * @param userToUnfollow Address of user to unfollow
     */
    function unfollowUser(address userToUnfollow) external profileExists(msg.sender) profileExists(userToUnfollow) {
        require(profiles[msg.sender].following[userToUnfollow], "ParrotProfile: Not following user");

        Profile storage followerProfile = profiles[msg.sender];
        Profile storage followingProfile = profiles[userToUnfollow];

        followerProfile.following[userToUnfollow] = false;
        followingProfile.followers[msg.sender] = false;

        followerProfile.followingCount--;
        followingProfile.followerCount--;

        emit UserUnfollowed(msg.sender, userToUnfollow, block.timestamp);
    }

    /**
     * @dev Verify user profile (requires fee)
     */
    function verifyProfile() external payable profileExists(msg.sender) {
        require(msg.value >= verificationFee, "ParrotProfile: Insufficient verification fee");
        require(!profiles[msg.sender].isVerified, "ParrotProfile: Already verified");

        profiles[msg.sender].isVerified = true;
        profiles[msg.sender].lastActive = block.timestamp;

        emit UserVerified(msg.sender, block.timestamp);
    }

    /**
     * @dev Get profile by address
     * @param user Address of the user
     * @return Profile data
     */
    function getProfile(address user) external view profileExists(user) returns (
        string memory username,
        string memory displayName,
        string memory bio,
        string memory avatarHash,
        string memory coverHash,
        uint256 createdAt,
        uint256 lastActive,
        bool isVerified,
        uint256 followerCount,
        uint256 followingCount,
        uint256 postCount,
        uint256 totalLikes,
        uint256 totalRewards
    ) {
        Profile storage profile = profiles[user];
        return (
            profile.username,
            profile.displayName,
            profile.bio,
            profile.avatarHash,
            profile.coverHash,
            profile.createdAt,
            profile.lastActive,
            profile.isVerified,
            profile.followerCount,
            profile.followingCount,
            profile.postCount,
            profile.totalLikes,
            profile.totalRewards
        );
    }

    /**
     * @dev Get profile tags
     * @param user Address of the user
     * @return Array of tags
     */
    function getProfileTags(address user) external view profileExists(user) returns (string[] memory) {
        return profiles[user].tags;
    }

    /**
     * @dev Check if user is following another user
     * @param follower Follower address
     * @param following Following address
     * @return True if following
     */
    function isFollowing(address follower, address following) external view returns (bool) {
        if (!hasProfile[follower] || !hasProfile[following]) return false;
        return profiles[follower].following[following];
    }

    /**
     * @dev Update verification fee (only owner)
     * @param newFee New verification fee
     */
    function updateVerificationFee(uint256 newFee) external onlyOwner {
        verificationFee = newFee;
        emit VerificationFeeUpdated(newFee, block.timestamp);
    }

    /**
     * @dev Withdraw verification fees (only owner)
     */
    function withdrawFees() external onlyOwner {
        uint256 balance = address(this).balance;
        require(balance > 0, "ParrotProfile: No fees to withdraw");
        
        payable(owner()).transfer(balance);
    }

    /**
     * @dev Emergency function to recover stuck tokens (only owner)
     * @param token Address of the token to recover
     * @param amount Amount to recover
     */
    function emergencyRecover(address token, uint256 amount) external onlyOwner {
        require(token != address(0), "ParrotProfile: Cannot recover ETH");
        IERC20(token).transfer(owner(), amount);
    }
} 