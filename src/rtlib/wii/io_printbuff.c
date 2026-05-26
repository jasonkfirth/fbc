/*
    FreeBASIC runtime Wii console output
    ------------------------------------

    File: io_printbuff.c

    Purpose:

        Write FreeBASIC console output through libogc's stdout console.

    Responsibilities:

        - ensure the fallback console has been initialized
        - forward PRINT data to stdout

    This file intentionally does NOT contain:

        - text input
        - graphics text rendering
        - framebuffer presentation
*/

#include "../fb.h"

void fb_ConsolePrintBufferEx(const void *buffer, size_t len, int mask)
{
	(void)mask;

	if ((buffer == NULL) || (len == 0))
		return;

	fb_WiiVideoInit();
	fwrite(buffer, 1, len, stdout);
	fflush(stdout);
}

void fb_ConsolePrintBuffer(const char *buffer, int mask)
{
	fb_ConsolePrintBufferEx(buffer, buffer ? strlen(buffer) : 0, mask);
}

/* end of io_printbuff.c */
