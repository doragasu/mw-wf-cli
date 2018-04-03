/*********************************************************************++*//**
 * \brief wflash command line application. Uses wflash protocol to manage
 * the Flash memory of a MegaWiFi cartridge.
 *
 * \author Jesús Alonso (doragasu)
 * \date 2017
 *
 * \defgroup WfCli main
 * \{
 ****************************************************************************/
#include "util.h"
#ifdef __OS_WIN
#include <windows.h>
#else
#include <sys/ioctl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include "progbar.h"
#include "wflash.h"
#include "rom_head.h"

/// Maximum length of the file name string.
#define MAX_FILELEN		255
/// Maximum length of a write command.
#define MAX_WRITELEN	1376
/// Maximum length of a memory range.
#define MAX_MEM_RANGE	24
/// Major version of the comman-line application
#define VERSION_MAJOR	0x00
/// Minor version of the comman-line application
#define VERSION_MINOR	0x01

/// Structure containing a memory image (file, address and length)
typedef struct {
	char *file;		///< File name.
	uint32_t addr;	///< Memory address.
	uint32_t len;	///< Block length
} MemImage;

/// Commandline flags (for arguments without parameters).
typedef struct {
	union {
		uint16_t all;				///< Simultaneous access to all fields.
		struct {
			uint8_t verify:1;		///< Verify flash write if TRUE
			uint8_t verbose:1;		///< Print extra information on screen
			uint8_t flashId:1;		///< Show flash chip id
			uint8_t erase:1;		///< Erase flash
			uint8_t pushbutton:1;	///< Read pushbutton status
			uint8_t dry:1;			///< Dry run
			uint8_t boot:1;			///< Enter bootloader
			uint8_t noPatch:1;		///< Do not patch the source ROM
			uint8_t autoRun:1;		///< Run from entry point in cart header
			uint8_t unused:7;
		};
	};
	int cols;						///< Number of columns of the terminal
} Flags;

/// Available command-lline options
static const struct option opt[] = {
		{"wflash-addr",	required_argument,	NULL,   'a'},
		{"wflash-port",	required_argument,	NULL,   'p'},
        {"flash",       required_argument,  NULL,   'f'},
        {"read",        required_argument,  NULL,   'r'},
		{"auto-erase",	no_argument,		NULL,   'e'},
        {"sect-erase",  required_argument,  NULL,   's'},
        {"verify",      no_argument,        NULL,   'V'},
		{"no-patch",    no_argument,        NULL,   'n'},
		{"boot",        required_argument,  NULL,   'B'},
		{"auto-boot",   required_argument,  NULL,   'A'},
        {"flash-id",    no_argument,        NULL,   'i'},
		{"pushbutton",  no_argument,        NULL,   'P'},
        {"boot-ver",    no_argument,        NULL,   'b'},
		{"dry-run",     no_argument,		NULL,   'd'},
        {"version",     no_argument,        NULL,   'R'},
        {"verbose",     no_argument,        NULL,   'v'},
        {"help",        no_argument,        NULL,   'h'},
        {NULL,          0,                  NULL,    0 }
};

/// Description of each option
static const char *description[] = {
	"wflash server address (default 192.168.1.60)",
	"wflash server port (default 1989)",
	"Flash rom file",
	"Read ROM/Flash to file",
	"Automatically erase before write",
	"Erase flash range (with sector granularity)",
	"Verify flash after writing file",
	"Do not patch ROM. Warning, this will overwrite the bootloader!",
	"Run from Flash, at specified address",
	"Automatically run from entry point specified in ROM header",
	"Obtain flash chip identifiers",
	"Pushbutton status read (bit 1:event, bit0:pressed)",
	"Switch to bootloader mode",
	"Dry run: don't actually do anything",
	"Show program version",
	"Show additional information",
	"Print help screen and exit"
};

/// Default IP address of the MegaWiFi cartridge.
const static char defIp[] = "192.168.1.60";
/// Default port of the MegaWiFi cartridge.
const static uint16_t defPort = 1989;

/// 16-bit byte swap macro
#define ByteSwapWord(word)	do{(word) = ((word)>>8) | ((word)<<8);}while(0)

/************************************************************************//**
 * Print program version number.
 *
 * \param[in] prgName Current program name.
 ****************************************************************************/
void PrintVersion(char prgName[]) {
	printf("%s version %d.%d, doragasu 2017.\n", prgName,
			VERSION_MAJOR, VERSION_MINOR);
}

