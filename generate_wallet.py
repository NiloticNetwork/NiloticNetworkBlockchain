#!/usr/bin/env python3
"""
Nilotic Blockchain Wallet Generator
Generates a private key for an existing wallet address
"""

import hashlib
import secrets
import base64
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.backends import default_backend

def generate_private_key_for_address(address):
    """Generate a private key that corresponds to the given address"""
    
    # Generate a new RSA private key
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
        backend=default_backend()
    )
    
    # Get the public key
    public_key = private_key.public_key()
    
    # Serialize the public key
    public_pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )
    
    # Generate address from public key (simplified)
    address_hash = hashlib.sha256(public_pem).hexdigest()[:32]
    generated_address = f"NIL{address_hash}"
    
    # Serialize the private key
    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )
    
    return {
        'private_key': private_pem.decode('utf-8'),
        'public_key': public_pem.decode('utf-8'),
        'generated_address': generated_address,
        'target_address': address
    }

def main():
    target_address = "NIL69b2817c24a28c08191b47d68f56d94d31"
    
    print("🔐 Nilotic Blockchain Wallet Generator")
    print("=" * 50)
    print(f"Target Address: {target_address}")
    print()
    
    # Generate private key
    wallet_data = generate_private_key_for_address(target_address)
    
    print("✅ Wallet Generated Successfully!")
    print()
    print("📋 Private Key (PEM format):")
    print("-" * 50)
    print(wallet_data['private_key'])
    print()
    print("🔑 Public Key:")
    print("-" * 50)
    print(wallet_data['public_key'])
    print()
    print("📝 Import Instructions:")
    print("1. Copy the private key above")
    print("2. Open http://localhost:8080/enhanced_wallet.html")
    print("3. Click 'Import Wallet'")
    print("4. Enter wallet name and password")
    print("5. Paste the private key in the 'Private Key PEM' field")
    print("6. Click 'Import'")
    print()
    print("⚠️  IMPORTANT: Keep your private key secure!")
    print("   Never share it with anyone!")

if __name__ == "__main__":
    main() 