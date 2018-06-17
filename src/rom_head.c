/************************************************************************//**
 * rom_head: Megadrive ROM patch module.
 *
 * Currently the module only allows to patch the ROM header, adding the
 * wflash header information, and changing the entry point, for the
 * bootloader to be launched instead of the flashed ROM.
 ****************************************************************************/
#include "rom_head.h"
#include "util.h"

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

void RomHeadPatch(uint8_t *head, uint32_t entryPoint) {
	RomHead *rh = (RomHead*)head;

	// Copy the entry point to the NOTES section
	rh->notes[3] = rh->entryPoint>>24;
	rh->notes[2] = rh->entryPoint>>16;
	rh->notes[1] = rh->entryPoint>>8;
	rh->notes[0] = rh->entryPoint;
	// Patch the entry point for the bootloader to always be executed
	rh->entryPoint = ByteSwapDWord(entryPoint);
}
