#!/usr/bin/env python3
"""Quick diagnostic: list every file pycdlib can see on the disc."""
import sys
import pycdlib
from pathlib import Path

iso_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(input("ISO path: ").strip().strip('"'))

iso = pycdlib.PyCdlib()
iso.open(str(iso_path))

print(f"Listing all files on: {iso_path.name}\n")
total = 0
for dirpath, dirnames, filenames in iso.walk(iso_path="/"):
    print(f"[DIR] {dirpath}  (subdirs: {len(dirnames)}, files: {len(filenames)})")
    for f in filenames:
        total += 1
        print(f"       {f}")

print(f"\nTotal files visible in ISO filesystem: {total}")
iso.close()
