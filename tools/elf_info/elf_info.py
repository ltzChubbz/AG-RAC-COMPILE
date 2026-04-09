#!/usr/bin/env python3
"""
AG-RAC ELF Info Tool
====================
Parses and displays the PS2 ELF binary (SCUS_971.99) — sections, segments,
symbol table, and prepares a JSON export for Ghidra label import.

Usage:
    python elf_info.py --elf ../../extracted/SCUS_971.99
    python elf_info.py --elf ../../extracted/SCUS_971.99 --json elf_map.json
    python elf_info.py --elf ../../extracted/SCUS_971.99 --dump-strings
    python elf_info.py --elf ../../extracted/SCUS_971.99 --dump-section .text --out section.bin
"""

import argparse
import json
import struct
import sys
from dataclasses import dataclass, asdict
from pathlib import Path


# ─── ELF Constants ────────────────────────────────────────────────────────────

ELF_MAGIC = b"\x7fELF"

EI_CLASS_32 = 1
EI_CLASS_64 = 2
EI_DATA_LSB = 1   # Little-endian
EI_DATA_MSB = 2   # Big-endian

ET_EXEC = 2       # Executable
ET_DYN  = 3       # Shared object

EM_MIPS = 8       # MIPS architecture

SHT_NULL     = 0
SHT_PROGBITS = 1
SHT_SYMTAB   = 2
SHT_STRTAB   = 3
SHT_RELA     = 4
SHT_NOBITS   = 8  # .bss — no data in file
SHT_REL      = 9

SHF_WRITE     = 0x1
SHF_ALLOC     = 0x2
SHF_EXECINSTR = 0x4

PT_LOAD    = 1
PT_DYNAMIC = 2

STT_NOTYPE  = 0
STT_OBJECT  = 1
STT_FUNC    = 2
STT_SECTION = 3
STT_FILE    = 4

STB_LOCAL  = 0
STB_GLOBAL = 1
STB_WEAK   = 2

SHN_ABS   = 0xFFF1
SHN_UNDEF = 0x0000

SECTION_TYPE_NAMES = {
    SHT_NULL: "NULL", SHT_PROGBITS: "PROGBITS", SHT_SYMTAB: "SYMTAB",
    SHT_STRTAB: "STRTAB", SHT_RELA: "RELA", SHT_NOBITS: "NOBITS", SHT_REL: "REL",
}

SEGMENT_TYPE_NAMES = {
    0: "NULL", 1: "LOAD", 2: "DYNAMIC", 3: "INTERP", 4: "NOTE",
    5: "SHLIB", 6: "PHDR", 0x70000000: "MIPS_REGINFO",
    0x6FFFFFFA: "LOSUNW", 0x6FFFFFFB: "SUNWBSS",
}


# ─── Data Structures ──────────────────────────────────────────────────────────

@dataclass
class ElfHeader:
    magic: bytes
    ei_class: int        # 1=32-bit, 2=64-bit
    ei_data: int         # 1=LE, 2=BE
    ei_version: int
    ei_osabi: int
    e_type: int          # ET_EXEC, etc.
    e_machine: int       # EM_MIPS = 8
    e_version: int
    e_entry: int         # Entry point virtual address
    e_phoff: int         # Program header table offset
    e_shoff: int         # Section header table offset
    e_flags: int         # Processor-specific flags
    e_ehsize: int        # ELF header size (bytes)
    e_phentsize: int     # Program header entry size
    e_phnum: int         # Number of program headers
    e_shentsize: int     # Section header entry size
    e_shnum: int         # Number of section headers
    e_shstrndx: int      # Section name string table index


@dataclass
class SectionHeader:
    index: int
    name: str
    sh_type: int
    sh_flags: int
    sh_addr: int         # Virtual address in memory
    sh_offset: int       # Offset in file
    sh_size: int         # Size in bytes
    sh_link: int
    sh_info: int
    sh_addralign: int
    sh_entsize: int


@dataclass
class ProgramHeader:
    index: int
    p_type: int
    p_offset: int        # Offset in file
    p_vaddr: int         # Virtual address
    p_paddr: int         # Physical address
    p_filesz: int        # Size in file
    p_memsz: int         # Size in memory
    p_flags: int         # Permissions (RWX)
    p_align: int


