#!/usr/bin/env python3
"""
AG-RAC Raw Sector Extractor
============================
Ratchet & Clank 1 (PS2) stores all game data (levels, textures, models,
audio) outside the ISO 9660 filesystem, in raw sector blocks on the disc.
The locations of these blocks are encoded in the game ELF as a disc volume
table (TOC): a flat array of {uint32 lba, uint32 size_bytes} pairs.

This tool uses two strategies to find and extract those blocks:

  Strategy 1 – ELF TOC Scan (default):
      Parses the extracted game ELF to locate the disc volume table using
      heuristic detection: a monotonically-increasing sequence of (lba, size)
      pairs within plausible ranges.

  Strategy 2 – Heuristic Disc Scan (--scan):
      Reads every sector of the raw ISO looking for transitions between
      empty (all-zero) and non-empty regions, emitting each large non-zero
      block as a candidate data file.

Usage:
    # Primary method – requires the extracted ELF
    python raw_extract.py --iso RaC.iso --elf extracted/SCUS_971.99 --out extracted/wads/

    # Fallback – scan the whole disc (slower, no ELF needed)
    python raw_extract.py --iso RaC.iso --scan --out extracted/raw/

    # Extract a single known block
    python raw_extract.py --iso RaC.iso --lba 1350 --size 12345678 --out extracted/
"""

import argparse
import struct
import sys
from pathlib import Path

# ── Constants ────────────────────────────────────────────────────────────────
SECTOR_SIZE = 2048          # Standard PS2/CD-ROM sector size in bytes

# Plausible ranges for a disc volume table entry
TOC_MIN_LBA   = 20          # First usable LBA after ISO system area
TOC_MAX_LBA   = 360_000     # Upper bound (DVD: ~2.3M, CD: ~360K sectors)
TOC_MIN_BYTES = SECTOR_SIZE         # At least 1 sector of data
TOC_MAX_BYTES = 400 * 1024 * 1024  # 400 MB – no single block is larger

# Minimum run length to trust an ELF TOC detection
TOC_MIN_ENTRIES = 4

# Scan mode: minimum block size to report (discard tiny fragments)
SCAN_MIN_BLOCK_BYTES = 32 * SECTOR_SIZE   # 64 KB
SCAN_ZERO_GAP_LIMIT  = 8                  # sectors – tolerate short zero runs


# ── ELF helpers ──────────────────────────────────────────────────────────────
SHT_NULL   = 0
SHT_NOBITS = 8  # .bss — no file data


def read_elf_data_bytes(elf_path: Path) -> bytes:
    data = elf_path.read_bytes()
    if data[:4] != b'\x7fELF':
        raise ValueError(f"Not a valid ELF: {elf_path}")
    return data


# ── Strategy 1: ELF TOC scan ─────────────────────────────────────────────────
def find_toc_in_elf(elf_data: bytes, max_lba: int) -> list[tuple[int, int]]:
    """
    Search the ELF binary for a disc volume table.

    We scan every 4-byte-aligned position for (uint32 lba, uint32 size_bytes)
    pairs in plausible ranges AND with monotonically non-decreasing LBA values
    (game data is laid out sequentially on the disc). We return the longest
    such contiguous run, which is almost certainly the TOC.

    Returns: list of (lba, size_bytes) tuples, or [] if nothing convincing found.
    """
    def valid(lba: int, size: int) -> bool:
        return TOC_MIN_LBA <= lba <= max_lba and TOC_MIN_BYTES <= size <= TOC_MAX_BYTES

    best: list[tuple[int, int]] = []
    i = 0
    n = len(elf_data) - 8

    while i <= n:
        lba0, sz0 = struct.unpack_from('<II', elf_data, i)
        if not valid(lba0, sz0):
            i += 4
            continue

        # Extend forward, requiring non-decreasing LBA
        run = [(lba0, sz0)]
        prev_lba = lba0
        j = i + 8
        while j <= n:
            lba, sz = struct.unpack_from('<II', elf_data, j)
            if valid(lba, sz) and lba >= prev_lba:
                run.append((lba, sz))
                prev_lba = lba
                j += 8
            else:
                break

        if len(run) > len(best):
            best = run
        i = j  # skip past this run

    return best


