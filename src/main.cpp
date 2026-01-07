/*
6809sbc
A 6809 software-based computer system emulator, that can run .SREC/.HEX programs and ROM images. It is using the MC6809 library for CPU emulation.
2026, Dimitar Angelov https://github.com/izne/6809sbc/
*/

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <thread>
#include <conio.h>
#include "../MC6809/src/mc6809.hpp"

struct Version
{
	int major = 0;
	int minor = 2;
	const char* build_date = __DATE__;
	const char* build_time = __TIME__;
} v;

static uint8_t RAM[0x8000]; // RAM: 0x0000 - 0x7FFF (32K)
static uint8_t ROM[0x4000]; // ROM: 0xC000 - 0xFFFF (16K)
static uint16_t s_reset_vector = 0xFFFE;

static constexpr uint16_t ROM_BASE  = 0xC000;
static constexpr uint16_t RAM_LIMIT = 0x7FFF;
static constexpr uint16_t ROM_LIMIT = 0xFFFF;

uint16_t acia_base = 0xA000;

bool verbose = false, disassemble = false;
bool load_hex_to_ram(const std::string& path, uint16_t &first_addr);
bool load_srec_to_ram(const std::string& path, uint16_t &first_addr);
bool load_bin_to_rom(const std::string& path);

char text_buffer[512];

class cpu : public mc6809 {
public:
	cpu() {}

	uint8_t read8(uint16_t address) const override
	{
		if (true)
		{
			if (address == acia_base)
			{
				uint8_t status = 0x02;			// TDRE: always ready to write (bit 1 on 6850)
				if (_kbhit()) status |= 0x01;	// RDRF: key is pressed (bit 0 on 6850)
				return status;
			}
			if (address == acia_base + 1) return _kbhit() ? (uint8_t)_getch() : 0;
		}

		//if (address == 0xFFFE) { return (uint8_t)(s_reset_vector & 0x00FF); }
		//if (address == 0xFFFF) { return (uint8_t)((s_reset_vector >> 8) & 0x00FF); }
		//if (address == acia_base) { return 0x80; }

		if (address >= ROM_BASE) { return ROM[address - ROM_BASE]; }
		if (address <= RAM_LIMIT) { return RAM[address]; }
		
		return 0xFF;
	}

	void write8(uint16_t address, uint8_t value) const override {
		if (address == acia_base + 1)
		{
			if (value != 0) // do not print null values
			{
				fprintf(stdout, "%c", (char)value);
				fflush(stdout);
			}

			return;
		}

		if (address >= ROM_BASE) { return; }
		if (address <= RAM_LIMIT) { RAM[address] = value; return; }

		//if (address == 0xFFFE) { s_reset_vector = (s_reset_vector & 0xFF00) | (uint16_t)value; return; }
		//if (address == 0xFFFF) { s_reset_vector = (s_reset_vector & 0x00FF) | ((uint16_t)value << 8); return; }
	}
};