@dataclass
class Symbol:
    name: str
    st_value: int        # Symbol value (usually address)
    st_size: int
    st_info: int
    st_other: int
    st_shndx: int
    bind: str            # LOCAL, GLOBAL, WEAK
    sym_type: str        # NOTYPE, FUNC, OBJECT, etc.


# ─── ELF Parser ───────────────────────────────────────────────────────────────

class ElfParser:
    def __init__(self, path: Path):
        self.path = path
        with open(path, "rb") as f:
            self.data = f.read()
        self.header: ElfHeader | None = None
        self.sections: list[SectionHeader] = []
        self.segments: list[ProgramHeader] = []
        self.symbols: list[Symbol] = []
        self._parse()

    def _read(self, fmt: str, offset: int):
        size = struct.calcsize(fmt)
        return struct.unpack_from(fmt, self.data, offset)

    def _parse(self):
        self._parse_header()
        self._parse_sections()
        self._parse_segments()
        self._parse_symbols()

    def _parse_header(self):
        d = self.data
        if d[:4] != ELF_MAGIC:
            raise ValueError(f"Not a valid ELF file (magic={d[:4].hex()})")

        ei_class = d[4]
        ei_data  = d[5]
        endian   = "<" if ei_data == EI_DATA_LSB else ">"

        if ei_class == EI_CLASS_32:
            fmt = f"{endian}HHIIIIIHHHHHH"
            off = 16
        else:
            raise NotImplementedError("64-bit ELF not supported (PS2 uses 32-bit)")

        vals = struct.unpack_from(fmt, d, off)
        self.endian = endian
        self.header = ElfHeader(
            magic=d[:4],
            ei_class=ei_class,
            ei_data=ei_data,
            ei_version=d[6],
            ei_osabi=d[7],
            e_type=vals[0],
            e_machine=vals[1],
            e_version=vals[2],
            e_entry=vals[3],
            e_phoff=vals[4],
            e_shoff=vals[5],
            e_flags=vals[6],
            e_ehsize=vals[7],
            e_phentsize=vals[8],
            e_phnum=vals[9],
            e_shentsize=vals[10],
            e_shnum=vals[11],
            e_shstrndx=vals[12],
        )

    def _parse_sections(self):
        h = self.header
        shstrndx = h.e_shstrndx
        # First pass: read raw section headers
        raw_sections = []
        for i in range(h.e_shnum):
            off = h.e_shoff + i * h.e_shentsize
            vals = struct.unpack_from(f"{self.endian}IIIIIIIIII", self.data, off)
            raw_sections.append(vals)

        # Get string table data
        if shstrndx < len(raw_sections):
            sh_off = raw_sections[shstrndx][4]   # sh_offset
            sh_sz  = raw_sections[shstrndx][5]   # sh_size
            strtab = self.data[sh_off:sh_off + sh_sz]
        else:
            strtab = b""

        for i, vals in enumerate(raw_sections):
            name_off = vals[0]
            name = ""
            if strtab and name_off < len(strtab):
                end = strtab.index(b"\x00", name_off)
                name = strtab[name_off:end].decode("ascii", errors="replace")

            self.sections.append(SectionHeader(
                index=i, name=name,
                sh_type=vals[1], sh_flags=vals[2],
                sh_addr=vals[3], sh_offset=vals[4], sh_size=vals[5],
                sh_link=vals[6], sh_info=vals[7],
                sh_addralign=vals[8], sh_entsize=vals[9],
            ))

    def _parse_segments(self):
        h = self.header
        for i in range(h.e_phnum):
            off = h.e_phoff + i * h.e_phentsize
            vals = struct.unpack_from(f"{self.endian}IIIIIIII", self.data, off)
            self.segments.append(ProgramHeader(
                index=i, p_type=vals[0], p_offset=vals[1],
                p_vaddr=vals[2], p_paddr=vals[3], p_filesz=vals[4],
                p_memsz=vals[5], p_flags=vals[6], p_align=vals[7],
            ))

    def _parse_symbols(self):
        """Parse .symtab section if present (may not exist in release builds)."""
        symtab_sec = next((s for s in self.sections if s.sh_type == SHT_SYMTAB), None)
        if not symtab_sec:
            return

        strtab_sec = self.sections[symtab_sec.sh_link] if symtab_sec.sh_link < len(self.sections) else None
        strtab = b""
        if strtab_sec:
            strtab = self.data[strtab_sec.sh_offset:strtab_sec.sh_offset + strtab_sec.sh_size]

        entry_size = symtab_sec.sh_entsize or 16
        num_syms = symtab_sec.sh_size // entry_size

        for i in range(num_syms):
            off = symtab_sec.sh_offset + i * entry_size
            vals = struct.unpack_from(f"{self.endian}IIIBBH", self.data, off)
            # Elf32_Sym: st_name, st_value, st_size, st_info, st_other, st_shndx

            name_off = vals[0]
            name = ""
            if strtab and name_off < len(strtab):
                try:
                    end = strtab.index(b"\x00", name_off)
                    name = strtab[name_off:end].decode("ascii", errors="replace")
                except ValueError:
                    name = strtab[name_off:].decode("ascii", errors="replace")

            st_info = vals[3]
            bind_val = st_info >> 4
            type_val = st_info & 0xF

            BIND_NAMES = {STB_LOCAL: "LOCAL", STB_GLOBAL: "GLOBAL", STB_WEAK: "WEAK"}
            TYPE_NAMES = {STT_NOTYPE: "NOTYPE", STT_OBJECT: "OBJECT", STT_FUNC: "FUNC",
                          STT_SECTION: "SECTION", STT_FILE: "FILE"}

            self.symbols.append(Symbol(
                name=name,
                st_value=vals[1], st_size=vals[2],
                st_info=st_info, st_other=vals[4], st_shndx=vals[5],
                bind=BIND_NAMES.get(bind_val, f"UNK({bind_val})"),
                sym_type=TYPE_NAMES.get(type_val, f"UNK({type_val})"),
            ))

    def find_strings(self, min_len=5):
        """Extract printable ASCII strings from the binary."""
        strings = []
        current = []
        start = 0
        for i, b in enumerate(self.data):
            if 0x20 <= b < 0x7F:  # Printable ASCII
                if not current:
                    start = i
                current.append(chr(b))
            else:
                if len(current) >= min_len:
                    strings.append((start, "".join(current)))
                current = []
        return strings

    def dump_section(self, name: str) -> bytes | None:
        for s in self.sections:
            if s.name == name and s.sh_type != SHT_NOBITS:
                return self.data[s.sh_offset:s.sh_offset + s.sh_size]
        return None