/************************************************************************//**
 * Print utility help.
 *
 * \param[in] prgName Utility program name.
 ****************************************************************************/
void PrintHelp(char *prgName) {
	int i;

	PrintVersion(prgName);
	printf("Usage: %s [OPTIONS [OPTION_ARG]]\nSupported options:\n\n", prgName);
	for (i = 0; opt[i].name; i++) {
		printf(" -%c, --%s%s: %s.\n", opt[i].val, opt[i].name,
				opt[i].has_arg == required_argument?" <arg>":"",
				description[i]);
	}
	// Print additional info
	printf("For file arguments, it is possible to specify start address and "
		   "file length to read/write in bytes, with the following format:\n"
		   "    file_name:memory_address:file_length\n\n"
		   "Examples:\n"
		   " - Auto erase Flash and write entire ROM to cartridge: %s -ef rom_file\n"
		   " - Flash and verify 32 KiB to 0x700000: "
		   "%s -Vf rom_file:0x700000:32768\n"
		   " - Dump 1 MiB of the cartridge: %s -r rom_file::1048576\n",
		   prgName, prgName, prgName);
		   
}

/************************************************************************//**
 * Receives a MemImage pointer with full info in file name (e.g.
 * m->file = "rom.bin:6000:1"). Removes from m->file information other
 * than the file name, and fills the remaining structure fields if info
 * is provided (e.g. info in previous example would cause m = {"rom.bin",
 * 0x6000, 1}).
 *
 * \param[inout] m Pointer to the MemImage structure to parse.
 *
 * \return 0 if input properly parsed, non-zero if error.
 ****************************************************************************/
int ParseMemArgument(MemImage *m) {
	int i;
	char *addr = NULL;
	char *len  = NULL;
	char *endPtr;

	// Set address and length to default values
	m->len = m->addr = 0;

	// First argument is name. Find where it ends
	for (i = 0; i < (MAX_FILELEN + 1) && m->file[i] != '\0' &&
			m->file[i] != ':'; i++);
	// Check if end reached without finding end of string
	if (i == (MAX_FILELEN + 1)) return 1;
	if (m->file[i] == '\0') return 0;
	
	// End of token marker, search address
	m->file[i++] = '\0';
	addr = m->file + i;
	for (; i < (MAX_FILELEN + 1) && m->file[i] != '\0' && m->file[i] != ':';
			i++);
	// Check if end reached without finding end of string
	if (i == MAX_FILELEN + 1) return 1;
	// If end of token marker, search length
	if (m->file[i] == ':') {
		m->file[i++] = '\0';
		len = m->file + i;
		// Verify there's an end of string
		for (; i < (MAX_FILELEN + 1) && m->file[i] != '\0'; i++);
		if (m->file[i] != '\0') return 1;
	}
	// Convert strings to numbers and return
	if (addr && *addr) m->addr = strtol(addr, &endPtr, 0);
	if (m->addr == 0 && addr == endPtr) return 2;
	if (len  && *len)  m->len  = strtol(len, &endPtr, 0);
	if (m->len  == 0 && len  == endPtr) return 3;

	return 0;
}

/************************************************************************//**
 * Parses an input string containing a memory range, obtaining the address
 * and length. If any of these parameters is not present, they are set to
 * 0. Parameters are separated by colon character.
 *
 * \param[in]  inStr Input string containing the memory range.
 * \param[out] addr  Parsed address.
 * \param[out] len   Parsed length.
 *
 * \return 0 if OK, 1 if error.
 ****************************************************************************/
int ParseMemRange(char inStr[], uint32_t *addr, uint32_t *len) {
	int32_t i;
	char *saddr, *endPtr;
	char scratch;
	long val;

	// Seek end of string or field separator (:)
	for (i = 0; (i < (MAX_MEM_RANGE + 1)) && (inStr[i] != '\0') &&
			(inStr[i] != ':'); i++);
	
	if (i == (MAX_MEM_RANGE + 1)) return 1;
	// Store end of string or separator, and ensure proper end of string
	scratch = inStr[i];
	inStr[i++] = '\0';
	// Convert to long
	val = strtol(inStr, &endPtr, 0);
	if (*endPtr != '\0' || val < 0) return 1;
	*addr = val;
	// If we had field separator, repeat scan for length
	if (scratch == '\0') return 0;
	saddr = inStr + i;
	for (; (i < (MAX_MEM_RANGE + 1)) && (inStr[i] != '\0'); i++);
	if (i == (MAX_MEM_RANGE + 1)) return 1;
	val = strtol(saddr, &endPtr, 0);
	if (*endPtr != '\0' || val < 0) return 1;
	*len = val;
	return 0;
}


