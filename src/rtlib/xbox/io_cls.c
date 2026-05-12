/* console CLS statement */

#include "../fb.h"
#include <hal/debug.h>
#include <hal/video.h>

void fb_ConsoleClear( int mode )
{
	(void)mode;

	XVideoSetMode( 640, 480, 32, REFRESH_DEFAULT );
	debugClearScreen();
}

/* end of io_cls.c */
