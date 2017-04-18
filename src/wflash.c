/*********************************************************************++*//**
 * \brief Allows remotely programming ROMs to MegaWiFi cartridges, as well
 * as performing other support functions like erasing, reading and getting
 * Flash chip identifiers.
 *
 * \author Jesús Alonso (doragasu)
 * \addtogroup WFlash wflash
 * \{
 ****************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "wflash.h"
#include "util.h"
#include "cmds.h"

/// Local module data structure.
typedef struct {
	WfBuf buf;						///< Data buffer
	int sock;						///< Client socket
	struct in_addr *srvAddr;		///< Server address
	union {
		uint16_t flags;				///< Various flags
		struct {
			uint16_t connected:1;	///< Connected to server if TRUE
			uint16_t reserved:15;	///< Unused flags
		};
	};
} WfData;

// Local module data
static WfData d;

/************************************************************************//**
 * Module initialization. Must be called once before using the module.
 ****************************************************************************/
void WfInit(void) {
	memset(&d, 0, sizeof(WfData));
}

int WfConnect(char host[], uint16_t port) {
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	struct addrinfo *srvInfo;
	char strPort[6];
	int flag = 1; 

	// DNS lookup code
	snprintf(strPort, 5, "%d", port);
	strPort[5] = '\0';
	if ((getaddrinfo(host, strPort, &hints, &srvInfo) != 0) || (!srvInfo)) {
		PrintErr("DNS error for %s:%s\n", host, strPort);
		if (srvInfo) freeaddrinfo(srvInfo);
		return WF_ERROR;
	}
	d.srvAddr = &((struct sockaddr_in *)srvInfo->ai_addr)->sin_addr;

	// Create socket ...
	d.sock = socket(srvInfo->ai_family, srvInfo->ai_socktype, 0);
	if (d.sock < 0) {
		freeaddrinfo(srvInfo);
		PrintErr("Could not create socket!\n");
		return WF_ERROR;
	}
	// Disable Nagle algorithm
	if (setsockopt(d.sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
			sizeof(int))) {
		freeaddrinfo(srvInfo);
		close(d.sock);
		PrintErr("Could not set socket options!\n");
	}
	// ... and connect!
	if (connect(d.sock, srvInfo->ai_addr, srvInfo->ai_addrlen) != 0) {
		close(d.sock);
		freeaddrinfo(srvInfo);
		PrintErr("Could not connect to %s:%s.\n", host, strPort);
		return WF_ERROR;
	}

	d.connected = TRUE;
	// Connection succesful!
	freeaddrinfo(srvInfo);
	return WF_OK;
}

void WfClose(void) {
	if (d.connected) close(d.sock);
}

// NOTE: Data must be directly copied to d.buf.cmd.data
static inline int WfCmdSend(uint16_t cmd, uint16_t dataLen) {
	d.buf.cmd.cmd = cmd;
	d.buf.cmd.len = dataLen;
	if (send(d.sock, &d.buf, dataLen + WF_HEADLEN, 0) !=
			(dataLen + WF_HEADLEN)) {
		close(d.sock);
		d.connected = FALSE;
		PrintErr("Error sending data to server!\n");
		return WF_ERROR;
	}
	return dataLen + WF_HEADLEN;
}


static inline int WfReplyRecv(int dataLen) {
	int recvd;

	recvd = recv(d.sock, &d.buf, WF_HEADLEN + dataLen, 0);
	if ((recvd != (WF_HEADLEN + dataLen)) || (d.buf.cmd.cmd != WF_OK) ||
			(d.buf.cmd.len != dataLen)) {
		close(d.sock);
		d.connected = FALSE;
		PrintErr("Error receiving data from server!\n");
		return WF_ERROR;
	}
	return recvd;
}

/************************************************************************//**
 * Obtains the version numbers of the bootloader.
 *
 * \return A two byte array with the bootloader version numbers (the first
 * is the major number and the second is the minor number), or NULL if
 * the version numbers could not be obtained.
 ****************************************************************************/
uint8_t *WfBootVerGet(void) {
	// Send command and receive reply
	if (WfCmdSend(WF_CMD_VERSION_GET, 0) != WF_HEADLEN) {
		PrintErr("Error requesting wflash version.\n");
		return NULL;
	}
	if (WfReplyRecv(2) != (2 + WF_HEADLEN)) {
		PrintErr("Error receiving version data.\n");
		return NULL;
	}
	return d.buf.cmd.data;
}

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
uint8_t *WfFlashIdsGet(void) {
	if (WfCmdSend(WF_CMD_ID_GET, 0) != WF_HEADLEN) {
		PrintErr("Error requesting flash IDs.\n");
		return NULL;
	}
	if (WfReplyRecv(4) != (4 + WF_HEADLEN)) {
		PrintErr("Error receiving IDs.\n");
		return NULL;
	}
	return d.buf.cmd.data;
}

/************************************************************************//**
 * Erases an address range of the flash chip.
 *
 * \param[in] addr Start address of the range to erase.
 * \param[in] len  Length of the address to range.
 *
 * \return WF_OK if connection is successful, WF_ERROR otherwise.
 * \warning
 ****************************************************************************/
