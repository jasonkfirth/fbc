/* Xbox TCP device */

#include "../fb.h"

int fb_DevTcpOpen( FB_FILE *handle, const char *filename, size_t filename_len )
{
	(void)handle;
	(void)filename;
	(void)filename_len;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_DevTcpOpenServer( FB_FILE *handle, const char *filename, size_t filename_len )
{
	(void)handle;
	(void)filename;
	(void)filename_len;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle )
{
	(void)server_handle;
	(void)client_handle;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_DevTcpEocEx( FB_FILE *handle )
{
	(void)handle;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}
