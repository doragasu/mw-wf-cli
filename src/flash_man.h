/************************************************************************//**
 * \file
 *
 * \brief Flash Manager.
 *
 * \defgroup flash_man flash_man
 * \{
 * \brief Flash Manager
 *
 * Handles basic operations on flash chips (Program/Read/Erase) using MDMA
 * interface.
 *
 * \author doragasu
 * \date   2017
 ****************************************************************************/

#ifndef _FLASH_MAN_H_
#define _FLASH_MAN_H_

#include <QObject>
#include <stdint.h>

/// Chip length in bytes
#define FM_CHIP_LENGTH	0x400000

/************************************************************************//**
 * Flash Manager class.
 ****************************************************************************/
class FlashMan : public QObject {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Program a file to the flash chip.
	 *
	 * \param[in] filename  File name to program.
	 * \param[in] autoErase Erase the flash range where the file will be
	 *            flashed.
	 * \param[in] start     Word memory addressh where the file will be
	 *            programmed.
	 * \param[in] len       Number of words to write to the flash.
	 *
	 * \return A pointer to a buffer containing the file to flash (byte
	 * swapped) or NULL if the operation fails.
	 *
	 * \warning The user is responsible of freeing the buffer calling
	 * BufFree() when its contents are not needed anymore.
	 ************************************************************************/
	uint16_t *Program(const char filename[], bool autoErase,
			uint32_t *start, uint32_t *len);

	/********************************************************************//**
	 * Read a memory range from the flash chip.
	 *
	 * \param[in] start Word memory address to start reading from.
	 * \param[in] len   Number of words to read from flash.
	 *
	 * \return A pointer to the buffer containing the data read from the
	 * flash, or NULL if the read operation has failed.
	 *
	 * \warning The user is responsible of freeing the buffer calling
	 * BufFree() when its contents are not needed anymore.
	 ************************************************************************/
	uint16_t *Read(uint32_t start, uint32_t len);

	/********************************************************************//**
	 * Erases a memory range from the flash chip.
	 *
	 * \param[in] start Word memory address of the beginning of the range
	 *            to erase.
	 * \param[in] len   Length (in words) of the range to erase.
	 *
	 * \return 0 on success, non-zero if erase operation fails.
	 ************************************************************************/
	int RangeErase(uint32_t start, uint32_t len);

	/********************************************************************//**
	 * Issues a complete chip erase command to the flash chip.
	 *
	 * \return 0 on success, non-zero if erase operation fails.
	 ************************************************************************/
	int FullErase(void);

	/********************************************************************//**
	 * Frees a buffer previously allocated by Program() or Read().
	 *
	 * \param[in] buf The address of the buffer to free.
	 ************************************************************************/
	void BufFree(uint16_t *buf);

	/********************************************************************//**
	 * Obtains the flash chip 16-bit Manufacturer ID code.
	 *
	 * \param[out] manId The 16-bit Manufacturer ID of the flash chip.
	 *
	 * \return 0 on success, non-zero on error.
	 ************************************************************************/
	uint16_t ManIdGet(uint16_t *manId);

	/********************************************************************//**
	 * Obtains the 3 flash chip 16-bit Device ID codes.
	 *
	 * \param[out] devIds The 3 16-bit Device IDs of the flash chip.
	 *
	 * \return 0 on success, non-zero on error.
	 ************************************************************************/
	uint16_t DevIdGet(uint16_t devIds[3]);

signals:
	/********************************************************************//**
	 * RangeChanged signal. It is emitted by the Flash() and Read() methods
	 * when the length of the range ro flash/read is determinde.
	 *
	 * \param[in] min Lower value of the range.
	 * \param[in] max Higher value of the range.
	 ************************************************************************/
	void RangeChanged(int min, int max);

	/********************************************************************//**
	 * StatusChanged signal. It is emitted by the Flash() and Read() methods
	 * when their internal ostatus changes, to change the text of the
	 * control showing the operation status
	 *
	 * \param[in] status Status string.
	 ************************************************************************/
	void StatusChanged(const QString &status);

	/********************************************************************//**
	 * ValueChanged signal. It is emitted by the Flash() and Read() methods
	 * when the position of the flash/read cursor changes.
	 *
	 * \param[in] value Position of the flash/read cursor.
	 ************************************************************************/
	void ValueChanged(int value);
};

#endif /*_FLASH_MAN_H_*/

/** \} */

