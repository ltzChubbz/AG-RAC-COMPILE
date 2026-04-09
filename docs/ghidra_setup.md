# Ghidra Setup Guide — AG-RAC

Step-by-step instructions for loading the R&C ELF into Ghidra for analysis.

---

## Prerequisites

1. **Ghidra 11+** — Download from [ghidra-sre.org](https://ghidra-sre.org/)
2. **Java 17+** — Required by Ghidra (bundled with newer versions)
3. **Ghidra Emotion Engine (EE) Plugin** — Adds PS2 R5900 processor support  
   → [beardypig/ghidra-emotionengine](https://github.com/beardypig/ghidra-emotionengine)
4. **Extracted ELF** — Run `tools/iso_extract/iso_extract.py` first

---

## Step 1: Install the EE Processor Plugin

1. Download the latest `.zip` release from [ghidra-emotionengine releases](https://github.com/beardypig/ghidra-emotionengine/releases)
2. In Ghidra: **File → Install Extensions**
3. Click the `+` button, select the downloaded `.zip`
4. Restart Ghidra

---

## Step 2: Create a New Project

1. **File → New Project**
2. Choose **Non-Shared Project**
3. Name it `AG-RAC` and save it within this repository under `ghidra_projects/` (this directory is gitignored)

---

## Step 3: Import the ELF

1. **File → Import File**
2. Navigate to `extracted/SCUS_971.99`
3. Ghidra should auto-detect: **Format: ELF**, **Language: MIPS:LE:32:R5900** (if EE plugin is installed)
   - If the EE plugin is not detected, manually select: **MIPS → LE → 32 → R5900**
4. Click **OK** and let the importer run

---

## Step 4: Run Auto-Analysis

1. Double-click the imported binary to open CodeBrowser
2. Ghidra will ask to run auto-analysis — click **Yes**
3. Keep all default options, but additionally enable:
   - **Decompiler Parameter ID** — helps recover function signatures
   - **Stack Analysis** — recovers local variable layout

Auto-analysis takes 5–20 minutes depending on your machine.

---

## Step 5: Load Symbol Hints

As we identify function addresses, we maintain a Ghidra script to apply labels automatically.

1. **Window → Script Manager**
2. Run `tools/ghidra_scripts/apply_labels.py` (once it exists)
3. This will apply all confirmed symbol names from `docs/memory_map.md`

---

## Step 6: Export Disassembly

Once analysis is complete, export the disassembly for the project:

1. **File → Export Program**
2. Format: **Assembly** or use the Ghidra script `tools/ghidra_scripts/export_asm.py`
3. Output to `asm/` in the project root

---

## Tips for PS2 ELF Analysis

### Branch Delay Slots
MIPS has **branch delay slots** — the instruction immediately after a branch/jump always executes before the branch takes effect. Ghidra handles this correctly for MIPS, but be aware when reading listings.

### Calling Convention
The PS2 SDK (SN Systems ProDG) follows the standard MIPS O32 ABI:
- `$a0`–`$a3` — first 4 integer arguments
- `$f12`–`$f15` — first 4 float arguments  
- `$v0`–`$v1` — return values
- `$ra` — return address

### Identifying C++ Classes
Insomniac used C++. Look for:
- `this` pointer in `$a0` for all member functions
- Vtables: arrays of function pointers in `.rodata`
- Constructors called in `operator new` patterns

### PCSX2 Debugger Integration
Use PCSX2's built-in debugger (in debug builds) to:
- Set breakpoints at suspected function entry points
- Observe register values to confirm function signatures
- Cross-reference with Ghidra listings
