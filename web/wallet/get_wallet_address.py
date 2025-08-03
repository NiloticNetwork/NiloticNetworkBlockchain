#!/usr/bin/env python3
"""
Get Wallet Address for Mining
A simple script to create or get a wallet address for mining
"""

import requests
import json
import sys

def create_wallet(name="miner_wallet", password="miner_password"):
    """Create a new wallet for mining"""
    try:
        response = requests.post(
            "http://localhost:5500/wallet/create",
            json={"name": name, "password": password},
            headers={"Content-Type": "application/json"}
        )
        
        if response.status_code == 200:
            data = response.json()
            if data.get('status') == 'success':
                return data.get('address')
            else:
                print(f"Failed to create wallet: {data.get('error', 'Unknown error')}")
                return None
        else:
            print(f"Failed to create wallet: HTTP {response.status_code}")
            return None
            
    except requests.RequestException as e:
        print(f"Connection error: {e}")
        return None

def get_blockchain_info():
    """Get blockchain info to check if server is running"""
    try:
        response = requests.get("http://localhost:5500/info")
        if response.status_code == 200:
            return response.json()
        else:
            return None
    except requests.RequestException:
        return None

def main():
    print("🔍 Checking blockchain server...")
    
    info = get_blockchain_info()
    if not info:
        print("❌ Cannot connect to blockchain server at http://localhost:5500")
        print("Please make sure the blockchain server is running.")
        sys.exit(1)
    
    print("✅ Blockchain server is running")
    print(f"📊 Chain Height: {info.get('chainHeight', 0)}")
    
    print("\n🔑 Creating wallet for mining...")
    address = create_wallet()
    
    if address:
        print(f"✅ Wallet created successfully!")
        print(f"📝 Address: {address}")
        print(f"\n🚀 To start mining, run:")
        print(f"python miner.py --address {address}")
        print(f"\nOr with custom interval:")
        print(f"python miner.py --address {address} --interval 5")
    else:
        print("❌ Failed to create wallet")
        sys.exit(1)

if __name__ == "__main__":
    main() 