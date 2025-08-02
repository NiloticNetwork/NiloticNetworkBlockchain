const { ethers } = require("hardhat");

async function main() {
    const [deployer] = await ethers.getSigners();
    console.log("Deploying contracts with account:", deployer.address);
    console.log("Account balance:", (await deployer.getBalance()).toString());

    // Deploy SulweToken first
    console.log("\nDeploying SulweToken...");
    const SulweToken = await ethers.getContractFactory("SulweToken");
    const initialSupply = ethers.utils.parseEther("1000000000"); // 1 billion SLW
    const initialRewardPool = ethers.utils.parseEther("10000000"); // 10 million SLW for staking rewards
    const sulweToken = await SulweToken.deploy(initialSupply, initialRewardPool);
    await sulweToken.deployed();
    console.log("SulweToken deployed to:", sulweToken.address);

    // Deploy ParrotProfile
    console.log("\nDeploying ParrotProfile...");
    const ParrotProfile = await ethers.getContractFactory("ParrotProfile");
    const parrotProfile = await ParrotProfile.deploy();
    await parrotProfile.deployed();
    console.log("ParrotProfile deployed to:", parrotProfile.address);

    // Deploy ParrotPosts
    console.log("\nDeploying ParrotPosts...");
    const ParrotPosts = await ethers.getContractFactory("ParrotPosts");
    const parrotPosts = await ParrotPosts.deploy(sulweToken.address, parrotProfile.address);
    await parrotPosts.deployed();
    console.log("ParrotPosts deployed to:", parrotPosts.address);

    // Deploy ParrotNFT
    console.log("\nDeploying ParrotNFT...");
    const ParrotNFT = await ethers.getContractFactory("ParrotNFT");
    const parrotNFT = await ParrotNFT.deploy(sulweToken.address, parrotProfile.address);
    await parrotNFT.deployed();
    console.log("ParrotNFT deployed to:", parrotNFT.address);

    // Deploy ParrotDAO
    console.log("\nDeploying ParrotDAO...");
    const ParrotDAO = await ethers.getContractFactory("ParrotDAO");
    const parrotDAO = await ParrotDAO.deploy(sulweToken.address);
    await parrotDAO.deployed();
    console.log("ParrotDAO deployed to:", parrotDAO.address);

    // Grant minting role to ParrotPosts contract
    console.log("\nGranting minting role to ParrotPosts...");
    await sulweToken.grantRole(await sulweToken.MINTER_ROLE(), parrotPosts.address);
    console.log("Minting role granted to ParrotPosts");

    // Grant minting role to ParrotNFT contract
    console.log("\nGranting minting role to ParrotNFT...");
    await sulweToken.grantRole(await sulweToken.MINTER_ROLE(), parrotNFT.address);
    console.log("Minting role granted to ParrotNFT");

    // Transfer ownership of contracts to deployer
    console.log("\nSetting contract ownership...");
    await parrotProfile.transferOwnership(deployer.address);
    await parrotPosts.transferOwnership(deployer.address);
    await parrotNFT.transferOwnership(deployer.address);
    await parrotDAO.transferOwnership(deployer.address);
    console.log("Ownership set to deployer");

    // Deploy some initial SLW to contracts for rewards
    console.log("\nSetting up initial token distribution...");
    const postsRewardPool = ethers.utils.parseEther("1000000"); // 1M SLW for posts
    const nftRewardPool = ethers.utils.parseEther("500000"); // 500K SLW for NFTs
    const daoRewardPool = ethers.utils.parseEther("250000"); // 250K SLW for DAO

    await sulweToken.transfer(parrotPosts.address, postsRewardPool);
    await sulweToken.transfer(parrotNFT.address, nftRewardPool);
    await sulweToken.transfer(parrotDAO.address, daoRewardPool);

    console.log("Initial token distribution completed");

    // Verify contracts on explorer (if supported)
    console.log("\nWaiting for block confirmations...");
    await sulweToken.deployTransaction.wait(6);
    await parrotProfile.deployTransaction.wait(6);
    await parrotPosts.deployTransaction.wait(6);
    await parrotNFT.deployTransaction.wait(6);
    await parrotDAO.deployTransaction.wait(6);

    console.log("\n=== DEPLOYMENT SUMMARY ===");
    console.log("Network:", network.name);
    console.log("Deployer:", deployer.address);
    console.log("\nContract Addresses:");
    console.log("SulweToken:", sulweToken.address);
    console.log("ParrotProfile:", parrotProfile.address);
    console.log("ParrotPosts:", parrotPosts.address);
    console.log("ParrotNFT:", parrotNFT.address);
    console.log("ParrotDAO:", parrotDAO.address);
    console.log("\nInitial Token Distribution:");
    console.log("Total Supply:", ethers.utils.formatEther(initialSupply), "SLW");
    console.log("Staking Reward Pool:", ethers.utils.formatEther(initialRewardPool), "SLW");
    console.log("Posts Reward Pool:", ethers.utils.formatEther(postsRewardPool), "SLW");
    console.log("NFT Reward Pool:", ethers.utils.formatEther(nftRewardPool), "SLW");
    console.log("DAO Reward Pool:", ethers.utils.formatEther(daoRewardPool), "SLW");

    // Save deployment info
    const deploymentInfo = {
        network: network.name,
        deployer: deployer.address,
        contracts: {
            sulweToken: sulweToken.address,
            parrotProfile: parrotProfile.address,
            parrotPosts: parrotPosts.address,
            parrotNFT: parrotNFT.address,
            parrotDAO: parrotDAO.address
        },
        tokenInfo: {
            totalSupply: ethers.utils.formatEther(initialSupply),
            stakingRewardPool: ethers.utils.formatEther(initialRewardPool),
            postsRewardPool: ethers.utils.formatEther(postsRewardPool),
            nftRewardPool: ethers.utils.formatEther(nftRewardPool),
            daoRewardPool: ethers.utils.formatEther(daoRewardPool)
        },
        deploymentTime: new Date().toISOString()
    };

    const fs = require('fs');
    fs.writeFileSync(
        `deployment-${network.name}.json`,
        JSON.stringify(deploymentInfo, null, 2)
    );
    console.log(`\nDeployment info saved to deployment-${network.name}.json`);

    // Verify contracts if on supported network
    if (network.name !== "hardhat" && network.name !== "localhost") {
        console.log("\nVerifying contracts on explorer...");
        try {
            await hre.run("verify:verify", {
                address: sulweToken.address,
                constructorArguments: [initialSupply, initialRewardPool],
            });
        } catch (error) {
            console.log("SulweToken verification failed:", error.message);
        }

        try {
            await hre.run("verify:verify", {
                address: parrotProfile.address,
                constructorArguments: [],
            });
        } catch (error) {
            console.log("ParrotProfile verification failed:", error.message);
        }

        try {
            await hre.run("verify:verify", {
                address: parrotPosts.address,
                constructorArguments: [sulweToken.address, parrotProfile.address],
            });
        } catch (error) {
            console.log("ParrotPosts verification failed:", error.message);
        }

        try {
            await hre.run("verify:verify", {
                address: parrotNFT.address,
                constructorArguments: [sulweToken.address, parrotProfile.address],
            });
        } catch (error) {
            console.log("ParrotNFT verification failed:", error.message);
        }

        try {
            await hre.run("verify:verify", {
                address: parrotDAO.address,
                constructorArguments: [sulweToken.address],
            });
        } catch (error) {
            console.log("ParrotDAO verification failed:", error.message);
        }
    }

    console.log("\n=== DEPLOYMENT COMPLETE ===");
    console.log("All contracts deployed successfully!");
    console.log("Next steps:");
    console.log("1. Update frontend configuration with contract addresses");
    console.log("2. Update backend configuration with contract addresses");
    console.log("3. Test contract interactions");
    console.log("4. Deploy frontend and backend applications");
}

main()
    .then(() => process.exit(0))
    .catch((error) => {
        console.error(error);
        process.exit(1);
    }); 