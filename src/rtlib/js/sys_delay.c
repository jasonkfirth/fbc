/*
	FreeBASIC runtime library
	-------------------------

	File: js/sys_delay.c

	Purpose:

		Implement the FreeBASIC delay primitive for the JavaScript target.

	Responsibilities:

		- clamp negative delay requests
		- yield to the browser event loop during BASIC delay calls

	This file intentionally does NOT contain:

		- event dispatch
		- graphics presentation
		- keyboard or mouse polling
*/

#include "../fb.h"
#include <emscripten.h>

FBCALL void fb_Delay( int msecs )
{
	if( msecs < 0 )
		msecs = 0;

	/*
		JavaScript has one main thread for the user program, input dispatch,
		timers, audio callbacks, and canvas presentation.  A busy wait here
		would keep old BASIC input loops technically running while preventing
		the browser from repainting the screen or delivering input.

		emscripten_sleep() depends on Asyncify support at link time.  The JS
		compiler driver enables that support and also asks Emscripten to
		optimize the final wasm enough that large QB-era programs stay under
		browser function limits.
	*/
	emscripten_sleep( msecs );
}

/* end of js/sys_delay.c */
