# 6809sbc — A 6809 software-based computer

## Overview
A tiny, self-contained and flexible 6809 system simulator written in C/C++. It emulates a small 6809-based system and lets you load ROM images and/or a program to run on it. A basic ACIA is bridged to stdout/stdin providing terminal emulation.

## Dependencies
This project runs with MC6809 library to simulate the CPU instruction execution.
You need to have the MC6809 folder next to the 6809sbc/ folder.
Get the wonderful MC6809 at https://github.com/elmerucr/mc6809.git.

### Memory Map
- 32K RAM: 0x0000 – 0x7FFF
- 16K ROM: 0xC000 – 0xFFFF (combined.bin)
- ACIA: Control 0xA000 Data 0xA001
- Free: 0x8000 – 0xBFFF

### I/O behavior
- ACIA: on write, print the ASCII character to stdout and flush
- ACIA: on kbhit, transmit character code from host keyboard
- ACIA status return 0x01 and 0x02 (always ready)
- ROM region is read-only - writes are ignored
- 6809 Reset vector area (0xFFFE/0xFFFF)

### Loading modes
- Program in RAM: --load <filename>.HEX/.S09/.S19
  - The program is loaded into RAM (0x0000-0x7FFF)
  - The PC is set to the ORG address defined in the program
  - Execution starts automatically
- Firmware in ROM: --rom <filename>.BIN
  - A BIOS/firmware image is loaded into the ROM window (0xC000-0xFFFF)
  - Boot starts at the reset vector inside finename.bin.
  - Tested with combined.bin, the ROM image for Jeff Tranter and Grant Searle's 6809 SBCs.

## Usage
- Load (and run) a 6809 program in SREC/HEX format into RAM:
  - ./6809sbc --load hello.s19
- Load to specific address in RAM and run a 6809 program:
  - ./6809sbc --load hello.s19 --load-addr 0x3333
- Run ROM bios load and auto-run:
  - ./6809sbc --rom combined.bin
- Run ROM bios AND program load (boot from ROM, program is loaded in RAM only):
  - ./6809sbc --load hello.s19 --rom combined.bin
- Run with verbose output:
  - ./6809sbc --verbose --load hello.s19 --rom combined.bin
- Run and disassemble (for 1200 cycles), quick peek at program start
  - ./6809sbc --verbose -disassemble --load hello.s19



## Hello World example
A hello.s19 is included in examples/ (assembled from hello.asm using asm6809.exe -S) for you to enjoy. It loads a Hello World program into RAM at 0x1234 and auto-runs, printing "6809" to stdout via ACIA at 0xA000.

- hello.asm: simple Hello World program that prints via ACIA 0xA000
- hello.hex: ready-to-run Intel HEX file (assembled from hello.asm)
- hello.s19: ready-to-run Motorola SREC file (assembled from hello.asm)
