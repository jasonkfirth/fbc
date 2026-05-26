/*
    FreeBASIC runtime Wii wide console output
    -----------------------------------------

    File: io_printbuff_wstr.c

    Purpose:

        Convert WSTRING console output into byte-oriented libogc console text.

    Responsibilities:

        - preserve ASCII control characters and printable ASCII
        - make non-ASCII text visible as '?' instead of dropping it silently
        - forward all byte output through the normal Wii console print hook

    This file intentionally does NOT contain:

        - Unicode font rendering
        - character set conversion tables
        - graphics text drawing
*/

#include "../fb.h"

void fb_ConsolePrintBufferWstrEx(const FB_WCHAR *buffer, size_t chars, int mask)
{
	char text[256];
	size_t out_chars = 0;

	if ((buffer == NULL) || (chars == 0))
		return;

	while (chars > 0) {
		FB_WCHAR ch = *buffer++;

		if ((ch == '\r') || (ch == '\n') || ((ch >= 32) && (ch < 127)))
			text[out_chars++] = (char)ch;
		else
			text[out_chars++] = '?';

		if (out_chars == sizeof(text)) {
			fb_ConsolePrintBufferEx(text, out_chars, mask);
			out_chars = 0;
		}

		--chars;
	}

	if (out_chars > 0)
		fb_ConsolePrintBufferEx(text, out_chars, mask);
}

void fb_ConsolePrintBufferWstr(const FB_WCHAR *buffer, int mask)
{
	fb_ConsolePrintBufferWstrEx(buffer, buffer ? fb_wstr_Len(buffer) : 0, mask);
}

/* end of io_printbuff_wstr.c */
