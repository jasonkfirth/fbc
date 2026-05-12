/* sleep() function, Xbox */

#include "../fb.h"

void fb_ConsoleSleep( int msecs )
{
	/*
		The Xbox console backend has no keyboard input to poll while sleeping.
		Graphics mode installs its own sleep hook, so the console path only
		needs to wait for the requested duration.
	*/
	fb_Delay( msecs );
}
