/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_printbuff.c
 * Purpose: low-level print to console function.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

#define CTRL_ALWAYS 0x0800D101u
#define ENTER_UTF8  "\033%G"
#define EXIT_UTF8   "\033%@"

void fb_ConsolePrintBufferEx( const void *buffer, size_t len, int mask )
{
	size_t avail, avail_len, offset;
	const unsigned char *cbuffer = (const unsigned char *) buffer;
	int c;
	(void)mask;
	if( buffer == NULL && len != 0 )
		return;

	if (!fb_console_context.inited) {
		if( len != 0 )
			fwrite(buffer, len, 1, stdout);
		fflush(stdout);
		return;
	}

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );

	/* ToDo: handle scrolling for internal characters/attributes buffer? */
	avail = fb_hConsoleBufferSpan( &offset );
	avail_len = len;
	if (avail < avail_len)
		avail_len = avail;
	if( avail_len != 0 ) {
		memcpy(fb_console_context.char_buffer + offset, buffer, avail_len);
		memset(fb_console_context.attr_buffer + offset,
		       (unsigned char)(fb_console_context.fg_color | (fb_console_context.bg_color << 4)), avail_len);
	}

	for (; len; len--, cbuffer++) {
		c = *cbuffer;
		if( c == 0 ) 
			c = 32;

		if (c < 32) {
			if ((CTRL_ALWAYS >> c) & 0x1) {
				/* This character can't be printed, we must use unicode
				 * Enter UTF-8 and start constructing 0xF000 code
				 */
				fputs( ENTER_UTF8 "\xEF\x80", stdout );
				/* Set the last 6 bits */
				fputc( c | 0x80, stdout );
				/* Escape UTF-8 */
				fputs( EXIT_UTF8, stdout );
			} else
				fputc( c, stdout );
		} else
			fputc( c, stdout );

		if( avail != 0 ) {
			if( c == 10 || fb_console_context.cur_x >= fb_console_context.w - 1 ) {
				fb_console_context.cur_x = 1;
				if( fb_console_context.cur_y < fb_console_context.h )
					fb_console_context.cur_y++;
			} else {
				fb_console_context.cur_x++;
			}
		}
	}

	fflush( stdout );
	BG_UNLOCK( );
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	fb_ConsolePrintBufferEx( buffer, strlen(buffer), mask );
}

/* end of drivers/unix/io_printbuff.c */
