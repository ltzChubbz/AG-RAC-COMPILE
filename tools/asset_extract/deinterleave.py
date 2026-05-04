#!/usr/bin/env python3
"""
deinterleave.py
Deinterleaves a raw PS2 sector-based WAD archive into individual files
using the Table of Contents (TOC) from the game ELF.
"""

import sys
import struct
import os
from pathlib import Path

# PS2 sector size
SECTOR_SIZE = 2048

def get_toc(elf_path, toc_offset=0x942C0, count=141):
    """Read the TOC from the ELF binary."""
    with open(elf_path, 'rb') as f:
        f.seek(toc_offset)
        data = f.read(count * 16)
    
    toc = []
    for i in range(count):
        # Entry: { LBA, SizeInBytes, Word2, Word3 }
        lba, size, w2, w3 = struct.unpack_from('<IIII', data, i * 16)
        toc.append({'idx': i, 'lba': lba, 'size': size})
    return toc

def deinterleave_block(master_wad_path, toc, out_dir):
    """
    Deinterleaves files based on the TOC groupings.
    Grouping Logic: Files starting on adjacent LBAs are interleaved together.
    """
    master_wad_path = Path(master_wad_path)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    
    # Sort TOC by LBA
    toc.sort(key=lambda x: x['lba'])
    
    # Identify interleave groups
    groups = []
    if not toc:
        return
        
    current_group = [toc[0]]
    for entry in toc[1:]:
        # If the LBA is contiguous, it's part of the same interleave group
        if entry['lba'] == current_group[-1]['lba'] + 1:
            current_group.append(entry)
        else:
            groups.append(current_group)
            current_group = [entry]
    groups.append(current_group)
    
    # The LBA in the ELF is an absolute sector on the PS2 disc.
    # Our raw_wad_004.bin starts at the LBA of TOC[0].
    base_lba = toc[0]['lba']
    print(f"Base LBA of master archive: {base_lba}")
    
    with open(master_wad_path, 'rb') as source:
        for gid, group in enumerate(groups):
            stride = len(group)
            print(f"Processing Group {gid}: Stride {stride}, {len(group)} files starting at LBA {group[0]['lba']}")
            
            # Open output files
            out_files = []
            max_sectors = 0
            for entry in group:
                fname = out_dir / f"wad_{entry['idx']:03d}.bin"
                out_files.append({
                    'handle': open(fname, 'wb'),
                    'rem_bytes': entry['size'],
                    'start_offset': (entry['lba'] - base_lba) * SECTOR_SIZE
                })
                sectors = (entry['size'] + SECTOR_SIZE - 1) // SECTOR_SIZE
                if sectors > max_sectors:
                    max_sectors = sectors

            # Deinterleave
            # PS2 Interleave: [G0:S0][G1:S0]...[G(N-1):S0] [G0:S1][G1:S1]...
            # We seek to the start of the group relative to raw_wad_004.bin
            group_base_offset = (group[0]['lba'] - base_lba) * SECTOR_SIZE
            
            for s in range(max_sectors):
                # Seek to start of this multi-sector chunk
                chunk_offset = group_base_offset + (s * stride * SECTOR_SIZE)
                source.seek(chunk_offset)
                
                # Each sector in the chunk belongs to a different file in the group
                chunk_data = source.read(stride * SECTOR_SIZE)
                if not chunk_data:
                    break
                    
                for i in range(stride):
                    f_info = out_files[i]
                    if f_info['rem_bytes'] <= 0:
                        continue
                        
                    sector_data = chunk_data[i * SECTOR_SIZE : (i + 1) * SECTOR_SIZE]
                    write_size = min(f_info['rem_bytes'], SECTOR_SIZE)
                    f_info['handle'].write(sector_data[:write_size])
                    f_info['rem_bytes'] -= write_size

            # Close files
            for f in out_files:
                f['handle'].close()

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <elf_path> <master_wad_path> <out_dir>")
        sys.exit(1)
        
    elf = sys.argv[1]
    master = sys.argv[2]
    out = sys.argv[3]
    
    print(f"Deinterleaving {master} using {elf} ...")
    toc = get_toc(elf)
    deinterleave_block(master, toc, out)
    print("Done!")
