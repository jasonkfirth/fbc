/*
 * Project: FreeBASIC RISC OS console byte output
 * ----------------------------------------------
 *
 * File: io_printbuff.c
 *
 * Purpose:
 *
 *     Write FreeBASIC console bytes through the RISC OS VDU stream.
 *
 * Responsibilities:
 *
 *     - maintain the runtime console position and attribute buffers
 *     - prevent data bytes from being interpreted as VDU commands
 *     - flush completed output through UnixLib stdio
 *
 * This file intentionally does NOT contain:
 *
 *     - wide-character output
 *     - Wimp rendering
 *     - input handling
 */
/* low-level print to console function */

#include "../fb.h"
#include "../unix/fb_private_console.h"

void fb_ConsolePrintBufferEx( const void *buffer, size_t len, int mask )
{
	size_t avail, avail_len;
	const unsigned char *cbuffer = (const unsigned char *) buffer;
	unsigned int c;

	if (!__fb_con.inited) {
		fwrite(buffer, len, 1, stdout);
		fflush(stdout);
		return;
	}

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	/* ToDo: handle scrolling for internal characters/attributes buffer? */
	avail = (__fb_con.w * __fb_con.h) - (((__fb_con.cur_y - 1) * __fb_con.w) + __fb_con.cur_x - 1);
	avail_len = len;
	if (avail < avail_len)
		avail_len = avail;
	memcpy(__fb_con.char_buffer + ((__fb_con.cur_y - 1) * __fb_con.w) + __fb_con.cur_x - 1, buffer, avail_len);
	memset(__fb_con.attr_buffer + ((__fb_con.cur_y - 1) * __fb_con.w) + __fb_con.cur_x - 1, __fb_con.fg_color | (__fb_con.bg_color << 4), avail_len);

	for (; len; len--, cbuffer++) {
		c = *cbuffer;
		if( c == 0 )
			c = 32;

		/*
		 * TaskWindow output is a 7-bit VDU stream. C0 bytes are VDU
		 * commands, while high-half bytes may lose their high bit and
		 * become those same commands. Keep both ranges unambiguous.
		 */
		if( (c < 32) || (c >= 128) )
			fputc( '?', stdout );
		else
			fputc( c, stdout );

		__fb_con.cur_x++;
		if ((c == 10) || (__fb_con.cur_x >= __fb_con.w)) {
			__fb_con.cur_x = 1;
			__fb_con.cur_y++;
			if (__fb_con.cur_y > __fb_con.h)
				__fb_con.cur_y = __fb_con.h;
		}
	}

	fflush( stdout );
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	return fb_ConsolePrintBufferEx( buffer, strlen(buffer), mask );
}

/* end of io_printbuff.c */
