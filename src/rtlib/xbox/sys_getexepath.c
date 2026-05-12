/* get the executable path */

#include "../fb.h"
#include <nxdk/path.h>

static int hCopyMappedPath( char *dst, ssize_t maxlen, const char *ntpath, const char *device, const char *drive )
{
	size_t device_len;
	size_t drive_len;
	size_t tail_len;

	device_len = strlen( device );
	if( strncmp( ntpath, device, device_len ) != 0 )
		return FALSE;

	drive_len = strlen( drive );
	tail_len = strlen( ntpath + device_len );
	if( (drive_len + tail_len) >= (size_t)maxlen )
		return FALSE;

	memcpy( dst, drive, drive_len );
	memcpy( dst + drive_len, ntpath + device_len, tail_len + 1 );
	return TRUE;
}

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	char ntpath[MAX_PATH];
	char *p;

	if( maxlen <= 0 )
		return NULL;

	nxGetCurrentXbeNtPath( ntpath );

	if( !hCopyMappedPath( dst, maxlen, ntpath, "\\Device\\CdRom0\\", "D:\\" ) &&
	    !hCopyMappedPath( dst, maxlen, ntpath, "\\Device\\Harddisk0\\Partition1\\", "E:\\" ) &&
	    !hCopyMappedPath( dst, maxlen, ntpath, "\\Device\\Harddisk0\\Partition2\\", "C:\\" ) ) {
		if( strlen( ntpath ) >= (size_t)maxlen )
			return NULL;
		strcpy( dst, ntpath );
	}

	fb_hConvertPath( dst );

	p = strrchr( dst, '\\' );
	if( p != NULL )
		*p = '\0';
	else
		dst[0] = '\0';

	if( maxlen > 3 && dst[2] == '\0' && dst[1] == ':' ) {
		dst[2] = '\\';
		dst[3] = '\0';
	}

	return p;
}
