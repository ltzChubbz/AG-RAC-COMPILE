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
| ISO extraction tooling | 🟡 In Progress |
| ELF analysis tooling | 🟡 In Progress |
| Ghidra disassembly setup | 🔴 Planned |
| Matching C decompilation | 🔴 Planned |
| Asset pipeline (via Wrench) | 🔴 Planned |
| PS2 Hardware Abstraction Layer | 🔴 Planned |
| Native PC binary | 🔴 Planned |

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
│   └── asset_extract/      # Python: wrench wrapper for assets
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
