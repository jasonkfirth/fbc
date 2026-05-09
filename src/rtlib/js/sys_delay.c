#include "../fb.h"
#include <emscripten.h>

FBCALL void fb_Delay( int msecs )
{
	double until;

	if( msecs <= 0 )
		return;

	until = emscripten_get_now() + msecs;

	while( emscripten_get_now() < until )
	{
	}
}
