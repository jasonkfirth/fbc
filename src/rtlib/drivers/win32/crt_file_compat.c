/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/crt_file_compat.c

    Purpose:

        Supply the 64-bit file-length helper used by MinGW's fseeko64 on
        Windows 95-era C runtimes.

    Responsibilities:

        - obtain a 64-bit file size from a Win32 file handle
        - translate invalid descriptors and Win32 failures to errno

    This file intentionally does NOT contain:

        - general stdio replacements
        - file path conversion
        - operating-system version detection
*/

/* ------------------------------------------------------------------------- */
/* Legacy C runtime compatibility                                            */
/* ------------------------------------------------------------------------- */

#if defined( FB_WIN95_CRT ) && ( defined( __i386__ ) || defined( _M_IX86 ) )

#include <errno.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
#else
	#define FB_CRT_CDECL __cdecl
#endif

int64_t FB_CRT_CDECL fb_CrtFileLengthI64( int file_descriptor )
	__asm__( "__filelengthi64" );
int64_t FB_CRT_CDECL fb_CrtFileLengthI64( int file_descriptor )
{
	intptr_t os_handle;
	DWORD high_word = 0;
	DWORD low_word;
	DWORD win32_error;
	uint64_t file_size;

	os_handle = _get_osfhandle( file_descriptor );
	if( os_handle == (intptr_t)-1 ) {
		errno = EBADF;
		return -1;
	}

	SetLastError( NO_ERROR );
	low_word = GetFileSize( (HANDLE)os_handle, &high_word );
	win32_error = GetLastError( );
	if( low_word == INVALID_FILE_SIZE && win32_error != NO_ERROR ) {
		errno = EIO;
		return -1;
	}

	file_size = ((uint64_t)high_word << 32) | (uint64_t)low_word;
	if( file_size > (uint64_t)INT64_MAX ) {
		errno = EOVERFLOW;
		return -1;
	}

	return (int64_t)file_size;
}

#undef FB_CRT_CDECL

#else
/* Keep non-Win95 and non-x86 builds free of legacy CRT symbols. */
typedef int fb_crt_file_compat_empty_translation_unit;
#endif

/* end of drivers/win32/crt_file_compat.c */
