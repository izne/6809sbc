# 6809sim — Self-contained 6809 system simulator

## Overview
A tiny, self-contained flexible 6809 system simulator in C/C++. It lets you load a ROM bios image or a 6809 program run it. A basic ACIA is as well simulated and bridged to stdout/stdin providing basic terminal emulation.

### Memory Map
- 32K RAM: 0x0000 – 0x7FFF
- 16K ROM: 0xC000 – 0xFFFF (combined.bin)
- ACIA: Control 0xA000 Data 0xA001
- Free: 0x8000 – 0xBFFF

### I/O behavior
- ACIA: on write, print the ASCII character to stdout and flush
- ACIA: on kbhit, transmit character code from keyboard
- STATUS 0xA001: return 0x01 and 0x02 (always ready)
- ROM region is read-only; writes are ignored
- Reset vector area (0xFFFE/0xFFFF)

### Loading modes
- Program in RAM: --load <filename>.HEX/.S09/.S19
  - The program is loaded into RAM (0x0000-0x7FFF)
  - The PC is set to the ORG address defined in the program
  - Execution starts automatically
- Firmware in ROM: --rom <filename>.BIN
  - A BIOS/firmware image is loaded into the ROM window (0xC000-0xFFFF)
  - Boot starts at the reset vector inside finename.bin.
  - Tested with combined.bin, the ROM image for Jeff Tranter and Grant Searle's 6809 SBCs.

### Boot behavior
- Program mode: auto-run after loading by having the PC to the ORG address of the program. Load address could be replaced via --load-addr 0x1234.
- ROM mode: auto-runs after loading by setting the PC to the contents of the RESET VECTOR (in 0xFFFE).

## Using the tool
- Run RAM HEX load and auto-run:
  - ./6809sbc --load hello.s19
- Run ROM bios load and auto-run:
  - ./6809sbc --rom combined.bin

## Hello World example
A ready-made hello.s19 is  included in examples/ (assembled from hello.asm using asm6809.exe -H) for you to enjoy. It loads the Hello World program into RAM at 0x1234 and auto-runs, printing "6809" to stdout via ACIA at 0xA000.

- hello.asm: simple Hello World program that prints via ACIA 0xA000
- hello.hex: ready-to-run Intel HEX file (assembled from hello.asm)
- hello.s19: ready-to-run Motorola SREC file (assembled from hello.asm)
