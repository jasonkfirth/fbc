/* open TCP */

#include "fb.h"

FBCALL int fb_FileOpenTcp( FBSTRING *str_filename, unsigned int mode,
                           unsigned int access, unsigned int lock,
                           int fnum, int len, const char *encoding )
{
#ifdef DISABLE_TCP
	(void)str_filename;
	(void)mode;
	(void)access;
	(void)lock;
	(void)fnum;
	(void)len;
	(void)encoding;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	if( !FB_FILE_INDEX_VALID( fnum ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileOpenVfsEx( FB_FILE_TO_HANDLE( fnum ),
	                         str_filename,
	                         mode,
	                         access,
	                         lock,
	                         len,
	                         fb_hFileStrToEncoding( encoding ),
	                         fb_DevTcpOpen );
#endif
}

FBCALL int fb_FileOpenTcpServer( FBSTRING *str_filename, unsigned int mode,
                                 unsigned int access, unsigned int lock,
                                 int fnum, int len, const char *encoding )
{
#ifdef DISABLE_TCP
	(void)str_filename;
	(void)mode;
	(void)access;
	(void)lock;
	(void)fnum;
	(void)len;
	(void)encoding;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	if( !FB_FILE_INDEX_VALID( fnum ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileOpenVfsEx( FB_FILE_TO_HANDLE( fnum ),
	                         str_filename,
	                         mode,
	                         access,
	                         lock,
	                         len,
	                         fb_hFileStrToEncoding( encoding ),
	                         fb_DevTcpOpenServer );
#endif
}
