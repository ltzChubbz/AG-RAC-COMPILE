#!/usr/bin/env python3
"""
diff.py
Compares the assembly of our compiled object file against the original game ELF.
"""

import sys
import subprocess
from pathlib import Path
import disasm

def colorize(text, color_code):
    return f"\033[{color_code}m{text}\033[0m"

def main():
    if len(sys.argv) < 5:
        print(f"Usage: {sys.argv[0]} <orig_elf> <orig_start> <orig_stop> <our_object>")
        sys.exit(1)

    orig_elf = sys.argv[1]
    orig_start = sys.argv[2]
    orig_stop = sys.argv[3]
    our_obj = sys.argv[4]

    print("Disassembling Original...")
    orig_asm = disasm.disassemble_func(orig_elf, orig_start, orig_stop)

    print("Disassembling Custom Target...")
    # For a .o file, there's usually only one text section per function if using -ffunction-sections, 
    # but we'll try to just disassemble the whole thing and drop the intro.
    # Alternatively, we just use objdump -d without limits for our small test object file.
    our_asm = []
    try:
        cmd = [
            disasm.OBJDUMP,
            "-d", 
            "-M", "reg-names=numeric",
            our_obj
        ]
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        lines = res.stdout.split('\n')
        for line in lines:
            line = line.strip()
            if line and ":" in line and len(line) > 10 and not line.endswith(">:") and "file format" not in line:
                parts = line.split("\t")
                if len(parts) >= 3:
                    instruction = parts[2].strip()
                    our_asm.append(instruction)
    except subprocess.CalledProcessError as e:
        print(f"Failed to disassemble custom object: {e.stderr}")
        sys.exit(1)

    # Diff
    max_lines = max(len(orig_asm), len(our_asm))
    
    print("\n--- DIFF REPORT ---")
    print(f"{'ORIGINAL SCUS_971.99':<40} | {'OUR COMPILED C CODE'}")
    print("-" * 85)
    
    match_count = 0
    for i in range(max_lines):
        orig_inst = orig_asm[i] if i < len(orig_asm) else "<missing>"
        our_inst = our_asm[i] if i < len(our_asm) else "<missing>"
        
        # Super basic string diff - ignore branch target offsets for now
        # because those will resolve differently in object files.
        orig_base = orig_inst.split(',')[0] if ',' in orig_inst else orig_inst
        our_base = our_inst.split(',')[0] if ',' in our_inst else our_inst
        
        if orig_base.split() == our_base.split():
            output = f"{orig_inst:<40} | {our_inst}"
            print(colorize(output, "32")) # Green
            match_count += 1
        else:
            output = f"{orig_inst:<40} | {our_inst}"
            print(colorize(output, "31")) # Red
            
    pct = (match_count / max_lines * 100) if max_lines > 0 else 0
    print("-" * 85)
    print(f"Match: {pct:.2f}% ({match_count}/{max_lines} instructions matched)")
    
    if pct == 100:
        print(colorize("\nPERFECT MATCH! You are ready to drop this into a shared library.", "32"))
    else:
        print(colorize("\nMismatches found. Please tune compiler flags or inline structures.", "33"))
        sys.exit(1)

if __name__ == "__main__":
    main()
