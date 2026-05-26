/* fre() function */

#include "../fb.h"
#include <unistd.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
#if defined( _SC_AVPHYS_PAGES ) && defined( _SC_PAGESIZE )
	long pages = sysconf( _SC_AVPHYS_PAGES );
	long page_size = sysconf( _SC_PAGESIZE );

	if( (pages > 0) && (page_size > 0) )
		return (size_t)pages * (size_t)page_size;
#endif

	(void)mode;
	return 0;
}