# ─── Formatting Helpers ───────────────────────────────────────────────────────

def flags_str(flags: int) -> str:
    s = ""
    s += "R" if flags & 4 else "-"
    s += "W" if flags & 2 else "-"
    s += "X" if flags & 1 else "-"
    return s

def sh_flags_str(flags: int) -> str:
    s = ""
    s += "W" if flags & SHF_WRITE else " "
    s += "A" if flags & SHF_ALLOC else " "
    s += "X" if flags & SHF_EXECINSTR else " "
    return s


# ─── Display Functions ────────────────────────────────────────────────────────

def print_header(h: ElfHeader, path: Path):
    print(f"\n{'='*70}")
    print(f" AG-RAC ELF Info — {path.name}")
    print(f"{'='*70}")
    print(f"  File       : {path.resolve()}")
    print(f"  Size       : {path.stat().st_size:,} bytes")
    print(f"  Class      : ELF{'32' if h.ei_class == 1 else '64'}")
    print(f"  Data       : {'Little-endian (MIPS LE / PS2)' if h.ei_data == 1 else 'Big-endian'}")
    print(f"  Machine    : {'MIPS (PS2 Emotion Engine)' if h.e_machine == EM_MIPS else f'0x{h.e_machine:04X}'}")
    print(f"  Type       : {'Executable' if h.e_type == ET_EXEC else f'0x{h.e_type:04X}'}")
    print(f"  Entry      : 0x{h.e_entry:08X}")
    print(f"  ELF flags  : 0x{h.e_flags:08X}")
    print(f"  Sections   : {h.e_shnum}")
    print(f"  Segments   : {h.e_phnum}")


