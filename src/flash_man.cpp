#include <QApplication>
#include <QtWidgets/QMessageBox>
#include <stdlib.h>
#include "flash_man.h"
#include "util.h"
#include "wflash.h"
#include "rom_head.h"


FlashMan::FlashMan(QTcpSocket *socket) {
    this->socket = socket;
}

void FlashMan::ByteSwapBuf(uint8_t *buffer, uint32_t len) {
    uint16_t *buf = (uint16_t*)buffer;
    len >>= 1;
    uint32_t i;

    for (i = 0; i < len; i++) {
        ByteSwapWord(buf[1]);
    }
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

    socket->waitForReadyRead(-1);
	recvd = socket->read((char*)buf, WF_HEADLEN + dataLen);
    printf("recvd %d\n", recvd);
	if ((recvd != (WF_HEADLEN + dataLen)) || (buf->cmd.cmd != WF_OK) ||
			(buf->cmd.len != dataLen)) {
        socket->close();
        QMessageBox::warning(NULL, "Connection error", "Error receiving data!");
		return -1;
	}
	return recvd;
}

/// \note Does only send command, does not send payload
int FlashMan::ProgramCmd(uint32_t addr, uint32_t len) {
    WfBuf buf;

    buf.cmd.cmd = WF_CMD_PROGRAM;
    buf.cmd.len = 8;
    buf.cmd.mem.addr = addr;
    buf.cmd.mem.len  = len;

    if (CmdSend(&buf.cmd) < 0) {
        return 1;
    }

    if (ReplyRecv(&buf, 0) < 0) {
        return 1;
    }

    return 0;
}

/// \note Does only send command, does not read data
int FlashMan::ReadCmd(uint32_t addr, uint32_t len) {
    WfBuf buf;

    buf.cmd.cmd = WF_CMD_READ;
    buf.cmd.len = 8;
    buf.cmd.mem.addr = addr;
    buf.cmd.mem.len = len;

    if (CmdSend(&buf.cmd) < 0) {
        return 1;
    }

    if (ReplyRecv(&buf, 0) < 0) {
        return 1;
    }

    return 0;
}

int FlashMan::Program(const uint8_t *data, bool autoErase,
		uint32_t start, uint32_t len) {
	uint32_t addr;
	int toWrite;
	uint32_t i;

	// If requested, perform auto-erase
	if (autoErase) {
		emit StatusChanged("Auto erasing");
		QApplication::processEvents();
		DelayMs(1);
        if (RangeErase(start, len)) {
			return 1;
		}
	}

	emit RangeChanged(0, len);
	emit ValueChanged(0);
	emit StatusChanged("Programming");
	QApplication::processEvents();
    puts("PROGRAMMING");

    // Command successful, send payload
	for (i = 0, addr = start; i < len;) {
		toWrite = MIN(45 * WF_MAX_DATALEN, len - i);
        if (ProgramCmd(addr, toWrite)) {
            return 1;
        }
    	if (socket->write((char*)(data + i), toWrite) != toWrite) {
    		PrintErr("Error sending Flash Program payload.\n");
    		return 1;
    	}
        socket->waitForBytesWritten(-1);
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

uint8_t *FlashMan::Read(uint32_t start, uint32_t len) {
	uint8_t *readBuf;
	int toRead;
	uint32_t addr;
	uint32_t i;
    size_t readed;

	emit RangeChanged(0, len);
	emit ValueChanged(0);
	emit StatusChanged("Reading");
	QApplication::processEvents();

    printf("start: %u, length: %u\n", start, len);
    puts("ALLOC");
	readBuf = (uint8_t*)malloc(len);
	if (!readBuf) {
		return NULL;
	}

    puts("CMD");
    if (ReadCmd(start, len)) {
        return NULL;
    }

    puts("LOOP");
    // Command accepted, read payload
	for (i = 0, addr = start; i < len;) {
		toRead = MIN(WF_MAX_DATALEN, len - i);
        socket->waitForReadyRead(-1);
        readed = socket->read((char*)(readBuf + i), toRead);
        printf("payload %lu\n", readed);
		if (readed <= 0) {
            QMessageBox::warning(NULL, "Connection error", "Failed to read bytes");
			return NULL;
		}
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
    buf.cmd.len = 8;
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
        ver[i] = buf.cmd.data[i];
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
        ids[i] = buf.cmd.data[i];
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
    // Do byte swaps
    ByteSwapBuf(writeBuf, *len);

    // Patch header
    RomHeadPatch(writeBuf);


    return writeBuf;
}

int FlashMan::WriteFile(const char *path, uint8_t *data, uint32_t len) {
    size_t wrote;
    FILE *dump;
   
    dump = fopen(path, "wb");
    if (!dump) {
        perror(path);
        return 1;
    }
    ByteSwapBuf(data, len);
    wrote = fwrite(data, len, 1, dump);
    fclose(dump);
    if (wrote != 1) {
        QMessageBox::warning(NULL, "Error", "Writing to file failed!");
        return 1;
    }

    return 0;
}

void FlashMan::FreeBuffer(uint8_t *buf) const {
    if (buf) free(buf);
}

void FlashMan::BufFree(uint8_t *buf) {
	free(buf);
}