/************************************************************************//**
 * Prints a properly parsed memory image.
 *
 * \param[in] m Pointer to the parsed MemImage structure to print.
 ****************************************************************************/
void PrintMemImage(MemImage *m) {
	printf("%s", m->file);
	if (m->addr) printf(" at address 0x%06X", m->addr);
	if (m->len ) printf(" (%d bytes)", m->len);
}

/************************************************************************//**
 * Prints the error message related to an unsuccessful ParseMemRange() call.
 *
 * \param[in] code Error code returned by ParseMemRange().
 ****************************************************************************/
void PrintMemError(int code) {
	switch (code) {
		case 0: printf("Memory range OK.\n"); break;
		case 1: PrintErr("Invalid memory range string.\n"); break;
		case 2: PrintErr("Invalid memory address.\n"); break;
		case 3: PrintErr("Invalid memory length.\n"); break;
		default: PrintErr("Unknown memory specification error.\n");
	}
}

/************************************************************************//**
 * Allocs a buffer, reads a file to the buffer, and flashes the file pointed 
 * by the file argument. The buffer must be deallocated when not needed,
 * using free() call.
 *
 * \param[in] fWr      Memory image to flash.
 * \param[in] autoErase Set to true to perform an erase operation before
 *            flashing the memory image. Only the covered range is erased.
 * \param[in] noPatch  If nonzero, ROM must be written 1:1 (i.e. it will
 * 			  be neither patched nor trimmed).
 * \param[in] columns  Number of columns of the console, used to display
 *            the progress bar while flashing.
 *
 * \return Pointer to the allocated and flashed memory if OK, NULL if error.
 *
 * \note fWr.len is updated if not specified.
 * \warning A successfully allocated buffer must be freed externally to the
 *          function, using a free() call.
 ****************************************************************************/
uint16_t *AllocAndFlash(MemImage *fWr, int autoErase, int noPatch,
		int columns) {
    FILE *rom;
	uint16_t *writeBuf;
	uint32_t addr;
	int toWrite;
	uint32_t i;
	// Address string, e.g.: 0x123456
	char addrStr[9];

	// If header covered by range, but not completeley, reject flash command
	if (!autoErase && ((fWr->addr < ROM_HEAD_LEN && fWr->addr) ||
			(!fWr->addr && fWr->len < ROM_HEAD_LEN))) return NULL;
	// Open the file to flash
	if (!(rom = fopen(fWr->file, "rb"))) {
		perror(fWr->file);
		return NULL;
	}

	// Obtain length if not specified
	if (!fWr->len) {
	    fseek(rom, 0, SEEK_END);
	    fWr->len = ftell(rom);
	    fseek(rom, 0, SEEK_SET);
	}

    writeBuf = malloc(fWr->len);
	if (!writeBuf) {
		perror("Allocating write buffer RAM");
		fclose(rom);
		return NULL;
	}
    fread(writeBuf, fWr->len, 1, rom);
	fclose(rom);
	// If requested, perform auto-erase
	if (autoErase) {
		printf("Auto-erasing range 0x%06X:%06X...\n", fWr->addr, fWr->len);
		if (WfFlashErase(fWr->addr, fWr->len)) {
			free(writeBuf);
			PrintErr("Auto-erase failed!\n");
			return NULL;
		}
	}

	// If header is included in flash image, and unless prohibited
	if (!fWr->addr && !noPatch) RomHeadPatch((uint8_t*)writeBuf);

   	printf("Flashing ROM %s starting at 0x%06X...\n", fWr->file, fWr->addr);

	for (i = 0, addr = fWr->addr; i < fWr->len;) {
//		toWrite = MIN(2*1152, fWr->len - i);
//		toWrite = MIN(57600, fWr->len - i);
//		toWrite = MIN(1440, fWr->len - i);
		toWrite = MIN(64800, fWr->len - i);
		if (WfFlash(addr, toWrite, ((uint8_t*)writeBuf) + i)) {
			free(writeBuf);
			PrintErr("Couldn't write to cart!\n");
			return NULL;
		}
		// Update vars and draw progress bar
		i += toWrite;
		addr += toWrite;
   	    sprintf(addrStr, "0x%06X", addr);
   	    ProgBarDraw(i, fWr->len, columns, addrStr);
	}
   	putchar('\n');
	return writeBuf;
}

