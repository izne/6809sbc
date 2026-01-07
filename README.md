6809sim — Self-contained 6809 system simulator

What this is
A tiny, self-contained simulator under 6809sim/ that lets you load a ROM bios image or a RAM program written as an Intel HEX file and run it on a 6809 core from the existing MC6809 project. It uses the project’s mc6809 implementation in src/ and exposes a minimal RAM/ROM memory map with a simple ACIA I/O bridge that prints to stdout.

Memory Map (as requested)
- RAM: 0x0000 – 0x7FFF (32 KB)
- ACIA: TX at 0xA000 and 0xA001 (STATUS)
- ROM: 0xC000 – 0xFFFF (16 KB)
- Unused / reserved: 0x8000 – 0xBFFF

I/O behavior
- TX (0xA000): on write, print the ASCII character to stdout and flush
- STATUS (0xA001): read-only; return 0x80 (always ready)
- ROM region is read-only; writes are ignored
- Reset vector area (0xFFFE/0xFFFF): managed by a small shadow vector so RAM-mode and ROM-mode boots can be controlled at runtime

Loading modes
- RAM program (HEX): --ram <path>.HEX
  - The Intel HEX file is loaded into RAM (0x0000-0x7FFF)
  - The first data address found in the HEX stream becomes the auto-run start address
  - The reset vector (FFFE/FFFF) is updated to that start address so a reset boots into the RAM program
  - Execution starts automatically from the first loaded data address
- ROM bios: --rom <path>.BIN
  - A BIOS/firmware image is loaded into the ROM window (0xC000-0xFFFF)
  - Boot starts at 0xC000 (ROM_BASE). The reset vector inside bios.bin should be set to entry you want.
  - You can replace bios.bin with your combined.bin when you want to boot your SBC BIOS

Boot behavior
- RAM mode auto-runs after loading by setting the PC to the first data address loaded from HEX
- ROM mode auto-runs after loading by setting the PC to ROM_BASE (0xC000)
- The reset vector is kept in sync via a shadow vector in RAM mode; ROM mode uses bios.bin to provide the vector

Using the tool
- Build (in 6809sim/):
  - mkdir -p build && cd build
  - cmake .. -DCMAKE_BUILD_TYPE=Release
  - cmake --build . --config Release
- Run RAM HEX load and auto-run:
  - ./6809sim_main --ram path/to/your/program.hex
- Run ROM bios load and auto-run:
  - ./6809sim_main --rom path/to/bios.bin

Notes
- The code is designed to be self-contained and isolated to 6809sim/; it references the existing MC6809 sources under src/ (no changes to src/)
- You can extend the HEX loader later to support Motorola S-Records if desired
- The 32K RAM window and 16K ROM window map is aligned with your SBC hardware design and is simple to reason about inside the IDE

Hello Hex generation and usage
- I’ve added 6809sim/hello.asm (assembly example) and a small Python helper 6809sim/bin_to_hex.py to convert a binary Hello World image into an Intel HEX stream starting at 0x1000.
- To generate a HEX for your toolchain, assemble hello.asm to hello.bin, then run: python3 bin_to_hex.py hello.bin > hello.hex
- Then you can run: 6809sim_main --ram 6809sim/hello.hex

A ready-made hello.hex is now included in 6809sim/ (assembled from hello.asm using asm6809.exe -H). It loads the Hello World program into RAM at 0x1000 and auto-runs, printing "Hello World!" to stdout via ACIA at 0xA000.

If you want, I can also embed a pre-generated hello.hex, but it will be a simple data loader and require your own runtime code to print from ACIA. The preferred approach is to assemble your own binary via your familiar toolchain to match your SBC firmware.

Helpful files added in 6809sim/
- hello.asm: simple Hello World program that prints via ACIA 0xA000
- bin_to_hex.py: helper to convert a binary image to Intel HEX (start address default 0x1000; can be overridden)
- hello.hex: ready-to-run Intel HEX file (assembled from hello.asm)

Usage example (end-to-end)
1) Create hello.asm (or use your own assembly)
2) Assemble to hello.hex with your assembler (or use the provided hello.hex)
3) Run RAM load: 6809sim_main --ram 6809sim/hello.hex

If you want, I can add a ready-made hello.hex and a tiny bios.bin placeholder in the repo for quick-start testing. Just say the word and I’ll generate them.

Post-run Diagnostics
After a RAM or ROM run completes (e.g., when the program ends with SWI or reaches the cycle limit), the simulator prints a summary of the final CPU state. This is helpful for verifying correct execution without a debugger.

Example output:
```
=== Post-run Diagnostics ===
PC: 0x1000  A: 0x00  B: 0x00  X: 0x2014  Y: 0xA000  SP: 0x7FFF  CC: 0xD0  Cycles: 12345
```

- PC: Program counter (where execution stopped)
- A/B: Accumulator registers
- X/Y: Index registers
- SP: Stack pointer
- CC: Condition code register (flags)
- Cycles: Total CPU cycles consumed during the run