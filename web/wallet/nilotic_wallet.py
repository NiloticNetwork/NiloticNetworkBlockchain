#!/usr/bin/env python3
"""
Nilotic Blockchain Wallet - Python Client
A command-line wallet application for interacting with the Nilotic Blockchain
"""

import argparse
import json
import sys
import requests
from typing import Dict, Any, Optional, List
from datetime import datetime

class NiloticWallet:
    """Wallet client for the Nilotic Blockchain"""
    
    def __init__(self, blockchain_url: str = "http://localhost:8080"):
        """Initialize the wallet with the blockchain URL"""
        self.blockchain_url = blockchain_url
        self.current_address = None
    
    def set_address(self, address: str) -> None:
        """Set the current wallet address"""
        self.current_address = address
    
    def get_info(self) -> Dict[str, Any]:
        """Get blockchain info"""
        try:
            response = requests.get(f"{self.blockchain_url}/")
            return response.json()
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to connect to blockchain: {e}")
    
    def get_balance(self, address: Optional[str] = None) -> Dict[str, Any]:
        """Get the balance of an address"""
        addr = address or self.current_address
        if not addr:
            raise ValueError("No wallet address specified")
        
        try:
            response = requests.get(f"{self.blockchain_url}/balance", params={"address": addr})
            return response.json()
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to get balance: {e}")
    
    def send_transaction(self, recipient: str, amount: float, sender: Optional[str] = None) -> Dict[str, Any]:
        """Send a transaction"""
        send_addr = sender or self.current_address
        if not send_addr:
            raise ValueError("No sender address specified")
        
        try:
            payload = {
                "sender": send_addr,
                "recipient": recipient,
                "amount": amount
            }
            response = requests.post(
                f"{self.blockchain_url}/transaction",
                json=payload,
                headers={"Content-Type": "application/json"}
            )
            return response.json()
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to send transaction: {e}")
    
    def mine_block(self, address: Optional[str] = None) -> Dict[str, Any]:
        """Mine a new block"""
        miner_addr = address or self.current_address
        if not miner_addr:
            raise ValueError("No miner address specified")
        
        try:
            payload = {"address": miner_addr}
            response = requests.post(
                f"{self.blockchain_url}/mine",
                json=payload,
                headers={"Content-Type": "application/json"}
            )
            return response.json()
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to mine block: {e}")
    
    def get_blockchain(self) -> List[Dict[str, Any]]:
        """Get the entire blockchain"""
        try:
            response = requests.get(f"{self.blockchain_url}/chain")
            return response.json()
        except requests.RequestException as e:
            raise ConnectionError(f"Failed to get blockchain: {e}")
    
    def get_transactions(self, address: Optional[str] = None) -> List[Dict[str, Any]]:
        """Get all transactions for a specific address"""
        addr = address or self.current_address
        if not addr:
            raise ValueError("No wallet address specified")
        
        try:
            # This is a client-side implementation since there's no direct endpoint
            chain = self.get_blockchain()
            transactions = []
            
            for block in chain:
                try:
                    if block.get("data") and block["data"] != "Genesis Block":
                        data = json.loads(block["data"])
                        
                        # Check if it's a transaction related to our address
                        if ((data.get("type") == "transaction" and 
                             (data.get("sender") == addr or data.get("recipient") == addr)) or
                            (data.get("type") == "mining_reward" and data.get("miner") == addr)):
                            
                            # Add block index for reference
                            data["block_index"] = block.get("index", "unknown")
                            transactions.append(data)
                except (json.JSONDecodeError, KeyError) as e:
                    # Skip blocks with invalid data
                    pass
            
            return transactions
        except Exception as e:
            raise ConnectionError(f"Failed to get transactions: {e}")

    def get_all_transactions(self) -> List[Dict[str, Any]]:
        """Get all transactions in the blockchain"""
        try:
            # This is a client-side implementation since there's no direct endpoint
            chain = self.get_blockchain()
            transactions = []
            
            for block in chain:
                try:
                    if block.get("data") and block["data"] != "Genesis Block":
                        data = json.loads(block["data"])
                        
                        # Check if it's any type of transaction
                        if data.get("type") in ["transaction", "mining_reward"]:
                            # Add block index for reference
                            data["block_index"] = block.get("index", "unknown")
                            data["block_hash"] = block.get("hash", "unknown")
                            transactions.append(data)
                except (json.JSONDecodeError, KeyError) as e:
                    # Skip blocks with invalid data
                    pass
            
            return transactions
        except Exception as e:
            raise ConnectionError(f"Failed to get all transactions: {e}")


