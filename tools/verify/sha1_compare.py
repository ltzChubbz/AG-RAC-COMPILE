#!/usr/bin/env python3
"""
AG-RAC SHA1 Verification Tool
==============================
Compares sections of the original ELF against the newly compiled MIPS ELF.
Called automatically by CMake after a MIPS target build.

Usage:
    python sha1_compare.py <original_elf> <compiled_elf>

Returns exit code 0 on match, 1 on mismatch.
"""

import sys
import hashlib
import struct
from pathlib import Path


def read_elf_sections(path: Path) -> dict:
    """Read section data from an ELF file, keyed by section name."""
    data = path.read_bytes()
    if data[:4] != b"\x7fELF":
        raise ValueError(f"Not a valid ELF: {path}")

    ei_data = data[5]
    endian = "<" if ei_data == 1 else ">"

    # ELF header
    vals = struct.unpack_from(f"{endian}HHIIIIIHHHHHH", data, 16)
    e_shoff    = vals[5]
    e_shentsize = vals[10]
    e_shnum    = vals[11]
    e_shstrndx = vals[12]

    # Read raw section headers
    raw = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        raw.append(struct.unpack_from(f"{endian}IIIIIIIIII", data, off))

    # Get section name string table
    shstr_off = raw[e_shstrndx][4]
    shstr_sz  = raw[e_shstrndx][5]
    strtab = data[shstr_off:shstr_off + shstr_sz]

    sections = {}
    for i, r in enumerate(raw):
        name_off = r[0]
        end = strtab.index(b"\x00", name_off)
        name = strtab[name_off:end].decode("ascii", errors="replace")
        sh_type = r[1]
        sh_offset = r[4]
        sh_size = r[5]
        if sh_type not in (0, 8):  # skip NULL and NOBITS (.bss)
            sections[name] = data[sh_offset:sh_offset + sh_size]

    return sections


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <original_elf> <compiled_elf>")
        sys.exit(1)

    original_path = Path(sys.argv[1])
    compiled_path = Path(sys.argv[2])

    if not original_path.exists():
        print(f"ERROR: Original ELF not found: {original_path}")
        print("  Run 'python tools/iso_extract/iso_extract.py' first")
        sys.exit(1)

    if not compiled_path.exists():
        print(f"ERROR: Compiled ELF not found: {compiled_path}")
        sys.exit(1)

    try:
        original_sections = read_elf_sections(original_path)
        compiled_sections = read_elf_sections(compiled_path)
    except Exception as e:
        print(f"ERROR: Failed to parse ELF: {e}")
        sys.exit(1)

    print(f"\nAG-RAC SHA1 Verification")
    print(f"{'='*60}")
    print(f"  Original : {original_path.name}")
    print(f"  Compiled : {compiled_path.name}")
    print(f"{'='*60}")

    # Compare sections
    compare_sections = [".text", ".data", ".rodata"]
    all_match = True

    for section in compare_sections:
        if section not in original_sections:
            print(f"  {'?':>8}  {section:<12}  (not in original)")
            continue
        if section not in compiled_sections:
            print(f"  {'?':>8}  {section:<12}  (not in compiled)")
            continue

        orig_hash = sha1(original_sections[section])
        comp_hash = sha1(compiled_sections[section])
        match = orig_hash == comp_hash

        status = "✓ MATCH" if match else "✗ MISMATCH"
        print(f"  {status}  {section:<12}  {orig_hash[:16]}...")
        if not match:
            all_match = False
            size_orig = len(original_sections[section])
            size_comp = len(compiled_sections[section])
            print(f"             Original size: {size_orig:,} bytes  |  Compiled size: {size_comp:,} bytes")

    print(f"{'='*60}")
    if all_match:
        print(f"  RESULT: ✓ All sections match — decompilation is correct!")
        sys.exit(0)
    else:
        print(f"  RESULT: ✗ Sections do not match — continue refining C source")
        sys.exit(1)


if __name__ == "__main__":
    main()