/************************************************************************//**
 * Allocs a buffer and reads from cart. Does NOT save the buffer to a file.
 * Buffer must be deallocated using free() when not needed anymore.
 *
 * \param[in] fRd     Memory image to read.
 * \param[in] columns Number of columns of the console, used to display
 *                    the progress bar while flashing.
 *
 * \return Pointer to the allocated and readed memory if OK, NULL if error.
 *
 * \warning A successfully allocated buffer must be freed externally to the
 *          function, using a free() call.
 ****************************************************************************/
uint16_t *AllocAndRead(MemImage *fRd, int columns) {
	uint16_t *readBuf;
	int toRead;
	uint32_t addr;
	uint32_t i;
	// Address string, e.g.: 0x123456
	char addrStr[9];

	readBuf = malloc(fRd->len);
	if (!readBuf) {
		perror("Allocating read buffer RAM");
		return NULL;
	}
	printf("Reading cart starting at 0x%06X...\n", fRd->addr);

	fflush(stdout);
	for (i = 0, addr = fRd->addr; i < fRd->len;) {
		toRead = MIN(3840, fRd->len - i);
		if (WfRead(addr, toRead, ((uint8_t*)readBuf) + i)) {
			free(readBuf);
			PrintErr("Couldn't read from cart!\n");
			return NULL;
		}
		fflush(stdout);
		// Update vars and draw progress bar
		i += toRead;
		addr += toRead;
   	    sprintf(addrStr, "0x%06X", addr);
   	    ProgBarDraw(i, fRd->len, columns, addrStr);
	}
	putchar('\n');
	return readBuf;
}

/************************************************************************//**
 * Entry point. Parses command line and executes requested actions.
 *
 * \param[in] argc Number of input parameters.
 * \param[in] argv List of input parameters.
 *
 * \return 0 if completed successfully, non-zero if error.
 ****************************************************************************/
