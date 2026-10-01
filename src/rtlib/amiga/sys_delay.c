/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/sys_delay.c

    Purpose:
        Yield through AmigaDOS without depending on Unix sleep stubs.

    Responsibilities:
        - convert millisecond delays to bounded native DOS ticks

    This file intentionally does NOT contain:
        - keyboard waits, audio pacing, or system clock adjustment
*/

#include "../fb.h"
#include <proto/dos.h>

FBCALL void fb_Delay(int milliseconds)
{
	/* DOS Delay uses 50 ticks per second. Round positive intervals up, and
	   avoid adding to INT_MAX before division. Zero still yields one tick. */
	LONG ticks = 1;

	if( milliseconds > 0 ) {
		ticks = milliseconds / 20;
		if( milliseconds % 20 != 0 )
			++ticks;
	}
	Delay(ticks);
}

/* end of amiga/sys_delay.c */
