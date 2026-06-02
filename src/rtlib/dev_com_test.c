/* COMx device */

#include "fb.h"
#include <limits.h>

int fb_DevComTestProtocolEx
	(
		FB_FILE *handle,
		const char *filename,
		size_t filename_len,
		size_t *pPort
	)
{
    char ch;
    size_t i, port, digit;

    if( pPort ) {
        *pPort = 0;
    }

    if( strncasecmp(filename, "SER:", 4)==0 ) {
        if( pPort )
            *pPort = 1;
        return TRUE;
    }

    if( filename_len < 4 )
        return FALSE;
    
    if( strncasecmp(filename, "COM", 3) != 0 )
    	return strchr( filename, ':' ) != NULL;

    port = 0;
    i = 3;
    ch = filename[i];
    while( ch>='0' && ch<='9' ) {
        digit = ch - '0';
        if( port > ((size_t)INT_MAX - digit) / 10 )
            return FALSE;
        port = port * 10 + digit;
        ch = filename[++i];
    }

		/* removed to allow for open com "COM:"
    if( port==0 )
        return FALSE;
		*/
    if( ch!=':' )
        return FALSE;

    if( pPort )
        *pPort = port;

    return TRUE;
}

int fb_DevComTestProtocol
	(
		FB_FILE *handle,
		const char *filename,
		size_t filename_len
	)
{
    return fb_DevComTestProtocolEx( handle, filename, filename_len, NULL );
}
