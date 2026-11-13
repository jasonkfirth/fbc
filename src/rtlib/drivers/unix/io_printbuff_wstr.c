/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_printbuff_wstr.c
 * Purpose: low-level print to console function.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

#define ENTER_UTF8  "\e%G"
#define EXIT_UTF8   "\e%@"

void fb_ConsolePrintBufferWstrEx( const FB_WCHAR *buffer, size_t chars, int mask )
{
	size_t avail, avail_len;
	char *temp;

	if( !fb_console_context.inited )
	{
		ssize_t bytes;
		temp = alloca( chars * 4 + 1 );
		fb_WCharToUTF( FB_FILE_ENCOD_UTF8, buffer, chars, temp, &bytes );
		temp[bytes] = '\0';
		fwrite( temp, 1, bytes, stdout );
		fflush( stdout );
		return;
	}

	temp = alloca( chars * 4 + 1 );

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	/* ToDo: handle scrolling for internal characters/attributes buffer? */
	avail = (fb_console_context.w * fb_console_context.h) - (((fb_console_context.cur_y - 1) * fb_console_context.w) + fb_console_context.cur_x - 1);
	avail_len = chars;
	if (avail < avail_len)
		avail_len = avail;

	/*
		The console shadow buffer is byte-oriented, so it intentionally stores a
		narrow approximation while the terminal receives UTF-8 below.
	*/
	fb_wstr_ConvToA( temp, avail_len, buffer );

	memcpy( fb_console_context.char_buffer + ((fb_console_context.cur_y - 1) * fb_console_context.w) + fb_console_context.cur_x - 1,
	        temp,
	        avail_len );

	memset( fb_console_context.attr_buffer + ((fb_console_context.cur_y - 1) * fb_console_context.w) + fb_console_context.cur_x - 1,
	        fb_console_context.fg_color | (fb_console_context.bg_color << 4),
	        avail_len );

	/* convert wchar_t to UTF-8 */
	ssize_t bytes;

	fb_WCharToUTF( FB_FILE_ENCOD_UTF8, buffer, chars, temp, &bytes );
	/* add null-term */
	temp[bytes] = '\0';

	fputs( ENTER_UTF8, stdout );

	fputs( temp, stdout );

	fputs( EXIT_UTF8, stdout );

	/* update x and y coordinates.. */
	for( ; chars; chars--, buffer++ )
	{
		++fb_console_context.cur_x;
		if( (*buffer == FB_WCHAR_LITERAL('\n')) || (fb_console_context.cur_x >= fb_console_context.w) )
		{
			fb_console_context.cur_x = 1;
			++fb_console_context.cur_y;
			if( fb_console_context.cur_y > fb_console_context.h )
				fb_console_context.cur_y = fb_console_context.h;
		}
	}

	fflush( stdout );
}

void fb_ConsolePrintBufferWstr( const FB_WCHAR *buffer, int mask )
{
	return fb_ConsolePrintBufferWstrEx( buffer, fb_wstr_Len( buffer ), mask );
}

/* end of drivers/unix/io_printbuff_wstr.c */
