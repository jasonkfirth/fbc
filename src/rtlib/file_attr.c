/*
    FreeBASIC Runtime Library
    File: file_attr.c
    Purpose: Query file modes, handles, and host metadata snapshots.
    Responsibilities: Adapt runtime handles and native filesystem information.
    This file intentionally does NOT contain hashing or output publication policy.
*/

#include "fb.h"
#ifdef HOST_WIN32
	#include "win32/io_printer_private.h"
#endif
#include "dev_com_private.h"
#include "io_serial_private.h"
#include <sys/stat.h>
#include <errno.h>

#if defined(HOST_MINGW) && !defined(HOST_WINCE)
#include <io.h>
#define FILE_INFO_STAT struct __stat64
#define file_info_fstat _fstat64
#define file_info_fileno _fileno
#else
#define FILE_INFO_STAT struct stat
#define file_info_fstat fstat
#define file_info_fileno fileno
#endif

static int file_mode_map[] = { FB_FILE_ATTR_MODE_BINARY,   /* FB_FILE_MODE_BINARY = 0 */
	                           FB_FILE_ATTR_MODE_RANDOM,   /* FB_FILE_MODE_RANDOM = 1 */
    	                       FB_FILE_ATTR_MODE_INPUT,    /* FB_FILE_MODE_INPUT  = 2 */
    	                       FB_FILE_ATTR_MODE_OUTPUT,   /* FB_FILE_MODE_OUTPUT = 3 */
    	                       FB_FILE_ATTR_MODE_APPEND }; /* FB_FILE_MODE_APPEND = 4 */

FBCALL ssize_t fb_FileAttr( int handle, int returntype )
{
	ssize_t ret = 0;
	int err = 0;
	FB_FILE *file;

	file = FB_FILE_TO_HANDLE( handle );

	if( !file ) {
		ret = 0;
		err = FB_RTERROR_ILLEGALFUNCTIONCALL;
	} else {
		switch( returntype ) {
		case FB_FILE_ATTR_MODE:
			ret = file_mode_map[file->mode];
			err = FB_RTERROR_OK;
			break;

		case FB_FILE_ATTR_HANDLE:
			switch( file->type ) {
			case FB_FILE_TYPE_PRINTER:
				{
					DEV_LPT_INFO *lptinfo = file->opaque;
					if( lptinfo ) {
						#ifdef HOST_WIN32
							W32_PRINTER_INFO *printerinfo = lptinfo->driver_opaque;
							if( printerinfo ) {
								/* Win32: HANDLE */
								ret = (ssize_t)printerinfo->hPrinter;
								err = FB_RTERROR_OK;
							}
						#else
							/* Unix/DOS: CRT FILE* */
							ret = (ssize_t)lptinfo->driver_opaque;
							err = FB_RTERROR_OK;
						#endif
					}
				}
				break;

			case FB_FILE_TYPE_SERIAL:
				{
					DEV_COM_INFO *cominfo = file->opaque;
					if( cominfo ) {
						#ifdef HOST_WIN32
							W32_SERIAL_INFO *serialinfo = cominfo->hSerial;
							if( serialinfo ) {
								ret = (ssize_t)serialinfo->hDevice;
								err = FB_RTERROR_OK;
							}
						#elif defined HOST_LINUX
							LINUX_SERIAL_INFO *serialinfo = cominfo->hSerial;
							if( serialinfo ) {
								ret = serialinfo->sfd;
								err = FB_RTERROR_OK;
							}
						#elif defined HOST_DOS
							DOS_SERIAL_INFO *serialinfo = cominfo->hSerial;
							if( serialinfo ) {
								ret = serialinfo->com_num;
								err = FB_RTERROR_OK;
							}
						#endif
					}
				}
				break;

			default:
				ret = (ssize_t)file->opaque; /* CRT FILE* */
				err = FB_RTERROR_OK;
				break;
			}
			break;

		case FB_FILE_ATTR_ENCODING:
			ret = file->encod;
			err = FB_RTERROR_OK;
			break;

		default:
			ret = 0;
			err = FB_RTERROR_ILLEGALFUNCTIONCALL; 
			break;
		}
	}

	fb_ErrorSetNum( err );
	return ret;
}

/* ------------------------------------------------------------------------- */
/* Host metadata and opaque CRT positions                                    */
/* ------------------------------------------------------------------------- */

STATIC_ASSERT( sizeof( FB_FILE_INFO ) == 9 * sizeof( uint64_t ) );

