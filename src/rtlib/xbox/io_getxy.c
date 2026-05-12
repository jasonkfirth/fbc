#include "../fb.h"

FBCALL void fb_ConsoleGetXY( int *col, int *row )
{
	if( col != NULL )
		*col = 1;
	if( row != NULL )
		*row = 1;
}
