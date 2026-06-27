# AG-RAC — Ratchet & Clank Native PC Port

> A matching decompilation of *Ratchet & Clank* (PS2, 2002, NTSC-U `SCUS-97199`) targeting a native x86-64 PC build — modelled on [OpenGOAL](https://opengoal.dev/) (Jak & Daxter) and the [SM64 decompilation](https://github.com/n64decomp/sm64) project.

---

## ⚠️ Legal Notice

This repository contains **no copyrighted game assets or code**. It contains only:
- Original tooling scripts (MIT licensed)
- Decompiled C source code reverse-engineered from the binary (matching target)
- Documentation

To build this project, you **must supply your own legally obtained copy** of *Ratchet & Clank* (PS2) as an ISO image. The build system will extract the required assets and executable from your ISO at build time.

---

## Project Goals

| Goal | Status |
|------|--------|
| ISO extraction tooling | 🟢 Completed |
| ELF analysis tooling | 🟢 Completed |
| Ghidra disassembly setup | 🟢 Completed |
| Matching C decompilation | 🟡 In Progress |
| Stateful WAD decompressor (Formula 3) | 🟢 Completed |
| Embedded geometry WAD extraction | 🟢 Completed |
| TFrag terrain rendering pipeline | 🟢 Completed |
| Asset pipeline (Wrench & WAD Loader) | 🟡 In Progress |
| PS2 Hardware Abstraction Layer | 🟡 In Progress |
| Native PC binary | 🟡 In Progress |

---

## Progress Log

### Phase 13 — Stateful WAD Decompressor & TFrag Terrain Rendering

**Decompressor fix (Formula 3)**

The R&C PS2 WAD format uses a stateful LZ-variant compressor where each block decompresses into a sliding window seeded by the previous block's output. The correct lookback formula for the `flag < 0x20` case is:

```c
lb = current_dest_size - ((flag & 8) * 0x800) - (b1 * 0x40) - (b0 >> 2);
if ((u32)lb != current_dest_size) {
    m += 2;
    lb -= 0x4000;
} else if (m != 1) {
    /* skip to next 0x1000-aligned boundary */
}
```

The seed window must be initialised to **32 KB of zeroes** (not a copy of the ELF header), yielding **0 decompression errors** across all five level blocks.

**Level data layout & Veldin Sector Geometry Loader (`wad_106.bin` — Veldin, Planet 0)**

The level archive decompresses as a chained sequence. After decompression, the `RacLevelDataHeader` at the start of block 6 describes all sub-regions:

| Field | Offset (from block 6 base) |
|-------|--------------------------|
| `overlay` | `0x20` |
| `sound_bank` | `0x1EF0` |
| `core_index` (raw `LevelCoreHeader`) | `0x89EF0` |
| `core_data` (compressed geometry WAD) | `0xDAB3` (absolute in first-block decompressed output) |

While standard levels package their static terrain fragment definitions directly in the embedded core block, Veldin (`wad_106.bin`) uses sector-based geometry streaming. The custom level loader in `src/engine/wad.c` detects Veldin and constructs a virtual `TfragHeader` table mapping directly to the streamed 2048-byte sector geometry blocks starting at sector 638 (`0x13F000`) up to sector 2420. This allows the rendering pipeline to compile and load all 566 mesh sectors directly as GPU-ready terrain without crashing on memory pointer offsets.

**Rendering pipeline**

- `src/engine/wad.c` — zero-seeded stateful decompression; custom Veldin sector loader and virtual `TFrag` table mapper; standard levels fallback to embedded geometry WAD decompression.
- `src/renderer/mesh.c` — new `mesh_from_tfrag()` decodes VIF vertex packets with ADC-bit strip restart logic and uploads to GPU.
- `src/renderer/mesh.h` — `mesh_from_tfrag()` declaration.
- `pc/window/window.c` — on init, reads `TfragsHeader`, iterates all TFrag entries, builds `PcMesh` objects and renders them in the main loop. Fragment shader now samples the bound texture instead of outputting solid neon-pink.

---

## How It Works

Unlike emulation, this project recompiles the game's logic as a true native application:

```
PS2 ISO
  └─► [1] iso_extract  → ELF binary + raw data archives
  └─► [2] elf_info     → section/symbol map for Ghidra import
  └─► [3] Ghidra       → annotated MIPS disassembly
  └─► [4] C decomp     → matching C source (byte-for-byte)
  └─► [5] asset_extract→ textures/models/audio as modern formats
  └─► [6] PC HAL       → PS2 GS/VU/IOP → OpenGL + SDL2
  └─► [7] CMake build  → native Windows/Linux binary
```

---

## Getting Started

### Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| Python | 3.11+ | Tooling scripts |
| CMake | 3.26+ | Build system |
| Ghidra | 11+ | Static analysis |
| PCSX2 (debug) | latest | Dynamic analysis |
| SDL2 | 2.28+ | PC window/input/audio |
| mipsel-linux-gnu-gcc | 13+ | MIPS cross-compiler (WSL) |

### 1. Extract your ISO

```bash
cd tools/iso_extract
pip install -r requirements.txt
python iso_extract.py --iso /path/to/RatchetAndClank.iso --out ../../extracted/
```

### 2. Inspect the ELF

```bash
cd tools/elf_info
pip install -r requirements.txt
python elf_info.py --elf ../../extracted/SCUS_971.99
```

### 3. Open in Ghidra

See [`docs/ghidra_setup.md`](docs/ghidra_setup.md) for step-by-step instructions.

### 4. Build (MIPS matching target)

```bash
# In WSL with mipsel-linux-gnu-gcc installed
cmake -B build -DTARGET=mips
cmake --build build
# SHA1 comparison against original ELF runs automatically
```

### 5. Build (PC native)

```bash
cmake -B build_pc -DTARGET=pc
cmake --build build_pc
./build_pc/rac_pc
```

---

## Repository Structure

```
AG-RAC-COMPILE/
├── README.md
├── CMakeLists.txt
├── LICENSE
│
├── docs/                   # Architecture, file formats, memory map
│   ├── architecture.md
│   ├── file_formats.md
│   ├── memory_map.md
│   ├── ghidra_setup.md
│   └── functions/          # Per-subsystem function documentation
│
├── tools/
│   ├── iso_extract/        # Python: unpack ISO → ELF + data
│   ├── elf_info/           # Python: parse ELF headers and symbols
│   ├── asset_extract/      # Python: extract, analyze WAD, search meshes & refs
│   └── wad_viewer/         # Python: graphical WAD viewer tool
│
├── asm/
│   └── non_matchings/      # Functions not yet converted to C
│
├── src/                    # Decompiled C source (matching target)
│   ├── engine/
│   │   ├── math/
│   │   ├── memory/
│   │   └── string/
│   └── game/
│       ├── player/
│       ├── camera/
│       ├── enemy/
│       ├── weapon/
│       └── hud/
│
├── include/                # Reverse-engineered headers
│
├── assets/                 # Extracted game assets (gitignored)
│
└── pc/                     # PC-specific code
    ├── hal/                # PS2 Hardware Abstraction Layer
    │   ├── gs/             # Graphics Synthesizer → OpenGL
    │   ├── vu/             # Vector Units → CPU SIMD
    │   ├── iop/            # IOP → SDL2 audio + file I/O
    │   ├── dma/            # DMA controller shim
    │   └── pad/            # Gamepad → SDL2
    └── window/             # SDL2 window + main loop
```

---

## Related Projects

| Project | Relevance |
|---------|-----------|
| [chaoticgd/wrench](https://github.com/chaoticgd/wrench) | PS2 R&C modding tools — we use this for asset extraction |
| [OpenGOAL](https://github.com/open-goal/jak-project) | Primary inspiration — Jak & Daxter native PC port |
| [SM64 decompilation](https://github.com/n64decomp/sm64) | Matching decompilation methodology reference |
| [asm-differ](https://github.com/simonlindholm/asm-differ) | ASM comparison tooling |
| [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) | C matching assistant |

---

## Contributing

This project is in its earliest stages. If you want to help:
1. Join the Discord (link TBD)
2. Read `docs/architecture.md`
3. Pick an unmatched function from `asm/non_matchings/`
4. Submit a PR with your C version passing the SHA1 check

---

## License

Original code in this repository is licensed under the **MIT License**. See `LICENSE`.  
*Ratchet & Clank* is © Insomniac Games / Sony Interactive Entertainment. This project is not affiliated with or endorsed by them.
