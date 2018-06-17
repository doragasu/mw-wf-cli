/************************************************************************//**
 * \brief Megadrive ROM patch module.
 *
 * Currently the module only allows to patch the ROM header, adding the
 * wflash header information, and changing the entry point, for the
 * bootloader to be launched instead of the flashed ROM.
 *
 * \author Jesús Alonso (@doragasu)
 * \date   2017
 *
 * \defgroup RomHead rom_head
 * \{
 ****************************************************************************/

#ifndef _ROM_HEAD_H_
#define _ROM_HEAD_H_

#include <stdint.h>

/// Lengh of the complete header (including vectors) in bytes
#define ROM_HEAD_LEN	512

/// Default entry point (for 32 MBit ROMs with 32 KiB header)
#define ROM_HEAD_ENTRY_POINT    0x3F8000

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************//**
 * Patches the ROM header for the wflash bootloader to be launched instead
 * of the ROM, while trying to still make the ROM "launchable"
 *
 * \param[inout] head Pointer to the ROM, including the complete header
 ****************************************************************************/
void RomHeadPatch(uint8_t *head, uint32_t entryPoint);

#ifdef __cplusplus
}
#endif

#endif /*_ROM_HEAD_H_*/

/** \} */

