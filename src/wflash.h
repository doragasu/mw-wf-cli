/*********************************************************************++*//**
 * \brief Allows remotely programming ROMs to MegaWiFi cartridges, as well
 * as performing other support functions like erasing, reading and getting
 * Flash chip identifiers.
 *
 * \author Jesús Alonso (doragasu)
 * \date 2017
 * \defgroup WFlash wflash
 * \{
 ****************************************************************************/
#ifndef _WFLASH_H_
#define _WFLASH_H_

#include <stdint.h>

/// Function completed successfully
#define WF_OK		 0
/// Function completed with error
#define WF_ERROR	-1

/************************************************************************//**
 * Module initialization. Must be called once before using the module.
 ****************************************************************************/
void WfInit(void);

/************************************************************************//**
 * Connects to specified MegaWiFi host (address/IP and port).
 *
 * \param[in] host Host name of the MegaWiFi node. Address name and IP are
 *            supported.
 * \param[in] port TCP port number of the MegaWiFi host.
 *
 * \return WF_OK if connection is successful, WF_ERROR otherwise.
 ****************************************************************************/
int WfConnect(char host[], uint16_t port);

/************************************************************************//**
 * Closes a previously established connection with a MegaWiFi host.
 ****************************************************************************/
void WfClose(void);

/************************************************************************//**
 * Obtains the version numbers of the bootloader.
 *
 * \return A two byte array with the bootloader version numbers (the first
 * is the major number and the second is the minor number), or NULL if
 * the version numbers could not be obtained.
 ****************************************************************************/
uint8_t *WfBootVerGet(void);
	
/************************************************************************//**
 * Obtains the Flash chip identifiers.
 *
 * \return A four byte array with the Flash chip identifiers or NULL if the
 * Flash chip identifiers could not be obtained. The returned numbers are:
 * 1. The manufacturer ID
 * 2. The chip ID
 * 3. The chip ID (second byte)
 * 4. The chip ID (third byte)
 ****************************************************************************/
uint8_t *WfFlashIdsGet(void);

/************************************************************************//**
 * Erases an address range of the flash chip.
 *
 * \param[in] addr Start address of the range to erase.
 * \param[in] len  Length of the address to range.
 *
 * \return WF_OK if connection is successful, WF_ERROR otherwise.
 * \warning
 ****************************************************************************/
int WfFlashErase(uint32_t addr, uint32_t len);

/************************************************************************//**
 * Programs a data block to the specified Flash address
 *
 * \param[in] addr Address to which the block will be written.
 * \param[in] len  Length of the data block.
 * \param[in] data Data block to program to the Flash.
 *
 * \return The number of bytes programmed to the Flash chip.
 ****************************************************************************/
int WfFlash(uint32_t addr, uint32_t len, uint8_t data[]);

/************************************************************************//**
 * Reads a data block from the specified Flash address
 *
 * \param[in] addr Address from which to start reading.
 * \param[in] len  Length of the data block to read.
 * \param[in] buf  Buffer where the readed data will be placed.
 *
 * \return The number of bytes programmed to the Flash chip.
 ****************************************************************************/
uint32_t WfRead(uint32_t addr, uint32_t len, uint8_t buf[]);

/************************************************************************//**
 * Boots the ROM from the specified address.
 *
 * \param[in] addr Address from where to boot the ROM.
 *
 * \return WF_OK if boot command completed, or WF_ERROR if command failed.
 *
 * \note Because the bootloader will only wait a small fraction of time
 * before shutting down the WiFi module, it is possible that WF_ERROR is
 * returned on a boot command completed successfully.
 ****************************************************************************/
int WfBoot(uint32_t addr);

/************************************************************************//**
 * Boots the ROM automatically.
 *
 * \return WF_OK if boot command completed, or WF_ERROR if command failed.
 *
 * \note Because the bootloader will only wait a small fraction of time
 * before shutting down the WiFi module, it is possible that WF_ERROR is
 * returned on a boot command completed successfully.
 * \note The address used to boot is read from the beginning of the NOTES
 * field of the ROM header. The boot address is located there by the
 * ROM upload utility.
 ****************************************************************************/
int WfAutoRun(void);

#endif /*_WFLASH_H_*/

/** \} */