def print_sections(sections: list[SectionHeader]):
    print(f"\n{'─'*70}")
    print(f" Sections ({len(sections)})")
    print(f"{'─'*70}")
    print(f"  {'#':>3}  {'Name':<20} {'Type':<12} {'Flg':<4} {'VAddr':>10} {'Offset':>10} {'Size':>10}")
    print(f"  {'-'*3}  {'-'*20} {'-'*12} {'-'*4} {'-'*10} {'-'*10} {'-'*10}")
    for s in sections:
        type_name = SECTION_TYPE_NAMES.get(s.sh_type, f"0x{s.sh_type:X}")
        print(f"  {s.index:>3}  {s.name:<20} {type_name:<12} {sh_flags_str(s.sh_flags):<4} "
              f"0x{s.sh_addr:08X} {s.sh_offset:>10,} {s.sh_size:>10,}")


def print_segments(segments: list[ProgramHeader]):
    print(f"\n{'─'*70}")
    print(f" Segments ({len(segments)})")
    print(f"{'─'*70}")
    print(f"  {'#':>3}  {'Type':<15} {'Flg':<4} {'VAddr':>10} {'FileSz':>10} {'MemSz':>10} {'Align':>8}")
    print(f"  {'-'*3}  {'-'*15} {'-'*4} {'-'*10} {'-'*10} {'-'*10} {'-'*8}")
    for seg in segments:
        type_name = SEGMENT_TYPE_NAMES.get(seg.p_type, f"0x{seg.p_type:08X}")
        print(f"  {seg.index:>3}  {type_name:<15} {flags_str(seg.p_flags):<4} "
              f"0x{seg.p_vaddr:08X} {seg.p_filesz:>10,} {seg.p_memsz:>10,} {seg.p_align:>8,}")


def print_symbols(symbols: list[Symbol]):
    if not symbols:
        print(f"\n  (No symbol table — this is likely a release build stripped of debug info)")
        print(f"  Symbols must be recovered through Ghidra analysis.")
        return

    print(f"\n{'─'*70}")
    print(f" Symbol Table ({len(symbols)} entries)")
    print(f"{'─'*70}")
    func_syms = [s for s in symbols if s.sym_type == "FUNC" and s.name]
    obj_syms  = [s for s in symbols if s.sym_type == "OBJECT" and s.name]
    print(f"  Functions : {len(func_syms)}")
    print(f"  Objects   : {len(obj_syms)}")

    if func_syms:
        print(f"\n  Functions:")
        print(f"  {'Address':>10}  {'Size':>8}  {'Bind':<8}  Name")
        for s in sorted(func_syms, key=lambda x: x.st_value)[:50]:
            print(f"  0x{s.st_value:08X}  {s.st_size:>8,}  {s.bind:<8}  {s.name}")
        if len(func_syms) > 50:
            print(f"  ... and {len(func_syms)-50} more")


def print_text_summary(sections: list[SectionHeader]):
    """Print a summary of code and data size statistics."""
    text  = next((s for s in sections if s.name == ".text"), None)
    data  = next((s for s in sections if s.name == ".data"), None)
    bss   = next((s for s in sections if s.name == ".bss"), None)
    rodat = next((s for s in sections if s.name == ".rodata"), None)

    print(f"\n{'─'*70}")
    print(f" Code & Data Summary")
    print(f"{'─'*70}")
    if text:
        print(f"  .text   (code)       : {text.sh_size:>10,} bytes  ({text.sh_size/1024:.1f} KB) @ 0x{text.sh_addr:08X}")
    if data:
        print(f"  .data   (init data)  : {data.sh_size:>10,} bytes  ({data.sh_size/1024:.1f} KB) @ 0x{data.sh_addr:08X}")
    if bss:
        print(f"  .bss    (zero data)  : {bss.sh_size:>10,} bytes  ({bss.sh_size/1024:.1f} KB) @ 0x{bss.sh_addr:08X}")
    if rodat:
        print(f"  .rodata (const data) : {rodat.sh_size:>10,} bytes  ({rodat.sh_size/1024:.1f} KB) @ 0x{rodat.sh_addr:08X}")

    total_code = text.sh_size if text else 0
    total_data = (data.sh_size if data else 0) + (bss.sh_size if bss else 0)
    print(f"\n  Total code : {total_code:>10,} bytes  ({total_code/1024:.1f} KB)")
    print(f"  Total data : {total_data:>10,} bytes  ({total_data/1024:.1f} KB)")

    if total_code:
        # Very rough instruction count estimate
        instr_count = total_code // 4
        print(f"\n  ~{instr_count:,} MIPS instructions to decompile")
        print(f"  (Assuming ~15 instructions/function → ~{instr_count//15:,} functions)")


