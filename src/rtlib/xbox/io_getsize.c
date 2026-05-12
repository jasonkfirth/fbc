#include "../fb.h"

FBCALL void fb_ConsoleGetSize( int *cols, int *rows )
{
	if( cols != NULL )
		*cols = 80;
	if( rows != NULL )
		*rows = 25;
}
