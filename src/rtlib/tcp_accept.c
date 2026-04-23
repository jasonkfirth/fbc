/* TCP ACCEPT function */

#include "fb.h"
#ifndef DISABLE_TCP
#include "dev_tcp_private.h"

static FB_FILE_HOOKS hooks_dev_tcp_accept_reserved;
#endif

FBCALL int fb_TcpAccept( int fnum )
{
#ifdef DISABLE_TCP
	(void)fnum;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	int client_fnum;
	FB_FILE *server_handle;
	FB_FILE *client_handle;
	int res;

	server_handle = FB_FILE_TO_HANDLE( fnum );
	if( !FB_HANDLE_USED( server_handle ) || server_handle->type != FB_FILE_TYPE_TCPSERVER ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return 0;
	}

	FB_LOCK();
	client_fnum = fb_FileFree();
	if( client_fnum == 0 ) {
		FB_UNLOCK();
		fb_ErrorSetNum( FB_RTERROR_FILEIO );
		return 0;
	}

	client_handle = FB_FILE_TO_HANDLE( client_fnum );
	memset( client_handle, 0, sizeof( FB_FILE ) );
	client_handle->hooks = &hooks_dev_tcp_accept_reserved;
	FB_UNLOCK();

	res = fb_DevTcpAcceptHandle( server_handle, client_handle );
	if( res != FB_RTERROR_OK ) {
		FB_LOCK();
		memset( client_handle, 0, sizeof( FB_FILE ) );
		FB_UNLOCK();
		return 0;
	}

	return client_fnum;
#endif
}
