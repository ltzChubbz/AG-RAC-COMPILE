#!/usr/bin/env python3
"""
disasm.py
Wraps MIPS objdump to extract the raw assembly of a specific function
using start and end addresses. Useful for comparison with custom compiled code.

Usage: 
    python disasm.py <elf_path> <start_addr_hex> <stop_addr_hex>
"""

import sys
import subprocess
import os

# Default compiler prefix - user can change to 'mipsel-linux-gnu-' or 'ee-' 
OBJDUMP = os.environ.get("MIPS_OBJDUMP", "ee-objdump")

def disassemble_func(elf_path, start_addr, stop_addr):
    # Ensure ee-objdump is available
    try:
        cmd = [
            OBJDUMP,
            "-d",               # disassemble
            "--start-address=" + start_addr,
            "--stop-address=" + stop_addr,
            "-M", "reg-names=numeric", # Standardize register names for diffing
            elf_path
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    except FileNotFoundError:
        print(f"ERROR: Could not find '{OBJDUMP}'.")
        print("Please ensure your MIPS ps2dev or ProDG tools are in your PATH.")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"Objdump failed: {e.stderr}")
        sys.exit(1)

    # Clean the output to just the instructions
    lines = result.stdout.split('\n')
    asm_lines = []
    
    for line in lines:
        line = line.strip()
        # Look for typical objdump lines: " 00100000:   00000000    nop"
        if line and ":" in line and len(line) > 10:
            parts = line.split("\t")
            if len(parts) >= 3:
                instruction = parts[2].strip()
                asm_lines.append(instruction)
                
    return asm_lines

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <elf_path> <start_addr> <stop_addr>")
        sys.exit(1)
        
    start_addr = sys.argv[2]
    if not start_addr.startswith("0x"):
        start_addr = "0x" + start_addr
        
    stop_addr = sys.argv[3]
    if not stop_addr.startswith("0x"):
        stop_addr = "0x" + stop_addr
        
    asm = disassemble_func(sys.argv[1], start_addr, stop_addr)
    for line in asm:
        print(line)
