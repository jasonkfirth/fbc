#include "../fb.h"
#include <emscripten.h>

FBCALL void fb_Delay( int msecs )
{
	if( msecs < 0 )
		msecs = 0;

	/*
	 * JavaScript runs FreeBASIC code, event dispatch, timers, audio, and
	 * canvas presentation on the same browser thread.  SLEEP must therefore
	 * yield to the host event loop instead of busy waiting, especially for
	 * old game loops that use SLEEP 0 once per frame.
	 */
	emscripten_sleep( msecs );
}