int main( int argc, char **argv )
{
	// Command-line flags
	Flags f;
	// Rom file to write to flash
	MemImage fWr = {NULL, 0, 0};
	// Rom file to read from flash (default read length: 4 MiB)
	MemImage fRd = {NULL, 0, 4*1024*1024};
	// Error code for function calls
	int errCode;
	// Buffer pointer for writing data to cart
    uint16_t *write_buffer = NULL;
	// Buffer pointer for reading cart data
	uint16_t *read_buffer = NULL;
	// Server address string
	char *srvAddr = (char*)defIp;
	// Server IP port
	long srvPort = defPort;
	// For strtol
	char *endPtr;
	// Erase address
	uint32_t eraseAddr = 0;
	// Erase length
	uint32_t eraseLen = 0;
	// Boot address
	uint32_t bootAddr = 0;
	// Temporary uint16_t pointer
	uint8_t *tmp;

	// Loop iteration
	int i;

	// Set all flags to FALSE
	f.all = 0;
    // Reads console arguments
    if( argc > 1 ) {
        // Option index, for command line options parsing
        int opIdx = 0;
        // Character returned by getopt_long()
        int c;

        while ((c = getopt_long(argc, argv, "a:p:f:r:es:VnB:AiPbdRvh", opt, &opIdx)) != -1) {
			// Parse command-line options
            switch (c) {
				case 'a': // Set server address
					srvAddr = optarg;
					break;
					
				case 'p': // Set server port number
					srvPort = strtol(optarg, &endPtr, 0);
					if ((srvPort < 0) || (srvPort > 65535) || (*endPtr == '\0')) {
						PrintErr("Invalid port %s!\n", optarg);
						return 1;
					}
					break;

                case 'f': // Write flash
					fWr.file = optarg;
					if ((errCode = ParseMemArgument(&fWr))) {
						PrintErr("Error: On Flash write argument: ");
						PrintMemError(errCode);
						return 1;
					}
	                break;

                case 'r': // Read flash
					fRd.file = optarg;
					if ((errCode = ParseMemArgument(&fRd))) {
						PrintErr("Error: On Flash read argument: ");
						PrintMemError(errCode);
						return 1;
					}
                	break;

                case 'e': // Auto erase
					f.erase = TRUE;
                	break;

				case 's': // Sector range erase
					if ((errCode = ParseMemRange(optarg, &eraseAddr, &eraseLen)) ||
							(0 == eraseLen)) {
						PrintErr("Error: Invalid Flash erase range argument: %s\n", optarg);
						return 1;
					}
					break;

                case 'V': // Verify flash write
					f.verify = TRUE;
                	break;

                case 'n': // Do not patch ROM
					f.noPatch = TRUE;
                	break;

                case 'B': // Boot from address
					bootAddr = strtol(optarg, &endPtr, 0);
					if (bootAddr < 0x200) {
						PrintErr("Invalid boot address %s, must be 0x200 or greater.\n", optarg);
						return 1;
					}
                	break;

                case 'A': // Auto-run
					f.autoRun = TRUE;
	                break;

                case 'i': // Flash id
					f.flashId = TRUE;
	                break;

                case 'P': // Read pushbutton
					f.pushbutton = TRUE;
	                break;

                case 'b': // Get bootloader version
					f.boot = TRUE;
					break;

				case 'd': // Dry run
					f.dry = TRUE;
					break;

                case 'R': // Version
					PrintVersion(argv[0]);
                	return 0;

                case 'v': // Verbose
					f.verbose = TRUE;
                	break;

                case 'h': // Help
					PrintHelp(argv[0]);
                	return 0;

                case '?':       // Unknown switch
					putchar('\n');
                	PrintHelp(argv[0]);
                return 1;
            }
        }
    }
    else
    {
		printf("Nothing to do!\n");
		PrintHelp(argv[0]);
		return 0;
}

	if (optind < argc) {
		PrintErr("Unsupported parameter:");
		for (i = optind; i < argc; i++) PrintErr(" %s", argv[i]);
		PrintErr("\n\n");
		PrintHelp(argv[0]);
		return -1;
	}
	// Check for conflicting options
	if (f.erase) {
		if (eraseLen) {
			PrintErr("Sector erase and auto erase options cannot be used simultaneously!\n");
			return 1;
		}
		if (!fWr.file) {
			PrintErr("Auto erase option can only be used when performing writes!\n");
			return 1;
		}
	}
	if (f.autoRun && bootAddr) {
		PrintErr("Using run (from address) and auto-run options at the same time is not supported!\n");
		return 1;
	}

	if (f.verbose) {
		printf("Server address: %s:%d\n", srvAddr, (uint16_t)srvPort);
		printf("\nThe following actions will%s be performed (in order):\n",
				f.dry?" NOT":"");
		printf("==================================================%s\n\n",
				f.dry?"====":"");
		if (f.boot) printf(" - Show bootloader version.\n");
		if (f.flashId) printf(" - Show Flash chip identification.\n");
		if (f.erase) printf(" - Auto erase Flash.\n");
		else if (eraseLen)
			printf(" - Erase range %06X:%X.\n", eraseAddr, eraseLen);
		if (fWr.file) {
		   printf(" - Flash %s", f.verify?"and verify ":"");
		   PrintMemImage(&fWr); putchar('\n');
		}
		if (fRd.file) {
			printf(" - Read ROM/Flash to ");
			PrintMemImage(&fRd); putchar('\n');
		}
		if (bootAddr) {
			printf(" - Boot ROM from 0x%06X.\n", bootAddr);
		}
		if (f.pushbutton) {
			printf(" - Read pushbutton.\n");
		}
		if (f.boot) {
			printf(" - Enter bootloader\n");
		}
		printf("\n");
	}

	if (f.dry) return 0;

	// Detect number of columns (for progress bar drawing).
#ifdef __OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    f.cols = csbi.srWindow.Right - csbi.srWindow.Left;
#else
    struct winsize max;
    ioctl(0, TIOCGWINSZ , &max);
	f.cols = max.ws_col;

	// Also set transparent cursor
	printf("\e[?25l");
#endif

	// Connect to server
	if (WfConnect(srvAddr, srvPort)) {
		PrintErr("Error: couldn't connect to server at %s:%d.\n",
				srvAddr, (uint16_t)srvPort);
		goto dealloc_exit;
	}

	// TODO WARNING: Does not work if this is not here!!!
#ifdef __WIN32__
	Sleep(1000);
#else
    sleep(1);
#endif

	/****************** ↓↓↓↓↓↓ DO THE MAGIC HERE ↓↓↓↓↓↓ *******************/

	// Default exit status: OK
	errCode = 0;

	// Get bootloader version
	if (f.boot) {
		if (!(tmp = WfBootVerGet())) return -1;
		printf("WFlash version %d.%d\n", tmp[0], tmp[1]);
	}
	// GET IDs	
	if (f.flashId) {
		if ((tmp = WfFlashIdsGet()) == NULL) return -1;
		printf("Manufacturer ID: 0x%02X\n", tmp[0]);
		printf("Device IDs: 0x%02X:%02X:%02X\n", tmp[1],
			tmp[2], tmp[3]);
	}
	// Erase
	// Support sector erase!
	if (eraseLen) {
		printf("Erasing cart range 0x%06X:%06X...\n", eraseAddr, eraseLen);
		if (WfFlashErase(eraseAddr, eraseLen)) {
			printf("Erase failed!\n");
			return 1;
		}
		else printf("OK!\n");
	}
	// Flash
	if (fWr.file) {
		write_buffer = AllocAndFlash(&fWr, f.erase, f.noPatch, f.cols);
		if (!write_buffer) {
			PrintErr("Flash ROM error!\n");
			errCode = 1;
			goto dealloc_exit;
		}
	}

	// Boot ROM from address
	if (bootAddr) {
		printf("Booting ROM at address 0x%06X...\n", bootAddr);
		if (WfBoot(bootAddr) != WF_OK) {
			PrintErr("Boot ROM error!\n");
			errCode = 1;
			goto dealloc_exit;
		}
	}

	// Auto boot from header entry point
	if (f.autoRun) {
		printf("Auto-booting ROM...\n");
		if (WfAutoRun() != WF_OK) {
			PrintErr("Boot ROM error!\n");
			errCode = 1;
			goto dealloc_exit;
		}
	}

//	if (fRd.file || f.verify) {
//		// If verify is set, ignore addr and length set in command line.
//		if (f.verify) {
//			fRd.addr = fWr.addr;
//			fRd.len  = fWr.len;
//		}
//		read_buffer = AllocAndRead(&fRd, f.cols);
//		if (!read_buffer) {
//			errCode = 1;
//			goto dealloc_exit;
//		}
//		// Verify
//		if (f.verify) {
//			for (i = 0; i < fWr.len; i++) {
//				if (write_buffer[i] != read_buffer[i]) {
//					break;
//				}
//			}
//			if (i == fWr.len)
//				printf("Verify OK!\n");
//			else {
//				printf("Verify failed at addr 0x%07X!\n", i + fWr.addr);
//				printf("Wrote: 0x%04X; Read: 0x%04X\n", write_buffer[i],
//						read_buffer[i]);
//				// Set error, but we do not exit yet, because user might want
//				// to write readed data to a file!
//				errCode = 1;
//			}
//		}
//		// Write file
//		if (fRd.file) {
//			// Do byte swaps
//			for (i = 0; i < fRd.len; i++) ByteSwapWord(read_buffer[i]);
//        	FILE *dump = fopen(fRd.file, "wb");
//			if (!dump) {
//				perror(fRd.file);
//				errCode = 1;
//				goto dealloc_exit;
//			}
//	        fwrite(read_buffer, fRd.len<<1, 1, dump);
//	        fclose(dump);
//			printf("Wrote file %s.\n", fRd.file);
//		}
//	}
//
//	if (f.pushbutton) {
//		u16 retVal;
//		u8 butStat;
//		retVal = MDMA_button_get(&butStat);
//		if (retVal < 0) {
//			errCode = 1;
//		} else {
//			PrintVerb("Button status: 0x%02X.\n", butStat);
//			errCode = butStat;
//		}
//		goto dealloc_exit;
//	}
//
//	// WiFi Firmware upload
//	if (fWf.file) {
//		// Currlently, length argument is not supported, always the comlete
//		// file is flashed.
//		if (fWf.len) {
//			PrintErr("Warning: length parameter not supported for WiFi "
//					"firmware files, ignoring.\n");
//			errCode = 1;
//		}
//		else {
//			if (0 > EpBlobFlash(fWf.file, fWf.addr, f)) {
//				PrintErr("Error while uploading WiFi firmware!\n");
//				errCode = 1;
//			}
//		}
//	}

dealloc_exit:
	WfClose();
	if (write_buffer) free(write_buffer);
	if (read_buffer)  free(read_buffer);

#ifndef __OS_WIN
	// Restore cursor
	printf("\e[?25h");
#else
    WSACleanup();
#endif

    return errCode;
}

/** \} */

