/* low-level truncate / set end of file */

#include "../fb.h"

int fb_hFileSetEofEx( FILE *f )
{
	(void)f;
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}
