/*
 * Project: FreeBASIC RISC OS console byte output
 * ----------------------------------------------
 *
 * File: drivers/riscos/io_printbuff.c
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
 * This file intentionally does NOT contain: *
 *     - wide-character output
 *     - Wimp rendering
 *     - input handling
 */
/* low-level print to console function */

#include "../../fb.h"
#include "../unix/fb_private_console.h"

void fb_ConsolePrintBufferEx( const void *buffer, size_t len, int mask )
{
	size_t avail, avail_len;
	const unsigned char *cbuffer = (const unsigned char *) buffer;
	unsigned int c;

	if (!fb_console_context.inited) {
		fwrite(buffer, len, 1, stdout);
		fflush(stdout);
		return;
	}

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	/* ToDo: handle scrolling for internal characters/attributes buffer? */
	avail = (fb_console_context.w * fb_console_context.h) - (((fb_console_context.cur_y - 1) * fb_console_context.w) + fb_console_context.cur_x - 1);
	avail_len = len;
	if (avail < avail_len)
		avail_len = avail;
	memcpy(fb_console_context.char_buffer + ((fb_console_context.cur_y - 1) * fb_console_context.w) + fb_console_context.cur_x - 1, buffer, avail_len);
	memset(fb_console_context.attr_buffer + ((fb_console_context.cur_y - 1) * fb_console_context.w) + fb_console_context.cur_x - 1, fb_console_context.fg_color | (fb_console_context.bg_color << 4), avail_len);

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

		fb_console_context.cur_x++;
		if ((c == 10) || (fb_console_context.cur_x >= fb_console_context.w)) {
			fb_console_context.cur_x = 1;
			fb_console_context.cur_y++;
			if (fb_console_context.cur_y > fb_console_context.h)
				fb_console_context.cur_y = fb_console_context.h;
		}
	}

	fflush( stdout );
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	return fb_ConsolePrintBufferEx( buffer, strlen(buffer), mask );
}

/* end of drivers/riscos/io_printbuff.c */
