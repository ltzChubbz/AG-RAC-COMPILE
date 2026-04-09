#!/usr/bin/env python3
"""
AG-RAC ISO Extractor
====================
Unpacks a Ratchet & Clank (PS2) disc image (.iso, .bin) into:
  - The main ELF executable (SCUS_971.99)
  - All data archives (*.WAD, *.PS2, etc.)
  - IRX modules
  - FMV streams

Usage:
    python iso_extract.py --iso /path/to/RatchetAndClank.iso --out ../../extracted/
    python iso_extract.py --iso game.bin --out ../../extracted/ --game rac2

Supported game IDs:
    rac1  - Ratchet & Clank (SCUS-97199)         [DEFAULT]
    rac2  - Ratchet & Clank: Going Commando (SCUS-97268)
    rac3  - Ratchet & Clank: Up Your Arsenal (SCUS-97353)
    rac4  - Ratchet & Clank: Deadlocked (SCUS-97465)
"""

import argparse
import sys
import os
import shutil
from pathlib import Path

try:
    import pycdlib
except ImportError:
    print("ERROR: pycdlib is not installed. Run: pip install -r requirements.txt")
    sys.exit(1)


# Known game ELF names by game ID
GAME_ELFS = {
    "rac1": ["SCUS_971.99", "SCES_509.56", "SCPS_150.41"],  # US, PAL, JP
    "rac2": ["SCUS_972.68", "SCES_516.07", "SCPS_150.62"],
    "rac3": ["SCUS_973.53", "SCES_519.37", "SCPS_150.84"],
    "rac4": ["SCUS_974.65", "SCES_527.73"],
}

# Expected disc file types we care about
EXTRACT_EXTENSIONS = {
    ".WAD", ".PS2", ".IRX", ".ELF", ".PSS", ".STR", ".CNF",
    ".99",   # ELF with version suffix (e.g., SCUS_971.99)
    ".68", ".53", ".65", ".56", ".07", ".37", ".73",  # Other region ELF suffixes
}


def open_iso(iso_path: Path):
    """Open an ISO image using pycdlib. Supports ISO 9660 and bin/cue."""
    iso = pycdlib.PyCdlib()
    try:
        iso.open(str(iso_path))
        return iso
    except pycdlib.pycdlibexception.PyCdlibInvalidInput as e:
        print(f"ERROR: Failed to open ISO: {e}")
        print("  Make sure the file is a valid PS2 disc image (ISO 9660)")
        sys.exit(1)


def list_iso_contents(iso, verbose=False):
    """Walk the ISO directory tree and return all file entries."""
    entries = []
    for dir_record, file_path in iso.walk(iso_path="/"):
        if dir_record.is_file():
            entries.append(file_path)
            if verbose:
                size = dir_record.data_length
                print(f"  {file_path:<50} {size:>10,} bytes")
    return entries


def detect_game(entries: list[str]) -> tuple[str, str]:
    """Auto-detect which R&C game this is from the ELF filename."""
    for entry in entries:
        filename = Path(entry).name.upper()
        for game_id, elf_names in GAME_ELFS.items():
            for elf in elf_names:
                if filename == elf.upper() or filename.replace(".", "_") == elf.upper().replace(".", "_"):
                    return game_id, elf
    return "unknown", ""


def extract_file(iso, iso_path: str, out_path: Path):
    """Extract a single file from the ISO to the output path."""
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "wb") as f:
        iso.get_file_from_iso_fp(f, iso_path=iso_path)


def extract_all(iso, entries: list[str], out_dir: Path, game_id: str, filter_ext=None):
    """Extract all relevant files from the ISO to the output directory."""
    extracted = []
    skipped = []

    for iso_path in entries:
        path = Path(iso_path)
        ext = path.suffix.upper()
        filename = path.name.upper()

        # Determine if we should extract this file
        should_extract = False
        if filter_ext is None:
            # Default: extract known file types
            if ext in EXTRACT_EXTENSIONS or "." not in path.stem:
                should_extract = True
            # Always extract the ELF specifically
            for elf in GAME_ELFS.get(game_id, []):
                if filename == elf.upper():
                    should_extract = True
        else:
            should_extract = ext.lstrip(".") in [e.upper().lstrip(".") for e in filter_ext]

        if not should_extract:
            skipped.append(iso_path)
            continue

        # Build the output path, preserving directory structure
        # Strip the leading "/" and normalize
        relative = iso_path.lstrip("/").replace(";1", "")  # Remove ISO version suffix
        out_path = out_dir / relative

        try:
            print(f"  Extracting: {iso_path:<55} → {out_path.name}")
            extract_file(iso, iso_path, out_path)
            extracted.append(out_path)
        except Exception as e:
            print(f"  WARNING: Failed to extract {iso_path}: {e}")

    return extracted, skipped