static void hFileInfo( const FILE_INFO_STAT *native, FB_FILE_INFO *info )
{
	info->flags = FB_FILE_INFO_EXISTS;
#if defined(HOST_MINGW) && !defined(HOST_WINCE)
	/* fb.h disables MinGW's old POSIX names; use the CRT's native constants. */
	if( (native->st_mode & _S_IFMT) == _S_IFREG ) info->flags |= FB_FILE_INFO_REGULAR;
#else
	if( (native->st_mode & S_IFMT) == S_IFREG ) info->flags |= FB_FILE_INFO_REGULAR;
#endif
	if( native->st_ino != 0 ) info->flags |= FB_FILE_INFO_IDENTITY;
	info->identity[0] = native->st_dev;
	info->identity[1] = native->st_ino;
	info->bytes = native->st_size;
	info->modified = native->st_mtime;
	info->changed = native->st_ctime;
#if defined(__linux__)
	info->modified_fraction = native->st_mtim.tv_nsec;
	info->changed_fraction = native->st_ctim.tv_nsec;
#elif defined(HOST_DARWIN)
	info->modified_fraction = native->st_mtimespec.tv_nsec;
	info->changed_fraction = native->st_ctimespec.tv_nsec;
#endif
}

#if defined(HOST_MINGW) && !defined(HOST_WINCE)
static int hWindowsFileInfo( HANDLE handle, FB_FILE_INFO *info )
{
	BY_HANDLE_FILE_INFORMATION native;
	if( !GetFileInformationByHandle( handle, &native ) ) return 0;
	info->flags = FB_FILE_INFO_EXISTS | FB_FILE_INFO_IDENTITY;
	if( GetFileType( handle ) == FILE_TYPE_DISK &&
	    !(native.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) )
		info->flags |= FB_FILE_INFO_REGULAR;
	info->identity[0] = native.dwVolumeSerialNumber;
	info->identity[1] = native.nFileIndexHigh;
	info->identity[2] = native.nFileIndexLow;
	info->bytes = ((uint64_t)native.nFileSizeHigh << 32) | native.nFileSizeLow;
	/* FILETIME counts 100 ns units. Preserve the host's complete value without
	   requiring callers to know its epoch or the native structure layout. */
	info->modified = ((uint64_t)native.ftLastWriteTime.dwHighDateTime << 32) | native.ftLastWriteTime.dwLowDateTime;
	info->changed = ((uint64_t)native.ftCreationTime.dwHighDateTime << 32) | native.ftCreationTime.dwLowDateTime;
	return 1;
}
#endif

int fb_FileQueryInfo( const char *filename, int follow_links, FB_FILE_INFO *info )
{
	if( info == NULL ) return 0;
	memset( info, 0, sizeof( *info ) );
	if( filename == NULL || filename[0] == '\0' ) return 0;
#if defined(HOST_MINGW) && !defined(HOST_WINCE)
	DWORD attributes = GetFileAttributesA( filename );
	HANDLE handle;
	int ok;
	if( attributes == INVALID_FILE_ATTRIBUTES ) {
		DWORD error = GetLastError();
		return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
	}
	if( !follow_links && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) ) {
		info->flags = FB_FILE_INFO_EXISTS;
		return 1;
	}
	handle = CreateFileA( filename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
	if( handle == INVALID_HANDLE_VALUE ) return 0;
	ok = hWindowsFileInfo( handle, info );
	CloseHandle( handle );
	return ok;
#else
	FILE_INFO_STAT native;
	int status;
#if defined(HOST_UNIX) || defined(HOST_CYGWIN)
	status = follow_links ? stat( filename, &native ) : lstat( filename, &native );
#else
	(void)follow_links;
	status = stat( filename, &native );
#endif
	if( status != 0 ) return errno == ENOENT || errno == ENOTDIR;
	hFileInfo( &native, info );
	return 1;
#endif
}

int fb_FileQueryStreamInfo( void *stream, FB_FILE_INFO *info )
{
	FILE_INFO_STAT native;
	if( info == NULL ) return 0;
	memset( info, 0, sizeof( *info ) );
	if( stream == NULL ) return 0;
#if defined(HOST_MINGW) && !defined(HOST_WINCE)
	HANDLE handle = (HANDLE)_get_osfhandle( _fileno( stream ) );
	if( handle == INVALID_HANDLE_VALUE ) return 0;
	/* CRT fstat supplies a usable type for pipes and character streams. Only
	   disk handles have the persistent identity queried by the Windows API. */
	if( GetFileType( handle ) == FILE_TYPE_DISK ) return hWindowsFileInfo( handle, info );
#endif
	if( file_info_fstat( file_info_fileno( stream ), &native ) != 0 ) return 0;
	hFileInfo( &native, info );
	return 1;
}

void *fb_CrtFileSavePos( void *stream )
{
	fpos_t *position;
	if( stream == NULL ) return NULL;
	position = malloc( sizeof( *position ) );
	if( position == NULL ) return NULL;
	if( fgetpos( stream, position ) != 0 ) { free( position ); return NULL; }
	return position;
}

int fb_CrtFileRestorePos( void *stream, void *position )
{
	int ok;
	if( position == NULL ) return 0;
	ok = stream != NULL && fsetpos( stream, position ) == 0;
	free( position );
	return ok;
}

/* end of file_attr.c */
