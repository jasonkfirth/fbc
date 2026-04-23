/* EOC function */

#include "fb.h"
#ifndef DISABLE_TCP
#include "dev_tcp_private.h"
#endif

FBCALL int fb_Eoc( int fnum )
{
#ifdef DISABLE_TCP
	(void)fnum;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	FB_FILE *handle = FB_FILE_TO_HANDLE( fnum );

	if( !FB_HANDLE_USED( handle ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( handle->type == FB_FILE_TYPE_TCPSERVER )
		return FB_FALSE;

	if( handle->type != FB_FILE_TYPE_TCP )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_DevTcpEocEx( handle );
#endif
}