# ── Strategy 2: Heuristic disc scan ──────────────────────────────────────────
def scan_disc_for_blocks(iso_path: Path,
                         min_lba: int = TOC_MIN_LBA,
                         verbose: bool = False) -> list[tuple[int, int]]:
    """
    Walk every sector of the ISO image, finding contiguous non-zero regions.

    Reading is done in 256-sector (512 KB) chunks for speed.
    Returns list of (lba, size_bytes) tuples.
    """
    file_size   = iso_path.stat().st_size
    total_lbas  = file_size // SECTOR_SIZE
    NULL_SECTOR = bytes(SECTOR_SIZE)
    READ_CHUNK  = 256  # sectors per I/O call

    print(f"  Disc size : {total_lbas:,} sectors  ({file_size / 1024**2:.1f} MB)")
    print(f"  Scanning from LBA {min_lba} …")

    blocks: list[tuple[int, int]] = []
    block_start:    int | None = None
    last_nonzero:   int | None = None
    zero_run:       int        = 0

    def close_block():
        nonlocal block_start, last_nonzero, zero_run
        if block_start is not None:
            sz = (last_nonzero - block_start + 1) * SECTOR_SIZE
            if sz >= SCAN_MIN_BLOCK_BYTES:
                blocks.append((block_start, sz))
        block_start = last_nonzero = None
        zero_run = 0

    with open(iso_path, 'rb') as f:
        f.seek(min_lba * SECTOR_SIZE)
        lba = min_lba

        while lba < total_lbas:
            chunk_sects = min(READ_CHUNK, total_lbas - lba)
            chunk = f.read(chunk_sects * SECTOR_SIZE)
            if not chunk:
                break

            # Quick whole-chunk zero check
            if all(b == 0 for b in chunk):
                if block_start is not None:
                    zero_run += chunk_sects
                    if zero_run > SCAN_ZERO_GAP_LIMIT:
                        close_block()
                lba += chunk_sects
                continue

            for s in range(chunk_sects):
                sector    = chunk[s * SECTOR_SIZE:(s + 1) * SECTOR_SIZE]
                curr_lba  = lba + s

                if sector != NULL_SECTOR:
                    if block_start is None:
                        block_start = curr_lba
                    last_nonzero = curr_lba
                    zero_run     = 0
                else:
                    if block_start is not None:
                        zero_run += 1
                        if zero_run > SCAN_ZERO_GAP_LIMIT:
                            close_block()

            lba += chunk_sects

            if verbose or (lba % 8192 < READ_CHUNK):
                pct = lba / total_lbas * 100
                print(f"\r  Progress: {pct:5.1f}%  ({lba:,}/{total_lbas:,} sectors)", end='', flush=True)

    close_block()
    print()  # newline after progress
    return blocks


# ── Extraction ────────────────────────────────────────────────────────────────
def extract_block(iso_path: Path, lba: int, size_bytes: int, out_path: Path) -> int:
    """Extract a contiguous sector block from the ISO image."""
    offset = lba * SECTOR_SIZE
    out_path.parent.mkdir(parents=True, exist_ok=True)

    with open(iso_path, 'rb') as f:
        f.seek(offset)
        data = f.read(size_bytes)

    out_path.write_bytes(data)
    return len(data)


def blocks_to_names(blocks: list[tuple[int, int]],
                    label: str = "block") -> list[tuple[int, int, str]]:
    """Assign output names to blocks."""
    named = []
    for idx, (lba, sz) in enumerate(blocks):
        name = f"{label}_{idx:03d}.bin"
        named.append((lba, sz, name))
    return named


# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser(
        description="AG-RAC Raw Sector Extractor — extract game data from R&C PS2 disc image",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    ap.add_argument("--iso",  required=True,  type=Path, help="PS2 ISO/BIN disc image")
    ap.add_argument("--out",  required=True,  type=Path, help="Output directory")
    ap.add_argument("--elf",  type=Path, help="Extracted game ELF (SCUS_971.99)")
    ap.add_argument("--scan", action="store_true",
                    help="Heuristic disc scan instead of ELF TOC method")
    ap.add_argument("--lba",  type=int, help="Extract a single block at this LBA")
    ap.add_argument("--size", type=int, help="Size in bytes for --lba single-block mode")
    ap.add_argument("--min-lba", type=int, default=TOC_MIN_LBA,
                    help=f"Lowest LBA to scan/consider (default {TOC_MIN_LBA})")
    ap.add_argument("--verbose", "-v", action="store_true")
    args = ap.parse_args()

    if not args.iso.exists():
        print(f"ERROR: ISO not found: {args.iso}")
        sys.exit(1)

    args.out.mkdir(parents=True, exist_ok=True)
    disc_total_lbas = args.iso.stat().st_size // SECTOR_SIZE

    print(f"\nAG-RAC Raw Sector Extractor")
    print(f"{'='*60}")
    print(f"  ISO  : {args.iso}")
    print(f"  Out  : {args.out}")
    print(f"{'='*60}\n")

    # ── Mode 1: single block ──────────────────────────────────────────────────
    if args.lba is not None:
        if args.size is None:
            print("ERROR: --size is required with --lba")
            sys.exit(1)
        out_path = args.out / f"block_lba{args.lba}.bin"
        print(f"Extracting single block: LBA {args.lba}, {args.size:,} bytes → {out_path.name}")
        written = extract_block(args.iso, args.lba, args.size, out_path)
        print(f"  ✓ Wrote {written:,} bytes")
        sys.exit(0)

    # ── Mode 2: ELF TOC scan ──────────────────────────────────────────────────
    if not args.scan:
        if args.elf is None:
            print("ERROR: --elf is required for ELF TOC mode (or use --scan)")
            sys.exit(1)
        if not args.elf.exists():
            print(f"ERROR: ELF not found: {args.elf}")
            sys.exit(1)

        print(f"Reading ELF: {args.elf.name} …")
        elf_data = read_elf_data_bytes(args.elf)
        print(f"  ELF size : {len(elf_data):,} bytes")

        print(f"\nScanning ELF for disc volume table …")
        toc = find_toc_in_elf(elf_data, disc_total_lbas)

        if len(toc) < TOC_MIN_ENTRIES:
            print(f"\n  WARNING: Only {len(toc)} TOC-like entries found (need ≥ {TOC_MIN_ENTRIES}).")
            print("  Try --scan mode as a fallback.")
            sys.exit(1)

        print(f"  Found {len(toc)} TOC entries:\n")
        named = blocks_to_names(toc, label="wad")

    # ── Mode 3: heuristic disc scan ───────────────────────────────────────────
    else:
        print("Heuristic disc scan mode …\n")
        blocks = scan_disc_for_blocks(args.iso, min_lba=args.min_lba, verbose=args.verbose)
        if not blocks:
            print("No data blocks found.")
            sys.exit(1)
        print(f"  Found {len(blocks)} data block(s):\n")
        named = blocks_to_names(blocks, label="raw")

    # ── Print table ───────────────────────────────────────────────────────────
    print(f"  {'#':>4}  {'LBA':>8}  {'Size (bytes)':>15}  {'Size (MB)':>10}  Name")
    print(f"  {'-'*4}  {'-'*8}  {'-'*15}  {'-'*10}  {'-'*20}")
    for idx, (lba, sz, name) in enumerate(named):
        mb = sz / 1024**2
        print(f"  {idx:>4}  {lba:>8,}  {sz:>15,}  {mb:>9.2f}  {name}")

    print()

    # ── Extract all ───────────────────────────────────────────────────────────
    total_written = 0
    ok = 0

    for lba, sz, name in named:
        out_path = args.out / name
        try:
            written = extract_block(args.iso, lba, sz, out_path)
            total_written += written
            ok += 1
            status = f"✓ {written:,} bytes"
        except Exception as e:
            status = f"✗ ERROR: {e}"
        print(f"  Extracting LBA {lba:>8,}  → {name:<25}  {status}")

    # ── Manifest ──────────────────────────────────────────────────────────────
    manifest = args.out / "RAW_EXTRACT_MANIFEST.txt"
    with open(manifest, 'w') as f:
        f.write(f"AG-RAC Raw Sector Extract Manifest\n")
        f.write(f"===================================\n")
        f.write(f"Source ISO : {args.iso.resolve()}\n")
        f.write(f"Method     : {'disc scan' if args.scan else 'ELF TOC'}\n")
        f.write(f"Blocks     : {ok}\n\n")
        f.write(f"{'LBA':>10}  {'Size (bytes)':>15}  Filename\n")
        f.write(f"{'─'*10}  {'─'*15}  {'─'*30}\n")
        for lba, sz, name in named:
            f.write(f"{lba:>10,}  {sz:>15,}  {name}\n")

    print(f"\n{'='*60}")
    print(f"  Extracted : {ok}/{len(named)} blocks")
    print(f"  Total     : {total_written / 1024**2:.2f} MB")
    print(f"  Manifest  : {manifest}")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
