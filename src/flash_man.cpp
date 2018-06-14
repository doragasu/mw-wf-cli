#include <QApplication>
#include <QtWidgets/QMessageBox>
#include <stdlib.h>
#include "flash_man.h"
#include "util.h"
#include "wflash.h"


FlashMan::FlashMan(QTcpSocket *socket) {
    this->socket = socket;
}

int FlashMan::CmdSend(const WfCmd *cmd) {
    if (socket->write((char*)cmd, cmd->len + WF_HEADLEN) !=
            (cmd->len + WF_HEADLEN)) {
        QMessageBox::warning(NULL, "Connection error", "Sending data failed!");
		socket->close();
		return -1;
	}

	return (cmd->len + WF_HEADLEN);
}

int FlashMan::ReplyRecv(WfBuf *buf, int dataLen) {
	int recvd;

	recvd = socket->read((char*)buf, WF_HEADLEN + dataLen);
	if ((recvd != (WF_HEADLEN + dataLen)) || (buf->cmd.cmd != WF_OK) ||
			(buf->cmd.len != dataLen)) {
        socket->close();
        QMessageBox::warning(NULL, "Connection error", "Error receiving data!");
		return -1;
	}
	return recvd;
}

int FlashMan::Program(const uint8_t *buffer, bool autoErase,
		uint32_t start, uint32_t len) {
	uint32_t addr;
	int toWrite;
	uint32_t i;

	// If requested, perform auto-erase
	if (autoErase) {
		emit StatusChanged("Auto erasing");
		QApplication::processEvents();
		DelayMs(1);
//		if (MDMA_range_erase(*start, *len)) {
//			free(writeBuf);
//			return NULL;
//		}
	}

	emit RangeChanged(0, len);
	emit ValueChanged(0);
	emit StatusChanged("Programming");
	QApplication::processEvents();

	for (i = 0, addr = start; i < len;) {
		toWrite = MIN(32768, len - i);
//		if (MDMA_write(toWrite, addr, writeBuf + i)) {
//			free(writeBuf);
//			return NULL;
//		}
		// Update vars and draw progress bar
		i += toWrite;
		addr += toWrite;
		emit ValueChanged(i);
		QApplication::processEvents();
	}
	emit ValueChanged(i);
	emit StatusChanged("Done!");
	QApplication::processEvents();

	return 0;
}

uint16_t *FlashMan::Read(uint32_t start, uint32_t len) {
	uint16_t *readBuf;
	int toRead;
	uint32_t addr;
	uint32_t i;

	emit RangeChanged(0, len);
	emit ValueChanged(0);
	emit StatusChanged("Reading");
	QApplication::processEvents();

	readBuf = (uint16_t*)malloc(len<<1);
	if (!readBuf) {
		return NULL;
	}

	for (i = 0, addr = start; i < len;) {
		toRead = MIN(65536>>1, len - i);
//		if (MDMA_read(toRead, addr, readBuf + i)) {
//			free(readBuf);
//			return NULL;
//		}
		// Update vars and draw progress bar
		i += toRead;
		addr += toRead;
		emit ValueChanged(i);
		QApplication::processEvents();
	}
	emit ValueChanged(i);
	emit StatusChanged("Done");
	QApplication::processEvents();
	return readBuf;
}

int FlashMan::RangeErase(uint32_t start, uint32_t len) {
    WfBuf buf;

    buf.cmd.cmd = WF_CMD_ERASE;
    buf.cmd.mem.addr = start;
    buf.cmd.mem.len  = len;

    if (CmdSend(&buf.cmd) < 0) {
        return 1;
    }

    if (ReplyRecv(&buf, 0) < 0) {
        return 1;
    }

	return 0;
}

int FlashMan::FullErase(void) {
//	if (MDMA_cart_erase()) return -1;

	return 0;
}

int FlashMan::BootloaderVersionGet(uint8_t ver[2]) {
    WfBuf buf;

    buf.cmd.cmd = WF_CMD_VERSION_GET;
    buf.cmd.len = 0;

    if (CmdSend(&buf.cmd) < 0) {
        return 1;
    }

    if (ReplyRecv(&buf, 2) < 0) {
        return 1;
    }

    for (int i = 0; i < 2; i++) {
        ver[i] = buf.data[i];
    }

	return 0;
}

int FlashMan::IdsGet(uint8_t ids[4]) {
    WfBuf buf;

    buf.cmd.cmd = WF_CMD_ID_GET;
    buf.cmd.len = 0;

    if (CmdSend(&buf.cmd) < 0) {
        return 1;
    }

    if (ReplyRecv(&buf, 4) < 0) {
        return 1;
    }

    for (int i = 0; i < 4; i++) {
        ids[i] = buf.data[i];
    }

	return 0;
}

uint8_t *FlashMan::AllocFile(const char *path, uint32_t *len) {
    FILE *rom;
	uint8_t *writeBuf;
    size_t retVal;

	// Open the file to flash
	if (!(rom = fopen(path, "rb"))) return NULL;

	// Obtain length if not specified
	if (!(*len)) {
	    fseek(rom, 0, SEEK_END);
	    *len = ftell(rom);
	    fseek(rom, 0, SEEK_SET);
	}

    writeBuf = (uint8_t*)malloc(*len);
	if (!writeBuf) {
		fclose(rom);
		return NULL;
	}
    retVal = fread(writeBuf, *len, 1, rom);
	fclose(rom);
    if (retVal != 1) {
        free(writeBuf);
        return NULL;
    }

    /// \todo Byte swaps

    return writeBuf;
}

int FlashMan::WriteFile(const char *path, const uint8_t *buf, uint32_t len) {
}

void FlashMan::FreeBuffer(uint8_t *buf) const {
    if (buf) free(buf);
}

// NOTE: Data must be directly copied to d.buf.cmd.data
static inline int WfCmdSend(uint16_t cmd, uint16_t dataLen) {
	d.buf.cmd.cmd = cmd;
	d.buf.cmd.len = dataLen;
	if (send(d.sock, (char*)&d.buf, dataLen + WF_HEADLEN, 0) !=
			(dataLen + WF_HEADLEN)) {
		closesck(d.sock);
		d.connected = FALSE;
		PrintErr("Error sending data to server!\n");
		return WF_ERROR;
	}
	return dataLen + WF_HEADLEN;
}

void FlashMan::BufFree(uint16_t *buf) {
	free(buf);
}

