/* low-level print to console function */

#include "../fb.h"
#include <hal/debug.h>
#include <hal/video.h>

#define XBOX_CONSOLE_CHUNK 256

static int console_mode_ready = FALSE;

static void hConsoleInit( void )
{
	if( console_mode_ready )
		return;

	/*
		FreeBASIC console programs normally print before selecting any
		graphics mode.  nxdk's debug console draws into the active XVideo
		framebuffer, so give it a normal 640x480 target on first use.  gfxlib
		installs its own console hooks while a graphics screen is active, so
		the plain console hook can safely own this fallback mode.
	*/
	XVideoSetMode( 640, 480, 32, REFRESH_DEFAULT );

	console_mode_ready = TRUE;
}

void fb_ConsolePrintBufferEx( const void *buffer, size_t len, int mask )
{
	const char *src = (const char *)buffer;
	char text[XBOX_CONSOLE_CHUNK + 1];

	(void)mask;

	if( (src == NULL) || (len == 0) )
		return;

	hConsoleInit();

	while( len > 0 ) {
		size_t chars = len;
		size_t i;

		if( chars > XBOX_CONSOLE_CHUNK )
			chars = XBOX_CONSOLE_CHUNK;

		for( i = 0; i < chars; i++ ) {
			char ch = src[i];

			text[i] = (ch != '\0') ? ch : ' ';
		}

		text[chars] = '\0';
		debugPrint( "%s", text );

		src += chars;
		len -= chars;
	}
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	fb_ConsolePrintBufferEx( buffer, buffer ? strlen( buffer ) : 0, mask );
}

/* end of io_printbuff.c */
