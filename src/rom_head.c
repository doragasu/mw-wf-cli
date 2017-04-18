/************************************************************************//**
 * rom_head: Megadrive ROM patch module.
 *
 * Currently the module only allows to patch the ROM header, adding the
 * wflash header information, and changing the entry point, for the
 * bootloader to be launched instead of the flashed ROM.
 ****************************************************************************/
#include "rom_head.h"


/// ROM header information structure, including interrupt vectors
typedef struct {
	// 64 interrupt/exception vectors
	uint32_t stackPtr;
	uint32_t entryPoint;
	uint32_t busErrEx;
	uint32_t addrErrEx;
	uint32_t illegalInstrEx;
	uint32_t zeroDivEx;
	uint32_t chkInstr;
	uint32_t trapvInstr;
	uint32_t privViol;
	uint32_t trace;
	uint32_t line1010Emu;
	uint32_t line1111Emu;
	uint32_t errEx[13];
	uint32_t int0;
	uint32_t extInt;
	uint32_t int1;
	uint32_t hInt;
	uint32_t int2;
	uint32_t vInt;
	uint32_t int3[33];

	// Header information
    char console[16];       ///< Console Name (16) */
    char copyright[16];     ///< Copyright Information (16) */
    char title_local[48];   ///< Domestic Name (48) */
    char title_int[48];     ///< Overseas Name (48) */
    char serial[14];        ///< Serial Number (2, 12) */
    uint16_t checksum;      ///< Checksum (2) */
    char IOSupport[16];     ///< I/O Support (16) */
    uint32_t rom_start;     ///< ROM Start Address (4) */
    uint32_t rom_end;       ///< ROM End Address (4) */
    uint32_t ram_start;     ///< Start of Backup RAM (4) */
    uint32_t ram_end;       ///< End of Backup RAM (4) */
    char sram_sig[2];       ///< "RA" for save ram (2) */
    uint16_t sram_type;     ///< 0xF820 for save ram on odd bytes (2) */
    uint32_t sram_start;    ///< SRAM start address - normally 0x200001 (4) */
    uint32_t sram_end;      ///< SRAM end address - start + 2*sram_size (4) */
    char modem_support[12]; ///< Modem Support (24) */
    char notes[40];         ///< Memo (40) */
    char region[16];        ///< Country Support (16) */
} RomHead;



/// ROM header
static const RomHead h = {
	// Note: non-character fields are byte-swapped because this
	// program is meant for little-endian architectures, and m68k
	// is big endian.
	// Vectors
	 0x00FEFF00,  0x00E03F00,  0xB0E03F00,  0xC6E03F00,
	 0xDCE03F00,  0xF2E03F00,  0x08E13F00,  0x1EE13F00,
	 0x34E13F00,  0x4AE13F00,  0x60E13F00,  0x60E13F00,
	{0x76E13F00,  0x76E13F00,  0x76E13F00,  0x76E13F00,
	 0x76E13F00,  0x76E13F00,  0x76E13F00,  0x76E13F00,
	 0x76E13F00,  0x76E13F00,  0x76E13F00,  0x76E13F00,
	 0x76E13F00}, 0x8CE13F00,  0x9EE13F00,  0x8CE13F00,
	 0xB0E13F00,  0x8CE13F00,  0xC2E13F00, {0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00,
	 0x8CE13F00,  0x8CE13F00,  0x8CE13F00,  0x8CE13F00},

	// Header
    "SEGA MEGA DRIVE ",
    "(c)doragasu 2017",
    "wfboot: WiFi Flash bootloader                   ",
    "wfboot: WiFi Flash bootloader                   ",
    "GM 00000000-00",
    0x0000,
    "JD              ",
    0x00E03F00,
    0x00004000,
	0x0000FF00,
	0xFFFFFF00,
    "  ",
    0x0000,
    0x00200000,
    0xFF012000,
    "            ",
    "PART OF MEGAWIFI PROJECT                ",
    "JUE             "
};

/************************************************************************//**
 * Patches the ROM header for the wflash bootloader to be launched instead
 * of the ROM, while trying to still make the ROM "launchable"
 *
 * \param[inout] head Pointer to the ROM, including the complete header
 ****************************************************************************/
void RomHeadPatch(uint8_t *head) {
	RomHead *rh = (RomHead*)head;

	// Copy the entry point to the NOTES section
	rh->notes[3] = rh->entryPoint>>24;
	rh->notes[2] = rh->entryPoint>>16;
	rh->notes[1] = rh->entryPoint>>8;
	rh->notes[0] = rh->entryPoint;
	// Patch the entry point for the bootloader to always be executed
	rh->entryPoint = h.entryPoint;
}
//void RomHeadPatch(uint8_t *head) {
//	RomHead *rh = (RomHead*)head;
//	int i;
//	uint16_t csum;
//
//	// Save checksum of the ROM to program, to restore it later
//	csum = rh->checksum;
//	// Copy header information from 0x100
//	for (i = 0x100; i < ROM_HEAD_LEN; i++) head[i] = ((uint8_t*)&h)[i];
//	rh->checksum = csum;
//
//	// Patch the entry point for the program to point to the bootloader
//	rh->entryPoint = h.entryPoint;
//	// Maybe we should patch the stack pointer to ensure bootloader always
//	// runs, but this could cause compatibility problems, so cross fingers
//	// and do not patch it.
//	// rh->stackPtr = h.stackPtr
//
//}

