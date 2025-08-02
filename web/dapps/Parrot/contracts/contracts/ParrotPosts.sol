// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "@openzeppelin/contracts/access/Ownable.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/utils/Counters.sol";
import "@openzeppelin/contracts/utils/Strings.sol";
import "./SulweToken.sol";
import "./ParrotProfile.sol";

/**
 * @title ParrotPosts
 * @dev Manages social posts with IPFS storage, interactions, and rewards
 */
contract ParrotPosts is Ownable, ReentrancyGuard {
    using Counters for Counters.Counter;
    using Strings for string;

    // Post structure
    struct Post {
        uint256 id;
        address author;
        string contentHash; // IPFS hash for post content
        string mediaHash;   // IPFS hash for media (images, videos)
        uint256 createdAt;
        uint256 lastEdited;
        bool isEdited;
        bool isDeleted;
        uint256 likeCount;
        uint256 commentCount;
        uint256 shareCount;
        uint256 rewardAmount;
        string[] tags;
        mapping(address => bool) likedBy;
        mapping(address => uint256) userLikeTime;
    }

    // Comment structure
    struct Comment {
        uint256 id;
        uint256 postId;
        address author;
        string content;
        uint256 createdAt;
        uint256 likeCount;
        bool isDeleted;
        mapping(address => bool) likedBy;
    }

    // Reward structure
    struct PostReward {
        uint256 postId;
        address author;
        uint256 amount;
        uint256 timestamp;
        string reason; // "creation", "viral", "engagement", etc.
    }

    // State variables
    mapping(uint256 => Post) public posts;
    mapping(uint256 => Comment) public comments;
    mapping(address => uint256[]) public userPosts;
    mapping(address => uint256[]) public userLikedPosts;
    mapping(uint256 => uint256[]) public postComments;
    
    Counters.Counter private _postIdCounter;
    Counters.Counter private _commentIdCounter;
    
    uint256 public totalPosts;
    uint256 public totalComments;
    uint256 public totalLikes;
    uint256 public totalRewards;

    // Reward constants
    uint256 public constant POST_CREATION_REWARD = 10 * 1e18; // 10 SLW
    uint256 public constant LIKE_REWARD = 1 * 1e18; // 1 SLW
    uint256 public constant COMMENT_REWARD = 2 * 1e18; // 2 SLW
    uint256 public constant VIRAL_THRESHOLD = 100; // Posts with 100+ likes get viral bonus
    uint256 public constant VIRAL_BONUS = 50 * 1e18; // 50 SLW bonus

    // Contract references
    SulweToken public sulweToken;
    ParrotProfile public parrotProfile;

    // Events
    event PostCreated(uint256 indexed postId, address indexed author, string contentHash, uint256 timestamp);
    event PostEdited(uint256 indexed postId, address indexed author, string newContentHash, uint256 timestamp);
    event PostDeleted(uint256 indexed postId, address indexed author, uint256 timestamp);
    event PostLiked(uint256 indexed postId, address indexed user, uint256 timestamp);
    event PostUnliked(uint256 indexed postId, address indexed user, uint256 timestamp);
    event CommentAdded(uint256 indexed postId, uint256 indexed commentId, address indexed author, uint256 timestamp);
    event CommentLiked(uint256 indexed commentId, address indexed user, uint256 timestamp);
    event PostRewarded(uint256 indexed postId, address indexed author, uint256 amount, string reason, uint256 timestamp);
    event ViralPostBonus(uint256 indexed postId, address indexed author, uint256 bonus, uint256 timestamp);

    // Modifiers
    modifier onlyPostAuthor(uint256 postId) {
        require(posts[postId].author == msg.sender, "ParrotPosts: Not post author");
        _;
    }

    modifier postExists(uint256 postId) {
        require(posts[postId].id != 0, "ParrotPosts: Post does not exist");
        require(!posts[postId].isDeleted, "ParrotPosts: Post is deleted");
        _;
    }

    modifier commentExists(uint256 commentId) {
        require(comments[commentId].id != 0, "ParrotPosts: Comment does not exist");
        require(!comments[commentId].isDeleted, "ParrotPosts: Comment is deleted");
        _;
    }

    modifier hasProfile() {
        require(parrotProfile.hasProfile(msg.sender), "ParrotPosts: Profile required");
        _;
    }

    /**
     * @dev Constructor
     * @param _sulweToken Address of SLW token contract
     * @param _parrotProfile Address of profile contract
     */
    constructor(address _sulweToken, address _parrotProfile) {
        sulweToken = SulweToken(_sulweToken);
        parrotProfile = ParrotProfile(_parrotProfile);
    }

    /**
     * @dev Create a new post
     * @param contentHash IPFS hash for post content
     * @param mediaHash IPFS hash for media (optional)
     * @param tags Array of tags for the post
     */
    function createPost(
        string memory contentHash,
        string memory mediaHash,
        string[] memory tags
    ) external hasProfile nonReentrant {
        require(bytes(contentHash).length > 0, "ParrotPosts: Content required");

        _postIdCounter.increment();
        uint256 postId = _postIdCounter.current();

        Post storage post = posts[postId];
        post.id = postId;
        post.author = msg.sender;
        post.contentHash = contentHash;
        post.mediaHash = mediaHash;
        post.createdAt = block.timestamp;
        post.lastEdited = block.timestamp;
        post.isEdited = false;
        post.isDeleted = false;
        post.likeCount = 0;
        post.commentCount = 0;
        post.shareCount = 0;
        post.rewardAmount = 0;

        // Add tags
        for (uint i = 0; i < tags.length; i++) {
            post.tags.push(tags[i]);
        }

        // Add to user's posts
        userPosts[msg.sender].push(postId);
        totalPosts++;

        // Reward for post creation
        _rewardUser(msg.sender, POST_CREATION_REWARD, "post_creation");

        emit PostCreated(postId, msg.sender, contentHash, block.timestamp);
    }

    /**
     * @dev Edit a post
     * @param postId ID of the post to edit
     * @param newContentHash New IPFS hash for content
     * @param newMediaHash New IPFS hash for media
     */
    function editPost(
        uint256 postId,
        string memory newContentHash,
        string memory newMediaHash
    ) external onlyPostAuthor(postId) postExists(postId) {
        require(bytes(newContentHash).length > 0, "ParrotPosts: Content required");

        Post storage post = posts[postId];
        post.contentHash = newContentHash;
        post.mediaHash = newMediaHash;
        post.lastEdited = block.timestamp;
        post.isEdited = true;

        emit PostEdited(postId, msg.sender, newContentHash, block.timestamp);
    }

    /**
     * @dev Delete a post
     * @param postId ID of the post to delete
     */
    function deletePost(uint256 postId) external onlyPostAuthor(postId) postExists(postId) {
        posts[postId].isDeleted = true;
        emit PostDeleted(postId, msg.sender, block.timestamp);
    }

    /**
     * @dev Like a post
     * @param postId ID of the post to like
     */
    function likePost(uint256 postId) external hasProfile postExists(postId) {
        Post storage post = posts[postId];
        require(!post.likedBy[msg.sender], "ParrotPosts: Already liked");
        require(msg.sender != post.author, "ParrotPosts: Cannot like own post");

        post.likedBy[msg.sender] = true;
        post.userLikeTime[msg.sender] = block.timestamp;
        post.likeCount++;
        totalLikes++;

        userLikedPosts[msg.sender].push(postId);

        // Reward for liking
        _rewardUser(msg.sender, LIKE_REWARD, "like");

        // Check for viral bonus
        if (post.likeCount == VIRAL_THRESHOLD) {
            _rewardUser(post.author, VIRAL_BONUS, "viral_bonus");
            emit ViralPostBonus(postId, post.author, VIRAL_BONUS, block.timestamp);
        }

        emit PostLiked(postId, msg.sender, block.timestamp);
    }

    /**
     * @dev Unlike a post
     * @param postId ID of the post to unlike
     */
    function unlikePost(uint256 postId) external hasProfile postExists(postId) {
        Post storage post = posts[postId];
        require(post.likedBy[msg.sender], "ParrotPosts: Not liked");

        post.likedBy[msg.sender] = false;
        post.likeCount--;
        totalLikes--;

        // Remove from user's liked posts
        _removeFromArray(userLikedPosts[msg.sender], postId);

        emit PostUnliked(postId, msg.sender, block.timestamp);
    }

    /**
     * @dev Add a comment to a post
     * @param postId ID of the post to comment on
     * @param content Comment content
     */
    function addComment(uint256 postId, string memory content) external hasProfile postExists(postId) {
        require(bytes(content).length > 0, "ParrotPosts: Comment content required");

        _commentIdCounter.increment();
        uint256 commentId = _commentIdCounter.current();

        Comment storage comment = comments[commentId];
        comment.id = commentId;
        comment.postId = postId;
        comment.author = msg.sender;
        comment.content = content;
        comment.createdAt = block.timestamp;
        comment.likeCount = 0;
        comment.isDeleted = false;

        postComments[postId].push(commentId);
        posts[postId].commentCount++;
        totalComments++;

        // Reward for commenting
        _rewardUser(msg.sender, COMMENT_REWARD, "comment");

        emit CommentAdded(postId, commentId, msg.sender, block.timestamp);
    }

    /**
     * @dev Like a comment
     * @param commentId ID of the comment to like
     */
    function likeComment(uint256 commentId) external hasProfile commentExists(commentId) {
        Comment storage comment = comments[commentId];
        require(!comment.likedBy[msg.sender], "ParrotPosts: Already liked comment");
        require(msg.sender != comment.author, "ParrotPosts: Cannot like own comment");

        comment.likedBy[msg.sender] = true;
        comment.likeCount++;

        emit CommentLiked(commentId, msg.sender, block.timestamp);
    }

    /**
     * @dev Get post by ID
     * @param postId ID of the post
     * @return Post data
     */
    function getPost(uint256 postId) external view postExists(postId) returns (
        uint256 id,
        address author,
        string memory contentHash,
        string memory mediaHash,
        uint256 createdAt,
        uint256 lastEdited,
        bool isEdited,
        uint256 likeCount,
        uint256 commentCount,
        uint256 shareCount,
        uint256 rewardAmount
    ) {
        Post storage post = posts[postId];
        return (
            post.id,
            post.author,
            post.contentHash,
            post.mediaHash,
            post.createdAt,
            post.lastEdited,
            post.isEdited,
            post.likeCount,
            post.commentCount,
            post.shareCount,
            post.rewardAmount
        );
    }

    /**
     * @dev Get post tags
     * @param postId ID of the post
     * @return Array of tags
     */
    function getPostTags(uint256 postId) external view postExists(postId) returns (string[] memory) {
        return posts[postId].tags;
    }

    /**
     * @dev Check if user liked a post
     * @param postId ID of the post
     * @param user Address of the user
     * @return True if liked
     */
    function isPostLiked(uint256 postId, address user) external view returns (bool) {
        return posts[postId].likedBy[user];
    }

    /**
     * @dev Get user's posts
     * @param user Address of the user
     * @return Array of post IDs
     */
    function getUserPosts(address user) external view returns (uint256[] memory) {
        return userPosts[user];
    }

    /**
     * @dev Get user's liked posts
     * @param user Address of the user
     * @return Array of post IDs
     */
    function getUserLikedPosts(address user) external view returns (uint256[] memory) {
        return userLikedPosts[user];
    }

    /**
     * @dev Get post comments
     * @param postId ID of the post
     * @return Array of comment IDs
     */
    function getPostComments(uint256 postId) external view returns (uint256[] memory) {
        return postComments[postId];
    }

    /**
     * @dev Get comment by ID
     * @param commentId ID of the comment
     * @return Comment data
     */
    function getComment(uint256 commentId) external view commentExists(commentId) returns (
        uint256 id,
        uint256 postId,
        address author,
        string memory content,
        uint256 createdAt,
        uint256 likeCount
    ) {
        Comment storage comment = comments[commentId];
        return (
            comment.id,
            comment.postId,
            comment.author,
            comment.content,
            comment.createdAt,
            comment.likeCount
        );
    }

    /**
     * @dev Internal function to reward users
     * @param user Address of the user to reward
     * @param amount Amount to reward
     * @param reason Reason for reward
     */
    function _rewardUser(address user, uint256 amount, string memory reason) internal {
        sulweToken.mint(user, amount);
        totalRewards += amount;
        emit PostRewarded(0, user, amount, reason, block.timestamp);
    }

    /**
     * @dev Internal function to remove item from array
     * @param array Array to remove from
     * @param item Item to remove
     */
    function _removeFromArray(uint256[] storage array, uint256 item) internal {
        for (uint i = 0; i < array.length; i++) {
            if (array[i] == item) {
                array[i] = array[array.length - 1];
                array.pop();
                break;
            }
        }
    }

    /**
     * @dev Update reward amounts (only owner)
     * @param _postCreationReward New post creation reward
     * @param _likeReward New like reward
     * @param _commentReward New comment reward
     * @param _viralBonus New viral bonus
     */
    function updateRewards(
        uint256 _postCreationReward,
        uint256 _likeReward,
        uint256 _commentReward,
        uint256 _viralBonus
    ) external onlyOwner {
        // These would be stored as state variables if they were meant to be changeable
        // For now, they're constants, but this function shows how they could be made configurable
    }

    /**
     * @dev Emergency function to recover stuck tokens (only owner)
     * @param token Address of the token to recover
     * @param amount Amount to recover
     */
    function emergencyRecover(address token, uint256 amount) external onlyOwner {
        require(token != address(0), "ParrotPosts: Cannot recover ETH");
        IERC20(token).transfer(owner(), amount);
    }
} 