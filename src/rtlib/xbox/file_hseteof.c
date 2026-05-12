/* low-level truncate / set end of file */

#include "../fb.h"
#include <windows.h>
#include "pdclib/_PDCLIB_int.h"

int fb_hFileSetEofEx( FILE *f )
{
	if( f == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( fflush( f ) != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( SetEndOfFile( (HANDLE)f->handle ) == 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}
