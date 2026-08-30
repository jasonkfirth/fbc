/* console multikey() */

#include "../fb.h"
#include "fb_private_console.h"

int fb_ConsoleMultikey( int scancode )
{
	if( scancode < 0 ||
	    scancode >= (int)(sizeof(__fb_con.multikey) / sizeof(__fb_con.multikey[0])) ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return FB_FALSE;
	}

	return __fb_con.multikey[scancode] != 0? FB_TRUE: FB_FALSE;
}
