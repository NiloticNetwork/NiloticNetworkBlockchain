#!/usr/bin/env python3
"""
Simple Sample Wallet Creator for Nilotic Blockchain
Creates a wallet and loads it with tokens using the current miner
"""

import hashlib
import subprocess
import json
import time
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.backends import default_backend

def generate_wallet():
    """Generate a new wallet with private/public key pair"""
    
    # Generate RSA private key
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
        backend=default_backend()
    )
    
    # Get public key
    public_key = private_key.public_key()
    
    # Serialize keys
    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )
    
    public_pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )
    
    # Generate address from public key
    address_hash = hashlib.sha256(public_pem).hexdigest()[:32]
    address = f"NIL{address_hash}"
    
    return {
        'address': address,
        'private_key': private_pem.decode('utf-8'),
        'public_key': public_pem.decode('utf-8')
    }

def run_command(command):
    """Run a shell command and return the result"""
    try:
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except Exception as e:
        return "", str(e), 1

def get_current_miner():
    """Get the current miner address from the blockchain"""
    print("🔍 Getting current miner address...")
    stdout, stderr, code = run_command("curl -s http://localhost:5500/info")
    
    if code == 0:
        try:
            data = json.loads(stdout)
            # Get the latest block to see who mined it
            stdout2, stderr2, code2 = run_command("curl -s http://localhost:5500/block/latest")
            if code2 == 0:
                block_data = json.loads(stdout2)
                # Look for coinbase transaction
                for tx in block_data.get('transactions', []):
                    if isinstance(tx, str):
                        try:
                            tx_data = json.loads(tx)
                            if tx_data.get('sender') == 'COINBASE':
                                return tx_data.get('recipient')
                        except:
                            continue
            return "NILe2e477799a949869427c81e8bf2614c454"  # Default current miner
        except:
            return "NILe2e477799a949869427c81e8bf2614c454"
    return "NILe2e477799a949869427c81e8bf2614c454"

def check_balance(address):
    """Check balance of an address"""
    print(f"💰 Checking balance for {address}...")
    
    command = f'curl -s http://localhost:5500/balance/{address}'
    stdout, stderr, code = run_command(command)
    
    if code == 0:
        try:
            data = json.loads(stdout)
            balance = data.get('balance', 0)
            print(f"✅ Balance: {balance} NIL")
            return balance
        except:
            print("❌ Invalid response from balance endpoint")
            return 0
    else:
        print(f"❌ Balance check failed: {stderr}")
        return 0

def send_transaction(from_address, to_address, amount):
    """Send a transaction"""
    print(f"💸 Sending {amount} NIL from {from_address} to {to_address}...")
    
    command = f'curl -X POST http://localhost:5500/transaction -H "Content-Type: application/json" -d \'{{"sender": "{from_address}", "recipient": "{to_address}", "amount": {amount}}}\''
    stdout, stderr, code = run_command(command)
    
    if code == 0:
        try:
            data = json.loads(stdout)
            if data.get('status') == 'success':
                print(f"✅ Transaction sent successfully! ID: {data.get('transaction_id', 'Unknown')}")
                return True
            else:
                print(f"❌ Transaction failed: {data.get('error', 'Unknown error')}")
                return False
        except:
            print("❌ Invalid response from transaction endpoint")
            return False
    else:
        print(f"❌ Transaction command failed: {stderr}")
        return False

def mine_block(miner_address):
    """Mine a block to get rewards"""
    print(f"⛏️  Mining block for {miner_address}...")
    
    command = f'curl -X POST http://localhost:5500/mine -H "Content-Type: application/json" -d \'{{"miner_address": "{miner_address}"}}\''
    stdout, stderr, code = run_command(command)
    
    if code == 0:
        try:
            data = json.loads(stdout)
            if data.get('status') == 'success':
                print(f"✅ Block mined successfully! Reward: {data.get('reward', 0)} NIL")
                return True
            else:
                print(f"❌ Mining failed: {data.get('error', 'Unknown error')}")
                return False
        except:
            print("❌ Invalid response from mining endpoint")
            return False
    else:
        print(f"❌ Mining command failed: {stderr}")
        return False

def main():
    print("🔐 Nilotic Blockchain Sample Wallet Creator")
    print("=" * 60)
    
    # Get current miner
    current_miner = get_current_miner()
    print(f"⛏️  Current miner: {current_miner}")
    
    # Check miner balance
    miner_balance = check_balance(current_miner)
    
    if miner_balance < 50:
        print(f"\n⛏️  Miner has insufficient balance ({miner_balance} NIL). Mining a block...")
        if mine_block(current_miner):
            miner_balance = check_balance(current_miner)
        else:
            print("❌ Failed to mine block")
            return
    
    print("\n" + "=" * 60)
    
    # Generate sample wallet
    print("🎯 Generating sample wallet...")
    wallet = generate_wallet()
    
    print(f"✅ Wallet created successfully!")
    print(f"📋 Address: {wallet['address']}")
    print(f"🔑 Private Key: {wallet['private_key'][:50]}...")
    
    print("\n" + "=" * 60)
    
    # Check initial balance
    initial_balance = check_balance(wallet['address'])
    
    print("\n" + "=" * 60)
    
    # Send coins to the sample wallet
    amount_to_send = 25.0
    print(f"💸 Sending {amount_to_send} NIL to sample wallet...")
    
    if send_transaction(current_miner, wallet['address'], amount_to_send):
        # Mine another block to process the transaction
        print(f"\n⛏️  Mining block to process transaction...")
        if mine_block(current_miner):
            # Check final balances
            print("\n" + "=" * 60)
            print("📊 Final Balances:")
            print("-" * 30)
            
            final_miner_balance = check_balance(current_miner)
            final_wallet_balance = check_balance(wallet['address'])
            
            print("\n" + "=" * 60)
            print("🎉 Sample Wallet Setup Complete!")
            print(f"📋 Sample Wallet Address: {wallet['address']}")
            print(f"💰 Sample Wallet Balance: {final_wallet_balance} NIL")
            print(f"⛏️  Miner Balance: {final_miner_balance} NIL")
            
            # Save wallet info to file
            wallet_info = {
                'address': wallet['address'],
                'private_key': wallet['private_key'],
                'public_key': wallet['public_key'],
                'balance': final_wallet_balance,
                'created_at': time.strftime('%Y-%m-%d %H:%M:%S')
            }
            
            with open('sample_wallet.json', 'w') as f:
                json.dump(wallet_info, f, indent=2)
            
            print(f"\n💾 Wallet info saved to: sample_wallet.json")
            print("\n🔐 Private Key (keep secure):")
            print("-" * 50)
            print(wallet['private_key'])
            
        else:
            print("❌ Failed to mine block for transaction processing")
    else:
        print("❌ Failed to send transaction to sample wallet")

if __name__ == "__main__":
    main() 