def verify_elf(out_dir: Path, game_id: str) -> Path | None:
    """Find and verify the extracted ELF."""
    for elf_name in GAME_ELFS.get(game_id, []):
        elf_path = out_dir / elf_name
        if elf_path.exists():
            # Quick ELF magic check
            with open(elf_path, "rb") as f:
                magic = f.read(4)
            if magic == b"\x7fELF":
                return elf_path
            else:
                print(f"  WARNING: {elf_name} does not have valid ELF magic bytes!")
    return None


def write_extraction_manifest(out_dir: Path, game_id: str, iso_path: Path, extracted: list[Path]):
    """Write a manifest file listing what was extracted."""
    manifest_path = out_dir / "EXTRACTION_MANIFEST.txt"
    with open(manifest_path, "w") as f:
        f.write(f"AG-RAC Extraction Manifest\n")
        f.write(f"==========================\n")
        f.write(f"Source ISO : {iso_path.resolve()}\n")
        f.write(f"Game ID    : {game_id}\n")
        f.write(f"Extracted  : {len(extracted)} files\n\n")
        f.write("Files:\n")
        for p in sorted(extracted):
            size = p.stat().st_size if p.exists() else 0
            f.write(f"  {p.name:<50} {size:>10,} bytes\n")
    print(f"\n  Manifest written to: {manifest_path}")


def main():
    parser = argparse.ArgumentParser(
        description="AG-RAC: Extract PS2 Ratchet & Clank disc image",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--iso", required=True, type=Path,
                        help="Path to the PS2 ISO or BIN image")
    parser.add_argument("--out", required=True, type=Path,
                        help="Output directory for extracted files")
    parser.add_argument("--game", default="auto",
                        choices=["auto", "rac1", "rac2", "rac3", "rac4"],
                        help="Game to extract (default: auto-detect)")
    parser.add_argument("--list", action="store_true",
                        help="List disc contents and exit without extracting")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show detailed output")
    parser.add_argument("--force", action="store_true",
                        help="Overwrite existing output directory")

    args = parser.parse_args()

    # ── Validate input ────────────────────────────────────────────────────────
    if not args.iso.exists():
        print(f"ERROR: ISO not found: {args.iso}")
        sys.exit(1)

    print(f"\nAG-RAC ISO Extractor")
    print(f"{'='*50}")
    print(f"  Input ISO : {args.iso}")
    print(f"  Output dir: {args.out}")

    # ── Open the ISO ─────────────────────────────────────────────────────────
    print(f"\nOpening disc image...")
    iso = open_iso(args.iso)

    # ── List contents ─────────────────────────────────────────────────────────
    print(f"Reading directory tree...")
    entries = list_iso_contents(iso, verbose=args.list or args.verbose)
    print(f"  Found {len(entries)} files on disc")

    if args.list:
        iso.close()
        return

    # ── Auto-detect game ──────────────────────────────────────────────────────
    if args.game == "auto":
        game_id, elf_name = detect_game(entries)
        if game_id == "unknown":
            print("\nWARNING: Could not auto-detect game from disc contents.")
            print("  Known ELF names:", [e for names in GAME_ELFS.values() for e in names])
            print("  Use --game to specify manually.")
            game_id = "rac1"
        else:
            print(f"  Detected game: {game_id.upper()} (ELF: {elf_name})")
    else:
        game_id = args.game
        print(f"  Game: {game_id.upper()} (forced)")

    # ── Prepare output directory ──────────────────────────────────────────────
    if args.out.exists() and not args.force:
        existing = list(args.out.iterdir())
        if existing:
            print(f"\nWARNING: Output directory already exists and is not empty: {args.out}")
            print("  Use --force to overwrite")
            # Don't fail — just continue and overwrite individual files

    args.out.mkdir(parents=True, exist_ok=True)

    # ── Extract files ─────────────────────────────────────────────────────────
    print(f"\nExtracting files...")
    extracted, skipped = extract_all(iso, entries, args.out, game_id)

    iso.close()

    # ── Verify ELF ────────────────────────────────────────────────────────────
    print(f"\nVerifying extracted ELF...")
    elf_path = verify_elf(args.out, game_id)
    if elf_path:
        size = elf_path.stat().st_size
        print(f"  ✓ ELF valid: {elf_path.name} ({size:,} bytes)")
    else:
        print(f"  ✗ ELF not found or invalid — check game ID and disc image")

    # ── Summary ───────────────────────────────────────────────────────────────
    write_extraction_manifest(args.out, game_id, args.iso, extracted)

    print(f"\n{'='*50}")
    print(f"Extraction complete!")
    print(f"  Extracted : {len(extracted)} files")
    print(f"  Skipped   : {len(skipped)} files")
    print(f"  Output    : {args.out.resolve()}")
    if elf_path:
        print(f"\nNext step:")
        print(f"  python ../elf_info/elf_info.py --elf {elf_path}")


if __name__ == "__main__":
    main()
