const { PrismaClient } = require('@prisma/client');

const prisma = new PrismaClient();

async function testWalletDatabase() {
  try {
    console.log('🔍 Testing wallet database functionality...\n');

    // Test 1: Check if we can connect to database
    console.log('1. Testing database connection...');
    await prisma.$connect();
    console.log('✅ Database connected successfully\n');

    // Test 2: Check existing wallets
    console.log('2. Checking existing wallets...');
    const wallets = await prisma.userWallet.findMany({
      take: 5,
      orderBy: { createdAt: 'desc' }
    });
    
    console.log(`Found ${wallets.length} wallets in database:`);
    wallets.forEach((wallet, index) => {
      console.log(`  ${index + 1}. ${wallet.name} (${wallet.address}) - Type: ${wallet.type}`);
    });
    console.log('');

    // Test 3: Check users
    console.log('3. Checking users...');
    const users = await prisma.user.findMany({
      take: 5,
      include: {
        wallets: true
      }
    });
    
    console.log(`Found ${users.length} users in database:`);
    users.forEach((user, index) => {
      console.log(`  ${index + 1}. ${user.email} - Wallets: ${user.wallets.length}`);
    });
    console.log('');

    // Test 4: Create a test wallet
    console.log('4. Creating test wallet...');
    const testWallet = await prisma.userWallet.create({
      data: {
        address: 'NILtest123456789',
        name: 'Test Wallet',
        type: 'test',
        isPrimary: false,
        userId: users[0]?.id || 'test-user-id'
      }
    });
    console.log(`✅ Created test wallet: ${testWallet.name} (${testWallet.address})`);
    console.log('');

    // Test 5: Verify wallet was created
    console.log('5. Verifying wallet creation...');
    const createdWallet = await prisma.userWallet.findUnique({
      where: { id: testWallet.id }
    });
    
    if (createdWallet) {
      console.log('✅ Wallet successfully saved to database');
      console.log(`   - ID: ${createdWallet.id}`);
      console.log(`   - Name: ${createdWallet.name}`);
      console.log(`   - Address: ${createdWallet.address}`);
      console.log(`   - Type: ${createdWallet.type}`);
    } else {
      console.log('❌ Wallet not found in database');
    }

  } catch (error) {
    console.error('❌ Error testing wallet database:', error);
  } finally {
    await prisma.$disconnect();
    console.log('\n🔌 Database disconnected');
  }
}

testWalletDatabase(); 