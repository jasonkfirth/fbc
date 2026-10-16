/*
    FreeBASIC runtime console input
    ------------------------------
    File: unix/io_readstr.c
    Purpose: Read console lines through the active platform input service.
    Responsibilities: Retain interactive editing and bounded stream reads.
    This file intentionally does NOT contain: file ownership or key translation.
*/

#include "../fb.h"
#include "fb_private_console.h"

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	int k, x, y, cols;
	ssize_t pos = 0;
	char ch[2] = { 0, '\0' };

#ifdef HOST_AMIGA
	/* The native console remains initialized when Input() is redirected.
	   Files carry LF line endings and do not implement console key polling;
	   sending them through the interactive CR editor can wait forever. */
	if (fb_ConsoleIsRedirected(TRUE))
		return fgets(buffer, len, stdin);
#endif
	if (!__fb_con.inited)
		return fgets(buffer, len, stdin);

	fb_ConsoleGetSize(&cols, NULL);

	do {
		while( ((k = fb_hGetCh(TRUE)) == -1) || (k > 0xFF) )
			fb_Delay( 10 );

		fb_ConsoleGetXY(&x, &y);

		if (k == 8) {
			if (pos > 0) {
				x--;
				if (x <= 0) {
					x = cols;
					y--;
					if (y <= 0)
						x = y = 1;
				}
				fb_hTermOut(SEQ_LOCATE, x-1, y-1);
				fb_hTermOut(SEQ_DEL_CHAR, 0, 0);
				pos--;
			}
		} else if (k != '\t') {
			if (pos < len - 1) {
				buffer[pos++] = ch[0] = k;
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

/* end of unix/io_readstr.c */
