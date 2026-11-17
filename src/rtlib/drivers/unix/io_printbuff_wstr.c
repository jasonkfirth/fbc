/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_printbuff_wstr.c
 * Purpose: low-level print to console function.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

#define ENTER_UTF8  "\033%G"
#define EXIT_UTF8   "\033%@"

void fb_ConsolePrintBufferWstrEx( const FB_WCHAR *buffer, size_t chars, int mask )
{
	/* Short PRINT calls use fixed scratch storage. Longer calls own one heap
	   allocation, with a counted, terminated copy for the shadow conversion. */
	char local_buffer[256];
	FB_WCHAR local_shadow[64];
	char *temp = local_buffer;
	FB_WCHAR *shadow = local_shadow;
	void *allocation = NULL;
	size_t avail = 0, avail_len = 0, offset = 0, wide_bytes = 0, capacity;
	ssize_t bytes = 0;
	int initialized = fb_console_context.inited;

	(void)mask;
	if( (buffer == NULL && chars != 0) ||
	    chars > (size_t)(FB_STRSIZEMSK - 1) / 4 )
		return;
	capacity = chars * 4 + 1;

	if( initialized ) {
		/* Resize can replace both planes from the background thread. Keep
		   their span and cursor state protected through this PRINT call. */
		BG_LOCK();
		fb_hRecheckConsoleSize( TRUE );
		avail = fb_hConsoleBufferSpan( &offset );
		avail_len = (chars < avail) ? chars : avail;
		if( avail_len != 0 ) {
			if( avail_len >= SIZE_MAX / sizeof( *shadow ) )
				goto finished;
			wide_bytes = (avail_len + 1) * sizeof( *shadow );
		}
	}

	if( capacity > sizeof( local_buffer ) ||
	    avail_len >= sizeof( local_shadow ) / sizeof( local_shadow[0] ) ) {
		if( wide_bytes > SIZE_MAX - capacity )
			goto finished;
		allocation = malloc( wide_bytes + capacity );
		if( allocation == NULL )
			goto finished;
		shadow = allocation;
		temp = (char *)allocation + wide_bytes;
	}

	if( avail_len != 0 ) {
		/* ConvToA expects a terminated string, while this entry point owns
		   only chars counted units. Zero the unfilled shadow suffix too. */
		memcpy( shadow, buffer, avail_len * sizeof( *shadow ) );
		shadow[avail_len] = 0;
		memset( temp, 0, avail_len + 1 );
		fb_wstr_ConvToA( temp, (ssize_t)avail_len, shadow );
		memcpy( fb_console_context.char_buffer + offset, temp, avail_len );
		memset( fb_console_context.attr_buffer + offset,
		        (unsigned char)(fb_console_context.fg_color | (fb_console_context.bg_color << 4)), avail_len );
	}

	if( fb_WCharToUTF( FB_FILE_ENCOD_UTF8, buffer, (ssize_t)chars, temp, &bytes ) == NULL ||
	    bytes < 0 || (size_t)bytes >= capacity )
		goto finished;
	if( initialized )
		fputs( ENTER_UTF8, stdout );
	if( bytes != 0 )
		fwrite( temp, 1, (size_t)bytes, stdout );
	if( initialized )
		fputs( EXIT_UTF8, stdout );

	if( avail != 0 ) {
		/* Preserve the console's existing wrap convention without a signed
		   increment beyond the final representable row or column. */
		for( ; chars; chars--, buffer++ ) {
			if( *buffer == FB_WCHAR_LITERAL('\n') ||
			    fb_console_context.cur_x >= fb_console_context.w - 1 ) {
				fb_console_context.cur_x = 1;
				if( fb_console_context.cur_y < fb_console_context.h )
					fb_console_context.cur_y++;
			} else {
				fb_console_context.cur_x++;
			}
		}
	}
	fflush( stdout );

finished:
	free( allocation );
	if( initialized )
		BG_UNLOCK();
}

void fb_ConsolePrintBufferWstr( const FB_WCHAR *buffer, int mask )
{
	ssize_t chars = buffer ? fb_wstr_Len( buffer ) : 0;
	if( chars >= 0 )
		fb_ConsolePrintBufferWstrEx( buffer, (size_t)chars, mask );
}

/* end of drivers/unix/io_printbuff_wstr.c */
