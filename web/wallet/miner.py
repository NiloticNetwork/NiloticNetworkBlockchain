#!/usr/bin/env python3
"""
Nilotic Blockchain Miner
A continuous mining script that validates transactions and creates new blocks
"""

import requests
import time
import json
import sys
from datetime import datetime
from typing import Dict, Any, Optional

class NiloticMiner:
    """Miner for the Nilotic Blockchain"""
    
    def __init__(self, blockchain_url: str = "http://localhost:5500", miner_address: str = None):
        """Initialize the miner with blockchain URL and miner address"""
        self.blockchain_url = blockchain_url
        self.miner_address = miner_address
        self.is_mining = False
        self.blocks_mined = 0
        self.start_time = None
        
    def check_connection(self) -> bool:
        """Check if blockchain server is accessible"""
        try:
            response = requests.get(f"{self.blockchain_url}/info", timeout=5)
            return response.status_code == 200
        except requests.RequestException:
            return False
    
    def get_blockchain_info(self) -> Dict[str, Any]:
        """Get current blockchain information"""
        try:
            response = requests.get(f"{self.blockchain_url}/info")
            return response.json()
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to get blockchain info: {e}")
    
    def get_pending_transactions(self) -> list:
        """Get pending transactions that need to be mined"""
        try:
            response = requests.get(f"{self.blockchain_url}/pending")
            return response.json().get('transactions', [])
        except requests.RequestException:
            # If pending endpoint doesn't exist, return empty list
            return []
    
    def mine_block(self) -> Dict[str, Any]:
        """Mine a new block with pending transactions"""
        if not self.miner_address:
            raise ValueError("No miner address specified")
        
        try:
            payload = {
                "miner_address": self.miner_address,
                "timestamp": int(time.time() * 1000)
            }
            
            response = requests.post(
                f"{self.blockchain_url}/mine",
                json=payload,
                headers={"Content-Type": "application/json"}
            )
            
            if response.status_code == 200:
                result = response.json()
                self.blocks_mined += 1
                return result
            else:
                raise Exception(f"Mining failed with status {response.status_code}: {response.text}")
                
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to mine block: {e}")
    
    def start_mining(self, interval: int = 10) -> None:
        """Start continuous mining with specified interval"""
        if not self.miner_address:
            print("❌ Error: No miner address specified. Use --address to set miner address.")
            return
        
        print(f"🚀 Starting miner with address: {self.miner_address}")
        print(f"🔗 Blockchain URL: {self.blockchain_url}")
        print(f"⏱️  Mining interval: {interval} seconds")
        print("=" * 60)
        
        self.is_mining = True
        self.start_time = datetime.now()
        
        try:
            while self.is_mining:
                try:
                    # Check connection
                    if not self.check_connection():
                        print(f"❌ {datetime.now().strftime('%H:%M:%S')} - Cannot connect to blockchain server")
                        time.sleep(interval)
                        continue
                    
                    # Get blockchain info
                    info = self.get_blockchain_info()
                    chain_height = info.get('chainHeight', 0)
                    pending_tx = len(self.get_pending_transactions())
                    
                    print(f"📊 {datetime.now().strftime('%H:%M:%S')} - Chain Height: {chain_height}, Pending TX: {pending_tx}")
                    
                    # Mine block if there are pending transactions or every few intervals
                    if pending_tx > 0 or self.blocks_mined % 5 == 0:
                        print(f"⛏️  {datetime.now().strftime('%H:%M:%S')} - Mining new block...")
                        
                        result = self.mine_block()
                        
                        if result.get('success') or result.get('status') == 'success':
                            block_hash = result.get('block', {}).get('hash', 'unknown')
                            print(f"✅ {datetime.now().strftime('%H:%M:%S')} - Block mined successfully! Hash: {block_hash[:16]}...")
                        else:
                            print(f"❌ {datetime.now().strftime('%H:%M:%S')} - Mining failed: {result.get('message', 'Unknown error')}")
                    else:
                        print(f"⏳ {datetime.now().strftime('%H:%M:%S')} - No pending transactions, waiting...")
                    
                    # Show mining statistics
                    if self.blocks_mined > 0:
                        elapsed = (datetime.now() - self.start_time).total_seconds()
                        rate = self.blocks_mined / (elapsed / 3600)  # blocks per hour
                        print(f"📈 Mining Stats: {self.blocks_mined} blocks, {rate:.2f} blocks/hour")
                    
                    print("-" * 60)
                    
                except KeyboardInterrupt:
                    print("\n🛑 Mining stopped by user")
                    break
                except Exception as e:
                    print(f"❌ {datetime.now().strftime('%H:%M:%S')} - Error: {e}")
                
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print("\n🛑 Mining stopped by user")
        finally:
            self.stop_mining()
    
    def stop_mining(self) -> None:
        """Stop the mining process"""
        self.is_mining = False
        if self.start_time:
            elapsed = (datetime.now() - self.start_time).total_seconds()
            print(f"\n📊 Final Mining Statistics:")
            print(f"   Blocks mined: {self.blocks_mined}")
            print(f"   Total time: {elapsed:.1f} seconds")
            if elapsed > 0:
                print(f"   Average rate: {self.blocks_mined / (elapsed / 3600):.2f} blocks/hour")
    
    def get_mining_status(self) -> Dict[str, Any]:
        """Get current mining status"""
        try:
            response = requests.get(f"{self.blockchain_url}/mining/status")
            return response.json()
        except requests.RequestException:
            return {"isMining": False, "error": "Cannot connect to mining endpoint"}


def main():
    """Main function for the miner"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Nilotic Blockchain Miner")
    parser.add_argument("--url", default="http://localhost:5500", help="Blockchain API URL")
    parser.add_argument("--address", required=True, help="Miner address (wallet address)")
    parser.add_argument("--interval", type=int, default=10, help="Mining interval in seconds (default: 10)")
    parser.add_argument("--status", action="store_true", help="Check mining status and exit")
    
    args = parser.parse_args()
    
    miner = NiloticMiner(args.url, args.address)
    
    if args.status:
        # Check mining status
        status = miner.get_mining_status()
        print("Mining Status:")
        print(json.dumps(status, indent=2))
        return
    
    # Check connection first
    print("🔍 Checking blockchain connection...")
    if not miner.check_connection():
        print(f"❌ Cannot connect to blockchain server at {args.url}")
        print("Please make sure the blockchain server is running.")
        sys.exit(1)
    
    print("✅ Connected to blockchain server")
    
    # Start mining
    try:
        miner.start_mining(args.interval)
    except KeyboardInterrupt:
        print("\n🛑 Mining stopped by user")
    except Exception as e:
        print(f"❌ Mining error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main() 