# R&C PS2 File Format Reference

Documentation of all known file formats used in *Ratchet & Clank* (PS2, NTSC-U `SCUS-97199`).
This document is seeded from research by [chaoticgd/wrench](https://github.com/chaoticgd/wrench) and expanded as we reverse-engineer further.

---

## Disc Structure

The disc uses ISO 9660. The root directory contains:
```
SCUS_971.99     ← Main game ELF (the executable — our primary target)
SYSTEM.CNF      ← PS2 boot config (defines which ELF to load)
IRX/            ← IOP modules (.IRX ELF files for drivers)
STREAM/         ← FMV video streams (.PSS / .STR format)
*.WAD           ← Level and asset archives (see WAD format below)
```

### SYSTEM.CNF
Plain text. Example:
```
BOOT2 = cdrom0:\SCUS_971.99;1
VER = 1.01
VMODE = NTSC
HDDUNITPOWER = NICHDD
```

---

## ELF Binary (`SCUS_971.99`)

Standard MIPS ELF (Executable and Linkable Format). PS2 ELFs differ slightly from
standard Linux ELFs — they use custom PT_SCE_* program header types.

### Key Sections
| Section | Description |
|---------|-------------|
| `.text` | Game executable code (MIPS instructions) |
| `.data` | Initialized global data |
| `.bss`  | Uninitialized globals (zeroed at startup) |
| `.rodata` | Read-only data (strings, constants, lookup tables) |

### Loading
The ELF is loaded into EE RAM at the address specified in the ELF header.
The PS2 BIOS parses the ELF program headers and maps segments into RAM before jumping to the entry point.

**Tools:**
- `tools/elf_info/elf_info.py` — our ELF parser
- Ghidra with EE plugin — full static analysis

---

## WAD Archive

The primary asset container format. Most game data lives in `.WAD` files.
Research credit: [chaoticgd/wrench](https://github.com/chaoticgd/wrench)

### Header Structure (preliminary)
```c
typedef struct {
    u32 magic;          // Identifies archive type
    u32 num_entries;    // Number of sub-files
    u32 offsets[];      // Array of byte offsets to each entry
} WadHeader;
```

### Known WAD Contents
| Content Type | Internal Name | Description |
|-------------|--------------|-------------|
| Level geometry | TFrag | Terrain fragment meshes |
| Entity models | MOBY | Actor/object models |
| Background instances | TIE | Static background geometry instances |
| Collision mesh | COLLISION | Used for physics/collision queries |
| Textures | Various | Palettized (4bpp/8bpp) or 32bpp RGBA |
| Sound bank | VAG/VH/VB | ADPCM audio samples |
| Level overlay ELF | LEVEL ELF | Per-level code overlay (sub-ELF) |
| Instances | INSTANCES | Object placement data |

---

## TFrag — Terrain Fragment

The level terrain (ground, walls, platforms) is broken into small meshes called TFrags.
Each TFrag is a small, self-contained chunk of the level's static geometry.

### Key Fields (preliminary)
```c
typedef struct TFrag {
    Vec4 bsphere;           // Bounding sphere for culling
    u32  vertex_count;
    u32  index_count;
    u32  vertex_offset;     // Relative to TFrag data base
    u32  index_offset;
    u32  texture_id;
    u8   lod_count;         // Number of LOD levels
    // ... (additional fields TBD from Ghidra analysis)
} TFrag;
```

**Extraction:** Wrench can export TFrags as COLLADA files.

---

## MOBY — Actor/Entity Model

MOBYs are the main entity/actor format — used for Ratchet, Clank, enemies, weapons, pick-ups, etc.

### Key Fields (preliminary)
```c
typedef struct MobyClass {
    u32  joint_count;       // Skeleton bone count
    u32  animation_count;
    u32  mesh_count;
    u32  mesh_offset;
    // Joint/bone transforms, animation data, vertex weights...
} MobyClass;
```

Each MOBY instance in a level references a MOBY class (the mesh definition) by class ID.

**Extraction:** Wrench can export MOBYs as glTF 2.0 files.

---

## TIE — Background Instance Geometry

TIE (potentially "Terrain Instance Elements" — name unconfirmed) are high-frequency
background geometry instances (trees, rocks, pipes). Unlike MOBYs, they are static.

---

## Texture Formats

The PS2 uses palettized textures heavily (saves VRAM). Formats encountered:

| Format | Description |
|--------|-------------|
| PSMT4 | 4bpp palettized, 16-color CLUT |
| PSMT8 | 8bpp palettized, 256-color CLUT |
| PSMCT32 | 32bpp RGBA (less common, used for HUD/effects) |
| PSMCT16 | 16bpp (5-5-5-1 or 5-6-5) |

CLUT (Color Lookup Table) entries are stored in GS-native format. The palette also
uses the PS2's peculiar **PSMCT32 swizzle** — colors are stored in a 16×16 grid that must
be un-swizzled before use.

---

## Audio Formats

### VAG — Sony ADPCM
Single audio clip. Header + ADPCM data blocks.
```c
typedef struct VagHeader {
    char magic[4];          // "VAGp"
    u32  version;           // BE u32
    u32  reserved;
    u32  data_size;         // BE u32
    u32  sample_rate;       // BE u32
    u8   reserved2[12];
    char name[16];
} VagHeader;
```

### VH/VB — Sound Bank
- `.VH` — Sound bank header: sample metadata, loop points, pitch, ADSR envelope
- `.VB` — Sound bank body: raw ADPCM data for all samples in the bank

---

## Level Overlay ELFs

Each level has a small ELF "overlay" that contains level-specific game logic
(AI behavior customizations, scripted events, cutscene triggers). These are
loaded into RAM on top of the main ELF when the level loads.

File naming convention (preliminary): e.g., `LEVEL01.ELF`, `LEVEL02.ELF`

---

## FMV — Video Streams

Videos use the standard PS2 `.PSS` format (MPEG-2 video + SPU2 audio multiplexed).
These are not part of the decompilation target — they will be played back using
libavcodec in the PC build.

---

## IRX Modules

IOP driver modules. Standard IOP ELF format. Key modules:
- `LIBSDR.IRX` — Sound driver
- `SIO2MAN.IRX` — Controller/memory card manager
- `PADMAN.IRX` — Gamepad driver

These are replaced entirely by the PC HAL layer — no emulation needed.

---

## References

- [chaoticgd/wrench](https://github.com/chaoticgd/wrench) — primary source for WAD/MOBY/TFrag formats
- [PS2TEK](https://psi-rockin.github.io/ps2tek/) — PS2 hardware register reference
- [PS2DEV forums](https://forums.ps2dev.org/) — historical PS2 development discussions
- [The Cutting Room Floor — R&C](https://tcrf.net/Ratchet_%26_Clank_(PlayStation_2)) — unused content and file notes
