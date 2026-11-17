/*
    Project: FreeBASIC console qualification
    File: guest-arguments.c
    Purpose: Supply normal command arguments to unattended console programs.
    Responsibilities: Decode a bounded controller manifest before BASIC starts.
    This file does not select tests, change application sources or report results.
*/
#include <stdio.h>
#include <string.h>

#if defined(FBQUAL_WII)
#include <fat.h>
#define FBQUAL_ARGUMENTS "sd:/FBQARGS.BIN"
#define FBQUAL_ENTRY __wrap_main
#define FBQUAL_PROGRAM __real_main
#define FBQUAL_PROGRAM_NAME "sd:/FBQPROG.DOL"
#elif defined(FBQUAL_XBOX)
#define FBQUAL_ARGUMENTS "D:\\FBQARGS.BIN"
#define FBQUAL_ENTRY main
#define FBQUAL_PROGRAM XBoxStartup
#define FBQUAL_PROGRAM_NAME "D:\\FBQPROG.XBE"
#else
#error Select FBQUAL_WII or FBQUAL_XBOX for the argument adapter
#endif

extern int FBQUAL_PROGRAM(int argc, char **argv);

/* The manifest contains "FBQA1", one argument-count byte, then each argument
 * as a two-byte little-endian length and its bytes. argv[0] is supplied here.
 * Fixed storage bounds guest memory use; embedded NUL and trailing data are
 * rejected. Empty arguments and whitespace retain their ordinary meaning. */
int FBQUAL_ENTRY(int argc, char **argv)
{
	static char storage[8192];
	char *arguments[34];
	unsigned char header[6];
	size_t used = 0;
	unsigned int index;
	int invalid = 0;
	FILE *stream;
#if defined(FBQUAL_WII)
	/* GCC's Wii main prologue runs constructors after this wrapper is
	 * entered. Mount the controller medium before reading its manifest. */
	if (!fatInitDefault())
		return 125;
#endif
	stream = fopen(FBQUAL_ARGUMENTS, "rb");
	if (stream == NULL)
		return 125;
	(void)argc;
	(void)argv;
	if (fread(header, 1, sizeof(header), stream) != sizeof(header) ||
	    memcmp(header, "FBQA1", 5) != 0 || header[5] > 32) {
		fclose(stream);
		return 125;
	}
	arguments[0] = FBQUAL_PROGRAM_NAME;
	for (index = 0; index < header[5]; ++index) {
		unsigned char length_bytes[2];
		size_t length;
		if (fread(length_bytes, 1, 2, stream) != 2) {
			invalid = 1;
			break;
		}
		length = length_bytes[0] | ((unsigned int)length_bytes[1] << 8);
		if (length > 4095 || length >= sizeof(storage) - used ||
		    fread(storage + used, 1, length, stream) != length ||
		    memchr(storage + used, '\0', length) != NULL) {
			invalid = 1;
			break;
		}
		arguments[index + 1] = storage + used;
		storage[used + length] = '\0';
		used += length + 1;
	}
	if (fgetc(stream) != EOF || ferror(stream))
		invalid = 1;
	if (fclose(stream) != 0 || invalid)
		return 125;
	arguments[header[5] + 1] = NULL;
	return FBQUAL_PROGRAM(header[5] + 1, arguments);
}

/* end of guest-arguments.c */