static void usage(const char* progname)
{
	fprintf(stderr, "Usage:\n  %s --verbose \n--disassemble \n--load <filename[.hex|.s19|.s09|.srec]> \n--load-addr 0xXXXX \n--rom <filename.bin>\n", progname);
}
static bool file_to_buffer(const std::string& path, std::vector<uint8_t>& out)
{
	std::ifstream f(path, std::ios::binary | std::ios::ate);
	if (!f) return false;

	std::ifstream::pos_type sz = f.tellg();
	f.seekg(0, std::ios::beg);
	out.resize((size_t)sz);

	if (!f.read(reinterpret_cast<char*>(out.data()), sz)) return false;

	return true;
}
static bool load_bin_to_rom(const std::string& path)
{
	std::fill(ROM, ROM + sizeof(ROM), 0xFF);
	std::vector<uint8_t> buf;

	if (!file_to_buffer(path, buf)) return false;

	size_t to_copy = std::min(buf.size(), sizeof(ROM));
	memcpy(ROM, buf.data(), to_copy);

	s_reset_vector = (uint16_t(ROM[0x3FFE]) << 8) | uint16_t(ROM[0x3FFF]); // last 2 bytes of the ROM should be the reset vector
	if (verbose) printf("Reset vector (ROM): 0x%x\n", s_reset_vector);

	return true;
}
static bool load_srec_to_ram(const std::string& path, uint16_t &first_addr)
{
	first_addr = 0;
	std::ifstream f(path);

	if (!f.is_open())
	{
		fprintf(stderr, "Failed to open file '%s'\n", path.c_str());
		return false;
	}

	std::string line;
	bool got = false;

	while (std::getline(f, line))
	{
		if (line.empty() || line[0] != 'S') continue;

		char type = line[1];
		int ll = 0;
		if (line.size() >= 4) ll = std::strtol(line.substr(2, 2).c_str(), nullptr, 16);
		int addr_len = 0;

		if (type == '1' || type == '9') addr_len = 2;
		else if (type == '2') addr_len = 3;
		else if (type == '3') addr_len = 4;
		else continue;

		if ((int)line.size() < 4 + addr_len*2) continue;
		uint32_t addr = 0;

		for (int i = 0; i < addr_len; ++i)
		{
			std::string part = line.substr(4 + i*2, 2);
			addr = (addr << 8) | (uint32_t)std::strtoul(part.c_str(), nullptr, 16);
		}

		int data_len = ll - (addr_len + 1);
		size_t data_start = 4 + addr_len*2;

		for (int i = 0; i < data_len; ++i)
		{

			if (data_start + i*2 + 2 > line.size()) break;

			std::string byte_str = line.substr(data_start + i*2, 2);
			uint8_t byte = (uint8_t)std::strtoul(byte_str.c_str(), nullptr, 16);
			uint16_t a = (uint16_t)(addr + i);

			if (a <= RAM_LIMIT) { RAM[a] = byte; if (!got) { first_addr = a; got = true; } }
		}

		if (type == '9') break; // EOF
	}

	f.close();
	return got;
}
static bool load_hex_to_ram(const std::string& path, uint16_t &first_addr)
{
	first_addr = 0;
	std::ifstream f(path);

	if (!f.is_open()) {
		fprintf(stderr, "Failed to open file '%s'\n", path.c_str());
		return false;
	}

	uint16_t override = 0;
	bool have_override = false;
	std::string line;
	bool got_data = false;

	while (std::getline(f, line)) {
		if (line.empty() || line[0] != ':') continue;
		int bc = 0; unsigned int addr = 0; int type = 0;
		std::string bc_str = line.substr(1,2);
		std::string addr_str = line.substr(3,4);
		std::string type_str = line.substr(7,2);
		bc = std::strtol(bc_str.c_str(), nullptr, 16);
		addr = std::strtol(addr_str.c_str(), nullptr, 16);
		type = std::strtol(type_str.c_str(), nullptr, 16);

		if (type == 0) {
			const char* dptr = line.c_str() + 9;
			for (int i=0; i<bc; ++i) {
				unsigned int byte = 0;
				std::string byte_str(dptr + i*2, 2);
				byte = std::strtoul(byte_str.c_str(), nullptr, 16);
				uint16_t address = (uint16_t)(addr + i);
				if (address <= RAM_LIMIT) { RAM[address] = (uint8_t)byte; if (!got_data) { first_addr = address; got_data = true; } }
			}
		} else if (type == 1) {
			break;
		}
	}

	f.close();
	return got_data;
}
static void dump_memory(uint16_t start, uint16_t end)
{
	for (uint16_t a = start; a <= end; ++a) printf("%04x: %02x\n", a, RAM[a]);
}


