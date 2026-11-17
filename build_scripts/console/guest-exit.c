/*
    Project: FreeBASIC console qualification
    File: guest-exit.c

    Purpose:
        Report a console program's exit status to its emulator test controller.

    Responsibilities:
        - read the current run's bounded nonce from the test medium
        - publish a complete status file after the CRT's exit handlers
        - retain the target's normal final exit operation

    This file intentionally does NOT contain:
        - production runtime changes or application source rewriting
        - emulator setup, firmware assets or test selection
*/

#include <stdio.h>

#if defined(FBQUAL_WII)
#define FBQUAL_NONCE "sd:/FBQNONCE.TXT"
#define FBQUAL_TEMP "sd:/FBQEXIT.TMP"
#define FBQUAL_STATUS "sd:/FBQEXIT.TXT"
#elif defined(FBQUAL_XBOX)
#include <xboxkrnl/xboxkrnl.h>
#define FBQUAL_NONCE "D:\\FBQNONCE.TXT"
#define FBQUAL_TEMP "E:\\FBQEXIT.TMP"
#define FBQUAL_STATUS "E:\\FBQEXIT.TXT"
#else
#error Select FBQUAL_WII or FBQUAL_XBOX for the guest exit observer
#endif

/* A 128-bit nonce is stored as exactly 32 lowercase hexadecimal bytes. The
 * controller replaces it and removes old status files before every launch.
 * A status from another program or an interrupted write cannot pass a run. */
static void fbqual_write_exit_status(int status)
{
	char nonce[33];
	FILE *stream;
	size_t count;
	int extra;
	unsigned int index;

	stream = fopen(FBQUAL_NONCE, "rb");
	if (stream == NULL)
		return;
	count = fread(nonce, 1, sizeof(nonce) - 1, stream);
	extra = fgetc(stream);
	if (fclose(stream) != 0 || count != sizeof(nonce) - 1 || extra != EOF)
		return;
	for (index = 0; index < sizeof(nonce) - 1; ++index) {
		if (!((nonce[index] >= '0' && nonce[index] <= '9') ||
		      (nonce[index] >= 'a' && nonce[index] <= 'f')))
			return;
	}
	nonce[sizeof(nonce) - 1] = '\0';

	stream = fopen(FBQUAL_TEMP, "wb");
	if (stream == NULL)
		return;
	if (fprintf(stream, "FBQUAL %s %d DONE\n", nonce, status) < 0) {
		fclose(stream);
		remove(FBQUAL_TEMP);
		return;
	}
	if (fclose(stream) != 0) {
		remove(FBQUAL_TEMP);
		return;
	}
	if (rename(FBQUAL_TEMP, FBQUAL_STATUS) != 0)
		remove(FBQUAL_TEMP);
}

#if defined(FBQUAL_WII)
/* GNU ld supplies these names with --wrap=_exit. Newlib has already run its
 * exit handlers when it calls _exit; the observer then delegates to libogc. */
extern void __real__exit(int status) __attribute__((noreturn));
void __wrap__exit(int status) __attribute__((noreturn));
void __wrap__exit(int status)
{
	fbqual_write_exit_status(status);
	__real__exit(status);
}
#else
/* Link this object before nxdk's archives. PDCLib's ordinary exit() and
 * _Exit() retain their cleanup; only the platform's final reboot entry is
 * supplied here. The final operation matches nxdk's _PDCLIB_Exit provider. */
void _PDCLIB_Exit(int status)
{
	fbqual_write_exit_status(status);
	HalReturnToFirmware(HalQuickRebootRoutine);
	for (;;) {
		/* Firmware exit must not return to a partially destroyed runtime. */
	}
}
#endif

/* end of guest-exit.c */
