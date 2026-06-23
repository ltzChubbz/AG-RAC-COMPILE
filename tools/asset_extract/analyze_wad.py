#!/usr/bin/env python3
import sys
import os
from pathlib import Path

def analyze_wad(wad_path):
    wad_path = Path(wad_path)
    if not wad_path.exists():
        print(f"Error: {wad_path} does not exist")
        return

    with open(wad_path, 'rb') as f:
        data = f.read(512)
        
    print(f"--- {wad_path.name} ---")
    # Search for common signatures
    signatures = [b'WAD', b'MESH', b'TEX', b'ANIM', b'VAG', b'RGBA', b'VIF', b'GIF']
    found = []
    for sig in signatures:
        pos = data.find(sig)
        if pos != -1:
            found.append(f"{sig.decode()}@0x{pos:X}")
    
    if found:
        print(f"Signatures: {', '.join(found)}")
    else:
        print("No signatures found in header.")
        
    # Hex dump first 64 bytes
    hex_dump = data[:64].hex(' ')
    print(f"Hex: {hex_dump[:47]}...") # First row-ish
    print(f"Hex: {hex_dump[48:95]}...") # Second row-ish

if __name__ == "__main__":
    wads = sorted(Path('assets/wads').glob('*.bin'))
    # Skip the first one if it's entry 0 (often null)
    for w in wads[:10]:
        analyze_wad(w)
