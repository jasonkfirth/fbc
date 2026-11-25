/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/crt_iob_compat.c

    Purpose:

        Bridge the modern MinGW stdio helper to the C runtime supplied with
        Windows 95-era systems.

    Responsibilities:

        - route MinGW's indexed stream access to the Windows 95 stream table
        - map numbered stat imports to their 32-bit Windows 95 equivalents
        - keep the compatibility symbols inside programs that actually need them

    This file intentionally does NOT contain:

        - general C runtime replacements
        - stream allocation or ownership
        - operating-system version detection
*/

/* ------------------------------------------------------------------------- */
/* C runtime compatibility                                                   */
/* ------------------------------------------------------------------------- */

#if defined( __i386__ ) || defined( _M_IX86 )

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
	#define FB_CRT_WEAK __attribute__((weak))
	#if defined( FB_WIN95_CRT )
		#define FB_CRT_REQUIRED
	#else
		#define FB_CRT_REQUIRED __attribute__((weak))
	#endif
#else
	#define FB_CRT_CDECL __cdecl
	#define FB_CRT_WEAK
	#define FB_CRT_REQUIRED
#endif

extern int FB_CRT_CDECL _stat( const char *path, void *status );
extern int FB_CRT_CDECL _wstat( const unsigned short *path, void *status );
extern void *FB_CRT_CDECL __p__iob( void );

typedef int (FB_CRT_CDECL *FB_CRT_STAT_PROC)( const char *path, void *status );
typedef int (FB_CRT_CDECL *FB_CRT_WSTAT_PROC)( const unsigned short *path,
                                               void *status );
typedef void *(FB_CRT_CDECL *FB_CRT_IOB_PROC)( unsigned int index );

enum {
	/* The 32-bit Microsoft FILE layout used by MSVCRT40 is 32 bytes. */
	FB_CRT_FILE_SIZE = 32
};

/*
   Current MinGW headers obtain stdin, stdout, and stderr by indexing the
   stream table through __acrt_iob_func().  MSVCRT40 predates that helper, but
   its FILE entries use the same 32-byte layout expected by 32-bit MinGW.
*/
static void *FB_CRT_CDECL fb_CrtAcrtIobFunc( unsigned int index )
{
	unsigned char *streams = (unsigned char *)__p__iob( );

	return streams + (index * FB_CRT_FILE_SIZE);
}

/*
   The numbered names only distinguish the time_t layout in newer Microsoft
   runtimes.  Windows 95 and its MSVCRT40 use 32-bit time_t, so their _stat
   and _wstat structures already have the layout requested by _stat32.
*/
static int FB_CRT_CDECL fb_CrtStat32( const char *path, void *status )
{
	return _stat( path, status );
}

static int FB_CRT_CDECL fb_CrtWstat32( const unsigned short *path, void *status )
{
	return _wstat( path, status );
}

/*
   The Win95 graphics runtime uses MinGW's __ms_vsnprintf entry point.  The
   MSVCRT40 library does not provide it, so route bounded formatting through
   its _vsnprintf implementation.  Preserve the terminating NUL when the
   legacy CRT reports a full or truncated destination.
*/
int FB_CRT_CDECL __ms_vsnprintf( char *buffer, size_t count,
                                 const char *format, va_list args ) FB_CRT_REQUIRED;
int FB_CRT_CDECL __ms_vsnprintf( char *buffer, size_t count,
                                 const char *format, va_list args )
{
	int result;

	if( count == 0 )
		return 0;

	result = _vsnprintf( buffer, count, format, args );
	if( result < 0 || (size_t)result >= count )
		buffer[count - 1] = '\0';

	return result;
}

/*
   MinGW declares these helpers with dllimport and therefore emits references
   to their import-pointer symbols. Weak definitions let the selected target
   CRT provide its own implementation when available; the fallbacks satisfy
   symbols that MSVCRT40 does not export.
*/
FB_CRT_STAT_PROC fb_CrtImpStat32
	__asm__("__imp___stat32") FB_CRT_WEAK = fb_CrtStat32;
FB_CRT_WSTAT_PROC fb_CrtImpWstat32
	__asm__("__imp___wstat32") FB_CRT_WEAK = fb_CrtWstat32;
FB_CRT_IOB_PROC fb_CrtImpAcrtIobFunc
	__asm__("__imp____acrt_iob_func") FB_CRT_WEAK = fb_CrtAcrtIobFunc;

#undef FB_CRT_CDECL
#undef FB_CRT_REQUIRED
#undef FB_CRT_WEAK

#else
/* Keep the Win64 translation unit valid without defining 32-bit symbols. */
typedef int fb_crt_iob_compat_empty_translation_unit;
#endif

/* end of drivers/win32/crt_iob_compat.c */
