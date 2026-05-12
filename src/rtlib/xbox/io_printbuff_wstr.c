/* low-level print to console function */

#include "../fb.h"

void fb_ConsolePrintBufferWstrEx
	(
		const FB_WCHAR *buffer,
		size_t chars,
		int mask
	)
{
	char text[256];
	size_t out_chars = 0;

	(void)mask;

	if( (buffer == NULL) || (chars == 0) )
		return;

	while( chars > 0 ) {
		FB_WCHAR ch = *buffer++;

		/*
			nxdk's debug console is byte-oriented.  Keep ASCII control
			characters intact for line breaks and make non-ASCII WSTRING
			output visible without pretending to implement a font mapper.
		*/
		if( (ch == '\r') || (ch == '\n') || ((ch >= 32) && (ch < 127)) )
			text[out_chars++] = (char)ch;
		else
			text[out_chars++] = '?';

		if( out_chars == sizeof( text ) ) {
			fb_ConsolePrintBufferEx( text, out_chars, mask );
			out_chars = 0;
		}

		chars--;
	}

	if( out_chars > 0 )
		fb_ConsolePrintBufferEx( text, out_chars, mask );
}

void fb_ConsolePrintBufferWstr
	(
		const FB_WCHAR *buffer,
		int mask
	)
{
	return fb_ConsolePrintBufferWstrEx( buffer, buffer ? fb_wstr_Len( buffer ) : 0, mask );
}

/* end of io_printbuff_wstr.c */
