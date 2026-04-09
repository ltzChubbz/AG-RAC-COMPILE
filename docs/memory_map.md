# EE RAM Memory Map — Ratchet & Clank (NTSC-U, SCUS-97199)

This document tracks the known layout of the EE's 32 MB address space.
Addresses are populated through Ghidra static analysis and PCSX2 debugger sessions.

> **Status:** Skeleton — to be filled in as analysis progresses.

---

## EE Virtual Address Space

| Address Range | Size | Description |
|-------------|------|-------------|
| `0x00000000–0x01FFFFFF` | 32 MB | EE Main RAM |
| `0x02000000–0x0FFFFFFF` | — | Unmapped / reserved |
| `0x10000000–0x1001FFFF` | ~128 KB | EE Hardware Registers |
| `0x10003000` | — | GIF registers |
| `0x10008000` | — | VIF0 registers |
| `0x1000A000` | — | VIF1 registers |
| `0x11000000` | 4 KB | VU0 code memory |
| `0x11004000` | 4 KB | VU0 data memory |
| `0x11008000` | 4 KB | VU1 code memory |
| `0x1100C000` | 4 KB | VU1 data memory |
| `0x12000000–0x1200FFFF` | 64 KB | GS Privileged Registers |
| `0x1FC00000–0x1FFFFFFF` | 4 MB | BIOS ROM |
| `0x70000000–0x70003FFF` | 16 KB | Scratchpad SRAM |
| `0x80000000–0x81FFFFFF` | 32 MB | EE RAM mirror (kseg0, cached) |
| `0xA0000000–0xA1FFFFFF` | 32 MB | EE RAM mirror (kseg1, uncached) |

---

## ELF Load Address

**TBD** — to be read from the ELF program header by `tools/elf_info/elf_info.py`.

Typical R&C1 load address (preliminary): `0x00100000`

---

## Known Code Regions

> All addresses TBD — to be populated from Ghidra analysis.

| Address | Symbol | Description |
|---------|--------|-------------|
| TBD | `main` / entry | ELF entry point |
| TBD | `GameLoop` | Main game loop |
| TBD | `RenderFrame` | Top-level render dispatch |
| TBD | `UpdatePlayer` | Player update function |
| TBD | `UpdateCamera` | Camera update function |

---

## Known Data Regions

| Address | Symbol | Description |
|---------|--------|-------------|
| TBD | `gPlayerState` | Player state struct |
| TBD | `gCamera` | Camera state |
| TBD | `gMobyInstances` | Level MOBY instance array |
| TBD | `gLoadedLevel` | Currently loaded level data pointer |

---

## GS Privileged Registers (at `0x12000000`)

| Offset | Register | Description |
|--------|---------|-------------|
| `+0x0000` | `PMODE` | Readout circuit mode |
| `+0x0020` | `SMODE2` | NTSC/PAL sync mode |
| `+0x00A0` | `DISPFB1` | Display frame buffer 1 |
| `+0x00C0` | `DISPLAY1` | Display 1 settings |
| `+0x1000` | `CSR` | System status/control |

---

## References

- [PS2TEK Hardware Registers](https://psi-rockin.github.io/ps2tek/) — complete HW register reference
- PCSX2 source code — `pcsx2/Hardware.cpp` for memory-mapped I/O handling