int main(int argc, char** argv)
{
	std::string load_path;
	std::string rom_path;
	uint16_t first_addr = 0;
	uint16_t explicit_load_addr = 0;
	bool has_explicit_load_addr = false;
	bool keep_on_moving = true;
	const uint32_t MAX_CYCLES = 2000000; 
	uint32_t cycles = 0;
	uint16_t pc_start = 0;
	
	printf("\n6809sbc v%i.%i, built %s, %s - 2026 Dimitar Angelov\n", v.major, v.minor, v.build_time, v.build_date);

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "--load" && i+1 < argc) { load_path = argv[++i]; }
		else if (arg == "--load-addr" && i+1 < argc) { explicit_load_addr = (uint16_t)strtoul(argv[++i], nullptr, 16); has_explicit_load_addr = true; }
		else if (arg == "--acia-addr" && i + 1 < argc) { acia_base = (uint16_t)strtoul(argv[++i], nullptr, 16); }
		else if (arg == "--rom" && i+1 < argc) { rom_path = argv[++i]; }
		else if (arg == "--verbose" && i + 1 < argc) { verbose = true; }
		else if (arg == "--disassemble" && i + 1 < argc) { disassemble = true; }
		else { usage(argv[0]); return 1; }
	}

	bool rom_present = !rom_path.empty();
	bool prg_present = !load_path.empty();
	
	// reset RAM/ROM
	std::fill(RAM, RAM + sizeof(RAM), 0x00);
	std::fill(ROM, ROM + sizeof(ROM), 0xFF);

	// load ROM
	if (!rom_path.empty()) {
		if (!load_bin_to_rom(rom_path)) {
			fprintf(stderr, "Error: failed to load ROM from '%s'\n", rom_path.c_str());
			return 1;
		}
	}
	
	// load program
	if (!load_path.empty())
	{
		std::string path_lower = load_path;
		std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(), ::tolower);
		bool ok = false;
		if (path_lower.size() >= 4 && path_lower.substr(path_lower.size()-4) == ".hex") {
			ok = load_hex_to_ram(load_path, first_addr);
		} else if ((path_lower.size() >= 4 && (path_lower.substr(path_lower.size()-4) == ".s19" || path_lower.substr(path_lower.size()-4) == ".s09")) ||
			(path_lower.size() >= 5 && path_lower.substr(path_lower.size()-5) == ".srec")) {
			ok = load_srec_to_ram(load_path, first_addr);
		} else {
			fprintf(stderr, "Error: unsupported load format '%s'\n", load_path.c_str());
			return 1;
		}
		if (!ok) {
			fprintf(stderr, "Error: failed to load file '%s' into RAM\n", load_path.c_str());
			return 1;
		}

		uint16_t load_addr = (has_explicit_load_addr ? explicit_load_addr : first_addr);
		if (load_addr == 0) load_addr = 0x1000;
		if(verbose) printf("Loaded %s to address 0x%04X\n", load_path.c_str(), load_addr);
		pc_start = load_addr;
	}
	else pc_start = s_reset_vector;

	if (verbose) printf("ACIA: Control 0x%X Data 0x%X\n", acia_base, acia_base + 1);

	// CPU
	cpu cpu;
	static bool nmi_pin = true, firq_pin = true, irq_pin = true;
	cpu.assign_nmi_line(&nmi_pin);
	cpu.assign_firq_line(&firq_pin);
	cpu.assign_irq_line(&irq_pin);

	cpu.set_sp(0x7FF0);
	cpu.reset();
	
	if (!rom_present)
	{
		if (verbose) printf("Setting PC: 0x%x\n\n", pc_start); else puts("\n");
		cpu.set_pc(pc_start);
	}

	// run
	while (keep_on_moving)
	{ 
		if (disassemble && cycles < 1200)
		{
			uint16_t temp_pc = cpu.get_pc();
			temp_pc += cpu.disassemble_instruction(text_buffer, 512, temp_pc);
			printf("%s\n", text_buffer);
		}
		
		if (cpu.get_pc() == 0xFFFF) keep_on_moving = false;

		uint16_t c = cpu.execute();
		cycles += c; 
	}

	if (verbose)
	{
		printf("\n\nPC:0x%04X A:0x%02X B:0x%02X X:0x%04X Y:0x%04X SP:0x%04X CC:0x%02X Cycles:%u\n",
			cpu.get_pc(), cpu.get_ac(), cpu.get_br(), cpu.get_xr(), cpu.get_yr(), cpu.get_sp(), cpu.get_cc(), cycles);
	}
	else 
		puts("\n");

	return 0;
}
