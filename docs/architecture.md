# PS2 Emotion Engine Architecture — R&C Engine Technical Reference

This document describes the PlayStation 2 hardware architecture and maps each subsystem to its equivalent in the AG-RAC PC port.

---

## PS2 Hardware Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    PlayStation 2 Architecture                   │
│                                                                 │
│  ┌────────────────────────────────────────────────────────┐    │
│  │              Emotion Engine (EE) — Main CPU            │    │
│  │  MIPS R5900 @ 294 MHz, 128-bit registers               │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │    │
│  │  │   VU0    │  │   VU1    │  │  FPU / SIMD Ops      │ │    │
│  │  │(co-proc) │  │(DMA-fed) │  │  (128-bit MMI)       │ │    │
│  │  └──────────┘  └────┬─────┘  └──────────────────────┘ │    │
│  └───────────────────── │ ──────────────────────────────────┘    │
│                         │ GIF (Graphics Interface)               │
│  ┌──────────────────────▼────────────────────────────────┐     │
│  │           GS — Graphics Synthesizer                   │     │
│  │  4MB eDRAM frame buffer                               │     │
│  │  Rasterizer, texture mapping, alpha blend             │     │
│  └───────────────────────────────────────────────────────┘     │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  IOP — I/O Processor (MIPS R3000 @ 36 MHz)              │  │
│  │  Handles: CD/DVD, SPU2 (audio), USB, memory card, pad   │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  RAM: 32 MB RDRAM (EE) + 2 MB (IOP) + 16 KB Scratchpad        │
└─────────────────────────────────────────────────────────────────┘
```

---

## EE — Emotion Engine (Main CPU)

### Overview
- **ISA**: MIPS III with PS2-specific 128-bit extensions (MMI — Multimedia Instructions)
- **Clock**: ~294 MHz
- **Registers**: 32 × 128-bit general-purpose registers (though typically used as 32/64-bit)
- **Cache**: 16 KB I-cache, 8 KB D-cache, 16 KB scratchpad SRAM (fast, at `0x70000000`)

### AG-RAC Mapping
The EE's MIPS code becomes native x86-64 code after decompilation and recompilation. No emulation needed — the C source represents EE logic directly.

| PS2 EE Feature | AG-RAC PC Equivalent |
|---------------|---------------------|
| MIPS GPRs | x86-64 registers (mapped by compiler) |
| 128-bit MMI ops | SSE2/AVX2 SIMD intrinsics |
| Scratchpad SRAM | Stack / L1 cache (no direct equivalent) |
| MIPS FPU | x87 / SSE FPU |

---

## VU0 / VU1 — Vector Units

### Overview
The PS2 has two programmable co-processors for vector math:

- **VU0**: Attached to the EE as a co-processor. Used for physics, animation, and matrices. Accessed via `COP2` MIPS instructions.
- **VU1**: Fed by DMA (GIF path). Used almost exclusively for geometry transformation and lighting (the "geometry pipeline"). Runs custom **VU microcode**.

### VU Architecture
- 32 × 128-bit float registers (`VF00`–`VF31`)
- 16 × 16-bit integer registers (`VI00`–`VI15`)
- VLIW: upper and lower instruction slots execute in parallel
- 4096 bytes of instruction memory + 4096 bytes of data memory each

### AG-RAC Mapping (`pc/hal/vu/`)

| PS2 VU Feature | AG-RAC PC Equivalent |
|---------------|---------------------|
| VU0 COP2 instructions | Inline SSE/AVX SIMD (`__m128` operations) |
| VU1 microcode programs | C functions using `<immintrin.h>` SIMD |
| VU DMA transfers | Function call arguments / memory buffers |

The VU microcode programs (geometry shaders) will initially be translated to software C/SIMD, then optionally moved to GPU compute shaders for performance.

---

## GS — Graphics Synthesizer

### Overview
- **Rasterizer** only — no vertex transformation (that's VU1's job)
- **4 MB eDRAM** frame buffer (on-die, very fast)
- Accepts **GIFtags** (packed drawing commands) via the GIF interface
- Features: flat/gouraud shading, texture mapping, alpha blending, fog, z-buffer

### GIFtag Format
The GS is commanded via 128-bit GIFtag packets. Each packet carries:
- A tag header specifying the primitive type and number of vertices
- Vertex attribute data (RGBAQ, UV, XYZ)

### AG-RAC Mapping (`pc/hal/gs/`)

| PS2 GS Feature | AG-RAC PC Equivalent |
|---------------|---------------------|
| GIFtag packets | Translated to OpenGL draw calls |
| GS registers (ALPHA, TEST, etc.) | OpenGL state machine settings |
| eDRAM frame buffer | OpenGL FBO (Framebuffer Object) |
| Texture upload to GS | OpenGL `glTexImage2D` |
| Alpha blend modes | `glBlendFunc` / `glBlendEquation` |
| Z-buffer | OpenGL depth buffer |

---

## IOP — I/O Processor

### Overview
- MIPS R3000A @ 36 MHz (same as original PlayStation CPU)
- Runs Sony's proprietary OS kernel (BIOS modules)
- Handles all I/O: CD/DVD reads, controller input (via SIO2), memory card, USB
- Audio via **SPU2** (Sound Processing Unit 2): 48 voices, ADPCM decoder

### AG-RAC Mapping (`pc/hal/iop/`)

| PS2 IOP Feature | AG-RAC PC Equivalent |
|----------------|---------------------|
| CD/DVD file I/O | Windows/POSIX file I/O (`fopen`/`fread`) |
| Controller (SIO2) | SDL2 gamepad + keyboard (`SDL_GameController`) |
| Memory card | Save file in `%APPDATA%/AG-RAC/` |
| SPU2 ADPCM audio | Decoded VAG/PSX-ADPCM → SDL2 audio stream |

---

## DMA Controller

### Overview
The EE and IOP each have a DMA controller that moves data between components without CPU intervention. Critical paths:
- **GIF DMA (ch10)**: EE → GS (geometry/drawing packets)
- **VIF0 DMA (ch0)**: EE → VU0
- **VIF1 DMA (ch1)**: EE → VU1

### AG-RAC Mapping (`pc/hal/dma/`)
In the PC build, DMA transfers become synchronous function calls. The DMA abstraction layer is a thin no-op wrapper that directly calls the target subsystem function.

---

## Memory Map

See [`memory_map.md`](memory_map.md) for the full EE virtual address space map.

Key regions:
| Address | Size | Description |
|---------|------|-------------|
| `0x00000000` | 32 MB | EE RAM |
| `0x70000000` | 16 KB | Scratchpad SRAM |
| `0x80000000` | 32 MB | EE RAM (kseg0, cached) |
| `0xA0000000` | 32 MB | EE RAM (kseg1, uncached) |
| `0x12000000` | — | GS privileged registers |
| `0x10000000` | — | EE hardware registers |

---

## File Format Overview

See [`file_formats.md`](file_formats.md) for a complete breakdown of every known R&C data format.

Quick reference:
| Extension | Description |
|-----------|-------------|
| `SCUS_971.99` | Main game ELF executable |
| `.WAD` | Archive container (levels, assets) |
| `.PS2` | General data blob |
| TFrag | Terrain fragment geometry (in WAD) |
| MOBY | Entity/actor model format (in WAD) |
| TIE | Background instance geometry (in WAD) |
| `.VAG` | Sony ADPCM audio |
| `.VB` / `.VH` | Sound bank body / header |