def print_json(data: Any) -> None:
    """Pretty print JSON data"""
    print(json.dumps(data, indent=2))


def main() -> None:
    """Main CLI function"""
    parser = argparse.ArgumentParser(description="Nilotic Blockchain Wallet")
    parser.add_argument("--url", default="http://localhost:5500", help="Blockchain API URL")
    parser.add_argument("--address", help="Wallet address to use")
    
    subparsers = parser.add_subparsers(dest="command", help="Command to execute")
    
    # Info command
    subparsers.add_parser("info", help="Get blockchain info")
    
    # Balance command
    balance_parser = subparsers.add_parser("balance", help="Get wallet balance")
    balance_parser.add_argument("--address", help="Address to check (overrides the global address)")
    
    # Send command
    send_parser = subparsers.add_parser("send", help="Send a transaction")
    send_parser.add_argument("recipient", help="Recipient address")
    send_parser.add_argument("amount", type=float, help="Amount to send")
    send_parser.add_argument("--from", dest="sender", help="Sender address (overrides the global address)")
    
    # Mine command
    mine_parser = subparsers.add_parser("mine", help="Mine a new block")
    mine_parser.add_argument("--address", help="Miner address (overrides the global address)")
    
    # Chain command
    subparsers.add_parser("chain", help="Get the full blockchain")
    
    # Transactions command
    tx_parser = subparsers.add_parser("transactions", help="Get transactions for an address")
    tx_parser.add_argument("--address", help="Address to check (overrides the global address)")
    
    # All transactions command
    subparsers.add_parser("all-transactions", help="Get all transactions in the blockchain")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    wallet = NiloticWallet(args.url)
    
    if args.address:
        wallet.set_address(args.address)
    
    try:
        if args.command == "info":
            info = wallet.get_info()
            print("Blockchain Info:")
            print_json(info)
        
        elif args.command == "balance":
            addr = args.address if hasattr(args, 'address') and args.address else None
            balance = wallet.get_balance(addr)
            print(f"Balance for {addr or wallet.current_address}:")
            print_json(balance)
        
        elif args.command == "send":
            sender = args.sender if hasattr(args, 'sender') and args.sender else None
            result = wallet.send_transaction(args.recipient, args.amount, sender)
            print("Transaction Result:")
            print_json(result)
        
        elif args.command == "mine":
            addr = args.address if hasattr(args, 'address') and args.address else None
            result = wallet.mine_block(addr)
            print("Mining Result:")
            print_json(result)
        
        elif args.command == "chain":
            chain = wallet.get_blockchain()
            print("Blockchain:")
            print_json(chain)
        
        elif args.command == "transactions":
            addr = args.address if hasattr(args, 'address') and args.address else None
            transactions = wallet.get_transactions(addr)
            
            if not transactions:
                print(f"No transactions found for {addr or wallet.current_address}")
                return
            
            print(f"Transactions for {addr or wallet.current_address}:")
            print_json(transactions)
        
        elif args.command == "all-transactions":
            transactions = wallet.get_all_transactions()
            
            if not transactions:
                print("No transactions found in the blockchain")
                return
            
            print(f"All transactions in the blockchain ({len(transactions)} total):")
            print_json(transactions)
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()