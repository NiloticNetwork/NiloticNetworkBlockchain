// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "@openzeppelin/contracts/token/ERC721/ERC721.sol";
import "@openzeppelin/contracts/token/ERC721/extensions/ERC721URIStorage.sol";
import "@openzeppelin/contracts/token/ERC721/extensions/ERC721Enumerable.sol";
import "@openzeppelin/contracts/access/Ownable.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/utils/Counters.sol";
import "@openzeppelin/contracts/utils/Strings.sol";
import "./SulweToken.sol";
import "./ParrotProfile.sol";

/**
 * @title ParrotNFT
 * @dev NFT contract for Parrot platform with Nilotic cultural themes
 */
contract ParrotNFT is ERC721, ERC721URIStorage, ERC721Enumerable, Ownable, ReentrancyGuard {
    using Counters for Counters.Counter;
    using Strings for string;

    // NFT structure
    struct NFTMetadata {
        uint256 tokenId;
        address creator;
        string name;
        string description;
        string imageHash; // IPFS hash for image
        string animationHash; // IPFS hash for animation (optional)
        uint256 createdAt;
        uint256 price;
        bool isForSale;
        bool isAuction;
        uint256 auctionEndTime;
        uint256 highestBid;
        address highestBidder;
        string category; // "art", "music", "story", "culture", "star"
        string[] tags;
        uint256 royaltyPercentage; // Creator royalty percentage (0-1000 = 0-100%)
    }

    // Auction structure
    struct Auction {
        uint256 tokenId;
        uint256 startPrice;
        uint256 currentPrice;
        uint256 startTime;
        uint256 endTime;
        address highestBidder;
        uint256 highestBid;
        bool isActive;
    }

    // State variables
    mapping(uint256 => NFTMetadata) public nftMetadata;
    mapping(uint256 => Auction) public auctions;
    mapping(address => uint256[]) public creatorNFTs;
    mapping(string => uint256[]) public categoryNFTs;
    
    Counters.Counter private _tokenIdCounter;
    
    uint256 public totalNFTs;
    uint256 public totalSales;
    uint256 public totalVolume;
    uint256 public platformFee = 25; // 2.5% (250 basis points)
    uint256 public constant FEE_DENOMINATOR = 1000;
    
    // Contract references
    SulweToken public sulweToken;
    ParrotProfile public parrotProfile;

    // Nilotic cultural categories
    string[] public culturalCategories = [
        "art", "music", "story", "culture", "star", "heritage",
        "tradition", "ceremony", "dance", "craft", "folklore"
    ];

    // Events
    event NFTMinted(uint256 indexed tokenId, address indexed creator, string name, uint256 timestamp);
    event NFTSold(uint256 indexed tokenId, address indexed seller, address indexed buyer, uint256 price, uint256 timestamp);
    event NFTAuctionCreated(uint256 indexed tokenId, uint256 startPrice, uint256 endTime, uint256 timestamp);
    event NFTAuctionBid(uint256 indexed tokenId, address indexed bidder, uint256 bidAmount, uint256 timestamp);
    event NFTAuctionEnded(uint256 indexed tokenId, address indexed winner, uint256 finalPrice, uint256 timestamp);
    event NFTListed(uint256 indexed tokenId, uint256 price, uint256 timestamp);
    event NFTDelisted(uint256 indexed tokenId, uint256 timestamp);
    event PlatformFeeUpdated(uint256 newFee, uint256 timestamp);

    // Modifiers
    modifier onlyNFTOwner(uint256 tokenId) {
        require(ownerOf(tokenId) == msg.sender, "ParrotNFT: Not NFT owner");
        _;
    }

    modifier nftExists(uint256 tokenId) {
        require(_exists(tokenId), "ParrotNFT: NFT does not exist");
        _;
    }

    modifier hasProfile() {
        require(parrotProfile.hasProfile(msg.sender), "ParrotNFT: Profile required");
        _;
    }

    modifier auctionExists(uint256 tokenId) {
        require(auctions[tokenId].isActive, "ParrotNFT: Auction does not exist");
        _;
    }

    /**
     * @dev Constructor
     * @param _sulweToken Address of SLW token contract
     * @param _parrotProfile Address of profile contract
     */
    constructor(
        address _sulweToken,
        address _parrotProfile
    ) ERC721("ParrotNFT", "PNFT") {
        sulweToken = SulweToken(_sulweToken);
        parrotProfile = ParrotProfile(_parrotProfile);
    }

    /**
     * @dev Mint a new NFT
     * @param name NFT name
     * @param description NFT description
     * @param imageHash IPFS hash for image
     * @param animationHash IPFS hash for animation (optional)
     * @param category Cultural category
     * @param tags Array of tags
     * @param royaltyPercentage Creator royalty percentage (0-1000)
     */
    function mintNFT(
        string memory name,
        string memory description,
        string memory imageHash,
        string memory animationHash,
        string memory category,
        string[] memory tags,
        uint256 royaltyPercentage
    ) external hasProfile nonReentrant {
        require(bytes(name).length > 0, "ParrotNFT: Name required");
        require(bytes(imageHash).length > 0, "ParrotNFT: Image required");
        require(royaltyPercentage <= 1000, "ParrotNFT: Royalty too high");
        require(_isValidCategory(category), "ParrotNFT: Invalid category");

        _tokenIdCounter.increment();
        uint256 tokenId = _tokenIdCounter.current();

        // Create NFT metadata
        nftMetadata[tokenId] = NFTMetadata({
            tokenId: tokenId,
            creator: msg.sender,
            name: name,
            description: description,
            imageHash: imageHash,
            animationHash: animationHash,
            createdAt: block.timestamp,
            price: 0,
            isForSale: false,
            isAuction: false,
            auctionEndTime: 0,
            highestBid: 0,
            highestBidder: address(0),
            category: category,
            tags: tags,
            royaltyPercentage: royaltyPercentage
        });

        // Mint NFT
        _safeMint(msg.sender, tokenId);
        
        // Set token URI
        string memory tokenURI = _generateTokenURI(tokenId);
        _setTokenURI(tokenId, tokenURI);

        // Update state
        creatorNFTs[msg.sender].push(tokenId);
        categoryNFTs[category].push(tokenId);
        totalNFTs++;

        emit NFTMinted(tokenId, msg.sender, name, block.timestamp);
    }

    /**
     * @dev List NFT for sale
     * @param tokenId ID of the NFT
     * @param price Sale price in SLW
     */
    function listNFT(uint256 tokenId, uint256 price) external onlyNFTOwner(tokenId) nftExists(tokenId) {
        require(price > 0, "ParrotNFT: Price must be greater than 0");
        require(!nftMetadata[tokenId].isForSale, "ParrotNFT: Already listed");
        require(!nftMetadata[tokenId].isAuction, "ParrotNFT: Cannot list auctioned NFT");

        nftMetadata[tokenId].price = price;
        nftMetadata[tokenId].isForSale = true;

        emit NFTListed(tokenId, price, block.timestamp);
    }

    /**
     * @dev Buy NFT
     * @param tokenId ID of the NFT to buy
     */
    function buyNFT(uint256 tokenId) external hasProfile nftExists(tokenId) nonReentrant {
        NFTMetadata storage metadata = nftMetadata[tokenId];
        require(metadata.isForSale, "ParrotNFT: NFT not for sale");
        require(msg.sender != ownerOf(tokenId), "ParrotNFT: Cannot buy own NFT");

        uint256 price = metadata.price;
        address seller = ownerOf(tokenId);
        address creator = metadata.creator;

        // Calculate fees
        uint256 platformFeeAmount = (price * platformFee) / FEE_DENOMINATOR;
        uint256 creatorRoyalty = (price * metadata.royaltyPercentage) / FEE_DENOMINATOR;
        uint256 sellerAmount = price - platformFeeAmount - creatorRoyalty;

        // Transfer SLW tokens
        require(sulweToken.transferFrom(msg.sender, address(this), price), "ParrotNFT: Transfer failed");
        
        // Distribute payments
        if (seller != creator) {
            sulweToken.transfer(seller, sellerAmount);
            sulweToken.transfer(creator, creatorRoyalty);
        } else {
            sulweToken.transfer(seller, sellerAmount + creatorRoyalty);
        }

        // Transfer NFT
        _transfer(seller, msg.sender, tokenId);

        // Update metadata
        metadata.isForSale = false;
        metadata.price = 0;

        // Update state
        totalSales++;
        totalVolume += price;

        emit NFTSold(tokenId, seller, msg.sender, price, block.timestamp);
    }

    /**
     * @dev Create auction for NFT
     * @param tokenId ID of the NFT
     * @param startPrice Starting price
     * @param duration Auction duration in seconds
     */
    function createAuction(
        uint256 tokenId,
        uint256 startPrice,
        uint256 duration
    ) external onlyNFTOwner(tokenId) nftExists(tokenId) {
        require(startPrice > 0, "ParrotNFT: Start price must be greater than 0");
        require(duration >= 3600, "ParrotNFT: Duration too short");
        require(duration <= 604800, "ParrotNFT: Duration too long");
        require(!nftMetadata[tokenId].isForSale, "ParrotNFT: NFT is listed for sale");
        require(!nftMetadata[tokenId].isAuction, "ParrotNFT: Auction already exists");

        nftMetadata[tokenId].isAuction = true;
        nftMetadata[tokenId].auctionEndTime = block.timestamp + duration;

        auctions[tokenId] = Auction({
            tokenId: tokenId,
            startPrice: startPrice,
            currentPrice: startPrice,
            startTime: block.timestamp,
            endTime: block.timestamp + duration,
            highestBidder: address(0),
            highestBid: 0,
            isActive: true
        });

        emit NFTAuctionCreated(tokenId, startPrice, block.timestamp + duration, block.timestamp);
    }

    /**
     * @dev Place bid on auction
     * @param tokenId ID of the NFT
     * @param bidAmount Bid amount
     */
    function placeBid(uint256 tokenId, uint256 bidAmount) external hasProfile auctionExists(tokenId) nonReentrant {
        Auction storage auction = auctions[tokenId];
        require(block.timestamp < auction.endTime, "ParrotNFT: Auction ended");
        require(bidAmount > auction.highestBid, "ParrotNFT: Bid too low");
        require(msg.sender != ownerOf(tokenId), "ParrotNFT: Cannot bid on own NFT");

        // Refund previous bidder if exists
        if (auction.highestBidder != address(0)) {
            sulweToken.transfer(auction.highestBidder, auction.highestBid);
        }

        // Transfer new bid
        require(sulweToken.transferFrom(msg.sender, address(this), bidAmount), "ParrotNFT: Transfer failed");

        // Update auction
        auction.highestBidder = msg.sender;
        auction.highestBid = bidAmount;
        auction.currentPrice = bidAmount;

        nftMetadata[tokenId].highestBid = bidAmount;
        nftMetadata[tokenId].highestBidder = msg.sender;

        emit NFTAuctionBid(tokenId, msg.sender, bidAmount, block.timestamp);
    }

    /**
     * @dev End auction
     * @param tokenId ID of the NFT
     */
    function endAuction(uint256 tokenId) external auctionExists(tokenId) {
        Auction storage auction = auctions[tokenId];
        require(block.timestamp >= auction.endTime, "ParrotNFT: Auction not ended");

        if (auction.highestBidder != address(0)) {
            address seller = ownerOf(tokenId);
            address creator = nftMetadata[tokenId].creator;
            uint256 finalPrice = auction.highestBid;

            // Calculate fees
            uint256 platformFeeAmount = (finalPrice * platformFee) / FEE_DENOMINATOR;
            uint256 creatorRoyalty = (finalPrice * nftMetadata[tokenId].royaltyPercentage) / FEE_DENOMINATOR;
            uint256 sellerAmount = finalPrice - platformFeeAmount - creatorRoyalty;

            // Distribute payments
            if (seller != creator) {
                sulweToken.transfer(seller, sellerAmount);
                sulweToken.transfer(creator, creatorRoyalty);
            } else {
                sulweToken.transfer(seller, sellerAmount + creatorRoyalty);
            }

            // Transfer NFT
            _transfer(seller, auction.highestBidder, tokenId);

            // Update metadata
            nftMetadata[tokenId].isAuction = false;
            nftMetadata[tokenId].auctionEndTime = 0;
            nftMetadata[tokenId].highestBid = 0;
            nftMetadata[tokenId].highestBidder = address(0);

            auction.isActive = false;

            // Update state
            totalSales++;
            totalVolume += finalPrice;

            emit NFTAuctionEnded(tokenId, auction.highestBidder, finalPrice, block.timestamp);
        } else {
            // No bids, end auction without sale
            auction.isActive = false;
            nftMetadata[tokenId].isAuction = false;
        }
    }

    /**
     * @dev Delist NFT from sale
     * @param tokenId ID of the NFT
     */
    function delistNFT(uint256 tokenId) external onlyNFTOwner(tokenId) nftExists(tokenId) {
        require(nftMetadata[tokenId].isForSale, "ParrotNFT: NFT not listed");

        nftMetadata[tokenId].isForSale = false;
        nftMetadata[tokenId].price = 0;

        emit NFTDelisted(tokenId, block.timestamp);
    }

    /**
     * @dev Get NFT metadata
     * @param tokenId ID of the NFT
     * @return Metadata structure
     */
    function getNFTMetadata(uint256 tokenId) external view nftExists(tokenId) returns (
        uint256 id,
        address creator,
        string memory name,
        string memory description,
        string memory imageHash,
        string memory animationHash,
        uint256 createdAt,
        uint256 price,
        bool isForSale,
        bool isAuction,
        uint256 auctionEndTime,
        uint256 highestBid,
        address highestBidder,
        string memory category,
        uint256 royaltyPercentage
    ) {
        NFTMetadata storage metadata = nftMetadata[tokenId];
        return (
            metadata.tokenId,
            metadata.creator,
            metadata.name,
            metadata.description,
            metadata.imageHash,
            metadata.animationHash,
            metadata.createdAt,
            metadata.price,
            metadata.isForSale,
            metadata.isAuction,
            metadata.auctionEndTime,
            metadata.highestBid,
            metadata.highestBidder,
            metadata.category,
            metadata.royaltyPercentage
        );
    }

    /**
     * @dev Get NFT tags
     * @param tokenId ID of the NFT
     * @return Array of tags
     */
    function getNFTTags(uint256 tokenId) external view nftExists(tokenId) returns (string[] memory) {
        return nftMetadata[tokenId].tags;
    }

    /**
     * @dev Get creator's NFTs
     * @param creator Address of the creator
     * @return Array of token IDs
     */
    function getCreatorNFTs(address creator) external view returns (uint256[] memory) {
        return creatorNFTs[creator];
    }

    /**
     * @dev Get NFTs by category
     * @param category Category name
     * @return Array of token IDs
     */
    function getNFTsByCategory(string memory category) external view returns (uint256[] memory) {
        return categoryNFTs[category];
    }

    /**
     * @dev Get auction details
     * @param tokenId ID of the NFT
     * @return Auction details
     */
    function getAuction(uint256 tokenId) external view returns (
        uint256 id,
        uint256 startPrice,
        uint256 currentPrice,
        uint256 startTime,
        uint256 endTime,
        address highestBidder,
        uint256 highestBid,
        bool isActive
    ) {
        Auction storage auction = auctions[tokenId];
        return (
            auction.tokenId,
            auction.startPrice,
            auction.currentPrice,
            auction.startTime,
            auction.endTime,
            auction.highestBidder,
            auction.highestBid,
            auction.isActive
        );
    }

    /**
     * @dev Update platform fee (only owner)
     * @param newFee New platform fee
     */
    function updatePlatformFee(uint256 newFee) external onlyOwner {
        require(newFee <= 100, "ParrotNFT: Fee too high");
        platformFee = newFee;
        emit PlatformFeeUpdated(newFee, block.timestamp);
    }

    /**
     * @dev Withdraw platform fees (only owner)
     */
    function withdrawFees() external onlyOwner {
        uint256 balance = sulweToken.balanceOf(address(this));
        require(balance > 0, "ParrotNFT: No fees to withdraw");
        sulweToken.transfer(owner(), balance);
    }

    /**
     * @dev Internal function to generate token URI
     * @param tokenId ID of the token
     * @return Token URI
     */
    function _generateTokenURI(uint256 tokenId) internal view returns (string memory) {
        return string(abi.encodePacked(
            "ipfs://",
            nftMetadata[tokenId].imageHash
        ));
    }

    /**
     * @dev Internal function to check if category is valid
     * @param category Category to check
     * @return True if valid
     */
    function _isValidCategory(string memory category) internal view returns (bool) {
        for (uint i = 0; i < culturalCategories.length; i++) {
            if (keccak256(bytes(category)) == keccak256(bytes(culturalCategories[i]))) {
                return true;
            }
        }
        return false;
    }

    /**
     * @dev Required overrides
     */
    function _beforeTokenTransfer(
        address from,
        address to,
        uint256 tokenId,
        uint256 batchSize
    ) internal virtual override(ERC721, ERC721Enumerable) {
        super._beforeTokenTransfer(from, to, tokenId, batchSize);
    }

    function _burn(uint256 tokenId) internal virtual override(ERC721, ERC721URIStorage) {
        super._burn(tokenId);
    }

    function tokenURI(uint256 tokenId) public view virtual override(ERC721, ERC721URIStorage) returns (string memory) {
        return super.tokenURI(tokenId);
    }

    function supportsInterface(bytes4 interfaceId) public view virtual override(ERC721, ERC721Enumerable) returns (bool) {
        return super.supportsInterface(interfaceId);
    }
} 