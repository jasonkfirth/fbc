/*
    Project: FreeBASIC runtime console input
    ------------------------------
    File: drivers/unix/io_readstr.c
    Purpose: Read console lines through the active platform input service.
    Responsibilities: Retain interactive editing and bounded stream reads.
    This file intentionally does NOT contain: file ownership or key translation.
*/

#include "../../fb.h"
#include "fb_private_console.h"
#include <limits.h>

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	int k, x, y, cols;
	size_t pos = 0u;
	char ch[2] = { 0, '\0' };

	if( (buffer == NULL) || (len <= 0) )
		return NULL;
	/* libc fgets takes an int capacity, matching the public read dispatcher. */
	if( len > INT_MAX )
		len = INT_MAX;

#ifdef HOST_AMIGA
	/* The native console remains initialized when Input() is redirected.
	   Files carry LF line endings and do not implement console key polling;
	   sending them through the interactive CR editor can wait forever. */
	if (fb_ConsoleIsRedirected(TRUE))
		return fgets(buffer, (int)len, stdin);
#endif
	if (!fb_console_context.inited)
		return fgets(buffer, (int)len, stdin);

	fb_ConsoleGetSize(&cols, NULL);
	if (cols <= 0)
		cols = 1;

	do {
		while( ((k = fb_hGetCh(TRUE)) < 0) || (k > 0xFF) )
			fb_Delay( 10 );

		fb_ConsoleGetXY(&x, &y);

		if (k == 8) {
			if (pos > 0u) {
				if (x > 1) {
					x--;
				} else {
					x = cols;
					if (y > 1)
						y--;
					else {
						x = y = 1;
					}
				}
				fb_hTermOut(SEQ_LOCATE, x-1, y-1);
				fb_hTermOut(SEQ_DEL_CHAR, 0, 0);
				pos--;
			}
		} else if (k != '\t') {
			if (pos + 1u < (size_t)len) {
				/* Only byte-valued keys reach the line buffer. */
				buffer[pos++] = ch[0] = (char)k;
				fb_ConsolePrintBuffer(ch, 0);
				if (x == cols)
					fputc( '\n', stdout );
			}
		}
	} while (k != '\r');

	fputc( '\n', stdout );
	buffer[pos] = '\0';

	return buffer;
}

/* end of drivers/unix/io_readstr.c */