int WfFlashErase(uint32_t addr, uint32_t len) {
	d.buf.cmd.dwdata[0] = addr;
	d.buf.cmd.dwdata[1] = len;

	if (WfCmdSend(WF_CMD_ERASE, 2 * 4) != (2 * 4 + WF_HEADLEN)) {
		PrintErr("Error requesting FLASH ERASE.\n");
		return WF_ERROR;
	}
	if (WfReplyRecv(0) != WF_HEADLEN) {
		PrintErr("Error executing FLASH ERASE.\n");
		return WF_ERROR;
	}
	return WF_OK;
}

/************************************************************************//**
 * Programs a data block to the specified Flash address
 *
 * \param[in] addr Address to which the block will be written.
 * \param[in] len  Length of the data block.
 * \param[in] data Data block to program to the Flash.
 *
 * \return The number of bytes programmed to the Flash chip.
 ****************************************************************************/
int WfFlash(uint32_t addr, uint32_t len, uint8_t data[]) {
//	uint32_t i;
//	uint16_t toSend;

	// Writing uses two stages:
	// 1. write command is issued.
	// 2. Once acknowledge, data is sent in chuncks of WF_MAX_DATALEN bytes
	//    maximum
	d.buf.cmd.dwdata[0] = addr;
	d.buf.cmd.dwdata[1] = len;
	if (WfCmdSend(WF_CMD_PROGRAM, 2 * 4) != (2 * 4 + WF_HEADLEN)) {
		PrintErr("Error requesting Flash Program.\n");
		return WF_ERROR;
	}
	if (WfReplyRecv(0) != WF_HEADLEN) {
		PrintErr("Error receiving Flash Program confirmation.\n");
		return WF_ERROR;
	}
	// Send data payload in chuncks of WF_MAX_DATALEN bytes
//	for (i = 0; i < len; i += toSend) {
//		toSend = MIN(len - i, WF_MAX_DATALEN);
//		memcpy(d.buf.data, data + i, toSend);
//		if (send(d.sock, d.buf.data, toSend, 0) != toSend) {
//			PrintErr("Error sending Flash Program payload.\n");
//			return WF_ERROR;
//		}
//	}
	if (send(d.sock, data, len, 0) != len) {
		PrintErr("Error sending data!\n");
		return WF_ERROR;
	}
	return WF_OK;
}

/************************************************************************//**
 * Reads a data block from the specified Flash address
 *
 * \param[in] addr Address from which to start reading.
 * \param[in] len  Length of the data block to read.
 * \param[in] buf  Buffer where the readed data will be placed.
 *
 * \return The number of bytes programmed to the Flash chip.
 ****************************************************************************/
uint32_t WfRead(uint32_t addr, uint32_t len, uint8_t buf[]) {
	uint32_t total;
	ssize_t recvd;

	// Reading uses two stages:
	// 1. Read command is issued.
	// 2. Once acknowledged, data is received in chuncks of WF_MAX_DATALEN
	//    bytes maximum (controlled automatically by recv().
	d.buf.cmd.dwdata[0] = addr;
	d.buf.cmd.dwdata[1] = len;
	if (WfCmdSend(WF_CMD_READ, 2 * 4) != (2 * 4 + WF_HEADLEN)) {
		PrintErr("Error requesting ROM read.\n");
		return 0;
	}
	if (WfReplyRecv(0) != WF_HEADLEN) {
		PrintErr("Error receiving ROM read confirmation.\n");
		return 0;
	}
	// Receive data
	total = 0;
	while ((recvd = recv(d.sock, buf + total, len - total, 0)) > 0) {
		total += recvd;
	}

	return total;
}

/************************************************************************//**
 * Reads a data block from the specified Flash address
 *
 * \param[in] addr Address from where to boot the ROM.
 *
 * \return WF_OK if boot command completed, or WF_ERROR if command failed.
 *
 * \note Because the bootloader will only wait a small fraction of time
 * before shutting down the WiFi module, it is possible that WF_ERROR is
 * returned on a boot command completed successfully.
 ****************************************************************************/
int WfBoot(uint32_t addr) {
	d.buf.cmd.dwdata[0] = addr;

	if (WfCmdSend(WF_CMD_RUN, 4) != (4 + WF_HEADLEN)) {
		PrintErr("Error requesting ROM boot.\n");
		return WF_ERROR;
	}
	if (WfReplyRecv(0) != WF_HEADLEN) {
		PrintErr("Error receiving ROM boot confirmation.\n");
		return WF_ERROR;
	}
	return WF_OK;
}

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
int WfAutoRun(void) {
	if (WfCmdSend(WF_CMD_AUTORUN, 0) != (WF_HEADLEN)) {
		PrintErr("Error requesting ROM boot.\n");
		return WF_ERROR;
	}
	if (WfReplyRecv(0) != WF_HEADLEN) {
		PrintErr("Error receiving ROM boot confirmation.\n");
		return WF_ERROR;
	}
	return WF_OK;
}

/** \} */
