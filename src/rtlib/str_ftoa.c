/* float to string, internal usage */

#include "fb.h"

char *fb_hFloat2Str( double val, char *buffer, int digits, int mask )
{
	ssize_t len, maxlen;
	char *p;

	if( digits < 0 ) {
		buffer[0] = '\0';
		return NULL;
	}

	if( mask & FB_F2A_ADDBLANK )
		p = &buffer[1];
	else
		p = buffer;

	maxlen = 1+digits+6+1;

	len = snprintf( p, maxlen, "%.*g", digits, val );

	if( len <= 0 || len >= maxlen )
	{
		buffer[0] = '\0';
		return NULL;
	}

	if( len > 0 )
	{
		/* skip the dot at end if any */
		if( p[len-1] == '.' )
			p[len-1] = '\0';
	}

	/* */
	if( (mask & FB_F2A_ADDBLANK) > 0 )
	{
		if( p[0] != '-' )
		{
			buffer[0] = ' ';
			return &buffer[0];
		}
		else
			return p;
	}
	else
		return p;
}