def export_json(parser: ElfParser, out_path: Path):
    """Export ELF map as JSON for tooling integration."""
    data = {
        "file": str(parser.path),
        "entry_point": f"0x{parser.header.e_entry:08X}",
        "sections": [
            {
                "index": s.index,
                "name": s.name,
                "type": SECTION_TYPE_NAMES.get(s.sh_type, hex(s.sh_type)),
                "addr": f"0x{s.sh_addr:08X}",
                "offset": s.sh_offset,
                "size": s.sh_size,
                "flags": sh_flags_str(s.sh_flags),
            }
            for s in parser.sections
        ],
        "segments": [
            {
                "index": seg.index,
                "type": SEGMENT_TYPE_NAMES.get(seg.p_type, hex(seg.p_type)),
                "vaddr": f"0x{seg.p_vaddr:08X}",
                "filesz": seg.p_filesz,
                "memsz": seg.p_memsz,
                "flags": flags_str(seg.p_flags),
            }
            for seg in parser.segments
        ],
        "symbols": [
            {
                "name": s.name,
                "addr": f"0x{s.st_value:08X}",
                "size": s.st_size,
                "type": s.sym_type,
                "bind": s.bind,
            }
            for s in parser.symbols if s.name
        ],
    }
    with open(out_path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"\n  JSON export written to: {out_path}")


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="AG-RAC: Parse and display PS2 ELF binary information",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--elf", required=True, type=Path,
                        help="Path to the ELF binary (e.g., extracted/SCUS_971.99)")
    parser.add_argument("--json", type=Path, metavar="OUT",
                        help="Export ELF map as JSON to this file")
    parser.add_argument("--dump-strings", action="store_true",
                        help="Dump printable ASCII strings from the binary")
    parser.add_argument("--dump-section", metavar="NAME",
                        help="Dump raw bytes of a named section (e.g., .text)")
    parser.add_argument("--out", type=Path,
                        help="Output path for --dump-section")
    parser.add_argument("--no-symbols", action="store_true",
                        help="Skip symbol table output")
    args = parser.parse_args()

    # ── Load ELF ──────────────────────────────────────────────────────────────
    if not args.elf.exists():
        print(f"ERROR: ELF not found: {args.elf}")
        print("  Run 'python tools/iso_extract/iso_extract.py' to extract your ISO first.")
        sys.exit(1)

    try:
        elf = ElfParser(args.elf)
    except ValueError as e:
        print(f"ERROR: {e}")
        sys.exit(1)

    # ── Display ───────────────────────────────────────────────────────────────
    print_header(elf.header, args.elf)
    print_text_summary(elf.sections)
    print_sections(elf.sections)
    print_segments(elf.segments)
    if not args.no_symbols:
        print_symbols(elf.symbols)

    # ── String dump ───────────────────────────────────────────────────────────
    if args.dump_strings:
        print(f"\n{'─'*70}")
        print(f" Embedded Strings (min length 5)")
        print(f"{'─'*70}")
        strings = elf.find_strings()
        for offset, s in strings[:200]:
            print(f"  0x{offset:08X}  {s!r}")
        if len(strings) > 200:
            print(f"  ... and {len(strings)-200} more")

    # ── Section dump ──────────────────────────────────────────────────────────
    if args.dump_section:
        section_data = elf.dump_section(args.dump_section)
        if section_data is None:
            print(f"\nERROR: Section '{args.dump_section}' not found or has no data")
        else:
            out_path = args.out or Path(f"{args.dump_section.lstrip('.')}.bin")
            with open(out_path, "wb") as f:
                f.write(section_data)
            print(f"\n  Section '{args.dump_section}' ({len(section_data):,} bytes) written to: {out_path}")

    # ── JSON export ───────────────────────────────────────────────────────────
    if args.json:
        export_json(elf, args.json)

    print(f"\n{'='*70}")
    print(f" Next steps:")
    print(f"   1. See docs/ghidra_setup.md to import this ELF into Ghidra")
    print(f"   2. Run with --json elf_map.json to export for tooling")
    print(f"   3. Run with --dump-strings to find embedded debug strings")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()
