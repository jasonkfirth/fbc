/* console scrolling for when VIEW is used */

#include "../fb.h"

void fb_ConsoleScroll( int nrows )
{
    /*
    	The JavaScript console target has no addressable text screen to scroll.
    	Keeping this as a no-op matches the rest of the JS console backend.
    */
    (void)nrows;
}
