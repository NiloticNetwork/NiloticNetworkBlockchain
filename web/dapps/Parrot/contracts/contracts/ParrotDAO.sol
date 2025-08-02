// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "@openzeppelin/contracts/access/Ownable.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/utils/Counters.sol";
import "@openzeppelin/contracts/utils/Strings.sol";
import "./SulweToken.sol";

/**
 * @title ParrotDAO
 * @dev DAO governance contract for Parrot platform using SLW tokens
 */
contract ParrotDAO is Ownable, ReentrancyGuard {
    using Counters for Counters.Counter;
    using Strings for string;

    // Proposal structure
    struct Proposal {
        uint256 id;
        address proposer;
        string title;
        string description;
        string action; // "upgrade", "moderation", "revenue", "feature", "parameter"
        uint256 forVotes;
        uint256 againstVotes;
        uint256 abstainVotes;
        uint256 startTime;
        uint256 endTime;
        bool executed;
        bool canceled;
        mapping(address => Receipt) receipts;
    }

    // Vote receipt structure
    struct Receipt {
        bool hasVoted;
        bool support; // true = for, false = against
        uint256 votes;
    }

    // Governance parameters
    struct GovernanceParams {
        uint256 proposalThreshold; // Minimum SLW required to create proposal
        uint256 votingDelay; // Delay before voting starts
        uint256 votingPeriod; // Duration of voting period
        uint256 quorumVotes; // Minimum votes required for proposal to pass
        uint256 executionDelay; // Delay before proposal can be executed
    }

    // State variables
    mapping(uint256 => Proposal) public proposals;
    mapping(address => uint256) public proposalCount;
    mapping(address => uint256) public lastVoteTime;
    
    Counters.Counter private _proposalIdCounter;
    
    uint256 public totalProposals;
    uint256 public totalVotes;
    uint256 public totalExecuted;
    
    // Contract references
    SulweToken public sulweToken;
    
    // Governance parameters
    GovernanceParams public governanceParams;
    
    // Events
    event ProposalCreated(uint256 indexed proposalId, address indexed proposer, string title, uint256 timestamp);
    event ProposalVoted(uint256 indexed proposalId, address indexed voter, bool support, uint256 votes, uint256 timestamp);
    event ProposalExecuted(uint256 indexed proposalId, address indexed executor, uint256 timestamp);
    event ProposalCanceled(uint256 indexed proposalId, address indexed canceler, uint256 timestamp);
    event GovernanceParamsUpdated(uint256 proposalThreshold, uint256 votingDelay, uint256 votingPeriod, uint256 quorumVotes, uint256 executionDelay, uint256 timestamp);

    // Modifiers
    modifier onlyProposer(uint256 proposalId) {
        require(proposals[proposalId].proposer == msg.sender, "ParrotDAO: Not proposer");
        _;
    }

    modifier proposalExists(uint256 proposalId) {
        require(proposals[proposalId].id != 0, "ParrotDAO: Proposal does not exist");
        _;
    }

    modifier proposalActive(uint256 proposalId) {
        require(proposals[proposalId].id != 0, "ParrotDAO: Proposal does not exist");
        require(!proposals[proposalId].canceled, "ParrotDAO: Proposal canceled");
        require(!proposals[proposalId].executed, "ParrotDAO: Proposal already executed");
        _;
    }

    modifier votingActive(uint256 proposalId) {
        require(block.timestamp >= proposals[proposalId].startTime, "ParrotDAO: Voting not started");
        require(block.timestamp <= proposals[proposalId].endTime, "ParrotDAO: Voting ended");
        _;
    }

    modifier canExecute(uint256 proposalId) {
        require(block.timestamp >= proposals[proposalId].endTime + governanceParams.executionDelay, "ParrotDAO: Execution delay not met");
        _;
    }

    /**
     * @dev Constructor
     * @param _sulweToken Address of SLW token contract
     */
    constructor(address _sulweToken) {
        sulweToken = SulweToken(_sulweToken);
        
        // Initialize governance parameters
        governanceParams = GovernanceParams({
            proposalThreshold: 1000 * 1e18, // 1000 SLW
            votingDelay: 1 days,
            votingPeriod: 7 days,
            quorumVotes: 10000 * 1e18, // 10,000 SLW
            executionDelay: 2 days
        });
    }

    /**
     * @dev Create a new proposal
     * @param title Proposal title
     * @param description Proposal description
     * @param action Action type
     */
    function createProposal(
        string memory title,
        string memory description,
        string memory action
    ) external nonReentrant {
        require(bytes(title).length > 0, "ParrotDAO: Title required");
        require(bytes(description).length > 0, "ParrotDAO: Description required");
        require(bytes(action).length > 0, "ParrotDAO: Action required");
        
        uint256 proposerBalance = sulweToken.balanceOf(msg.sender);
        require(proposerBalance >= governanceParams.proposalThreshold, "ParrotDAO: Insufficient balance for proposal");

        _proposalIdCounter.increment();
        uint256 proposalId = _proposalIdCounter.current();

        Proposal storage proposal = proposals[proposalId];
        proposal.id = proposalId;
        proposal.proposer = msg.sender;
        proposal.title = title;
        proposal.description = description;
        proposal.action = action;
        proposal.forVotes = 0;
        proposal.againstVotes = 0;
        proposal.abstainVotes = 0;
        proposal.startTime = block.timestamp + governanceParams.votingDelay;
        proposal.endTime = block.timestamp + governanceParams.votingDelay + governanceParams.votingPeriod;
        proposal.executed = false;
        proposal.canceled = false;

        proposalCount[msg.sender]++;
        totalProposals++;

        emit ProposalCreated(proposalId, msg.sender, title, block.timestamp);
    }

    /**
     * @dev Vote on a proposal
     * @param proposalId ID of the proposal
     * @param support True for yes, false for no
     */
    function vote(uint256 proposalId, bool support) external proposalActive(proposalId) votingActive(proposalId) {
        require(!proposals[proposalId].receipts[msg.sender].hasVoted, "ParrotDAO: Already voted");

        uint256 votes = sulweToken.balanceOf(msg.sender);
        require(votes > 0, "ParrotDAO: No voting power");

        Proposal storage proposal = proposals[proposalId];
        Receipt storage receipt = proposal.receipts[msg.sender];

        receipt.hasVoted = true;
        receipt.support = support;
        receipt.votes = votes;

        if (support) {
            proposal.forVotes += votes;
        } else {
            proposal.againstVotes += votes;
        }

        lastVoteTime[msg.sender] = block.timestamp;
        totalVotes++;

        emit ProposalVoted(proposalId, msg.sender, support, votes, block.timestamp);
    }

    /**
     * @dev Execute a proposal
     * @param proposalId ID of the proposal to execute
     */
    function executeProposal(uint256 proposalId) external proposalActive(proposalId) canExecute(proposalId) {
        Proposal storage proposal = proposals[proposalId];
        
        require(proposal.forVotes + proposal.againstVotes >= governanceParams.quorumVotes, "ParrotDAO: Quorum not met");
        require(proposal.forVotes > proposal.againstVotes, "ParrotDAO: Proposal not passed");

        proposal.executed = true;
        totalExecuted++;

        // Execute the proposal action
        _executeAction(proposal.action, proposalId);

        emit ProposalExecuted(proposalId, msg.sender, block.timestamp);
    }

    /**
     * @dev Cancel a proposal (only proposer)
     * @param proposalId ID of the proposal to cancel
     */
    function cancelProposal(uint256 proposalId) external onlyProposer(proposalId) proposalActive(proposalId) {
        require(block.timestamp < proposals[proposalId].startTime, "ParrotDAO: Voting already started");

        proposals[proposalId].canceled = true;

        emit ProposalCanceled(proposalId, msg.sender, block.timestamp);
    }

    /**
     * @dev Get proposal details
     * @param proposalId ID of the proposal
     * @return Proposal details
     */
    function getProposal(uint256 proposalId) external view proposalExists(proposalId) returns (
        uint256 id,
        address proposer,
        string memory title,
        string memory description,
        string memory action,
        uint256 forVotes,
        uint256 againstVotes,
        uint256 abstainVotes,
        uint256 startTime,
        uint256 endTime,
        bool executed,
        bool canceled
    ) {
        Proposal storage proposal = proposals[proposalId];
        return (
            proposal.id,
            proposal.proposer,
            proposal.title,
            proposal.description,
            proposal.action,
            proposal.forVotes,
            proposal.againstVotes,
            proposal.abstainVotes,
            proposal.startTime,
            proposal.endTime,
            proposal.executed,
            proposal.canceled
        );
    }

    /**
     * @dev Get vote receipt for a user
     * @param proposalId ID of the proposal
     * @param voter Address of the voter
     * @return Vote receipt
     */
    function getReceipt(uint256 proposalId, address voter) external view proposalExists(proposalId) returns (
        bool hasVoted,
        bool support,
        uint256 votes
    ) {
        Receipt storage receipt = proposals[proposalId].receipts[voter];
        return (
            receipt.hasVoted,
            receipt.support,
            receipt.votes
        );
    }

    /**
     * @dev Check if proposal can be executed
     * @param proposalId ID of the proposal
     * @return True if can be executed
     */
    function canExecuteProposal(uint256 proposalId) external view returns (bool) {
        Proposal storage proposal = proposals[proposalId];
        
        if (proposal.canceled || proposal.executed) return false;
        if (block.timestamp < proposal.endTime + governanceParams.executionDelay) return false;
        if (proposal.forVotes + proposal.againstVotes < governanceParams.quorumVotes) return false;
        if (proposal.forVotes <= proposal.againstVotes) return false;
        
        return true;
    }

    /**
     * @dev Get governance parameters
     * @return Governance parameters
     */
    function getGovernanceParams() external view returns (
        uint256 proposalThreshold,
        uint256 votingDelay,
        uint256 votingPeriod,
        uint256 quorumVotes,
        uint256 executionDelay
    ) {
        return (
            governanceParams.proposalThreshold,
            governanceParams.votingDelay,
            governanceParams.votingPeriod,
            governanceParams.quorumVotes,
            governanceParams.executionDelay
        );
    }

    /**
     * @dev Update governance parameters (only owner)
     * @param _proposalThreshold New proposal threshold
     * @param _votingDelay New voting delay
     * @param _votingPeriod New voting period
     * @param _quorumVotes New quorum votes
     * @param _executionDelay New execution delay
     */
    function updateGovernanceParams(
        uint256 _proposalThreshold,
        uint256 _votingDelay,
        uint256 _votingPeriod,
        uint256 _quorumVotes,
        uint256 _executionDelay
    ) external onlyOwner {
        governanceParams.proposalThreshold = _proposalThreshold;
        governanceParams.votingDelay = _votingDelay;
        governanceParams.votingPeriod = _votingPeriod;
        governanceParams.quorumVotes = _quorumVotes;
        governanceParams.executionDelay = _executionDelay;

        emit GovernanceParamsUpdated(_proposalThreshold, _votingDelay, _votingPeriod, _quorumVotes, _executionDelay, block.timestamp);
    }

    /**
     * @dev Internal function to execute proposal actions
     * @param action Action to execute
     * @param proposalId ID of the proposal
     */
    function _executeAction(string memory action, uint256 proposalId) internal {
        // This would contain the actual execution logic for different proposal types
        // For now, it's a placeholder that can be extended based on specific needs
        
        if (keccak256(bytes(action)) == keccak256(bytes("upgrade"))) {
            // Handle contract upgrades
        } else if (keccak256(bytes(action)) == keccak256(bytes("moderation"))) {
            // Handle moderation policy changes
        } else if (keccak256(bytes(action)) == keccak256(bytes("revenue"))) {
            // Handle revenue distribution changes
        } else if (keccak256(bytes(action)) == keccak256(bytes("feature"))) {
            // Handle new feature proposals
        } else if (keccak256(bytes(action)) == keccak256(bytes("parameter"))) {
            // Handle parameter changes
        }
    }

    /**
     * @dev Get voting power for an address
     * @param account Address to check
     * @return Voting power
     */
    function getVotingPower(address account) external view returns (uint256) {
        return sulweToken.balanceOf(account);
    }

    /**
     * @dev Get proposal state
     * @param proposalId ID of the proposal
     * @return State string
     */
    function getProposalState(uint256 proposalId) external view returns (string memory) {
        if (!proposals[proposalId].id != 0) return "Nonexistent";
        if (proposals[proposalId].canceled) return "Canceled";
        if (proposals[proposalId].executed) return "Executed";
        if (block.timestamp < proposals[proposalId].startTime) return "Pending";
        if (block.timestamp <= proposals[proposalId].endTime) return "Active";
        if (proposals[proposalId].forVotes <= proposals[proposalId].againstVotes) return "Defeated";
        if (proposals[proposalId].forVotes + proposals[proposalId].againstVotes < governanceParams.quorumVotes) return "Defeated";
        if (block.timestamp <= proposals[proposalId].endTime + governanceParams.executionDelay) return "Succeeded";
        return "Expired";
    }
} 